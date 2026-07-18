#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>

#include "halofpx-context-store-publication.h"

#include <condition_variable>
#include <array>
#include <cstdint>
#include <functional>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <type_traits>
#include <vector>

namespace {

using halofpx::context_store_publication_anchor;
using halofpx::context_store_publication_backend;
using halofpx::context_store_publication_max_objects_v1;
using halofpx::context_store_publication_request;
using halofpx::context_store_publication_result;
using halofpx::context_store_publication_root_fence;
using halofpx::context_store_publication_status;
using halofpx::context_store_publication_step_result;
using halofpx::context_store_publication_writer;

static_assert(std::tuple_size_v<decltype(halofpx::context_store_anchor_body::store_uuid)> == 16);
static_assert(!std::is_aggregate_v<halofpx::context_store_publication_anchor>);

const std::array<uint8_t, 32> publication_master = { 1,2,3,4,5,6,7,8 };
const std::array<uint8_t, 32> alternate_publication_master = { 8,7,6,5,4,3,2,1 };

halofpx::context_store_anchor_key_record publication_key(uint64_t generation = 9, char suffix = 'a') {
    halofpx::context_store_anchor_key_record key;
    key.disposition = halofpx::context_store_key_disposition::active;
    const std::string key_id = std::string("publication-key-") + suffix;
    key.key_id.size = static_cast<uint8_t>(key_id.size());
    std::copy(key_id.begin(), key_id.end(), key.key_id.bytes.begin());
    key.generation = generation;
    key.master_key = { publication_master.data(), publication_master.size() };
    return key;
}

halofpx::context_store_anchor_key_record alternate_master_key() {
    auto key = publication_key();
    key.master_key = { alternate_publication_master.data(), alternate_publication_master.size() };
    return key;
}

halofpx::context_store_anchor_body make_anchor_body(uint64_t generation) {
    halofpx::context_store_anchor_body value;
    value.store_uuid[0] = 1;
    value.namespace_id[0] = 2;
    value.checkpoint_lineage_id[0] = 3;
    value.policy_epoch = 4;
    value.manifest_key_generation = 5;
    value.authority_epoch = 6;
    value.generation = generation;
    value.selected_manifest_digest[0] = static_cast<uint8_t>(0x20 + generation);
    value.has_predecessor = generation > 1;
    value.predecessor_manifest_digest[0] = generation <= 1 ? 0 : static_cast<uint8_t>(0x1f + generation);
    return value;
}

context_store_publication_anchor make_anchor(
        uint64_t generation,
        halofpx::context_store_anchor_key_record key = publication_key()) {
    const auto body = make_anchor_body(generation);
    std::array<uint8_t, halofpx::context_store_anchor_max_bytes> encoded {};
    const auto result = halofpx::context_store_encode_anchor_v1(body, key, encoded.data(), encoded.size());
    assert(result.status == halofpx::context_store_anchor_status::authenticated_unadmitted);
    assert(result.authenticated_carrier() != nullptr);
    return *result.authenticated_carrier();
}

context_store_publication_anchor make_anchor(halofpx::context_store_anchor_body body,
        halofpx::context_store_anchor_key_record key = publication_key()) {
    std::array<uint8_t, halofpx::context_store_anchor_max_bytes> encoded {};
    const auto result = halofpx::context_store_encode_anchor_v1(body, key, encoded.data(), encoded.size());
    assert(result.status == halofpx::context_store_anchor_status::authenticated_unadmitted);
    return *result.authenticated_carrier();
}

context_store_publication_anchor modified_anchor(
        uint64_t generation,
        const std::function<void(halofpx::context_store_anchor_body &)> & mutate,
        halofpx::context_store_anchor_key_record key = publication_key()) {
    auto body = make_anchor_body(generation);
    mutate(body);
    return make_anchor(body, key);
}

bool same_anchor(
        const context_store_publication_anchor & left,
        const context_store_publication_anchor & right) {
    return left.authenticated() && right.authenticated() &&
        left.envelope_size() == right.envelope_size() &&
        *left.envelope_digest() == *right.envelope_digest() &&
        std::equal(left.envelope_data(), left.envelope_data() + left.envelope_size(), right.envelope_data());
}

context_store_publication_request make_request(size_t object_count = 2) {
    context_store_publication_request request;
    request.attempt_id[0] = 0xa5;
    request.expected_predecessor = make_anchor(7);
    request.next = make_anchor(8);
    request.object_count = object_count;
    return request;
}

class scripted_backend final : public context_store_publication_backend {
public:
    context_store_publication_anchor current = make_anchor(7);
    std::vector<std::string> calls;
    size_t fail_at = 0;
    context_store_publication_step_result failure = context_store_publication_step_result::storage_error;
    bool anchor_replaced = false;
    bool anchor_synced = false;
    bool throw_at_failure = false;
    bool block_read = false;
    halofpx::context_store_publication_id observed_attempt_id {};
    context_store_publication_anchor * shared_current = nullptr;
    std::function<void()> before_replace;
    context_store_publication_step_result abandon_result = context_store_publication_step_result::ok;
    bool throw_abandon = false;
    size_t abandon_count = 0;
    size_t uncertain_fence_count = 0;
    context_store_publication_step_result uncertain_fence_result = context_store_publication_step_result::ok;
    halofpx::context_store_digest verified_manifest_digest = make_anchor_body(8).selected_manifest_digest;

