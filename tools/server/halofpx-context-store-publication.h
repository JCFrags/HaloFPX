#pragma once

#include "halofpx-context-store.h"

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>

namespace halofpx {

using context_store_publication_id = std::array<uint8_t, 32>;

// Matches the accepted v1 manifest object bound. The offline coordinator
// rejects larger synthetic plans before calling an injected backend.
constexpr size_t context_store_publication_max_objects_v1 = 128;

struct context_store_publication_anchor {
    context_store_publication_id store_id {};
    context_store_publication_id namespace_id {};
    context_store_publication_id checkpoint_lineage_id {};
    uint64_t policy_epoch = 0;
    uint64_t key_generation = 0;
    uint64_t authority_epoch = 0;
    uint64_t generation = 0;
    context_store_digest manifest_digest {};
    context_store_digest predecessor_manifest_digest {};
};

struct context_store_publication_request {
    context_store_publication_anchor expected_predecessor;
    context_store_publication_anchor next;
    size_t object_count = 0;
};

enum class context_store_publication_step_result : uint8_t {
    ok,
    already_equal,
    conflict,
    no_space,
    quota_exhausted,
    reserve_exhausted,
    read_only,
    io_error,
    interrupted,
    storage_error,
    sync_error,
};

// Offline injection seam. L05a intentionally provides no concrete filesystem
// implementation. A later backend must prove every primitive's no-follow,
// no-replace, same-filesystem, exact-byte, and synchronization semantics.
class context_store_publication_backend {
public:
    virtual ~context_store_publication_backend();

    virtual context_store_publication_step_result read_anchor(
        context_store_publication_anchor & anchor) = 0;

    virtual context_store_publication_step_result stage_object(size_t index) = 0;
    virtual context_store_publication_step_result write_object(size_t index) = 0;
    virtual context_store_publication_step_result verify_object(size_t index) = 0;
    virtual context_store_publication_step_result sync_object_file(size_t index) = 0;
    virtual context_store_publication_step_result publish_object_no_replace(size_t index) = 0;
    virtual context_store_publication_step_result sync_object_directory(size_t index) = 0;

    virtual context_store_publication_step_result stage_manifest() = 0;
    virtual context_store_publication_step_result write_manifest() = 0;
    // Success must return the digest of the exact canonical authenticated
    // manifest bytes that were verified and are about to be published.
    virtual context_store_publication_step_result verify_manifest(
        context_store_digest & verified_digest) = 0;
    virtual context_store_publication_step_result sync_manifest_file() = 0;
    virtual context_store_publication_step_result publish_manifest_no_replace() = 0;
    virtual context_store_publication_step_result sync_manifest_directory() = 0;

    virtual context_store_publication_step_result replace_anchor_atomically(
        const context_store_publication_anchor & next) = 0;
    virtual context_store_publication_step_result sync_anchor() = 0;
};

enum class context_store_publication_status : uint8_t {
    published,
    invalid_request,
    stale_predecessor,
    writer_busy,
    object_collision,
    manifest_collision,
    manifest_identity_mismatch,
    storage_error,
    sync_error,
    anchor_visibility_uncertain,
};

struct context_store_publication_result {
    context_store_publication_status status = context_store_publication_status::invalid_request;
    size_t completed_steps = 0;
    bool anchor_replaced = false;
    bool durability_acknowledged = false;
};

// One noncopyable instance must be owned by each configured publication root.
// Every in-process coordinator for that root must share it. A later concrete
// backend must separately prove cross-process and stale-attempt fencing.
class context_store_publication_root_fence {
public:
    context_store_publication_root_fence() = default;
    context_store_publication_root_fence(const context_store_publication_root_fence &) = delete;
    context_store_publication_root_fence & operator=(const context_store_publication_root_fence &) = delete;

private:
    friend class context_store_publication_writer;
    std::atomic_flag active_ = ATOMIC_FLAG_INIT;
};

// Synchronous coordinator. It owns no path, bytes, key, thread, queue, or
// backend. Exceptions are caught and cannot escape this boundary.
class context_store_publication_writer {
public:
    explicit context_store_publication_writer(
        context_store_publication_root_fence & root_fence) noexcept;

    context_store_publication_result publish(
        const context_store_publication_request & request,
        context_store_publication_backend & backend) noexcept;

private:
    context_store_publication_root_fence & root_fence_;
};

const char * context_store_publication_status_name(
    context_store_publication_status status) noexcept;

} // namespace halofpx
