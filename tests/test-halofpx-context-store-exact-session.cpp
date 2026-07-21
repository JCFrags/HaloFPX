#ifdef NDEBUG
#undef NDEBUG
#endif

#include "halofpx-context-store-exact-session.h"
#include "halofpx-context-store-scope.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace {

using halofpx::context_store_exact_session_inputs_v1;
using halofpx::context_store_exact_session_status_v1;
using halofpx::context_store_format_digest;

template <size_t Size>
halofpx::context_store_scope_bytes view(const std::array<uint8_t, Size> & value) noexcept {
    return { value.data(), value.size() };
}

halofpx::context_store_scope_bytes view(std::string_view value) noexcept {
    return { reinterpret_cast<const uint8_t *>(value.data()), value.size() };
}

bool zero(const context_store_format_digest & value) noexcept {
    return std::all_of(value.begin(), value.end(), [](uint8_t byte) { return byte == 0; });
}

struct fixture {
    std::array<uint8_t, 32> key {};
    std::array<int32_t, 6> tokens { 1, 23, 24, 255, 256, 70000 };
    context_store_exact_session_inputs_v1 value;

    fixture() {
        for (size_t i = 0; i < key.size(); ++i) key[i] = static_cast<uint8_t>(i);
        value.derivation_key = { key.data(), key.size() };
        for (size_t i = 0; i < value.scope_namespace.size(); ++i) {
            value.scope_namespace[i] = static_cast<uint8_t>(0x20 + i);
            value.compatibility_root[i] = static_cast<uint8_t>(0x40 + i);
            value.global_plan_digest[i] = static_cast<uint8_t>(0x60 + i);
            value.rank_ownership_digest[i] = static_cast<uint8_t>(0x80 + i);
            value.rank_placement_digest[i] = static_cast<uint8_t>(0xa0 + i);
        }
        value.tokens = tokens.data();
        value.token_count = tokens.size();
        value.logical_boundary = tokens.size();
        value.output_boundary = 4;
        value.profile = halofpx::context_store_exact_session_profile_v1::target_only_greedy_memoryless;
        value.topology_epoch = 7;
        value.world_size = 2;
        value.rank = 1;
    }
};

void expect_rejected(
        const context_store_exact_session_inputs_v1 & value,
        context_store_exact_session_status_v1 expected) {
    const auto result = halofpx::context_store_resolve_exact_session_v1(value);
    assert(result.status == expected);
    assert(!result.resolved());
    assert(zero(result.session_id));
}

void deterministic_independent_golden() {
    fixture source;
    const context_store_format_digest expected = {
        0x90,0x24,0x01,0x52,0xfe,0x04,0x49,0xba,
        0x92,0xa1,0x74,0x6d,0xcd,0xf8,0x04,0xd7,
        0xcc,0xa5,0x5f,0x00,0x34,0xd7,0xa9,0x6d,
        0x84,0x6c,0xc5,0x9d,0x87,0xf1,0xa2,0x5c,
    };
    const auto first = halofpx::context_store_resolve_exact_session_v1(source.value);
    const auto second = halofpx::context_store_resolve_exact_session_v1(source.value);
    assert(first.resolved() && second.resolved());
    assert(first.session_id == second.session_id);
    assert(first.session_id == expected);
}

void every_binding_mutates_identifier() {
    fixture source;
    const auto baseline = halofpx::context_store_resolve_exact_session_v1(source.value).session_id;
    auto changed = source.value;
    auto changed_key = source.key;
    changed_key[0] ^= 1;
    changed.derivation_key = { changed_key.data(), changed_key.size() };
    assert(halofpx::context_store_resolve_exact_session_v1(changed).session_id != baseline);

#define HALOFPX_MUTATE_DIGEST(field) do { \
    changed = source.value; changed.field[0] ^= 1; \
    assert(halofpx::context_store_resolve_exact_session_v1(changed).session_id != baseline); \
} while (false)
    HALOFPX_MUTATE_DIGEST(scope_namespace);
    HALOFPX_MUTATE_DIGEST(compatibility_root);
    HALOFPX_MUTATE_DIGEST(global_plan_digest);
    HALOFPX_MUTATE_DIGEST(rank_ownership_digest);
    HALOFPX_MUTATE_DIGEST(rank_placement_digest);
#undef HALOFPX_MUTATE_DIGEST

    fixture changed_tokens;
    changed_tokens.tokens[2] ^= 1;
    assert(halofpx::context_store_resolve_exact_session_v1(changed_tokens.value).session_id != baseline);
    changed = source.value; changed.output_boundary = 3;
    assert(halofpx::context_store_resolve_exact_session_v1(changed).session_id != baseline);
    changed = source.value; changed.topology_epoch = 8;
    assert(halofpx::context_store_resolve_exact_session_v1(changed).session_id != baseline);
    changed = source.value; changed.world_size = 3;
    assert(halofpx::context_store_resolve_exact_session_v1(changed).session_id != baseline);
    changed = source.value; changed.rank = 0;
    assert(halofpx::context_store_resolve_exact_session_v1(changed).session_id != baseline);
}

