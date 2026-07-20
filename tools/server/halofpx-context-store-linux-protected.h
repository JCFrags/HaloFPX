#pragma once

#include "halofpx-context-store-linux-direct.h"
#include "halofpx-context-store-protected-canary-anchor.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace halofpx {

constexpr size_t context_store_linux_protected_operator_key_bytes = 32;

struct context_store_linux_protected_root_authority {
    context_store_format_digest scope_key {};
    context_store_format_digest direct_root_key {};
    context_store_format_digest anchor_root_key {};
};

bool context_store_linux_protected_derive_root_authority(
    const uint8_t * operator_key,
    size_t operator_key_size,
    const std::array<uint8_t, 16> & store_uuid,
    context_store_linux_protected_root_authority & output) noexcept;

enum class context_store_linux_protected_test_failpoint : uint8_t {
    none,
    ambiguous_before_anchor_rename,
    ambiguous_after_anchor_rename,
};

struct context_store_linux_protected_config {
    context_store_linux_direct_config direct;
    const char * anchor_root_path = nullptr;
    context_store_linux_direct_root_identity expected_anchor_root {};
    std::array<uint8_t, 16> store_uuid {};
    const uint8_t * direct_root_key = nullptr;
    size_t direct_root_key_size = 0;
    const uint8_t * anchor_root_key = nullptr;
    size_t anchor_root_key_size = 0;
    context_store_linux_protected_test_failpoint test_failpoint =
        context_store_linux_protected_test_failpoint::none;
};

enum class context_store_linux_protected_open_status : uint8_t {
    opened,
    invalid_configuration,
    roots_rejected,
    writer_busy,
    direct_unavailable,
    unsupported,
    io_error,
};

enum class context_store_linux_protected_lookup_status : uint8_t {
    hit,
    miss_not_found,
    miss_unanchored,
    miss_incompatible,
    miss_corrupt,
    lineage_quarantined,
    invalid_request,
    unavailable,
};

enum class context_store_linux_protected_publish_status : uint8_t {
    published,
    recovered_durable,
    already_exists,
    conflict,
    unreachable,
    lineage_quarantined,
    invalid_request,
    quota_exceeded,
    reserve_exhausted,
    unavailable,
    unsupported,
    io_error,
};

class context_store_linux_protected {
public:
    context_store_linux_protected() noexcept = default;
    ~context_store_linux_protected() noexcept;
    context_store_linux_protected(const context_store_linux_protected &) = delete;
    context_store_linux_protected & operator=(const context_store_linux_protected &) = delete;
    context_store_linux_protected(context_store_linux_protected &&) noexcept;
    context_store_linux_protected & operator=(context_store_linux_protected &&) noexcept;

    bool available() const noexcept;

    context_store_linux_protected_lookup_status lookup(
        const context_store_format_digest & scope,
        const context_store_format_digest & session,
        const context_store_format_digest & compatibility,
        context_store_linux_direct_value & output) const noexcept;

    context_store_linux_protected_publish_status publish(
        const context_store_format_digest & scope,
        const context_store_format_digest & session,
        const context_store_format_digest & compatibility,
        const int32_t * tokens,
        size_t token_count,
        const uint8_t * state,
        size_t state_size) noexcept;

private:
    context_store_linux_direct direct_;
    int anchor_root_fd_ = -1;
    int anchor_staging_fd_ = -1;
    int anchor_lock_fd_ = -1;
    uint64_t anchor_device_ = 0;
    uint64_t anchor_mount_id_ = 0;
    uint32_t anchor_owner_uid_ = 0;
    std::array<uint8_t, 16> store_uuid_ {};
    context_store_format_digest direct_root_key_ {};
    context_store_format_digest anchor_root_key_ {};
    context_store_linux_protected_test_failpoint test_failpoint_ =
        context_store_linux_protected_test_failpoint::none;

    friend context_store_linux_protected_open_status context_store_linux_protected_open(
        const context_store_linux_protected_config &,
        context_store_linux_protected &) noexcept;
};

context_store_linux_protected_open_status context_store_linux_protected_open(
    const context_store_linux_protected_config & config,
    context_store_linux_protected & output) noexcept;

const char * context_store_linux_protected_open_status_name(
    context_store_linux_protected_open_status status) noexcept;
const char * context_store_linux_protected_lookup_status_name(
    context_store_linux_protected_lookup_status status) noexcept;
const char * context_store_linux_protected_publish_status_name(
    context_store_linux_protected_publish_status status) noexcept;

} // namespace halofpx
