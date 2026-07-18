#pragma once

#include "halofpx-context-store-anchor.h"

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>

namespace halofpx {

using context_store_publication_id = std::array<uint8_t, 32>;
using context_store_digest = context_store_format_digest;
using context_store_publication_anchor = context_store_authenticated_anchor;

// Matches the accepted v1 manifest object bound. The offline coordinator
// rejects larger synthetic plans before calling an injected backend.
constexpr size_t context_store_publication_max_objects_v1 = 128;

struct context_store_publication_request {
    // Caller-owned, cryptographically fresh operation identity. It is not an
    // anchor field and conveys no authority by itself; a qualified backend
    // binds it to this exact predecessor/next transition for replay fencing.
    context_store_publication_id attempt_id {};
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
    // Protected authority reports no anchor for this lineage. Ordinary
    // publication cannot bootstrap it.
    anchor_absent,
    // A conclusive compare-and-swap rejection: the protected anchor did not
    // equal the supplied predecessor and the replacement was not applied.
    stale_predecessor,
    // The supplied attempt is unknown, closed, uncertain, replayed, or bound
    // to a different transition. The operation was not applied.
    attempt_fenced,
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
    virtual context_store_publication_step_result begin_attempt(
        const context_store_publication_id & attempt_id,
        const context_store_publication_anchor & expected_predecessor,
        const context_store_publication_anchor & next,
        size_t object_count) = 0;

    virtual context_store_publication_step_result stage_object(const context_store_publication_id & attempt_id, size_t index) = 0;
    virtual context_store_publication_step_result write_object(const context_store_publication_id & attempt_id, size_t index) = 0;
    virtual context_store_publication_step_result verify_object(const context_store_publication_id & attempt_id, size_t index) = 0;
    virtual context_store_publication_step_result sync_object_file(const context_store_publication_id & attempt_id, size_t index) = 0;
    virtual context_store_publication_step_result publish_object_no_replace(const context_store_publication_id & attempt_id, size_t index) = 0;
    virtual context_store_publication_step_result sync_object_directory(const context_store_publication_id & attempt_id, size_t index) = 0;

    virtual context_store_publication_step_result stage_manifest(const context_store_publication_id & attempt_id) = 0;
    virtual context_store_publication_step_result write_manifest(const context_store_publication_id & attempt_id) = 0;
    // Success must return the digest of the exact canonical authenticated
    // manifest bytes that were verified and are about to be published.
    virtual context_store_publication_step_result verify_manifest(
        const context_store_publication_id & attempt_id,
        context_store_digest & verified_digest) = 0;
    virtual context_store_publication_step_result sync_manifest_file(const context_store_publication_id & attempt_id) = 0;
    virtual context_store_publication_step_result publish_manifest_no_replace(const context_store_publication_id & attempt_id) = 0;
    virtual context_store_publication_step_result sync_manifest_directory(const context_store_publication_id & attempt_id) = 0;

    virtual context_store_publication_step_result replace_anchor_atomically(
        const context_store_publication_id & attempt_id,
        const context_store_publication_anchor & expected_predecessor,
        const context_store_publication_anchor & next) = 0;
    virtual context_store_publication_step_result sync_anchor(
        const context_store_publication_id & attempt_id,
        const context_store_publication_anchor & next) = 0;
    // Durability may be acknowledged only after this exact terminal close.
    virtual context_store_publication_step_result close_durable_attempt(
        const context_store_publication_id & attempt_id,
        const context_store_publication_anchor & next) = 0;

    // Required fencing transitions. A backend must reject all subsequent
    // mutations for an abandoned or uncertain attempt. These calls are not a
    // substitute for persistent reconciliation in a concrete backend.
    virtual context_store_publication_step_result abandon_attempt(
        const context_store_publication_id & attempt_id) = 0;
    virtual context_store_publication_step_result fence_attempt_uncertain(
        const context_store_publication_id & attempt_id) = 0;
};

enum class context_store_publication_status : uint8_t {
    published,
    invalid_request,
    bootstrap_required,
    stale_predecessor,
    attempt_fenced,
    writer_busy,
    object_collision,
    manifest_collision,
    manifest_identity_mismatch,
    storage_error,
    sync_error,
    anchor_visibility_uncertain,
    attempt_fencing_uncertain,
};

struct context_store_publication_result {
    context_store_publication_status status = context_store_publication_status::invalid_request;
    size_t completed_steps = 0;
    bool anchor_replaced = false;
    bool durability_acknowledged = false;
    // True only when an uncertain backend transition was explicitly confirmed
    // fenced for this root. False requires external root quarantine.
    bool attempt_fence_confirmed = false;
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
