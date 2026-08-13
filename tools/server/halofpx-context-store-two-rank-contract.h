#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace halofpx {

// Test-only contract for issue #26. This is deliberately a closed two-rank
// model; it is not a general collective, a server API, or an RPC wire type.
constexpr uint32_t context_store_two_rank_world_size = 2;
constexpr size_t context_store_two_rank_count = 2;
constexpr size_t context_store_two_rank_digest_bytes = 32;
constexpr size_t context_store_two_rank_attempt_ledger_size = 64;

using context_store_two_rank_digest =
    std::array<uint8_t, context_store_two_rank_digest_bytes>;

struct context_store_two_rank_checkpoint_identity {
    uint32_t world_size = 0;
    std::array<uint32_t, context_store_two_rank_count> ordered_ranks {};
    uint64_t generation = 0;
    uint64_t token_count = 0;
    uint64_t token_boundary = 0;
    context_store_two_rank_digest model_shards_digest {};
    context_store_two_rank_digest runtime_state_abi_digest {};
    context_store_two_rank_digest kv_representation_digest {};
    context_store_two_rank_digest partition_plan_digest {};
    context_store_two_rank_digest topology_digest {};
    context_store_two_rank_digest checkpoint_digest {};
    context_store_two_rank_digest token_prefix_digest {};
    std::array<context_store_two_rank_digest, context_store_two_rank_count>
        rank_ownership_digests {};
};

struct context_store_two_rank_operation {
    context_store_two_rank_checkpoint_identity checkpoint;
    context_store_two_rank_digest attempt_nonce {};
};

struct context_store_two_rank_capture_request {
    uint32_t logical_rank = 0;
    uint64_t expected_bytes = 0;
    uint32_t expected_components = 0;
};

enum class context_store_two_rank_receipt_phase : uint8_t {
    capture = 1,
    stage = 2,
    commit_apply = 3,
};

enum class context_store_two_rank_receipt_status : uint8_t {
    durable = 1,
    ready = 2,
    applied = 3,
    definitely_not_applied = 4,
    missing = 5,
    corrupt = 6,
    incompatible = 7,
    timed_out = 8,
    rejected = 9,
};

// Every positive receipt echoes the complete stable checkpoint identity and
// the fresh operation nonce. object_digest is allowed to be equal across
// ranks; rank and receipt identity, not content equality, define duplicates.
struct context_store_two_rank_receipt {
    context_store_two_rank_receipt_phase phase =
        context_store_two_rank_receipt_phase::capture;
    context_store_two_rank_receipt_status status =
        context_store_two_rank_receipt_status::rejected;
    context_store_two_rank_checkpoint_identity checkpoint;
    context_store_two_rank_digest attempt_nonce {};
    uint32_t logical_rank = 0;
    context_store_two_rank_digest ownership_digest {};
    context_store_two_rank_digest object_digest {};
    // Stage and commit receipts must echo the durable capture authority from
    // the manifest object. Capture receipts leave this field zeroed.
    context_store_two_rank_digest source_receipt_nonce {};
    context_store_two_rank_digest receipt_nonce {};
    uint64_t verified_bytes = 0;
    uint32_t verified_components = 0;
};

struct context_store_two_rank_object {
    uint32_t logical_rank = 0;
    context_store_two_rank_digest ownership_digest {};
    context_store_two_rank_digest object_digest {};
    context_store_two_rank_digest durable_receipt_nonce {};
    uint64_t verified_bytes = 0;
    uint32_t verified_components = 0;
};

struct context_store_two_rank_manifest {
    context_store_two_rank_checkpoint_identity checkpoint;
    std::array<context_store_two_rank_object, context_store_two_rank_count> ranks {};
};

enum class context_store_two_rank_publication_outcome : uint8_t {
    published,
    definitely_not_published,
    outcome_unknown,
};

// A real adapter must not collapse response loss into definitely_not_applied.
// outcome_unknown means live remote mutation may have occurred.
enum class context_store_two_rank_commit_outcome : uint8_t {
    definitely_not_applied,
    applied,
    outcome_unknown,
};