    context_store_publication_step_result read_anchor(context_store_publication_anchor & anchor) override {
        if (block_read) {
            std::unique_lock<std::mutex> lock(mutex_);
            read_entered_ = true;
            condition_.notify_all();
            condition_.wait(lock, [&] { return release_read_; });
        }
        auto result = invoke("read_anchor");
        if (complete(result)) anchor = shared_current ? *shared_current : current;
        return result;
    }
    context_store_publication_step_result begin_attempt(
            const halofpx::context_store_publication_id & attempt_id,
            const context_store_publication_anchor &,
            const context_store_publication_anchor &,
            size_t) override {
        observed_attempt_id = attempt_id;
        return invoke("begin_attempt");
    }
    context_store_publication_step_result stage_object(const halofpx::context_store_publication_id &, size_t index) override { return invoke(object("stage_object", index)); }
    context_store_publication_step_result write_object(const halofpx::context_store_publication_id &, size_t index) override { return invoke(object("write_object", index)); }
    context_store_publication_step_result verify_object(const halofpx::context_store_publication_id &, size_t index) override { return invoke(object("verify_object", index)); }
    context_store_publication_step_result sync_object_file(const halofpx::context_store_publication_id &, size_t index) override { return invoke(object("sync_object_file", index)); }
    context_store_publication_step_result publish_object_no_replace(const halofpx::context_store_publication_id &, size_t index) override { return invoke(object("publish_object", index)); }
    context_store_publication_step_result sync_object_directory(const halofpx::context_store_publication_id &, size_t index) override { return invoke(object("sync_object_directory", index)); }
    context_store_publication_step_result stage_manifest(const halofpx::context_store_publication_id &) override { return invoke("stage_manifest"); }
    context_store_publication_step_result write_manifest(const halofpx::context_store_publication_id &) override { return invoke("write_manifest"); }
    context_store_publication_step_result verify_manifest(
            const halofpx::context_store_publication_id &,
            halofpx::context_store_digest & digest) override {
        auto result = invoke("verify_manifest");
        if (result == context_store_publication_step_result::ok) {
            digest = verified_manifest_digest;
        }
        return result;
    }
    context_store_publication_step_result sync_manifest_file(const halofpx::context_store_publication_id &) override { return invoke("sync_manifest_file"); }
    context_store_publication_step_result publish_manifest_no_replace(const halofpx::context_store_publication_id &) override { return invoke("publish_manifest"); }
    context_store_publication_step_result sync_manifest_directory(const halofpx::context_store_publication_id &) override { return invoke("sync_manifest_directory"); }
    context_store_publication_step_result replace_anchor_atomically(
            const halofpx::context_store_publication_id & attempt_id,
            const context_store_publication_anchor & expected_predecessor,
            const context_store_publication_anchor & next) override {
        auto result = invoke("replace_anchor");
        if (complete(result)) {
            if (before_replace) before_replace();
            assert(attempt_id != halofpx::context_store_publication_id {});
            observed_attempt_id = attempt_id;
            auto & selected = shared_current ? *shared_current : current;
            if (!same_anchor(selected, expected_predecessor)) {
                return context_store_publication_step_result::stale_predecessor;
            }
            selected = next;
            anchor_replaced = true;
        }
        return result;
    }
    context_store_publication_step_result sync_anchor(
            const halofpx::context_store_publication_id &,
            const context_store_publication_anchor &) override {
        auto result = invoke("sync_anchor");
        if (complete(result)) anchor_synced = true;
        return result;
    }
    context_store_publication_step_result close_durable_attempt(
            const halofpx::context_store_publication_id &,
            const context_store_publication_anchor &) override {
        return invoke("close_attempt");
    }
    context_store_publication_step_result abandon_attempt(
            const halofpx::context_store_publication_id &) override {
        ++abandon_count;
        if (throw_abandon) throw std::runtime_error("abandon injected");
        return abandon_result;
    }
    context_store_publication_step_result fence_attempt_uncertain(
            const halofpx::context_store_publication_id &) override {
        ++uncertain_fence_count;
        return uncertain_fence_result;
    }

