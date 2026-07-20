#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>

#include "halofpx-context-store-state-transformer-v1.h"

#include <array>
#include <cstdint>
#include <vector>

namespace {

class fake_state_api final : public halofpx::context_store_transformer_state_api_v1 {
public:
    size_t announced_size = 4;
    size_t capture_return = 4;
    size_t restore_return = 4;
    size_t get_size_calls = 0;
    size_t get_data_calls = 0;
    size_t set_data_calls = 0;
    llama_context * last_context = nullptr;
    llama_seq_id last_sequence = -1;
    llama_state_seq_flags last_flags = UINT32_MAX;
    std::vector<uint8_t> restored;

    size_t get_size(
            llama_context * ctx,
            llama_seq_id seq_id,
            llama_state_seq_flags flags) const noexcept override {
        auto & self = const_cast<fake_state_api &>(*this);
        ++self.get_size_calls;
        self.last_context = ctx;
        self.last_sequence = seq_id;
        self.last_flags = flags;
        return announced_size;
    }

    size_t get_data(
            llama_context * ctx,
            uint8_t * destination,
            size_t size,
            llama_seq_id seq_id,
            llama_state_seq_flags flags) const noexcept override {
        auto & self = const_cast<fake_state_api &>(*this);
        ++self.get_data_calls;
        self.last_context = ctx;
        self.last_sequence = seq_id;
        self.last_flags = flags;
        for (size_t index = 0; index < size; ++index) {
            destination[index] = static_cast<uint8_t>(0xa0 + index);
        }
        return capture_return;
    }

