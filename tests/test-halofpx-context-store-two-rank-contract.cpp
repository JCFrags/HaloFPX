#include "halofpx-context-store-two-rank-contract.h"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>

#include <array>
#include <functional>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace {

using namespace halofpx;

static_assert(!std::is_copy_constructible_v<context_store_two_rank_coordinator>);
static_assert(!std::is_copy_assignable_v<context_store_two_rank_coordinator>);
static_assert(!std::is_move_constructible_v<context_store_two_rank_coordinator>);
static_assert(!std::is_move_assignable_v<context_store_two_rank_coordinator>);

context_store_two_rank_digest digest(uint8_t seed) {
    context_store_two_rank_digest result {};
    for (size_t index = 0; index < result.size(); ++index) {
        result[index] = static_cast<uint8_t>(seed + index);
    }
    return result;
}

context_store_two_rank_operation operation(uint8_t attempt_seed = 0x70) {
    context_store_two_rank_operation result;
    result.checkpoint.world_size = 2;
    result.checkpoint.ordered_ranks = { 0, 1 };
    result.checkpoint.generation = 7;
    result.checkpoint.token_count = 96;
    result.checkpoint.token_boundary = 96;
    result.checkpoint.model_shards_digest = digest(0x10);
    result.checkpoint.runtime_state_abi_digest = digest(0x20);
    result.checkpoint.kv_representation_digest = digest(0x30);
    result.checkpoint.partition_plan_digest = digest(0x40);
    result.checkpoint.topology_digest = digest(0x50);
    result.checkpoint.checkpoint_digest = digest(0x60);
    result.checkpoint.token_prefix_digest = digest(0x68);
    result.checkpoint.rank_ownership_digests = { digest(0x80), digest(0x90) };
    result.attempt_nonce = digest(attempt_seed);
    return result;
}

std::array<context_store_two_rank_capture_request, 2> capture_requests() {
    return {{ { 0, 4096, 2 }, { 1, 8192, 4 } }};
}

context_store_two_rank_receipt capture_receipt(
        const context_store_two_rank_operation & value,
        const context_store_two_rank_capture_request & request,
        uint8_t object_seed,
        uint8_t receipt_seed) {
    context_store_two_rank_receipt result;
    result.phase = context_store_two_rank_receipt_phase::capture;
    result.status = context_store_two_rank_receipt_status::durable;
    result.checkpoint = value.checkpoint;
    result.attempt_nonce = value.attempt_nonce;
    result.logical_rank = request.logical_rank;
    result.ownership_digest =
        value.checkpoint.rank_ownership_digests[request.logical_rank];
    result.object_digest = digest(object_seed);
    result.receipt_nonce = digest(receipt_seed);
    result.verified_bytes = request.expected_bytes;
    result.verified_components = request.expected_components;
    return result;
}

context_store_two_rank_manifest manifest(
        const context_store_two_rank_operation & value) {
    context_store_two_rank_manifest result;
    result.checkpoint = value.checkpoint;
    for (size_t index = 0; index < result.ranks.size(); ++index) {
        auto & rank = result.ranks[index];
        rank.logical_rank = static_cast<uint32_t>(index);
        rank.ownership_digest = value.checkpoint.rank_ownership_digests[index];
        rank.object_digest = digest(static_cast<uint8_t>(0xa0 + index * 4));
        rank.durable_receipt_nonce = digest(static_cast<uint8_t>(0xb0 + index * 4));
        rank.verified_bytes = index == 0 ? 4096 : 8192;
        rank.verified_components = index == 0 ? 2 : 4;
    }
    return result;
}

context_store_two_rank_receipt stage_receipt(
        const context_store_two_rank_operation & value,
        const context_store_two_rank_object & object,
        uint8_t receipt_seed) {
    context_store_two_rank_receipt result;
    result.phase = context_store_two_rank_receipt_phase::stage;
    result.status = context_store_two_rank_receipt_status::ready;
    result.checkpoint = value.checkpoint;
    result.attempt_nonce = value.attempt_nonce;
    result.logical_rank = object.logical_rank;
    result.ownership_digest = object.ownership_digest;
    result.object_digest = object.object_digest;
    result.source_receipt_nonce = object.durable_receipt_nonce;
    result.receipt_nonce = digest(receipt_seed);
    result.verified_bytes = object.verified_bytes;
    result.verified_components = object.verified_components;
    return result;
}