struct context_store_two_rank_commit_result {
    context_store_two_rank_commit_outcome outcome =
        context_store_two_rank_commit_outcome::outcome_unknown;
    context_store_two_rank_receipt receipt;
};

enum class context_store_two_rank_status : uint8_t {
    published,
    restore_accepted,
    invalid_contract,
    attempt_replayed,
    attempt_ledger_full,
    invalid_manifest,
    receipt_mismatch,
    duplicate_receipt,
    rank_missing,
    rank_corrupt,
    rank_incompatible,
    rank_timed_out,
    rank_rejected,
    publication_not_completed,
    publication_outcome_unknown,
    commit_not_applied,
    commit_outcome_unknown,
    recreation_required,
};

struct context_store_two_rank_result {
    context_store_two_rank_status status =
        context_store_two_rank_status::invalid_contract;
    context_store_two_rank_manifest manifest;
    bool publication_authorized = false;
    bool commit_authorized = false;
    // Contract acceptance only. This does not report a production cache hit.
    bool restore_accepted = false;
    // Advisory is intentionally fail-closed: the adapter must stop restore and
    // inference on the affected live contexts until it recreates them.
    bool recreation_required = false;
};

class context_store_two_rank_provider {
public:
    virtual ~context_store_two_rank_provider() = default;

    virtual context_store_two_rank_receipt capture(
        const context_store_two_rank_operation & operation,
        const context_store_two_rank_capture_request & request) noexcept = 0;

    virtual context_store_two_rank_receipt stage(
        const context_store_two_rank_operation & operation,
        const context_store_two_rank_object & object) noexcept = 0;

    virtual context_store_two_rank_commit_result commit_apply(
        const context_store_two_rank_operation & operation,
        const context_store_two_rank_object & object,
        const context_store_two_rank_receipt & ready_receipt) noexcept = 0;

    // Idempotent, attempt-scoped staging cleanup only. It MUST NOT roll back,
    // delete, or modify live state that may already have committed.
    virtual void abort(const context_store_two_rank_operation & operation) noexcept = 0;
};

class context_store_two_rank_publisher {
public:
    virtual ~context_store_two_rank_publisher() = default;

    virtual context_store_two_rank_publication_outcome publish(
        const context_store_two_rank_operation & operation,
        const context_store_two_rank_manifest & manifest,
        const std::array<context_store_two_rank_receipt,
                         context_store_two_rank_count> & durable_receipts) noexcept = 0;
};

// The adapter must own exactly one instance for its full process lifetime and
// serialize calls. Attempt nonces are consumed before any provider callback
// and retained in this instance's bounded ledger.
class context_store_two_rank_coordinator {
public:
    context_store_two_rank_coordinator() = default;
    context_store_two_rank_coordinator(
        const context_store_two_rank_coordinator &) = delete;
    context_store_two_rank_coordinator & operator=(
        const context_store_two_rank_coordinator &) = delete;
    context_store_two_rank_coordinator(
        context_store_two_rank_coordinator &&) = delete;
    context_store_two_rank_coordinator & operator=(
        context_store_two_rank_coordinator &&) = delete;

    context_store_two_rank_result capture_and_publish(
        const context_store_two_rank_operation & operation,
        const std::array<context_store_two_rank_capture_request,
                         context_store_two_rank_count> & requests,
        const std::array<context_store_two_rank_provider *,
                         context_store_two_rank_count> & providers,
        context_store_two_rank_publisher & publisher) noexcept;

    context_store_two_rank_result restore(
        const context_store_two_rank_operation & operation,
        const context_store_two_rank_manifest & manifest,
        const std::array<context_store_two_rank_provider *,
                         context_store_two_rank_count> & providers) noexcept;

private:
    std::array<context_store_two_rank_digest,
               context_store_two_rank_attempt_ledger_size> used_attempts_ {};
    size_t used_attempt_count_ = 0;
};

const char * context_store_two_rank_status_name(
    context_store_two_rank_status status) noexcept;

} // namespace halofpx
