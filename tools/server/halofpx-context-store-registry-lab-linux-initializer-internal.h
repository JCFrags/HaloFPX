#pragma once

#include "halofpx-context-store-registry-lab-wire.h"

#include <array>
#include <cstdint>

namespace halofpx {

// Internal L05t linkage seam only. It performs no filesystem access or mutation.
bool context_store_registry_lab_linux_initializer_predecessor_digest_v1(
    const uint8_t * data, size_t size, context_store_format_digest & output) noexcept;

namespace registry_lab::linux_initializer {

enum class sealed_input_status : std::uint8_t {
    predecessor_authenticated_pins_matched_no_root_access,
    transport_validated_no_root_access,
    invalid_request_no_mutation,
    unsupported_no_mutation,
    unavailable_no_mutation,
    io_failure_no_mutation,
};

struct sealed_input_request {
    context_store_registered_id expected_key_id;
    std::uint64_t expected_key_generation = 0;
    context_store_format_digest expected_predecessor_digest {};
    context_store_registered_id expected_registry_id;
    std::uint64_t expected_registry_epoch = 0;
    context_store_format_digest expected_authority_base_scope_commitment {};
    context_store_format_digest expected_policy_commitment {};
    std::uint64_t expected_predecessor_high_water = 0;
    context_store_format_digest expected_key_continuity_commitment {};
};

// This audit carries only nonsecret aggregate facts. It proves the exact fd4
// bytes authenticated under the supplied credential and matched the launcher
// pins; it is not issuer, origin, latestness, filesystem, initialization, or
// mutation authority.
struct sealed_input_audit {
    sealed_input_status result = sealed_input_status::invalid_request_no_mutation;
    bool secure_storage_locked = false;
    bool fd_table_unshared = false;
    bool exclusive_execution_context = false;
    bool credential_transport_validated = false;
    bool predecessor_transport_validated = false;
    bool descriptor_identities_distinct = false;
    bool descriptor_aliases_absent = false;
    bool expected_key_tuple_matched = false;
    bool predecessor_digest_matched = false;
    bool predecessor_authenticated_under_supplied_credential = false;
    bool launcher_receipt_matched = false;
    bool transport_final_revalidation_matched = false;
    bool descriptors_closed = false;
    bool secure_storage_wiped = false;
    bool secure_storage_unlocked = false;
    bool secure_storage_unmapped = false;
    bool signal_mask_restored = false;
    bool root_or_fixture_accessed = false;
    std::uint32_t input_syscall_count = 0;
    std::uint32_t root_or_fixture_syscall_count = 0;
    std::uint32_t credential_package_size = 0;
    std::uint32_t predecessor_envelope_size = 0;
};

// Consumes descriptors 3 and 4 once. It performs no parent/root/fixture syscall
// and returns no reusable credential/predecessor authority.
sealed_input_audit inspect_sealed_inputs_once(const sealed_input_request & input) noexcept;

} // namespace registry_lab::linux_initializer

} // namespace halofpx