    size_t set_data(
            llama_context * ctx,
            const uint8_t * source,
            size_t size,
            llama_seq_id seq_id,
            llama_state_seq_flags flags) const noexcept override {
        auto & self = const_cast<fake_state_api &>(*this);
        ++self.set_data_calls;
        self.last_context = ctx;
        self.last_sequence = seq_id;
        self.last_flags = flags;
        self.restored.assign(source, source + size);
        return restore_return;
    }
};

llama_context * fake_context() {
    return reinterpret_cast<llama_context *>(static_cast<uintptr_t>(0x1234));
}

halofpx::context_store_identity identity(uint8_t seed = 1) {
    halofpx::context_store_identity value;
    value.compatibility_root.fill(seed);
    value.scope_namespace.fill(static_cast<uint8_t>(seed + 1));
    value.checkpoint_lineage_id.fill(static_cast<uint8_t>(seed + 2));
    value.policy_epoch = 9;
    return value;
}

halofpx::context_store_transformer_profile_v1 admitted_profile() {
    halofpx::context_store_transformer_profile_v1 profile;
    profile.target_only = true;
    profile.world_size = 1;
    profile.rank = 0;
    profile.architecture = halofpx::context_store_transformer_architecture_v1::transformer;
    profile.greedy_memoryless_sampling = true;
    return profile;
}

constexpr halofpx::context_store_transformer_limits_v1 limits { 64, 8 };
const std::array<llama_token, 3> tokens { 11, 22, 33 };

halofpx::context_store_transformer_capture_result_v1 capture(fake_state_api & api) {
    return halofpx::context_store_capture_transformer_state_v1_with_api(
        api, fake_context(), 7, tokens.data(), tokens.size(), identity(), admitted_profile(), limits);
}

void test_capture_and_restore() {
    fake_state_api api;
    const auto result = capture(api);
    assert(result.status == halofpx::context_store_transformer_status_v1::captured);
    assert(result.snapshot.tokens == std::vector<llama_token>(tokens.begin(), tokens.end()));
    assert((result.snapshot.state == std::vector<uint8_t>{ 0xa0, 0xa1, 0xa2, 0xa3 }));
    assert(api.get_size_calls == 1 && api.get_data_calls == 1);
    assert(api.last_context == fake_context() && api.last_sequence == 7);
    assert(api.last_flags == LLAMA_STATE_SEQ_FLAGS_NONE);

    const auto status = halofpx::context_store_restore_transformer_state_v1_with_api(
        api, fake_context(), 8, result.snapshot, tokens.data(), tokens.size(),
        identity(), admitted_profile(), limits);
    assert(status == halofpx::context_store_transformer_status_v1::restored);
    assert(api.set_data_calls == 1);
    assert(api.last_sequence == 8 && api.last_flags == LLAMA_STATE_SEQ_FLAGS_NONE);
    assert(api.restored == result.snapshot.state);
}

void test_profile_is_closed() {
    const auto accepted = admitted_profile();
    assert(halofpx::context_store_transformer_profile_v1_is_admitted(accepted));

    std::vector<halofpx::context_store_transformer_profile_v1> rejected;
    auto add = [&](auto mutate) {
        auto profile = accepted;
        mutate(profile);
        rejected.push_back(profile);
    };
    add([](auto & value) { value.target_only = false; });
    add([](auto & value) { value.world_size = 2; });
    add([](auto & value) { value.rank = 1; });
    add([](auto & value) { value.architecture = halofpx::context_store_transformer_architecture_v1::recurrent; });
    add([](auto & value) { value.architecture = halofpx::context_store_transformer_architecture_v1::hybrid; });
    add([](auto & value) { value.has_draft_context = true; });
    add([](auto & value) { value.has_speculative_state = true; });
    add([](auto & value) { value.has_mtp_state = true; });
    add([](auto & value) { value.has_multimodal_state = true; });
    add([](auto & value) { value.has_adapters = true; });
    add([](auto & value) { value.has_grammar_state = true; });
    add([](auto & value) { value.has_tool_state = true; });
    add([](auto & value) { value.has_sampler_state = true; });
    add([](auto & value) { value.greedy_memoryless_sampling = false; });

    for (const auto & profile : rejected) {
        fake_state_api api;
        const auto result = halofpx::context_store_capture_transformer_state_v1_with_api(
            api, fake_context(), 0, tokens.data(), tokens.size(), identity(), profile, limits);
        assert(result.status == halofpx::context_store_transformer_status_v1::unsupported_profile);
        assert(api.get_size_calls == 0 && api.get_data_calls == 0);
    }
}

void test_capture_limits_and_partial_failure() {
    fake_state_api api;
    auto result = halofpx::context_store_capture_transformer_state_v1_with_api(
        api, fake_context(), 0, tokens.data(), 0, identity(), admitted_profile(), limits);
    assert(result.status == halofpx::context_store_transformer_status_v1::empty_tokens);
    assert(api.get_size_calls == 0);

    const halofpx::context_store_transformer_limits_v1 token_cap { 64, 2 };
    result = halofpx::context_store_capture_transformer_state_v1_with_api(
        api, fake_context(), 0, tokens.data(), tokens.size(), identity(), admitted_profile(), token_cap);
    assert(result.status == halofpx::context_store_transformer_status_v1::token_limit_exceeded);
    assert(api.get_size_calls == 0);

    api.announced_size = 65;
    result = capture(api);
    assert(result.status == halofpx::context_store_transformer_status_v1::state_limit_exceeded);
    assert(api.get_data_calls == 0);

    api.announced_size = 4;
    api.capture_return = 3;
    result = capture(api);
    assert(result.status == halofpx::context_store_transformer_status_v1::state_capture_failed);
    assert(result.snapshot.state.empty());
    assert(result.snapshot.tokens.empty());
}

void expect_restore_rejected_without_mutation(
        fake_state_api & api,
        const halofpx::context_store_transformer_snapshot_v1 & snapshot,
        const llama_token * expected,
        size_t expected_count,
        const halofpx::context_store_identity & expected_identity,
        const halofpx::context_store_transformer_profile_v1 & expected_profile,
        const halofpx::context_store_transformer_limits_v1 & expected_limits,
        halofpx::context_store_transformer_status_v1 status) {
    const size_t calls_before = api.set_data_calls;
    assert(halofpx::context_store_restore_transformer_state_v1_with_api(
        api, fake_context(), 0, snapshot, expected, expected_count,
        expected_identity, expected_profile, expected_limits) == status);
    assert(api.set_data_calls == calls_before);
}

void test_restore_prevalidation_never_mutates() {
    fake_state_api api;
    const auto good = capture(api).snapshot;

    expect_restore_rejected_without_mutation(
        api, good, tokens.data(), tokens.size(), identity(4), admitted_profile(), limits,
        halofpx::context_store_transformer_status_v1::incompatible_identity);

    auto different_profile = admitted_profile();
    different_profile.has_adapters = true;
    expect_restore_rejected_without_mutation(
        api, good, tokens.data(), tokens.size(), identity(), different_profile, limits,
        halofpx::context_store_transformer_status_v1::unsupported_profile);

    auto corrupt_profile = good;
    corrupt_profile.profile.has_mtp_state = true;
    expect_restore_rejected_without_mutation(
        api, corrupt_profile, tokens.data(), tokens.size(), identity(), admitted_profile(), limits,
        halofpx::context_store_transformer_status_v1::incompatible_profile);

    auto wrong_tokens = tokens;
    wrong_tokens[1] = 99;
    expect_restore_rejected_without_mutation(
        api, good, wrong_tokens.data(), wrong_tokens.size(), identity(), admitted_profile(), limits,
        halofpx::context_store_transformer_status_v1::token_mismatch);

    auto no_state = good;
    no_state.state.clear();
    expect_restore_rejected_without_mutation(
        api, no_state, tokens.data(), tokens.size(), identity(), admitted_profile(), limits,
        halofpx::context_store_transformer_status_v1::state_unavailable);

    const halofpx::context_store_transformer_limits_v1 state_cap { 3, 8 };
    expect_restore_rejected_without_mutation(
        api, good, tokens.data(), tokens.size(), identity(), admitted_profile(), state_cap,
        halofpx::context_store_transformer_status_v1::state_limit_exceeded);
}

void test_restore_failure_is_typed() {
    fake_state_api api;
    const auto good = capture(api).snapshot;
    api.restore_return = 3;
    assert(halofpx::context_store_restore_transformer_state_v1_with_api(
        api, fake_context(), 0, good, tokens.data(), tokens.size(),
        identity(), admitted_profile(), limits) ==
        halofpx::context_store_transformer_status_v1::state_restore_failed);
    assert(api.set_data_calls == 1);
}

} // namespace

int main() {
    test_capture_and_restore();
    test_profile_is_closed();
    test_capture_limits_and_partial_failure();
    test_restore_prevalidation_never_mutates();
    test_restore_failure_is_typed();
    return 0;
}