void invalid_zero_and_overflow_inputs() {
    fixture source;
    auto changed = source.value;
    changed.derivation_key = {};
    expect_rejected(changed, context_store_exact_session_status_v1::invalid_key);
    std::array<uint8_t, 32> zero_key {};
    changed = source.value; changed.derivation_key = { zero_key.data(), zero_key.size() };
    expect_rejected(changed, context_store_exact_session_status_v1::invalid_key);
    changed = source.value; changed.scope_namespace.fill(0);
    expect_rejected(changed, context_store_exact_session_status_v1::invalid_scope_namespace);
    changed = source.value; changed.compatibility_root.fill(0);
    expect_rejected(changed, context_store_exact_session_status_v1::invalid_compatibility_root);
    changed = source.value; changed.tokens = nullptr;
    expect_rejected(changed, context_store_exact_session_status_v1::invalid_tokens);
    changed = source.value; changed.token_count = 0;
    expect_rejected(changed, context_store_exact_session_status_v1::invalid_tokens);
    changed = source.value; changed.token_count = halofpx::context_store_exact_session_max_tokens + 1ULL;
    expect_rejected(changed, context_store_exact_session_status_v1::invalid_tokens);
    fixture negative; negative.tokens[0] = -1;
    expect_rejected(negative.value, context_store_exact_session_status_v1::invalid_tokens);
    changed = source.value; changed.logical_boundary = 0;
    expect_rejected(changed, context_store_exact_session_status_v1::invalid_boundaries);
    changed = source.value; changed.logical_boundary = UINT64_MAX;
    expect_rejected(changed, context_store_exact_session_status_v1::invalid_boundaries);
    changed = source.value; changed.output_boundary = 0;
    expect_rejected(changed, context_store_exact_session_status_v1::invalid_boundaries);
    changed = source.value; changed.output_boundary = changed.logical_boundary + 1;
    expect_rejected(changed, context_store_exact_session_status_v1::invalid_boundaries);
    changed = source.value;
    changed.profile = halofpx::context_store_exact_session_profile_v1::unset;
    expect_rejected(changed, context_store_exact_session_status_v1::invalid_profile);
    changed = source.value;
    changed.profile = static_cast<halofpx::context_store_exact_session_profile_v1>(2);
    expect_rejected(changed, context_store_exact_session_status_v1::invalid_profile);
    changed = source.value; changed.global_plan_digest.fill(0);
    expect_rejected(changed, context_store_exact_session_status_v1::invalid_topology_digest);
    changed = source.value; changed.rank_ownership_digest.fill(0);
    expect_rejected(changed, context_store_exact_session_status_v1::invalid_topology_digest);
    changed = source.value; changed.rank_placement_digest.fill(0);
    expect_rejected(changed, context_store_exact_session_status_v1::invalid_topology_digest);
    changed = source.value; changed.topology_epoch = 0;
    expect_rejected(changed, context_store_exact_session_status_v1::invalid_topology);
    changed = source.value; changed.world_size = 0;
    expect_rejected(changed, context_store_exact_session_status_v1::invalid_topology);
    changed = source.value; changed.world_size = halofpx::context_store_manifest_max_ranks + 1ULL;
    expect_rejected(changed, context_store_exact_session_status_v1::invalid_topology);
    changed = source.value; changed.rank = changed.world_size;
    expect_rejected(changed, context_store_exact_session_status_v1::invalid_topology);
}

void token_boundaries_are_unambiguous() {
    fixture left;
    const std::array<int32_t, 2> left_tokens { 1, 23 };
    left.value.tokens = left_tokens.data(); left.value.token_count = 2;
    left.value.logical_boundary = 2; left.value.output_boundary = 1;
    fixture right;
    const std::array<int32_t, 3> right_tokens { 1, 2, 3 };
    right.value.tokens = right_tokens.data(); right.value.token_count = 3;
    right.value.logical_boundary = 3; right.value.output_boundary = 1;
    const auto a = halofpx::context_store_resolve_exact_session_v1(left.value);
    const auto b = halofpx::context_store_resolve_exact_session_v1(right.value);
    assert(a.resolved() && b.resolved() && a.session_id != b.session_id);
}

context_store_format_digest private_scope(
        const std::array<uint8_t, 32> & key,
        std::string_view principal,
        const context_store_format_digest & compatibility) {
    const halofpx::context_store_scope_policy_v1 policy {
        view(key), view("l10b-policy"), view("test-issuer"), view(principal),
        view("private"), 1, compatibility,
    };
    const auto result = halofpx::context_store_resolve_private_scope_v1(policy);
    assert(result.resolved());
    return result.namespace_id;
}

void principal_changes_only_through_opaque_scope() {
    fixture source;
    const auto alice_scope = private_scope(source.key, "alice", source.value.compatibility_root);
    const auto bob_scope = private_scope(source.key, "bob", source.value.compatibility_root);
    auto alice = source.value; alice.scope_namespace = alice_scope;
    auto bob = source.value; bob.scope_namespace = bob_scope;
    const auto a = halofpx::context_store_resolve_exact_session_v1(alice);
    const auto b = halofpx::context_store_resolve_exact_session_v1(bob);
    assert(a.resolved() && b.resolved() && a.session_id != b.session_id);
}

} // namespace

int main() {
    deterministic_independent_golden();
    every_binding_mutates_identifier();
    invalid_zero_and_overflow_inputs();
    token_boundaries_are_unambiguous();
    principal_changes_only_through_opaque_scope();
    return 0;
}