    void wait_until_read_entered() {
        std::unique_lock<std::mutex> lock(mutex_);
        condition_.wait(lock, [&] { return read_entered_; });
    }
    void release_read() {
        std::lock_guard<std::mutex> lock(mutex_);
        release_read_ = true;
        condition_.notify_all();
    }

private:
    std::mutex mutex_;
    std::condition_variable condition_;
    bool read_entered_ = false;
    bool release_read_ = false;

    static bool complete(context_store_publication_step_result result) {
        return result == context_store_publication_step_result::ok ||
            result == context_store_publication_step_result::already_equal;
    }
    static std::string object(const char * name, size_t index) {
        return std::string(name) + "[" + std::to_string(index) + "]";
    }
    context_store_publication_step_result invoke(std::string name) {
        calls.push_back(std::move(name));
        if (fail_at != 0 && calls.size() == fail_at) {
            if (throw_at_failure) throw std::runtime_error("injected");
            return failure;
        }
        return context_store_publication_step_result::ok;
    }
};

void assert_unacknowledged(const context_store_publication_result & result) {
    assert(result.status != context_store_publication_status::published);
    assert(!result.durability_acknowledged);
}

void test_complete_order() {
    context_store_publication_root_fence fence;
    context_store_publication_writer writer(fence);
    scripted_backend backend;
    auto result = writer.publish(make_request(), backend);
    assert(result.status == context_store_publication_status::published);
    assert(result.completed_steps == 23);
    assert(result.anchor_replaced && result.durability_acknowledged);
    assert(backend.anchor_replaced && backend.anchor_synced);
    assert(backend.observed_attempt_id == make_request().attempt_id);
    const std::vector<std::string> expected = {
        "read_anchor",
        "begin_attempt",
        "stage_object[0]", "write_object[0]", "verify_object[0]",
        "sync_object_file[0]", "publish_object[0]", "sync_object_directory[0]",
        "stage_object[1]", "write_object[1]", "verify_object[1]",
        "sync_object_file[1]", "publish_object[1]", "sync_object_directory[1]",
        "stage_manifest", "write_manifest", "verify_manifest",
        "sync_manifest_file", "publish_manifest", "sync_manifest_directory",
        "replace_anchor", "sync_anchor", "close_attempt"
    };
    assert(backend.calls == expected);
}

void test_every_failure_boundary() {
    for (size_t fail_at = 1; fail_at <= 23; ++fail_at) {
        context_store_publication_root_fence fence;
        context_store_publication_writer writer(fence);
        scripted_backend backend;
        backend.fail_at = fail_at;
        backend.failure = (fail_at == 6 || fail_at == 8 || fail_at == 12 || fail_at == 14 ||
                           fail_at == 18 || fail_at == 20 || fail_at == 22) ?
            context_store_publication_step_result::sync_error :
            context_store_publication_step_result::storage_error;
        auto result = writer.publish(make_request(), backend);
        assert_unacknowledged(result);
        assert(result.completed_steps == fail_at - 1);
        assert(backend.calls.size() == fail_at);
        if (fail_at < 21) {
            assert(!backend.anchor_replaced);
            assert(!result.anchor_replaced);
        } else if (fail_at == 21) {
            assert(!backend.anchor_replaced);
            assert(!result.anchor_replaced);
            assert(result.status == context_store_publication_status::anchor_visibility_uncertain);
        } else {
            assert(backend.anchor_replaced);
            assert(result.anchor_replaced);
            assert(result.status == context_store_publication_status::anchor_visibility_uncertain);
        }
    }
}

void test_collision_and_idempotent_existing() {
    for (size_t fail_at : { size_t(7), size_t(13) }) {
        context_store_publication_root_fence fence;
        context_store_publication_writer writer(fence);
        scripted_backend backend;
        backend.fail_at = fail_at;
        backend.failure = context_store_publication_step_result::conflict;
        auto result = writer.publish(make_request(), backend);
        assert(result.status == context_store_publication_status::object_collision);
        assert(!backend.anchor_replaced);
    }
    {
        context_store_publication_root_fence fence;
        context_store_publication_writer writer(fence);
        scripted_backend backend;
        backend.fail_at = 19;
        backend.failure = context_store_publication_step_result::conflict;
        auto result = writer.publish(make_request(), backend);
        assert(result.status == context_store_publication_status::manifest_collision);
        assert(!backend.anchor_replaced);
    }
    for (size_t equal_at : { size_t(7), size_t(13), size_t(19) }) {
        context_store_publication_root_fence fence;
        context_store_publication_writer writer(fence);
        scripted_backend backend;
        backend.fail_at = equal_at;
        backend.failure = context_store_publication_step_result::already_equal;
        auto result = writer.publish(make_request(), backend);
        assert(result.status == context_store_publication_status::published);
        assert(result.durability_acknowledged);
    }
    for (size_t invalid_equal_at : { size_t(1), size_t(2), size_t(4), size_t(6), size_t(17), size_t(18), size_t(21), size_t(22), size_t(23) }) {
        context_store_publication_root_fence fence;
        context_store_publication_writer writer(fence);
        scripted_backend backend;
        backend.fail_at = invalid_equal_at;
        backend.failure = context_store_publication_step_result::already_equal;
        auto result = writer.publish(make_request(), backend);
        assert(result.status == context_store_publication_status::storage_error ||
               result.status == context_store_publication_status::anchor_visibility_uncertain ||
               result.status == context_store_publication_status::attempt_fencing_uncertain);
        assert_unacknowledged(result);
    }
}

void test_identity_and_request_rejection() {
    const auto reject = [](context_store_publication_request request) {
        context_store_publication_root_fence fence;
        context_store_publication_writer writer(fence);
        scripted_backend backend;
        auto result = writer.publish(request, backend);
        assert(result.status == context_store_publication_status::invalid_request);
        assert(backend.calls.empty());
    };
    auto request = make_request(); request.object_count = 0; reject(request);
    request = make_request(); request.attempt_id = {}; reject(request);
    request = make_request(context_store_publication_max_objects_v1 + 1); reject(request);
    request = make_request(); request.next = modified_anchor(8, [](auto & b) { b.generation = 7; }); reject(request);
    request = make_request(); request.next = make_anchor(9); reject(request);
    request = make_request(); request.next = modified_anchor(8, [](auto & b) { b.predecessor_manifest_digest[0] ^= 1; }); reject(request);
    request = make_request(); request.next = modified_anchor(8, [](auto & b) { b.store_uuid[0] ^= 1; }); reject(request);
    request = make_request(); request.next = modified_anchor(8, [](auto & b) { b.namespace_id[0] ^= 1; }); reject(request);
    request = make_request(); request.next = modified_anchor(8, [](auto & b) { b.checkpoint_lineage_id[0] ^= 1; }); reject(request);
    request = make_request(); request.next = modified_anchor(8, [](auto & b) { ++b.policy_epoch; }); reject(request);
    request = make_request(); request.next = modified_anchor(8, [](auto & b) { ++b.manifest_key_generation; }); reject(request);
    request = make_request(); request.next = modified_anchor(8, [](auto & b) { --b.authority_epoch; }); reject(request);
    request = make_request(); request.next = modified_anchor(8, [](auto & b) { ++b.authority_epoch; }); reject(request);
    request = make_request(); request.next = make_anchor(8, publication_key(10)); reject(request);
    request = make_request(); request.next = make_anchor(8, publication_key(9, 'b')); reject(request);
    request = make_request(); request.next = make_anchor(8, alternate_master_key()); reject(request);
    request = make_request(); request.expected_predecessor = modified_anchor(7, [](auto & b) { b.generation = UINT64_MAX; }); reject(request);
    request = make_request(); request.expected_predecessor = {}; reject(request);
    request = make_request(); request.next = {}; reject(request);

    {
        context_store_publication_root_fence fence;
        context_store_publication_writer writer(fence);
        scripted_backend backend;
        backend.current = {};
        const auto result = writer.publish(make_request(), backend);
        assert(result.status == context_store_publication_status::storage_error);
        assert(result.completed_steps == 1 && backend.calls.size() == 1);
    }
    {
        context_store_publication_root_fence fence;
        context_store_publication_writer writer(fence);
        scripted_backend backend;
        backend.fail_at = 1;
        backend.failure = context_store_publication_step_result::anchor_absent;
        const auto result = writer.publish(make_request(), backend);
        assert(result.status == context_store_publication_status::bootstrap_required);
        assert(result.completed_steps == 0 && backend.calls.size() == 1);
    }

    const auto reject_stale = [](context_store_publication_anchor current) {
        context_store_publication_root_fence fence;
        context_store_publication_writer writer(fence);
        scripted_backend backend;
        backend.current = current;
        auto result = writer.publish(make_request(), backend);
        assert(result.status == context_store_publication_status::stale_predecessor);
        assert(result.completed_steps == 1);
        assert(backend.calls.size() == 1);
    };
    reject_stale(modified_anchor(7, [](auto & b) { b.store_uuid[0] ^= 1; }));
    reject_stale(modified_anchor(7, [](auto & b) { b.namespace_id[0] ^= 1; }));
    reject_stale(modified_anchor(7, [](auto & b) { b.checkpoint_lineage_id[0] ^= 1; }));
    reject_stale(modified_anchor(7, [](auto & b) { ++b.policy_epoch; }));
    reject_stale(modified_anchor(7, [](auto & b) { ++b.manifest_key_generation; }));
    reject_stale(modified_anchor(7, [](auto & b) { ++b.authority_epoch; }));
    reject_stale(modified_anchor(7, [](auto & b) { ++b.generation; }));
    reject_stale(modified_anchor(7, [](auto & b) { b.selected_manifest_digest[0] ^= 1; }));
    reject_stale(modified_anchor(7, [](auto & b) { b.predecessor_manifest_digest[0] ^= 1; }));
    reject_stale(make_anchor(7, publication_key(10)));

    context_store_publication_root_fence fence;
    context_store_publication_writer writer(fence);
    scripted_backend backend;
    backend.verified_manifest_digest[0] ^= 1;
    auto mismatch = writer.publish(make_request(), backend);
    assert(mismatch.status == context_store_publication_status::manifest_identity_mismatch);
    assert(mismatch.completed_steps == 16);
    assert(!mismatch.anchor_replaced && !mismatch.durability_acknowledged);
    assert(backend.calls.back() == "verify_manifest");
}

void test_exception_and_single_writer_fence() {
    {
        context_store_publication_root_fence fence;
        context_store_publication_writer writer(fence);
        scripted_backend backend;
        backend.fail_at = 5;
        backend.throw_at_failure = true;
        auto result = writer.publish(make_request(), backend);
        assert(result.status == context_store_publication_status::storage_error);
        assert_unacknowledged(result);
    }
    {
        context_store_publication_root_fence fence;
        context_store_publication_writer writer(fence);
        scripted_backend backend;
        backend.fail_at = 21;
        backend.throw_at_failure = true;
        auto result = writer.publish(make_request(), backend);
        assert(result.status == context_store_publication_status::anchor_visibility_uncertain);
        assert(!result.anchor_replaced);
        assert_unacknowledged(result);
    }
    {
        context_store_publication_root_fence fence;
        context_store_publication_writer writer(fence);
        scripted_backend backend;
        backend.fail_at = 22;
        backend.throw_at_failure = true;
        auto result = writer.publish(make_request(), backend);
        assert(result.status == context_store_publication_status::anchor_visibility_uncertain);
        assert(result.anchor_replaced);
        assert_unacknowledged(result);
    }
    {
        context_store_publication_root_fence fence;
        context_store_publication_writer first_writer(fence);
        context_store_publication_writer second_writer(fence);
        scripted_backend first;
        first.block_read = true;
        context_store_publication_result first_result;
        std::thread active([&] { first_result = first_writer.publish(make_request(), first); });
        first.wait_until_read_entered();
        scripted_backend second;
        auto busy = second_writer.publish(make_request(), second);
        assert(busy.status == context_store_publication_status::writer_busy);
        assert(second.calls.empty());
        first.release_read();
        active.join();
        assert(first_result.status == context_store_publication_status::published);
    }
}

void test_cross_fence_stale_compare_and_swap() {
    // Distinct fences model coordinators that cannot see one another's
    // in-process exclusion (for example, separate processes). Both initially
    // observe generation 7; the nested coordinator wins the final CAS, and
    // the original coordinator is conclusively rejected without replacement.
    context_store_publication_anchor shared = make_anchor(7);
    context_store_publication_root_fence first_fence;
    context_store_publication_root_fence second_fence;
    context_store_publication_writer first_writer(first_fence);
    context_store_publication_writer second_writer(second_fence);
    scripted_backend first;
    scripted_backend second;
    first.shared_current = &shared;
    second.shared_current = &shared;
    auto second_request = make_request();
    second_request.attempt_id[0] = 0xb6;
    context_store_publication_result second_result;
    first.before_replace = [&] {
        second_result = second_writer.publish(second_request, second);
    };

    auto first_result = first_writer.publish(make_request(), first);
    assert(second_result.status == context_store_publication_status::published);
    assert(first_result.status == context_store_publication_status::stale_predecessor);
    assert(first_result.completed_steps == 20);
    assert(!first_result.anchor_replaced && !first_result.durability_acknowledged);
    assert(shared.body() != nullptr && shared.body()->generation == 8);
    assert(first.calls.back() == "replace_anchor");
}

void test_final_cas_compares_every_predecessor_field() {
    const std::vector<std::function<context_store_publication_anchor()>> mutations = {
        [] { return modified_anchor(7, [](auto & b) { b.store_uuid[0] ^= 1; }); },
        [] { return modified_anchor(7, [](auto & b) { b.namespace_id[0] ^= 1; }); },
        [] { return modified_anchor(7, [](auto & b) { b.checkpoint_lineage_id[0] ^= 1; }); },
        [] { return modified_anchor(7, [](auto & b) { ++b.policy_epoch; }); },
        [] { return modified_anchor(7, [](auto & b) { ++b.manifest_key_generation; }); },
        [] { return modified_anchor(7, [](auto & b) { ++b.authority_epoch; }); },
        [] { return modified_anchor(7, [](auto & b) { ++b.generation; }); },
        [] { return modified_anchor(7, [](auto & b) { b.selected_manifest_digest[0] ^= 1; }); },
        [] { return modified_anchor(7, [](auto & b) { b.predecessor_manifest_digest[0] ^= 1; }); },
        [] { return make_anchor(7, publication_key(10)); },
        [] { return make_anchor(7, publication_key(9, 'b')); },
        [] { return make_anchor(7, alternate_master_key()); },
    };
    for (const auto & mutate : mutations) {
        context_store_publication_anchor shared = make_anchor(7);
        context_store_publication_root_fence fence;
        context_store_publication_writer writer(fence);
        scripted_backend backend;
        backend.shared_current = &shared;
        auto expected_mutated = mutate();
        backend.before_replace = [&] { shared = expected_mutated; };
        auto result = writer.publish(make_request(), backend);
        assert(result.status == context_store_publication_status::stale_predecessor);
        assert(result.completed_steps == 20);
        assert(!result.anchor_replaced && !result.durability_acknowledged);
        assert(same_anchor(shared, expected_mutated));
    }
}

void test_status_names() {
    assert(std::string(halofpx::context_store_publication_status_name(
        context_store_publication_status::published)) == "published");
    assert(std::string(halofpx::context_store_publication_status_name(
        context_store_publication_status::bootstrap_required)) == "bootstrap_required");
    assert(std::string(halofpx::context_store_publication_status_name(
        static_cast<context_store_publication_status>(255))) == "unknown");
}

void test_abandonment_and_begin_uncertainty() {
    for (bool throws : { false, true }) {
        context_store_publication_root_fence fence;
        context_store_publication_writer writer(fence);
        scripted_backend backend;
        backend.fail_at = 3;
        backend.failure = context_store_publication_step_result::io_error;
        backend.throw_abandon = throws;
        if (!throws) backend.abandon_result = context_store_publication_step_result::interrupted;
        auto result = writer.publish(make_request(), backend);
        assert(result.status == context_store_publication_status::attempt_fencing_uncertain);
        assert(result.attempt_fence_confirmed);
        assert(!result.anchor_replaced && !result.durability_acknowledged);
        assert(backend.abandon_count == 1);
    }
    {
        context_store_publication_root_fence fence;
        context_store_publication_writer writer(fence);
        scripted_backend backend;
        backend.fail_at = 2;
        backend.failure = context_store_publication_step_result::interrupted;
        auto result = writer.publish(make_request(), backend);
        assert(result.status == context_store_publication_status::attempt_fencing_uncertain);
        assert(backend.uncertain_fence_count == 1);
        assert(result.attempt_fence_confirmed);
    }
    {
        context_store_publication_root_fence fence;
        context_store_publication_writer writer(fence);
        scripted_backend backend;
        backend.fail_at = 2;
        backend.failure = context_store_publication_step_result::interrupted;
        backend.uncertain_fence_result = context_store_publication_step_result::io_error;
        auto result = writer.publish(make_request(), backend);
        assert(result.status == context_store_publication_status::attempt_fencing_uncertain);
        assert(!result.attempt_fence_confirmed);
    }
}

} // namespace

int main() {
    test_complete_order();
    test_every_failure_boundary();
    test_collision_and_idempotent_existing();
    test_identity_and_request_rejection();
    test_exception_and_single_writer_fence();
    test_cross_fence_stale_compare_and_swap();
    test_final_cas_compares_every_predecessor_field();
    test_status_names();
    test_abandonment_and_begin_uncertainty();
    return 0;
}