context_store_two_rank_receipt applied_receipt(
        const context_store_two_rank_operation & value,
        const context_store_two_rank_object & object,
        const context_store_two_rank_receipt & ready) {
    auto result = ready;
    result.phase = context_store_two_rank_receipt_phase::commit_apply;
    result.status = context_store_two_rank_receipt_status::applied;
    result.checkpoint = value.checkpoint;
    result.attempt_nonce = value.attempt_nonce;
    result.logical_rank = object.logical_rank;
    return result;
}

context_store_two_rank_receipt not_applied_receipt(
        const context_store_two_rank_operation & value,
        const context_store_two_rank_object & object,
        const context_store_two_rank_receipt & ready) {
    auto result = applied_receipt(value, object, ready);
    result.status = context_store_two_rank_receipt_status::definitely_not_applied;
    return result;
}

struct fake_provider final : context_store_two_rank_provider {
    uint32_t rank = 0;
    std::vector<std::string> * trace = nullptr;
    int * publish_calls = nullptr;
    context_store_two_rank_receipt capture_value;
    context_store_two_rank_receipt stage_value;
    context_store_two_rank_commit_result commit_value;
    int capture_calls = 0;
    int stage_calls = 0;
    int commit_calls = 0;
    int abort_calls = 0;

    context_store_two_rank_receipt capture(
            const context_store_two_rank_operation &,
            const context_store_two_rank_capture_request & request) noexcept override {
        assert(request.logical_rank == rank);
        assert(publish_calls == nullptr || *publish_calls == 0);
        ++capture_calls;
        trace->push_back("capture:" + std::to_string(rank));
        return capture_value;
    }

    context_store_two_rank_receipt stage(
            const context_store_two_rank_operation &,
            const context_store_two_rank_object & object) noexcept override {
        assert(object.logical_rank == rank);
        ++stage_calls;
        trace->push_back("stage:" + std::to_string(rank));
        return stage_value;
    }

    context_store_two_rank_commit_result commit_apply(
            const context_store_two_rank_operation &,
            const context_store_two_rank_object & object,
            const context_store_two_rank_receipt & ready) noexcept override {
        assert(object.logical_rank == rank && ready.logical_rank == rank);
        ++commit_calls;
        trace->push_back(rank == 1 ? "commit-remote:1" : "apply-local:0");
        return commit_value;
    }

    void abort(const context_store_two_rank_operation &) noexcept override {
        ++abort_calls;
        trace->push_back("abort:" + std::to_string(rank));
    }
};

struct fake_publisher final : context_store_two_rank_publisher {
    std::vector<std::string> * trace = nullptr;
    context_store_two_rank_publication_outcome outcome =
        context_store_two_rank_publication_outcome::published;
    int calls = 0;

    context_store_two_rank_publication_outcome publish(
            const context_store_two_rank_operation & operation_value,
            const context_store_two_rank_manifest & manifest_value,
            const std::array<context_store_two_rank_receipt, 2> & receipts) noexcept override {
        ++calls;
        trace->push_back("publish:0,1");
        assert(manifest_value.checkpoint.checkpoint_digest ==
            operation_value.checkpoint.checkpoint_digest);
        assert(manifest_value.ranks[0].logical_rank == 0);
        assert(manifest_value.ranks[1].logical_rank == 1);
        assert(receipts[0].status == context_store_two_rank_receipt_status::durable);
        assert(receipts[1].status == context_store_two_rank_receipt_status::durable);
        return outcome;
    }
};

struct capture_fixture {
    context_store_two_rank_operation value = operation();
    std::array<context_store_two_rank_capture_request, 2> requests = capture_requests();
    std::vector<std::string> trace;
    fake_publisher publisher;
    fake_provider rank0;
    fake_provider rank1;
    std::array<context_store_two_rank_provider *, 2> providers;

