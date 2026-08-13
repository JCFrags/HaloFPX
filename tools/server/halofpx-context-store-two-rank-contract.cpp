#include "halofpx-context-store-two-rank-contract.h"

#include <algorithm>

namespace halofpx {
namespace {

constexpr std::array<uint32_t, context_store_two_rank_count> expected_ranks = { 0, 1 };

bool nonzero(const context_store_two_rank_digest & value) noexcept {
    uint8_t combined = 0;
    for (const uint8_t byte : value) combined |= byte;
    return combined != 0;
}

bool same_checkpoint(
        const context_store_two_rank_checkpoint_identity & left,
        const context_store_two_rank_checkpoint_identity & right) noexcept {
    return left.world_size == right.world_size &&
        left.ordered_ranks == right.ordered_ranks &&
        left.generation == right.generation &&
        left.token_count == right.token_count &&
        left.token_boundary == right.token_boundary &&
        left.model_shards_digest == right.model_shards_digest &&
        left.runtime_state_abi_digest == right.runtime_state_abi_digest &&
        left.kv_representation_digest == right.kv_representation_digest &&
        left.partition_plan_digest == right.partition_plan_digest &&
        left.topology_digest == right.topology_digest &&
        left.checkpoint_digest == right.checkpoint_digest &&
        left.token_prefix_digest == right.token_prefix_digest &&
        left.rank_ownership_digests == right.rank_ownership_digests;
}

bool valid_checkpoint(
        const context_store_two_rank_checkpoint_identity & checkpoint) noexcept {
    if (checkpoint.world_size != context_store_two_rank_world_size ||
        checkpoint.ordered_ranks != expected_ranks ||
        checkpoint.generation == 0 || checkpoint.token_count == 0 ||
        checkpoint.token_boundary != checkpoint.token_count ||
        !nonzero(checkpoint.model_shards_digest) ||
        !nonzero(checkpoint.runtime_state_abi_digest) ||
        !nonzero(checkpoint.kv_representation_digest) ||
        !nonzero(checkpoint.partition_plan_digest) ||
        !nonzero(checkpoint.topology_digest) ||
        !nonzero(checkpoint.checkpoint_digest) ||
        !nonzero(checkpoint.token_prefix_digest)) {
        return false;
    }
    return std::all_of(
        checkpoint.rank_ownership_digests.begin(),
        checkpoint.rank_ownership_digests.end(), nonzero);
}

bool valid_operation(const context_store_two_rank_operation & operation) noexcept {
    return valid_checkpoint(operation.checkpoint) && nonzero(operation.attempt_nonce);
}

context_store_two_rank_status failure_status(
        context_store_two_rank_receipt_status status) noexcept {
    switch (status) {
        case context_store_two_rank_receipt_status::missing:
            return context_store_two_rank_status::rank_missing;
        case context_store_two_rank_receipt_status::corrupt:
            return context_store_two_rank_status::rank_corrupt;
        case context_store_two_rank_receipt_status::incompatible:
            return context_store_two_rank_status::rank_incompatible;
        case context_store_two_rank_receipt_status::timed_out:
            return context_store_two_rank_status::rank_timed_out;
        case context_store_two_rank_receipt_status::rejected:
        case context_store_two_rank_receipt_status::definitely_not_applied:
            return context_store_two_rank_status::rank_rejected;
        case context_store_two_rank_receipt_status::durable:
        case context_store_two_rank_receipt_status::ready:
        case context_store_two_rank_receipt_status::applied:
            return context_store_two_rank_status::receipt_mismatch;
    }
    return context_store_two_rank_status::rank_rejected;
}

bool exact_receipt_base(
        const context_store_two_rank_receipt & receipt,
        const context_store_two_rank_operation & operation,
        uint32_t logical_rank,
        const context_store_two_rank_digest & object_digest,
        uint64_t expected_bytes,
        uint32_t expected_components) noexcept {
    return same_checkpoint(receipt.checkpoint, operation.checkpoint) &&
        receipt.attempt_nonce == operation.attempt_nonce &&
        receipt.logical_rank == logical_rank &&
        receipt.ownership_digest ==
            operation.checkpoint.rank_ownership_digests[logical_rank] &&
        receipt.object_digest == object_digest &&
        nonzero(receipt.object_digest) && nonzero(receipt.receipt_nonce) &&
        receipt.verified_bytes == expected_bytes &&
        receipt.verified_components == expected_components;
}

bool exact_capture_receipt(
        const context_store_two_rank_receipt & receipt,
        const context_store_two_rank_operation & operation,
        const context_store_two_rank_capture_request & request) noexcept {
    return receipt.phase == context_store_two_rank_receipt_phase::capture &&
        receipt.status == context_store_two_rank_receipt_status::durable &&
        same_checkpoint(receipt.checkpoint, operation.checkpoint) &&
        receipt.attempt_nonce == operation.attempt_nonce &&
        receipt.logical_rank == request.logical_rank &&
        receipt.ownership_digest ==
            operation.checkpoint.rank_ownership_digests[request.logical_rank] &&
        nonzero(receipt.object_digest) &&
        !nonzero(receipt.source_receipt_nonce) && nonzero(receipt.receipt_nonce) &&
        receipt.verified_bytes == request.expected_bytes &&
        receipt.verified_components == request.expected_components;
}

bool exact_stage_receipt(
        const context_store_two_rank_receipt & receipt,
        const context_store_two_rank_operation & operation,
        const context_store_two_rank_object & object) noexcept {
    return receipt.phase == context_store_two_rank_receipt_phase::stage &&
        receipt.status == context_store_two_rank_receipt_status::ready &&
        receipt.source_receipt_nonce == object.durable_receipt_nonce &&
        exact_receipt_base(receipt, operation, object.logical_rank,
            object.object_digest, object.verified_bytes, object.verified_components);
}

bool exact_applied_receipt(
        const context_store_two_rank_receipt & receipt,
        const context_store_two_rank_operation & operation,
        const context_store_two_rank_object & object,
        const context_store_two_rank_receipt & ready) noexcept {
    return receipt.phase == context_store_two_rank_receipt_phase::commit_apply &&
        receipt.status == context_store_two_rank_receipt_status::applied &&
        receipt.source_receipt_nonce == object.durable_receipt_nonce &&
        receipt.receipt_nonce == ready.receipt_nonce &&
        exact_receipt_base(receipt, operation, object.logical_rank,
            object.object_digest, object.verified_bytes, object.verified_components);
}

bool exact_not_applied_receipt(
        const context_store_two_rank_receipt & receipt,
        const context_store_two_rank_operation & operation,
        const context_store_two_rank_object & object,
        const context_store_two_rank_receipt & ready) noexcept {
    return receipt.phase == context_store_two_rank_receipt_phase::commit_apply &&
        receipt.status ==
            context_store_two_rank_receipt_status::definitely_not_applied &&
        receipt.source_receipt_nonce == object.durable_receipt_nonce &&
        receipt.receipt_nonce == ready.receipt_nonce &&
        exact_receipt_base(receipt, operation, object.logical_rank,
            object.object_digest, object.verified_bytes, object.verified_components);
}

bool valid_manifest(
        const context_store_two_rank_manifest & manifest,
        const context_store_two_rank_operation & operation) noexcept {
    if (!same_checkpoint(manifest.checkpoint, operation.checkpoint)) return false;
    for (size_t index = 0; index < manifest.ranks.size(); ++index) {
        const auto & object = manifest.ranks[index];
        if (object.logical_rank != expected_ranks[index] ||
            object.ownership_digest !=
                operation.checkpoint.rank_ownership_digests[index] ||
            !nonzero(object.object_digest) ||
            !nonzero(object.durable_receipt_nonce) ||
            object.verified_bytes == 0 || object.verified_components == 0) {
            return false;
        }
    }
    return manifest.ranks[0].durable_receipt_nonce !=
        manifest.ranks[1].durable_receipt_nonce;
}

void abort_attempted(
        const context_store_two_rank_operation & operation,
        const std::array<context_store_two_rank_provider *,
                         context_store_two_rank_count> & providers,
        size_t attempted_count) noexcept {
    while (attempted_count != 0) {
        --attempted_count;
        providers[attempted_count]->abort(operation);
    }
}

enum class attempt_admission : uint8_t {
    admitted,
    invalid,
    replayed,
    ledger_full,
};

attempt_admission consume_attempt(
        const context_store_two_rank_operation & operation,
        std::array<context_store_two_rank_digest,
                   context_store_two_rank_attempt_ledger_size> & used_attempts,
        size_t & used_attempt_count) noexcept {
    if (!valid_operation(operation)) return attempt_admission::invalid;
    if (std::find(used_attempts.begin(),
            used_attempts.begin() + used_attempt_count,
            operation.attempt_nonce) != used_attempts.begin() + used_attempt_count) {
        return attempt_admission::replayed;
    }
    if (used_attempt_count == used_attempts.size()) {
        return attempt_admission::ledger_full;
    }
    used_attempts[used_attempt_count++] = operation.attempt_nonce;
    return attempt_admission::admitted;
}

context_store_two_rank_result admission_failure(attempt_admission admission) noexcept {
    context_store_two_rank_result result;
    if (admission == attempt_admission::replayed) {
        result.status = context_store_two_rank_status::attempt_replayed;
    } else if (admission == attempt_admission::ledger_full) {
        result.status = context_store_two_rank_status::attempt_ledger_full;
    }
    return result;
}

bool valid_providers(
        const std::array<context_store_two_rank_provider *,
                         context_store_two_rank_count> & providers) noexcept {
    return providers[0] != nullptr && providers[1] != nullptr &&
        providers[0] != providers[1];
}

} // namespace

context_store_two_rank_result context_store_two_rank_coordinator::capture_and_publish(
        const context_store_two_rank_operation & operation,
        const std::array<context_store_two_rank_capture_request,
                         context_store_two_rank_count> & requests,
        const std::array<context_store_two_rank_provider *,
                         context_store_two_rank_count> & providers,
        context_store_two_rank_publisher & publisher) noexcept {
    context_store_two_rank_result result;
    if (!valid_providers(providers) ||
        requests[0].logical_rank != 0 || requests[1].logical_rank != 1 ||
        requests[0].expected_bytes == 0 || requests[1].expected_bytes == 0 ||
        requests[0].expected_components == 0 || requests[1].expected_components == 0) {
        return result;
    }

    const auto admission = consume_attempt(
        operation, used_attempts_, used_attempt_count_);
    if (admission != attempt_admission::admitted) {
        return admission_failure(admission);
    }

    std::array<context_store_two_rank_receipt, context_store_two_rank_count> receipts;
    size_t attempted_count = 0;
    for (size_t index = 0; index < receipts.size(); ++index) {
        ++attempted_count;
        receipts[index] = providers[index]->capture(operation, requests[index]);
        if (receipts[index].status != context_store_two_rank_receipt_status::durable) {
            result.status = failure_status(receipts[index].status);
            abort_attempted(operation, providers, attempted_count);
            return result;
        }
        if (!exact_capture_receipt(receipts[index], operation, requests[index])) {
            result.status = context_store_two_rank_status::receipt_mismatch;
            abort_attempted(operation, providers, attempted_count);
            return result;
        }
        if (index != 0 &&
            receipts[index].receipt_nonce == receipts[0].receipt_nonce) {
            result.status = context_store_two_rank_status::duplicate_receipt;
            abort_attempted(operation, providers, attempted_count);
            return result;
        }
    }

    result.manifest.checkpoint = operation.checkpoint;
    for (size_t index = 0; index < receipts.size(); ++index) {
        auto & object = result.manifest.ranks[index];
        object.logical_rank = static_cast<uint32_t>(index);
        object.ownership_digest = receipts[index].ownership_digest;
        object.object_digest = receipts[index].object_digest;
        object.durable_receipt_nonce = receipts[index].receipt_nonce;
        object.verified_bytes = receipts[index].verified_bytes;
        object.verified_components = receipts[index].verified_components;
    }

    result.publication_authorized = true;
    const auto published = publisher.publish(operation, result.manifest, receipts);
    if (published == context_store_two_rank_publication_outcome::published) {
        result.status = context_store_two_rank_status::published;
    } else if (published ==
            context_store_two_rank_publication_outcome::definitely_not_published) {
        result.status = context_store_two_rank_status::publication_not_completed;
        abort_attempted(operation, providers, attempted_count);
    } else {
        result.status = context_store_two_rank_status::publication_outcome_unknown;
    }
    return result;
}

context_store_two_rank_result context_store_two_rank_coordinator::restore(
        const context_store_two_rank_operation & operation,
        const context_store_two_rank_manifest & manifest,
        const std::array<context_store_two_rank_provider *,
                         context_store_two_rank_count> & providers) noexcept {
    context_store_two_rank_result result;
    if (!valid_providers(providers)) return result;

    const auto admission = consume_attempt(
        operation, used_attempts_, used_attempt_count_);
    if (admission != attempt_admission::admitted) {
        return admission_failure(admission);
    }
    if (!valid_manifest(manifest, operation)) {
        result.status = context_store_two_rank_status::invalid_manifest;
        return result;
    }
    result.manifest = manifest;

    std::array<context_store_two_rank_receipt, context_store_two_rank_count> ready;
    size_t attempted_count = 0;
    for (size_t index = 0; index < ready.size(); ++index) {
        ++attempted_count;
        ready[index] = providers[index]->stage(operation, manifest.ranks[index]);
        if (ready[index].status != context_store_two_rank_receipt_status::ready) {
            result.status = failure_status(ready[index].status);
            abort_attempted(operation, providers, attempted_count);
            return result;
        }
        if (!exact_stage_receipt(ready[index], operation, manifest.ranks[index])) {
            result.status = context_store_two_rank_status::receipt_mismatch;
            abort_attempted(operation, providers, attempted_count);
            return result;
        }
        if (index != 0 && ready[index].receipt_nonce == ready[0].receipt_nonce) {
            result.status = context_store_two_rank_status::duplicate_receipt;
            abort_attempted(operation, providers, attempted_count);
            return result;
        }
    }

    result.commit_authorized = true;
    const auto remote = providers[1]->commit_apply(
        operation, manifest.ranks[1], ready[1]);
    if (remote.outcome == context_store_two_rank_commit_outcome::outcome_unknown) {
        result.status = context_store_two_rank_status::commit_outcome_unknown;
        result.recreation_required = true;
        abort_attempted(operation, providers, attempted_count);
        return result;
    }
    if (remote.outcome ==
            context_store_two_rank_commit_outcome::definitely_not_applied) {
        if (exact_not_applied_receipt(
                remote.receipt, operation, manifest.ranks[1], ready[1])) {
            result.status = context_store_two_rank_status::commit_not_applied;
        } else {
            // The remote dispatch occurred, but the refusal does not exactly
            // authorize a definitely-not-applied conclusion for this attempt.
            result.status = context_store_two_rank_status::recreation_required;
            result.recreation_required = true;
        }
        abort_attempted(operation, providers, attempted_count);
        return result;
    }
    if (remote.outcome != context_store_two_rank_commit_outcome::applied) {
        // Reject out-of-domain values after dispatch as uncertain mutation.
        result.status = context_store_two_rank_status::recreation_required;
        result.recreation_required = true;
        abort_attempted(operation, providers, attempted_count);
        return result;
    }
    if (!exact_applied_receipt(
            remote.receipt, operation, manifest.ranks[1], ready[1])) {
        result.status = context_store_two_rank_status::recreation_required;
        result.recreation_required = true;
        abort_attempted(operation, providers, attempted_count);
        return result;
    }

    const auto local = providers[0]->commit_apply(
        operation, manifest.ranks[0], ready[0]);
    if (local.outcome != context_store_two_rank_commit_outcome::applied ||
        !exact_applied_receipt(
            local.receipt, operation, manifest.ranks[0], ready[0])) {
        result.status = context_store_two_rank_status::recreation_required;
        result.recreation_required = true;
        abort_attempted(operation, providers, attempted_count);
        return result;
    }

    result.status = context_store_two_rank_status::restore_accepted;
    result.restore_accepted = true;
    return result;
}

const char * context_store_two_rank_status_name(
        context_store_two_rank_status status) noexcept {
    switch (status) {
        case context_store_two_rank_status::published: return "published";
        case context_store_two_rank_status::restore_accepted: return "restore-accepted";
        case context_store_two_rank_status::invalid_contract: return "invalid-contract";
        case context_store_two_rank_status::attempt_replayed: return "attempt-replayed";
        case context_store_two_rank_status::attempt_ledger_full: return "attempt-ledger-full";
        case context_store_two_rank_status::invalid_manifest: return "invalid-manifest";
        case context_store_two_rank_status::receipt_mismatch: return "receipt-mismatch";
        case context_store_two_rank_status::duplicate_receipt: return "duplicate-receipt";
        case context_store_two_rank_status::rank_missing: return "rank-missing";
        case context_store_two_rank_status::rank_corrupt: return "rank-corrupt";
        case context_store_two_rank_status::rank_incompatible: return "rank-incompatible";
        case context_store_two_rank_status::rank_timed_out: return "rank-timed-out";
        case context_store_two_rank_status::rank_rejected: return "rank-rejected";
        case context_store_two_rank_status::publication_not_completed:
            return "publication-not-completed";
        case context_store_two_rank_status::publication_outcome_unknown:
            return "publication-outcome-unknown";
        case context_store_two_rank_status::commit_not_applied: return "commit-not-applied";
        case context_store_two_rank_status::commit_outcome_unknown:
            return "commit-outcome-unknown";
        case context_store_two_rank_status::recreation_required:
            return "recreation-required";
    }
    return "unknown";
}

} // namespace halofpx
