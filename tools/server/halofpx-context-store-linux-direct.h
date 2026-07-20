#pragma once

#include "halofpx-context-store-auth.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace halofpx {

constexpr size_t context_store_linux_direct_master_key_bytes = 32;
constexpr size_t context_store_linux_direct_max_entries_limit = 64;
constexpr size_t context_store_linux_direct_max_tokens = 1024 * 1024;
constexpr size_t context_store_linux_direct_max_state_bytes = 512ULL * 1024 * 1024;

struct context_store_linux_direct_root_identity {
    uint64_t device = 0;
    uint64_t mount_id = 0;
    uint32_t owner_uid = 0;
};

enum class context_store_linux_direct_identity_status : uint8_t {
    inspected,
    invalid_path,
    unsupported,
    rejected,
    io_error,
};

context_store_linux_direct_identity_status context_store_linux_direct_inspect_root(
    const char * path,
    context_store_linux_direct_root_identity & output) noexcept;

struct context_store_linux_direct_config {
    const char * root_path = nullptr;
    const uint8_t * master_key = nullptr;
    size_t master_key_size = 0;
    uint64_t quota_bytes = 0;
    uint64_t reserve_bytes = 0;
    size_t max_entries = 0;
    context_store_linux_direct_root_identity expected_root {};
};

enum class context_store_linux_direct_open_status : uint8_t {
    opened,
    invalid_configuration,
    root_rejected,
    writer_busy,
    accounting_rejected,
    unsupported,
    io_error,
};

enum class context_store_linux_direct_lookup_status : uint8_t {
    hit,
    miss_not_found,
    miss_incompatible,
    miss_corrupt,
    invalid_request,
    unavailable,
};

enum class context_store_linux_direct_publish_status : uint8_t {
    published,
    already_exists,
    conflict,
    invalid_request,
    quota_exceeded,
    reserve_exhausted,
    unavailable,
    unsupported,
    io_error,
};

struct context_store_linux_direct_value {
    std::vector<int32_t> tokens;
    std::vector<uint8_t> state;
};

class context_store_linux_direct {
public:
    context_store_linux_direct() noexcept = default;
    ~context_store_linux_direct() noexcept;
    context_store_linux_direct(const context_store_linux_direct &) = delete;
    context_store_linux_direct & operator=(const context_store_linux_direct &) = delete;
    context_store_linux_direct(context_store_linux_direct &&) noexcept;
    context_store_linux_direct & operator=(context_store_linux_direct &&) noexcept;

    bool available() const noexcept;
    uint64_t accounted_bytes() const noexcept;
    size_t entry_count() const noexcept;

    context_store_linux_direct_lookup_status lookup(
        const context_store_format_digest & scope,
        const context_store_format_digest & session,
        const context_store_format_digest & compatibility,
        context_store_linux_direct_value & output) const noexcept;

    context_store_linux_direct_publish_status publish(
        const context_store_format_digest & scope,
        const context_store_format_digest & session,
        const context_store_format_digest & compatibility,
        const int32_t * tokens,
        size_t token_count,
        const uint8_t * state,
        size_t state_size) noexcept;

private:
    int root_fd_ = -1;
    int staging_fd_ = -1;
    int lock_fd_ = -1;
    uint64_t device_ = 0;
    uint64_t mount_id_ = 0;
    uint32_t owner_uid_ = 0;
    uint64_t quota_bytes_ = 0;
    uint64_t reserve_bytes_ = 0;
    uint64_t accounted_bytes_ = 0;
    size_t max_entries_ = 0;
    size_t entry_count_ = 0;
    context_store_format_digest master_key_ {};

    friend context_store_linux_direct_open_status context_store_linux_direct_open(
        const context_store_linux_direct_config &, context_store_linux_direct &) noexcept;
};

context_store_linux_direct_open_status context_store_linux_direct_open(
    const context_store_linux_direct_config & config,
    context_store_linux_direct & output) noexcept;

const char * context_store_linux_direct_open_status_name(
    context_store_linux_direct_open_status status) noexcept;
const char * context_store_linux_direct_lookup_status_name(
    context_store_linux_direct_lookup_status status) noexcept;
const char * context_store_linux_direct_publish_status_name(
    context_store_linux_direct_publish_status status) noexcept;

} // namespace halofpx