    capture_fixture() : providers { &rank0, &rank1 } {
        publisher.trace = &trace;
        rank0.rank = 0;
        rank1.rank = 1;
        rank0.trace = &trace;
        rank1.trace = &trace;
        rank0.publish_calls = &publisher.calls;
        rank1.publish_calls = &publisher.calls;
        rank0.capture_value = capture_receipt(value, requests[0], 0xc0, 0xd0);
        rank1.capture_value = capture_receipt(value, requests[1], 0xc4, 0xd4);
    }
};

struct restore_fixture {
    context_store_two_rank_operation value = operation();
    context_store_two_rank_manifest candidate = manifest(value);
    std::vector<std::string> trace;
    fake_provider rank0;
    fake_provider rank1;
    std::array<context_store_two_rank_provider *, 2> providers;

    restore_fixture() : providers { &rank0, &rank1 } {
        rank0.rank = 0;
        rank1.rank = 1;
        rank0.trace = &trace;
        rank1.trace = &trace;
        rank0.stage_value = stage_receipt(value, candidate.ranks[0], 0xe0);
        rank1.stage_value = stage_receipt(value, candidate.ranks[1], 0xe4);
        rank0.commit_value.outcome = context_store_two_rank_commit_outcome::applied;
        rank1.commit_value.outcome = context_store_two_rank_commit_outcome::applied;
        rank0.commit_value.receipt =
            applied_receipt(value, candidate.ranks[0], rank0.stage_value);
        rank1.commit_value.receipt =
            applied_receipt(value, candidate.ranks[1], rank1.stage_value);
    }
};

void test_capture_success_and_replay() {
    capture_fixture fixture;
    // Equal content is legal; ordered rank/receipt authority remains distinct.
    fixture.rank1.capture_value.object_digest = fixture.rank0.capture_value.object_digest;
    context_store_two_rank_coordinator coordinator;
    const auto result = coordinator.capture_and_publish(
        fixture.value, fixture.requests, fixture.providers, fixture.publisher);
    assert(result.status == context_store_two_rank_status::published);
    assert(result.publication_authorized && !result.commit_authorized);
    assert(!result.restore_accepted && !result.recreation_required);
    assert(fixture.publisher.calls == 1);
    assert((fixture.trace ==
        std::vector<std::string> { "capture:0", "capture:1", "publish:0,1" }));
    assert(result.manifest.ranks[0].logical_rank == 0);
    assert(result.manifest.ranks[1].logical_rank == 1);

    const auto replay = coordinator.capture_and_publish(
        fixture.value, fixture.requests, fixture.providers, fixture.publisher);
    assert(replay.status == context_store_two_rank_status::attempt_replayed);
    assert(fixture.publisher.calls == 1 && fixture.trace.size() == 3);
}

void test_capture_failures_are_whole_attempts() {
    const std::array<std::pair<context_store_two_rank_receipt_status,
                               context_store_two_rank_status>, 5> failures = {{
        { context_store_two_rank_receipt_status::missing,
          context_store_two_rank_status::rank_missing },
        { context_store_two_rank_receipt_status::corrupt,
          context_store_two_rank_status::rank_corrupt },
        { context_store_two_rank_receipt_status::incompatible,
          context_store_two_rank_status::rank_incompatible },
        { context_store_two_rank_receipt_status::timed_out,
          context_store_two_rank_status::rank_timed_out },
        { context_store_two_rank_receipt_status::rejected,
          context_store_two_rank_status::rank_rejected },
    }};
    for (size_t rank = 0; rank < 2; ++rank) {
        for (const auto & failure : failures) {
            capture_fixture fixture;
            (rank == 0 ? fixture.rank0 : fixture.rank1).capture_value.status = failure.first;
            context_store_two_rank_coordinator coordinator;
            const auto result = coordinator.capture_and_publish(
                fixture.value, fixture.requests, fixture.providers, fixture.publisher);
            assert(result.status == failure.second);
            assert(!result.publication_authorized && !result.restore_accepted &&
                   !result.recreation_required && fixture.publisher.calls == 0);
            assert(fixture.rank0.abort_calls == 1);
            assert(fixture.rank1.abort_calls == (rank == 1 ? 1 : 0));
        }
    }
}

