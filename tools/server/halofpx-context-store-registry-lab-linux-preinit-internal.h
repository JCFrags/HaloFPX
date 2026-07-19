#ifndef HALOFPX_CONTEXT_STORE_REGISTRY_LAB_LINUX_PREINIT_INTERNAL_H
#define HALOFPX_CONTEXT_STORE_REGISTRY_LAB_LINUX_PREINIT_INTERNAL_H

#include <cstddef>
#include <cstdint>

namespace halofpx::registry_lab::linux_preinit {

constexpr std::size_t max_path_bytes = 4096;
constexpr std::size_t max_key_id_bytes = 128;
constexpr std::uint64_t future_logical_authority_bound = 16ULL * 1024ULL * 1024ULL;
constexpr std::uint64_t required_filesystem_reserve = 256ULL * 1024ULL * 1024ULL;

enum class status : std::uint8_t {
    ok_non_authoritative,
    invalid_request,
    unsupported,
    busy,
    unavailable,
    io_failure,
};

struct pinned_path_identity {
    char canonical_path[max_path_bytes + 1]{};
    std::uint32_t path_length = 0;
    std::uint64_t device = 0;
    std::uint64_t inode = 0;
    std::uint64_t mount_id = 0;
    std::uint64_t owner_uid = 0;
    std::uint32_t mode = 0;
    std::uint8_t filesystem_uuid[16]{};
    std::uint8_t subvolume_uuid[16]{};
};

struct request {
    pinned_path_identity parent{};
    pinned_path_identity candidate_root{};
    pinned_path_identity fixture{};
    std::uint64_t fixture_lock_device = 0;
    std::uint64_t fixture_lock_inode = 0;
    char expected_key_id[max_key_id_bytes + 1]{};
    std::uint16_t expected_key_id_length = 0;
    std::uint64_t expected_key_generation = 0;
};

// This audit is deliberately non-reusable: it exposes only public comparison,
// count, and ordering facts. It is not registry or mutation authority.
struct audit {
    status result = status::invalid_request;
    bool credential_admitted = false;
    bool credential_preceded_root_access = false;
    bool expected_key_tuple_matched = false;
    bool parent_identity_matched = false;
    bool candidate_root_identity_matched = false;
    bool fixture_identity_matched = false;
    bool fixture_lock_identity_matched = false;
    bool candidate_root_empty = false;
    bool fixture_layout_exact = false;
    bool filesystem_not_reported_read_only = false;
    bool ofd_lock_acquired = false;
    bool credential_scratch_wiped = false;
    bool credential_owner_wiped = false;
    bool credential_owner_unlocked = false;
    std::uint32_t credential_syscall_count = 0;
    std::uint32_t root_fixture_syscall_count = 0;
    std::uint32_t ofd_attempt_count = 0;
    std::uint64_t observed_filesystem_reserve = 0;
    std::uint64_t logical_authority_bound = future_logical_authority_bound;
};

// Consumes the process's single L05s session. Descriptor 3 is the only admitted
// credential channel and descriptor 4 must be absent. The call never throws.
audit qualify_once(const request & input) noexcept;

} // namespace halofpx::registry_lab::linux_preinit

#endif
