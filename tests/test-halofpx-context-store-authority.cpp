#include "halofpx-context-store-authority.h"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <algorithm>
#include <array>
#include <memory>
#include <string>
#include <thread>
#include <type_traits>
#include <vector>

static_assert(!std::is_aggregate_v<halofpx::context_store_bootstrap_plan>);
static_assert(!std::is_aggregate_v<halofpx::context_store_bootstrap_result>);
static_assert(!std::is_copy_constructible_v<halofpx::context_store_bootstrap_authority>);
static_assert(!std::is_copy_assignable_v<halofpx::context_store_bootstrap_authority>);
static_assert(!std::is_move_constructible_v<halofpx::context_store_bootstrap_authority>);
static_assert(!std::is_move_assignable_v<halofpx::context_store_bootstrap_authority>);

namespace {

halofpx::context_store_registered_id id(const std::string & value) {
    halofpx::context_store_registered_id result;
    assert(!value.empty() && value.size() <= halofpx::context_store_registered_id_max_bytes);
    result.size = static_cast<uint8_t>(value.size());
    std::copy(value.begin(), value.end(), result.bytes.begin());
    return result;
}

struct fixture {
    std::array<uint8_t, 64> anchor_key {};
    std::array<uint8_t, 64> admin_key {};
    halofpx::context_store_bootstrap_authority_config config;
};

void refresh(fixture & f) {
    f.config.anchor_signing_key.master_key = { f.anchor_key.data(), f.anchor_key.size() };
    f.config.bootstrap_admin_key.master_key = { f.admin_key.data(), f.admin_key.size() };
}

fixture make_fixture() {
    fixture f;
    for (size_t i = 0; i < f.anchor_key.size(); ++i) {
        f.anchor_key[i] = static_cast<uint8_t>(i + 1);
        f.admin_key[i] = static_cast<uint8_t>(0xf0 - i);
    }
    f.config.anchor_signing_key.disposition = halofpx::context_store_key_disposition::active;
    f.config.anchor_signing_key.key_id = id("anchor-key-v1");
    f.config.anchor_signing_key.generation = 7;
    f.config.bootstrap_admin_key.disposition = halofpx::context_store_key_disposition::active;
    f.config.bootstrap_admin_key.key_id = id("bootstrap-admin-v1");
    f.config.bootstrap_admin_key.generation = 11;
    f.config.store_uuid.fill(0x10);
    f.config.namespace_id.fill(0x20);
    f.config.policy_epoch = 3;
    f.config.checkpoint_lineage_id.fill(0x40);
    f.config.manifest_key_generation = 5;
    f.config.authority_epoch = 6;
    refresh(f);
    return f;
}

halofpx::context_store_bootstrap_request request(uint64_t attempt = 91, size_t objects = 8) {
    halofpx::context_store_bootstrap_request value;
    for (size_t i = 0; i < 8; ++i) value.attempt_id[31 - i] = static_cast<uint8_t>(attempt >> (i * 8));
    value.object_count = objects;
    value.selected_manifest_digest.fill(0x77);
    return value;
}

const halofpx::context_store_bootstrap_plan & require_plan(
        const halofpx::context_store_bootstrap_result & result) {
    assert(result.status == halofpx::context_store_bootstrap_status::authorized_unexecuted);
    assert(result.has_authorized_plan() && result.authorized_plan() != nullptr);
    return *result.authorized_plan();
}

void assert_no_plan(const halofpx::context_store_bootstrap_result & result) {
    assert(!result.has_authorized_plan());
    assert(result.authorized_plan() == nullptr);
}

void test_authorized_plan_and_source_ownership() {
    auto f = make_fixture();
    auto authority = std::make_unique<halofpx::context_store_bootstrap_authority>(f.config);
    assert(authority->valid());
    const auto before = authority->plan(request());
    const auto & plan = require_plan(before);
    assert(plan.authorized() && plan.attempt_id() != nullptr && *plan.attempt_id() == request().attempt_id && plan.object_count() == 8);
    assert(plan.selected_manifest_digest() != nullptr && *plan.selected_manifest_digest() == request().selected_manifest_digest);
    assert(plan.authority_snapshot_commitment() != nullptr && plan.authorization_commitment() != nullptr);
    assert(plan.bootstrap_admin_key_id() != nullptr && plan.bootstrap_admin_key_id()->size == id("bootstrap-admin-v1").size);
    assert(plan.bootstrap_admin_key_generation() == 11);
    assert(plan.anchor() != nullptr && plan.anchor()->authenticated());
    assert(plan.anchor()->body() != nullptr && plan.anchor()->body()->generation == 1);
    assert(!plan.anchor()->body()->has_predecessor);
    assert(plan.anchor()->body()->predecessor_manifest_digest == halofpx::context_store_format_digest {});
    assert(plan.anchor()->body()->selected_manifest_digest == request().selected_manifest_digest);

    const auto snapshot = *plan.authority_snapshot_commitment();
    const auto authorization = *plan.authorization_commitment();
    const auto envelope_digest = *plan.anchor()->envelope_digest();
    f.anchor_key.fill(0); f.admin_key.fill(0xff);
    f.config.store_uuid.fill(0xee); f.config.namespace_id.fill(0xdd);
    f.config.anchor_signing_key.key_id = id("mutated-anchor");
    f.config.bootstrap_admin_key.key_id = id("mutated-admin");
    const auto after = authority->plan(request());
    const auto & owned = require_plan(after);
    assert(*owned.authority_snapshot_commitment() == snapshot);
    assert(*owned.authorization_commitment() == authorization);
    assert(*owned.anchor()->envelope_digest() == envelope_digest);

    fixture temporary = make_fixture();
    auto lifetime = std::make_unique<halofpx::context_store_bootstrap_authority>(temporary.config);
    temporary = fixture {};
    assert(require_plan(lifetime->plan(request())).anchor()->authenticated());

    halofpx::context_store_bootstrap_plan retained;
    {
        auto scoped_fixture = make_fixture();
        halofpx::context_store_bootstrap_authority scoped(scoped_fixture.config);
        retained = require_plan(scoped.plan(request()));
    }
    assert(retained.authorized() && retained.anchor() != nullptr && retained.anchor()->authenticated());
    assert(*retained.attempt_id() == request().attempt_id);
}

void test_invalid_authority_and_purpose_separation() {
    for (bool anchor_key : { false, true }) {
        for (auto disposition : { halofpx::context_store_key_disposition::unknown,
                halofpx::context_store_key_disposition::revoked,
                halofpx::context_store_key_disposition::read_disabled }) {
            auto f = make_fixture();
            if (anchor_key) f.config.anchor_signing_key.disposition = disposition;
            else f.config.bootstrap_admin_key.disposition = disposition;
            halofpx::context_store_bootstrap_authority authority(f.config);
            assert(!authority.valid());
            const auto result = authority.plan(request());
            assert(result.status == halofpx::context_store_bootstrap_status::invalid_authority);
            assert_no_plan(result);
        }
    }
    auto check_invalid = [](fixture f) {
        halofpx::context_store_bootstrap_authority authority(f.config);
        assert(!authority.valid());
        assert_no_plan(authority.plan(request()));
    };
    auto f = make_fixture(); f.config.anchor_signing_key.master_key = {}; check_invalid(f);
    f = make_fixture(); f.config.bootstrap_admin_key.master_key = {}; check_invalid(f);
    f = make_fixture(); f.config.anchor_signing_key.key_id.size = 0; check_invalid(f);
    f = make_fixture(); f.config.bootstrap_admin_key.key_id.bytes[2] = '\0'; check_invalid(f);
    f = make_fixture(); f.config.anchor_signing_key.key_id.bytes[2] = static_cast<uint8_t>(0x80); check_invalid(f);
    f = make_fixture();
    f.config.bootstrap_admin_key.key_id = f.config.anchor_signing_key.key_id;
    f.config.bootstrap_admin_key.generation = f.config.anchor_signing_key.generation;
    check_invalid(f);
    f = make_fixture(); f.admin_key = f.anchor_key; check_invalid(f);
    f = make_fixture(); f.config.anchor_signing_key.generation = 0; check_invalid(f);
    f = make_fixture(); f.config.bootstrap_admin_key.generation = 0; check_invalid(f);
    f = make_fixture(); f.config.policy_epoch = 0; check_invalid(f);
    f = make_fixture(); f.config.manifest_key_generation = 0; check_invalid(f);
    f = make_fixture(); f.config.authority_epoch = 0; check_invalid(f);
    f = make_fixture(); f.config.store_uuid.fill(0); check_invalid(f);
    f = make_fixture(); f.config.namespace_id.fill(0); check_invalid(f);
    f = make_fixture(); f.config.checkpoint_lineage_id.fill(0); check_invalid(f);
}

void test_request_bounds_and_no_leakage() {
    auto f = make_fixture();
    halofpx::context_store_bootstrap_authority authority(f.config);
    const halofpx::context_store_bootstrap_plan empty;
    assert(!empty.authorized() && empty.attempt_id() == nullptr && empty.object_count() == 0);
    assert(empty.selected_manifest_digest() == nullptr && empty.authority_snapshot_commitment() == nullptr);
    assert(empty.authorization_commitment() == nullptr && empty.bootstrap_admin_key_id() == nullptr);
    assert(empty.bootstrap_admin_key_generation() == 0 && empty.anchor() == nullptr);
    for (const auto invalid : { request(0, 1), request(1, 0), request(1, 129) }) {
        const auto result = authority.plan(invalid);
        assert(result.status == halofpx::context_store_bootstrap_status::invalid_request);
        assert_no_plan(result);
    }
    auto zero_digest = request(); zero_digest.selected_manifest_digest.fill(0);
    const auto zero_result = authority.plan(zero_digest);
    assert(zero_result.status == halofpx::context_store_bootstrap_status::invalid_request);
    assert_no_plan(zero_result);
    for (size_t objects : { size_t(1), size_t(128) }) require_plan(authority.plan(request(1, objects)));
}

void test_bindings_and_key_changes() {
    auto base_fixture = make_fixture();
    halofpx::context_store_bootstrap_authority base(base_fixture.config);
    const auto base_result = base.plan(request());
    const auto base_snapshot = *require_plan(base_result).authority_snapshot_commitment();
    const auto base_auth = *require_plan(base_result).authorization_commitment();
    const auto base_anchor = *require_plan(base_result).anchor()->envelope_digest();

    const auto expect_changed = [&](fixture f, bool anchor_changes) {
        refresh(f);
        halofpx::context_store_bootstrap_authority changed(f.config);
        const auto result = changed.plan(request());
        const auto & plan = require_plan(result);
        assert(*plan.authority_snapshot_commitment() != base_snapshot);
        assert(*plan.authorization_commitment() != base_auth);
        assert((*plan.anchor()->envelope_digest() != base_anchor) == anchor_changes);
    };
    auto f = make_fixture(); f.anchor_key[3] ^= 1; expect_changed(f, true);
    f = make_fixture(); f.admin_key[3] ^= 1; expect_changed(f, false);
    f = make_fixture(); ++f.config.anchor_signing_key.generation; expect_changed(f, true);
    f = make_fixture(); ++f.config.bootstrap_admin_key.generation; expect_changed(f, false);
    f = make_fixture(); f.config.anchor_signing_key.key_id = id("anchor-key-v2"); expect_changed(f, true);
    f = make_fixture(); f.config.bootstrap_admin_key.key_id = id("bootstrap-admin-v2"); expect_changed(f, false);
    f = make_fixture(); f.config.store_uuid[0] ^= 1; expect_changed(f, true);
    f = make_fixture(); f.config.namespace_id[0] ^= 1; expect_changed(f, true);
    f = make_fixture(); ++f.config.policy_epoch; expect_changed(f, true);
    f = make_fixture(); f.config.checkpoint_lineage_id[0] ^= 1; expect_changed(f, true);
    f = make_fixture(); ++f.config.manifest_key_generation; expect_changed(f, true);
    f = make_fixture(); ++f.config.authority_epoch; expect_changed(f, true);

    halofpx::context_store_bootstrap_authority authority(base_fixture.config);
    for (auto changed_request : { request(92, 8), request(91, 9) }) {
        const auto & plan = require_plan(authority.plan(changed_request));
        assert(*plan.authority_snapshot_commitment() == base_snapshot);
        assert(*plan.authorization_commitment() != base_auth);
    }
    auto digest_request = request(); digest_request.selected_manifest_digest[0] ^= 1;
    const auto & digest_plan = require_plan(authority.plan(digest_request));
    assert(*digest_plan.authorization_commitment() != base_auth);
    assert(*digest_plan.anchor()->envelope_digest() != base_anchor);
    auto wide_attempt = request(); wide_attempt.attempt_id[0] = 1;
    const auto & wide_plan = require_plan(authority.plan(wide_attempt));
    assert(*wide_plan.authorization_commitment() != base_auth);
}

void test_determinism_and_concurrency() {
    auto f = make_fixture();
    halofpx::context_store_bootstrap_authority authority(f.config);
    const auto baseline_result = authority.plan(request());
    const auto baseline = *require_plan(baseline_result).authorization_commitment();
    std::array<halofpx::context_store_format_digest, 32> commitments {};
    std::array<halofpx::context_store_format_digest, 32> anchors {};
    std::vector<std::thread> threads;
    for (size_t i = 0; i < commitments.size(); ++i) threads.emplace_back([&, i] {
        const auto result = authority.plan(request());
        const auto & plan = require_plan(result);
        commitments[i] = *plan.authorization_commitment();
        anchors[i] = *plan.anchor()->envelope_digest();
    });
    for (auto & thread : threads) thread.join();
    for (size_t i = 0; i < commitments.size(); ++i) {
        assert(commitments[i] == baseline);
        assert(anchors[i] == *require_plan(baseline_result).anchor()->envelope_digest());
    }
    assert(std::string(halofpx::context_store_bootstrap_status_name(
        halofpx::context_store_bootstrap_status::authorized_unexecuted)) == "authorized-unexecuted");
}

} // namespace

int main() {
    test_authorized_plan_and_source_ownership();
    test_invalid_authority_and_purpose_separation();
    test_request_bounds_and_no_leakage();
    test_bindings_and_key_changes();
    test_determinism_and_concurrency();
}