void test_capture_exact_receipts_and_roster() {
    const std::vector<std::function<void(context_store_two_rank_receipt &)>> mutations = {
        [](auto & value) { value.phase = context_store_two_rank_receipt_phase::stage; },
        [](auto & value) { value.logical_rank = 0; },
        [](auto & value) { value.checkpoint.world_size = 1; },
        [](auto & value) { value.checkpoint.ordered_ranks = { 1, 0 }; },
        [](auto & value) { ++value.checkpoint.generation; },
        [](auto & value) { ++value.checkpoint.token_count; },
        [](auto & value) { --value.checkpoint.token_boundary; },
        [](auto & value) { value.checkpoint.model_shards_digest[0] ^= 1; },
        [](auto & value) { value.checkpoint.runtime_state_abi_digest[0] ^= 1; },
        [](auto & value) { value.checkpoint.kv_representation_digest[0] ^= 1; },
        [](auto & value) { value.checkpoint.partition_plan_digest[0] ^= 1; },
        [](auto & value) { value.checkpoint.topology_digest[0] ^= 1; },
        [](auto & value) { value.checkpoint.checkpoint_digest[0] ^= 1; },
        [](auto & value) { value.checkpoint.token_prefix_digest[0] ^= 1; },
        [](auto & value) { value.ownership_digest[0] ^= 1; },
        [](auto & value) { value.attempt_nonce[0] ^= 1; },
        [](auto & value) { value.object_digest.fill(0); },
        [](auto & value) { value.source_receipt_nonce = digest(0xf4); },
        [](auto & value) { value.receipt_nonce.fill(0); },
        [](auto & value) { ++value.verified_bytes; },
        [](auto & value) { ++value.verified_components; },
    };
    for (const auto & mutate : mutations) {
        capture_fixture fixture;
        mutate(fixture.rank1.capture_value);
        context_store_two_rank_coordinator coordinator;
        const auto result = coordinator.capture_and_publish(
            fixture.value, fixture.requests, fixture.providers, fixture.publisher);
        assert(result.status == context_store_two_rank_status::receipt_mismatch);
        assert(fixture.publisher.calls == 0 && !result.publication_authorized);
    }

    {
        capture_fixture fixture;
        fixture.rank1.capture_value.receipt_nonce = fixture.rank0.capture_value.receipt_nonce;
        context_store_two_rank_coordinator coordinator;
        const auto result = coordinator.capture_and_publish(
            fixture.value, fixture.requests, fixture.providers, fixture.publisher);
        assert(result.status == context_store_two_rank_status::duplicate_receipt);
        assert(fixture.publisher.calls == 0);
        assert(fixture.rank0.abort_calls == 1 && fixture.rank1.abort_calls == 1);
    }
    {
        capture_fixture fixture;
        fixture.rank0.capture_value =
            capture_receipt(fixture.value, fixture.requests[1], 0xc4, 0xd4);
        context_store_two_rank_coordinator coordinator;
        const auto result = coordinator.capture_and_publish(
            fixture.value, fixture.requests, fixture.providers, fixture.publisher);
        assert(result.status == context_store_two_rank_status::receipt_mismatch);
        assert(fixture.rank1.capture_calls == 0 && fixture.publisher.calls == 0);
    }
    {
        capture_fixture fixture;
        fixture.value.checkpoint.world_size = 1;
        context_store_two_rank_coordinator coordinator;
        const auto result = coordinator.capture_and_publish(
            fixture.value, fixture.requests, fixture.providers, fixture.publisher);
        assert(result.status == context_store_two_rank_status::invalid_contract);
        assert(fixture.trace.empty());
    }
    {
        capture_fixture fixture;
        fixture.requests[1].logical_rank = 0;
        context_store_two_rank_coordinator coordinator;
        const auto result = coordinator.capture_and_publish(
            fixture.value, fixture.requests, fixture.providers, fixture.publisher);
        assert(result.status == context_store_two_rank_status::invalid_contract);
        assert(fixture.trace.empty());
    }
    {
        capture_fixture fixture;
        fixture.providers[1] = fixture.providers[0];
        context_store_two_rank_coordinator coordinator;
        const auto result = coordinator.capture_and_publish(
            fixture.value, fixture.requests, fixture.providers, fixture.publisher);
        assert(result.status == context_store_two_rank_status::invalid_contract);
        assert(fixture.trace.empty());
    }
}

