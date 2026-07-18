#include "halofpx-context-store.h"

#include <atomic>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstring>
#include <memory>
#include <thread>
#include <vector>

namespace {

class test_candidate final : public halofpx::context_store_candidate {
public:
    explicit test_candidate(halofpx::context_store_identity identity) : identity_(identity) {
    }

    const halofpx::context_store_identity & identity() const noexcept override {
        return identity_;
    }

private:
    halofpx::context_store_identity identity_;
};

class test_generation final : public halofpx::context_store_generation {
public:
    explicit test_generation(halofpx::context_store_identity identity) : identity_(identity) {
    }

    const halofpx::context_store_identity & identity() const noexcept override {
        return identity_;
    }

private:
    halofpx::context_store_identity identity_;
};

bool identity_equal(
        const halofpx::context_store_identity & lhs,
        const halofpx::context_store_identity & rhs) {
    return lhs.compatibility_root == rhs.compatibility_root &&
           lhs.scope_namespace == rhs.scope_namespace &&
           lhs.checkpoint_lineage_id == rhs.checkpoint_lineage_id &&
           lhs.policy_epoch == rhs.policy_epoch;
}

} // namespace

int main() {
    auto provider = halofpx::make_disabled_context_store_provider();
    assert(provider != nullptr);
    assert(std::strcmp(provider->name(), "disabled") == 0);

    const auto capabilities = provider->capabilities();
    assert(!capabilities.persistent_reads);
    assert(!capabilities.persistent_writes);
    assert(!capabilities.enumeration);
    assert(!capabilities.anonymous_scope);
    assert(!capabilities.shared_scope);
    assert(capabilities.admitted_state_profiles == 0);
    assert(capabilities.admitted_codecs == 0);

    halofpx::context_store_lookup_request zero_request;
    const auto zero_request_before = zero_request;
    auto zero_result = provider->lookup(zero_request);
    assert(zero_result.status() == halofpx::context_store_lookup_status::miss_disabled);
    assert(!zero_result.is_hit());
    assert(zero_result.candidate() == nullptr);
    assert(identity_equal(zero_request.identity, zero_request_before.identity));

    halofpx::context_store_lookup_request hostile_request;
    hostile_request.identity.compatibility_root.fill(0xff);
    hostile_request.identity.scope_namespace.fill(0xff);
    hostile_request.identity.checkpoint_lineage_id.fill(0xff);
    hostile_request.identity.policy_epoch = UINT64_MAX;
    const auto hostile_request_before = hostile_request;
    auto hostile_result = provider->lookup(hostile_request);
    assert(hostile_result.status() == halofpx::context_store_lookup_status::miss_disabled);
    assert(!hostile_result.is_hit());
    assert(hostile_result.candidate() == nullptr);
    assert(identity_equal(hostile_request.identity, hostile_request_before.identity));

    test_generation generation(hostile_request.identity);
    halofpx::context_store_publish_request publish_request { &generation };
    assert(provider->publish(publish_request) == halofpx::context_store_publish_status::disabled);
    assert(publish_request.generation == &generation);
    assert(identity_equal(generation.identity(), hostile_request.identity));

    auto invalid_hit = halofpx::context_store_lookup_result::hit(nullptr);
    assert(invalid_hit.status() == halofpx::context_store_lookup_status::miss_incomplete);
    assert(!invalid_hit.is_hit());

    auto invalid_miss = halofpx::context_store_lookup_result::miss(
        halofpx::context_store_lookup_status::hit);
    assert(invalid_miss.status() == halofpx::context_store_lookup_status::miss_incomplete);
    assert(!invalid_miss.is_hit());

    auto valid_hit = halofpx::context_store_lookup_result::hit(
        std::make_unique<test_candidate>(zero_request.identity));
    assert(valid_hit.status() == halofpx::context_store_lookup_status::hit);
    assert(valid_hit.is_hit());
    assert(valid_hit.candidate() != nullptr);
    assert(identity_equal(valid_hit.candidate()->identity(), zero_request.identity));

    std::atomic<bool> concurrent_ok { true };
    std::vector<std::thread> threads;
    for (int thread_id = 0; thread_id < 8; ++thread_id) {
        threads.emplace_back([&provider, &hostile_request, &concurrent_ok]() {
            for (int iteration = 0; iteration < 1000; ++iteration) {
                auto result = provider->lookup(hostile_request);
                if (result.status() != halofpx::context_store_lookup_status::miss_disabled ||
                    result.is_hit() || result.candidate() != nullptr) {
                    concurrent_ok.store(false);
                    return;
                }
                if (provider->publish({}) != halofpx::context_store_publish_status::disabled) {
                    concurrent_ok.store(false);
                    return;
                }
            }
        });
    }
    for (auto & thread : threads) {
        thread.join();
    }
    assert(concurrent_ok.load());

    return 0;
}
