#pragma once

#include "halofpx-context-store-registry-lab-wire.h"

#include <array>
#include <cstddef>
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

constexpr std::size_t initialization_max_path_bytes = 4096;
constexpr std::uint64_t initialization_future_logical_authority_bound =
    16ULL * 1024ULL * 1024ULL;
constexpr std::uint64_t initialization_required_filesystem_reserve =
    256ULL * 1024ULL * 1024ULL;

enum class initialization_status : std::uint8_t {
    initialization_discard_required,
    preexisting_root_discard_required,
    invalid_request_no_mutation,
    unsupported_no_mutation,
    busy_no_mutation,
    unavailable_no_mutation,
    io_failure_no_mutation,
};

struct initialization_pinned_path_identity {
    char canonical_path[initialization_max_path_bytes + 1]{};
    std::uint32_t path_length = 0;
    std::uint64_t device = 0;
    std::uint64_t inode = 0;
    std::uint64_t mount_id = 0;
    std::uint64_t owner_uid = 0;
    std::uint32_t mode = 0;
    std::uint8_t filesystem_uuid[16]{};
    std::uint8_t subvolume_uuid[16]{};
};

struct initialization_request {
    sealed_input_request sealed_inputs{};
    initialization_pinned_path_identity parent{};
    initialization_pinned_path_identity candidate_root{};
    initialization_pinned_path_identity fixture{};
    std::uint64_t fixture_lock_device = 0;
    std::uint64_t fixture_lock_inode = 0;
};

// L05w/L05x expose only public comparison, ordering, generated-identity, and
// narrow discard-only construction facts.
// Even a clean anchor qualification is discard-required and returns no fd,
// absence proof, credential, predecessor, or reusable mutation authority.
struct initialization_audit {
    initialization_status result = initialization_status::invalid_request_no_mutation;
    sealed_input_audit sealed_inputs{};
    bool sealed_inputs_preceded_root_access = false;
    bool parent_identity_matched = false;
    bool candidate_root_identity_matched = false;
    bool fixture_identity_matched = false;
    bool fixture_lock_identity_matched = false;
    bool fixture_lock_acquired = false;
    bool root_guard_acquired = false;
    bool empty_root_final_revalidation_matched = false;
    bool generated_root_id_nonzero = false;
    bool generated_store_uuid_nonzero = false;
    bool generated_identity_scratch_wiped = false;
    bool discard_required_latched = false;
    bool writer_lock_created_no_replace = false;
    bool writer_lock_mode_revalidated = false;
    bool writer_lock_synchronized = false;
    bool writer_lock_ofd_acquired = false;
    bool writer_lock_root_sole_entry = false;
    bool lock_anchor_qualified = false;
    bool envelopes_directory_created_no_replace = false;
    bool envelopes_directory_validated = false;
    bool envelopes_directory_synchronized = false;
    bool attempts_directory_created_no_replace = false;
    bool attempts_directory_validated = false;
    bool attempts_directory_synchronized = false;
    bool staging_directory_created_no_replace = false;
    bool staging_directory_validated = false;
    bool staging_directory_synchronized = false;
    bool envelopes_directory_final_revalidation_matched = false;
    bool attempts_directory_final_revalidation_matched = false;
    bool staging_directory_final_revalidation_matched = false;
    bool root_directory_synchronized = false;
    bool directory_prefix_qualified = false;
    bool writer_lock_released = false;
    bool fixture_lock_released = false;
    bool root_guard_released = false;
    bool filesystem_not_reported_read_only = false;
    std::array<std::uint8_t, 32> generated_root_id{};
    std::array<std::uint8_t, 16> generated_store_uuid{};
    std::uint32_t root_fixture_syscall_count = 0;
    std::uint32_t fixture_ofd_attempt_count = 0;
    std::uint32_t writer_ofd_attempt_count = 0;
    std::uint64_t observed_filesystem_reserve = 0;
    std::uint64_t logical_authority_bound =
        initialization_future_logical_authority_bound;
};

// Consumes fd 3/fd 4 through the same file-private admission routine used by
// inspect_sealed_inputs_once(), before any parent/root/fixture syscall. It may
// create only writer.lock and always classifies a latched invocation as
// discard-required.
initialization_audit initialize_writer_lock_anchor_once(
    const initialization_request & input) noexcept;

// L05x has the same admission and writer-lock anchor as L05w, then continues
// only far enough to create, synchronize, and validate the fixed empty directory
// prefix. Every post-latch outcome remains discard-required.
initialization_audit initialize_directory_prefix_once(
    const initialization_request & input) noexcept;

} // namespace registry_lab::linux_initializer

} // namespace halofpx