void test_attempt_ledger_fails_closed() {
    context_store_two_rank_coordinator coordinator;
    for (size_t index = 0; index < context_store_two_rank_attempt_ledger_size; ++index) {
        capture_fixture fixture;
        fixture.value.attempt_nonce = digest(static_cast<uint8_t>(index + 1));
        fixture.rank0.capture_value =
            capture_receipt(fixture.value, fixture.requests[0], 0xc0, 0xd0);
        fixture.rank1.capture_value =
            capture_receipt(fixture.value, fixture.requests[1], 0xc4, 0xd4);
        const auto result = coordinator.capture_and_publish(
            fixture.value, fixture.requests, fixture.providers, fixture.publisher);
        assert(result.status == context_store_two_rank_status::published);
    }

    capture_fixture refused;
    refused.value.attempt_nonce = digest(0xf0);
    refused.rank0.capture_value =
        capture_receipt(refused.value, refused.requests[0], 0xc0, 0xd0);
    refused.rank1.capture_value =
        capture_receipt(refused.value, refused.requests[1], 0xc4, 0xd4);
    const auto result = coordinator.capture_and_publish(
        refused.value, refused.requests, refused.providers, refused.publisher);
    assert(result.status == context_store_two_rank_status::attempt_ledger_full);
    assert(refused.trace.empty() && refused.publisher.calls == 0);
}

void test_restore_success_and_replay() {
    restore_fixture fixture;
    context_store_two_rank_coordinator coordinator;
    const auto result = coordinator.restore(
        fixture.value, fixture.candidate, fixture.providers);
    assert(result.status == context_store_two_rank_status::restore_accepted);
    assert(result.commit_authorized && result.restore_accepted && !result.recreation_required);
    assert((fixture.trace == std::vector<std::string> {
        "stage:0", "stage:1", "commit-remote:1", "apply-local:0" }));
    assert(fixture.rank0.commit_calls == 1 && fixture.rank1.commit_calls == 1);

    const auto replay = coordinator.restore(
        fixture.value, fixture.candidate, fixture.providers);
    assert(replay.status == context_store_two_rank_status::attempt_replayed);
    assert(fixture.trace.size() == 4);
}

void test_restore_rejects_manifest_before_staging() {
    const std::vector<std::function<void(context_store_two_rank_manifest &)>> mutations = {
        [](auto & value) { value.checkpoint.world_size = 1; },
        [](auto & value) { value.ranks[0].logical_rank = 1; },
        [](auto & value) { value.ranks[1].logical_rank = 0; },
        [](auto & value) { value.ranks[1].ownership_digest[0] ^= 1; },
        [](auto & value) { value.ranks[1].object_digest.fill(0); },
        [](auto & value) { value.ranks[1].verified_bytes = 0; },
        [](auto & value) { value.ranks[1].verified_components = 0; },
        [](auto & value) {
            value.ranks[1].durable_receipt_nonce = value.ranks[0].durable_receipt_nonce;
        },
    };
    for (const auto & mutate : mutations) {
        restore_fixture fixture;
        mutate(fixture.candidate);
        context_store_two_rank_coordinator coordinator;
        const auto result = coordinator.restore(
            fixture.value, fixture.candidate, fixture.providers);
        assert(result.status == context_store_two_rank_status::invalid_manifest);
        assert(fixture.trace.empty() && !result.commit_authorized &&
               !result.recreation_required);
    }
}

void test_restore_binds_durable_capture_authority() {
    restore_fixture fixture;
    fixture.candidate.ranks[1].durable_receipt_nonce = digest(0xf8);
    context_store_two_rank_coordinator coordinator;
    const auto result = coordinator.restore(
        fixture.value, fixture.candidate, fixture.providers);
    assert(result.status == context_store_two_rank_status::receipt_mismatch);
    assert(!result.commit_authorized && !result.restore_accepted &&
           !result.recreation_required);
    assert(fixture.rank0.commit_calls == 0 && fixture.rank1.commit_calls == 0);
    assert((fixture.trace == std::vector<std::string> {
        "stage:0", "stage:1", "abort:1", "abort:0" }));
}

void test_restore_stage_failures_abort_without_commit() {
    const std::array<std::pair<context_store_two_rank_receipt_status,
                               context_store_two_rank_status>, 5> failures = {{
        { context_store_two_rank_receipt_status::missing,
          context_store_two_rank_status::rank_missing },
        { context_store_two_rank_receipt_status::corrupt,
          context_store_two_rank_status::rank_corrupt },
        { context_store_two_rank_receipt_status::incompatible,
          context_store_two_rank_status::rank_incompatible },
        { context_store_two_rank_receipt_status::timed_out,
          context_store_two_rank_status::rank_timed_out },
        { context_store_two_rank_receipt_status::rejected,
          context_store_two_rank_status::rank_rejected },
    }};
    for (const auto & failure : failures) {
        restore_fixture fixture;
        fixture.rank1.stage_value.status = failure.first;
        context_store_two_rank_coordinator coordinator;
        const auto result = coordinator.restore(
            fixture.value, fixture.candidate, fixture.providers);
        assert(result.status == failure.second);
        assert(!result.commit_authorized && !result.restore_accepted &&
               !result.recreation_required);
        assert(fixture.rank0.commit_calls == 0 && fixture.rank1.commit_calls == 0);
        assert((fixture.trace == std::vector<std::string> {
            "stage:0", "stage:1", "abort:1", "abort:0" }));
    }

    {
        restore_fixture fixture;
        fixture.rank1.stage_value.logical_rank = 0;
        context_store_two_rank_coordinator coordinator;
        const auto result = coordinator.restore(
            fixture.value, fixture.candidate, fixture.providers);
        assert(result.status == context_store_two_rank_status::receipt_mismatch);
        assert(fixture.rank0.commit_calls == 0 && fixture.rank1.commit_calls == 0);
    }
    {
        restore_fixture fixture;
        fixture.rank1.stage_value.receipt_nonce = fixture.rank0.stage_value.receipt_nonce;
        context_store_two_rank_coordinator coordinator;
        const auto result = coordinator.restore(
            fixture.value, fixture.candidate, fixture.providers);
        assert(result.status == context_store_two_rank_status::duplicate_receipt);
        assert(fixture.rank0.commit_calls == 0 && fixture.rank1.commit_calls == 0);
    }
}

void test_restore_commit_failure_boundary() {
    {
        restore_fixture fixture;
        fixture.rank1.commit_value.outcome =
            context_store_two_rank_commit_outcome::definitely_not_applied;
        fixture.rank1.commit_value.receipt = not_applied_receipt(
            fixture.value, fixture.candidate.ranks[1], fixture.rank1.stage_value);
        context_store_two_rank_coordinator coordinator;
        const auto result = coordinator.restore(
            fixture.value, fixture.candidate, fixture.providers);
        assert(result.status == context_store_two_rank_status::commit_not_applied);
        assert(result.commit_authorized && !result.restore_accepted &&
               !result.recreation_required && fixture.rank0.commit_calls == 0);
    }
    {
        restore_fixture fixture;
        fixture.rank1.commit_value.outcome =
            context_store_two_rank_commit_outcome::definitely_not_applied;
        fixture.rank1.commit_value.receipt = not_applied_receipt(
            fixture.value, fixture.candidate.ranks[1], fixture.rank1.stage_value);
        fixture.rank1.commit_value.receipt.attempt_nonce[0] ^= 1;
        context_store_two_rank_coordinator coordinator;
        const auto result = coordinator.restore(
            fixture.value, fixture.candidate, fixture.providers);
        assert(result.status == context_store_two_rank_status::recreation_required);
        assert(result.recreation_required && fixture.rank0.commit_calls == 0);
    }
    {
        restore_fixture fixture;
        fixture.rank1.commit_value.outcome =
            static_cast<context_store_two_rank_commit_outcome>(0xff);
        context_store_two_rank_coordinator coordinator;
        const auto result = coordinator.restore(
            fixture.value, fixture.candidate, fixture.providers);
        assert(result.status == context_store_two_rank_status::recreation_required);
        assert(result.recreation_required && fixture.rank0.commit_calls == 0);
    }
    {
        restore_fixture fixture;
        fixture.rank1.commit_value.outcome =
            context_store_two_rank_commit_outcome::outcome_unknown;
        context_store_two_rank_coordinator coordinator;
        const auto result = coordinator.restore(
            fixture.value, fixture.candidate, fixture.providers);
        assert(result.status == context_store_two_rank_status::commit_outcome_unknown);
        assert(result.recreation_required && !result.restore_accepted &&
               fixture.rank0.commit_calls == 0);
    }
    {
        restore_fixture fixture;
        fixture.rank1.commit_value.receipt.object_digest[0] ^= 1;
        context_store_two_rank_coordinator coordinator;
        const auto result = coordinator.restore(
            fixture.value, fixture.candidate, fixture.providers);
        assert(result.status == context_store_two_rank_status::recreation_required);
        assert(result.recreation_required && fixture.rank0.commit_calls == 0);
    }
    {
        restore_fixture fixture;
        fixture.rank0.commit_value.outcome =
            context_store_two_rank_commit_outcome::definitely_not_applied;
        fixture.rank0.commit_value.receipt = not_applied_receipt(
            fixture.value, fixture.candidate.ranks[0], fixture.rank0.stage_value);
        context_store_two_rank_coordinator coordinator;
        const auto result = coordinator.restore(
            fixture.value, fixture.candidate, fixture.providers);
        assert(result.status == context_store_two_rank_status::recreation_required);
        assert(result.recreation_required && !result.restore_accepted);
        assert((fixture.trace == std::vector<std::string> {
            "stage:0", "stage:1", "commit-remote:1", "apply-local:0",
            "abort:1", "abort:0" }));
    }
}

void test_publication_outcomes_are_typed() {
    {
        capture_fixture fixture;
        fixture.publisher.outcome =
            context_store_two_rank_publication_outcome::definitely_not_published;
        context_store_two_rank_coordinator coordinator;
        const auto result = coordinator.capture_and_publish(
            fixture.value, fixture.requests, fixture.providers, fixture.publisher);
        assert(result.status ==
            context_store_two_rank_status::publication_not_completed);
        assert(result.publication_authorized && !result.recreation_required);
        assert(fixture.rank0.abort_calls == 1 && fixture.rank1.abort_calls == 1);
    }
    {
        capture_fixture fixture;
        fixture.publisher.outcome =
            context_store_two_rank_publication_outcome::outcome_unknown;
        context_store_two_rank_coordinator coordinator;
        const auto result = coordinator.capture_and_publish(
            fixture.value, fixture.requests, fixture.providers, fixture.publisher);
        assert(result.status ==
            context_store_two_rank_status::publication_outcome_unknown);
        assert(result.publication_authorized && !result.restore_accepted);
        assert(fixture.rank0.abort_calls == 0 && fixture.rank1.abort_calls == 0);
    }
}

} // namespace

int main() {
    test_capture_success_and_replay();
    test_capture_failures_are_whole_attempts();
    test_capture_exact_receipts_and_roster();
    test_attempt_ledger_fails_closed();
    test_restore_success_and_replay();
    test_restore_rejects_manifest_before_staging();
    test_restore_binds_durable_capture_authority();
    test_restore_stage_failures_abort_without_commit();
    test_restore_commit_failure_boundary();
    test_publication_outcomes_are_typed();
    return 0;
}
