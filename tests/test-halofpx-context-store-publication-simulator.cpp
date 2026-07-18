#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>

#include "halofpx-context-store-publication-simulator.h"

#include <array>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace {

using namespace halofpx;

context_store_publication_anchor make_anchor(uint64_t generation) {
    context_store_publication_anchor value;
    value.store_id[0] = 1;
    value.namespace_id[0] = 2;
    value.checkpoint_lineage_id[0] = 3;
    value.policy_epoch = 4;
    value.key_generation = 5;
    value.authority_epoch = 6;
    value.generation = generation;
    value.manifest_digest[0] = static_cast<uint8_t>(0x20 + generation);
    value.predecessor_manifest_digest[0] = generation == 0 ? 0 : static_cast<uint8_t>(0x1f + generation);
    return value;
}

context_store_publication_request make_request() {
    context_store_publication_request request;
    request.attempt_id[0] = 0xa5;
    request.expected_predecessor = make_anchor(7);
    request.next = make_anchor(8);
    request.next.predecessor_manifest_digest = request.expected_predecessor.manifest_digest;
    request.object_count = 2;
    return request;
}

struct operation_key {
    context_store_publication_simulator_operation operation;
    size_t index;
};

std::vector<operation_key> operation_matrix() {
    const size_t none = context_store_publication_simulator_no_index;
    return {
        { context_store_publication_simulator_operation::read_anchor, none },
        { context_store_publication_simulator_operation::begin_attempt, none },
        { context_store_publication_simulator_operation::stage_object, 0 },
        { context_store_publication_simulator_operation::write_object, 0 },
        { context_store_publication_simulator_operation::verify_object, 0 },
        { context_store_publication_simulator_operation::sync_object_file, 0 },
        { context_store_publication_simulator_operation::publish_object, 0 },
        { context_store_publication_simulator_operation::sync_object_directory, 0 },
        { context_store_publication_simulator_operation::stage_object, 1 },
        { context_store_publication_simulator_operation::write_object, 1 },
        { context_store_publication_simulator_operation::verify_object, 1 },
        { context_store_publication_simulator_operation::sync_object_file, 1 },
        { context_store_publication_simulator_operation::publish_object, 1 },
        { context_store_publication_simulator_operation::sync_object_directory, 1 },
        { context_store_publication_simulator_operation::stage_manifest, none },
        { context_store_publication_simulator_operation::write_manifest, none },
        { context_store_publication_simulator_operation::verify_manifest, none },
        { context_store_publication_simulator_operation::sync_manifest_file, none },
        { context_store_publication_simulator_operation::publish_manifest, none },
        { context_store_publication_simulator_operation::sync_manifest_directory, none },
        { context_store_publication_simulator_operation::replace_anchor, none },
        { context_store_publication_simulator_operation::sync_anchor, none },
        { context_store_publication_simulator_operation::close_attempt, none },
    };
}

void test_success_and_exact_trace() {
    auto request = make_request();
    context_store_publication_root_fence fence;
    context_store_publication_writer writer(fence);
    context_store_publication_simulator simulator(
        request.expected_predecessor, request.next, request.object_count);
    auto result = writer.publish(request, simulator);
    assert(result.status == context_store_publication_status::published);
    assert(result.durability_acknowledged && result.anchor_replaced);
    assert(simulator.recover() == context_store_publication_simulator_recovery::new_generation);

    const auto operations = operation_matrix();
    assert(simulator.trace().size() == operations.size() * 2);
    for (size_t i = 0; i < operations.size(); ++i) {
        const auto & before = simulator.trace()[2 * i];
        const auto & after = simulator.trace()[2 * i + 1];
        assert(before.operation == operations[i].operation && before.index == operations[i].index);
        assert(after.operation == operations[i].operation && after.index == operations[i].index);
        assert(before.phase == context_store_publication_simulator_phase::before);
        assert(after.phase == context_store_publication_simulator_phase::after);
        assert(before.result == context_store_publication_step_result::ok);
        assert(after.result == context_store_publication_step_result::ok);
        assert(!before.injected && !after.injected);
    }
    for (const auto & object : simulator.objects()) {
        assert(object.published_live && object.published_durable);
        assert(!object.temp_live);
    }
    assert(simulator.manifest().published_live && simulator.manifest().published_durable);

    context_store_publication_root_fence repeat_fence;
    context_store_publication_writer repeat_writer(repeat_fence);
    context_store_publication_simulator repeat(
        request.expected_predecessor, request.next, request.object_count);
    assert(repeat_writer.publish(request, repeat).status == context_store_publication_status::published);
    assert(repeat.trace().size() == simulator.trace().size());
    for (size_t i = 0; i < simulator.trace().size(); ++i) {
        assert(repeat.trace()[i].operation == simulator.trace()[i].operation);
        assert(repeat.trace()[i].index == simulator.trace()[i].index);
        assert(repeat.trace()[i].phase == simulator.trace()[i].phase);
        assert(repeat.trace()[i].result == simulator.trace()[i].result);
        assert(repeat.trace()[i].injected == simulator.trace()[i].injected);
    }

    for (bool retain_namespace : { false, true }) {
        for (bool retain_anchor : { false, true }) {
            context_store_publication_root_fence local_fence;
            context_store_publication_writer local_writer(local_fence);
            context_store_publication_simulator local(
                request.expected_predecessor, request.next, request.object_count);
            auto local_result = local_writer.publish(request, local);
            assert(local_result.status == context_store_publication_status::published);
            local.crash({ retain_namespace, retain_anchor });
            assert(local.recover() == context_store_publication_simulator_recovery::new_generation);
        }
    }
}

context_store_publication_simulator_recovery expected_after_crash(
        context_store_publication_simulator_operation operation,
        context_store_publication_simulator_phase phase,
        bool retain_anchor) {
    if (operation == context_store_publication_simulator_operation::replace_anchor &&
        phase == context_store_publication_simulator_phase::after) {
        return retain_anchor ? context_store_publication_simulator_recovery::new_generation :
            context_store_publication_simulator_recovery::old_generation;
    }
    if (operation == context_store_publication_simulator_operation::sync_anchor) {
        if (phase == context_store_publication_simulator_phase::after) {
            return context_store_publication_simulator_recovery::new_generation;
        }
        return retain_anchor ? context_store_publication_simulator_recovery::new_generation :
            context_store_publication_simulator_recovery::old_generation;
    }
    if (operation == context_store_publication_simulator_operation::close_attempt) {
        return context_store_publication_simulator_recovery::new_generation;
    }
    return context_store_publication_simulator_recovery::old_generation;
}

void test_every_before_after_resource_failure_and_crash_policy() {
    const std::array<context_store_publication_step_result, 8> faults = {
        context_store_publication_step_result::no_space,
        context_store_publication_step_result::quota_exhausted,
        context_store_publication_step_result::reserve_exhausted,
        context_store_publication_step_result::read_only,
        context_store_publication_step_result::io_error,
        context_store_publication_step_result::interrupted,
        context_store_publication_step_result::storage_error,
        context_store_publication_step_result::sync_error,
    };
    const auto operations = operation_matrix();
    auto request = make_request();
    size_t runs = 0;

    for (const auto & operation : operations) {
        for (auto phase : { context_store_publication_simulator_phase::before,
                            context_store_publication_simulator_phase::after }) {
            for (auto fault : faults) {
                for (bool retain_namespace : { false, true }) {
                    for (bool retain_anchor : { false, true }) {
                        context_store_publication_root_fence fence;
                        context_store_publication_writer writer(fence);
                        context_store_publication_simulator simulator(
                            request.expected_predecessor, request.next, request.object_count);
                        simulator.set_failpoint({ true, operation.operation, operation.index, phase, fault });
                        auto result = writer.publish(request, simulator);
                        assert(result.status != context_store_publication_status::published);
                        assert(!result.durability_acknowledged);
                        size_t injected = 0;
                        for (const auto & trace : simulator.trace()) {
                            if (trace.injected) {
                                ++injected;
                                assert(trace.operation == operation.operation);
                                assert(trace.index == operation.index);
                                assert(trace.phase == phase);
                                assert(trace.result == fault);
                            }
                        }
                        assert(injected == 1);
                        simulator.crash({ retain_namespace, retain_anchor });
                        assert(simulator.recover() == expected_after_crash(
                            operation.operation, phase, retain_anchor));
                        ++runs;
                    }
                }
            }
        }
    }
    assert(runs == 23 * 2 * 8 * 4);
}

void test_namespace_live_durable_projection() {
    auto request = make_request();
    const size_t none = context_store_publication_simulator_no_index;
    for (bool retain_namespace : { false, true }) {
        {
            context_store_publication_root_fence fence;
            context_store_publication_writer writer(fence);
            context_store_publication_simulator simulator(
                request.expected_predecessor, request.next, request.object_count);
            simulator.set_failpoint({ true,
                context_store_publication_simulator_operation::publish_object, 0,
                context_store_publication_simulator_phase::after,
                context_store_publication_step_result::storage_error });
            assert(writer.publish(request, simulator).status == context_store_publication_status::storage_error);
            assert(simulator.objects()[0].published_live);
            assert(!simulator.objects()[0].published_durable);
            simulator.crash({ retain_namespace, false });
            assert(simulator.objects()[0].published_live == retain_namespace);
            assert(simulator.objects()[0].published_durable == retain_namespace);
            assert(simulator.retained_garbage_count() == (retain_namespace ? 1 : 0));
        }
        {
            context_store_publication_root_fence fence;
            context_store_publication_writer writer(fence);
            context_store_publication_simulator simulator(
                request.expected_predecessor, request.next, request.object_count);
            simulator.set_failpoint({ true,
                context_store_publication_simulator_operation::sync_object_directory, 0,
                context_store_publication_simulator_phase::after,
                context_store_publication_step_result::sync_error });
            assert(writer.publish(request, simulator).status == context_store_publication_status::sync_error);
            assert(simulator.objects()[0].published_live && simulator.objects()[0].published_durable);
            simulator.crash({ retain_namespace, false });
            assert(simulator.objects()[0].published_live && simulator.objects()[0].published_durable);
            assert(simulator.retained_garbage_count() == 1);
        }
        {
            context_store_publication_root_fence fence;
            context_store_publication_writer writer(fence);
            context_store_publication_simulator simulator(
                request.expected_predecessor, request.next, request.object_count);
            simulator.set_failpoint({ true,
                context_store_publication_simulator_operation::publish_manifest, none,
                context_store_publication_simulator_phase::after,
                context_store_publication_step_result::storage_error });
            assert(writer.publish(request, simulator).status == context_store_publication_status::storage_error);
            assert(simulator.manifest().published_live && !simulator.manifest().published_durable);
            simulator.crash({ retain_namespace, false });
            assert(simulator.manifest().published_live == retain_namespace);
            assert(simulator.manifest().published_durable == retain_namespace);
            assert(simulator.retained_garbage_count() == (retain_namespace ? 3 : 2));
        }
        {
            context_store_publication_root_fence fence;
            context_store_publication_writer writer(fence);
            context_store_publication_simulator simulator(
                request.expected_predecessor, request.next, request.object_count);
            simulator.set_failpoint({ true,
                context_store_publication_simulator_operation::sync_manifest_directory, none,
                context_store_publication_simulator_phase::after,
                context_store_publication_step_result::sync_error });
            assert(writer.publish(request, simulator).status == context_store_publication_status::sync_error);
            assert(simulator.manifest().published_live && simulator.manifest().published_durable);
            simulator.crash({ retain_namespace, false });
            assert(simulator.manifest().published_live && simulator.manifest().published_durable);
            assert(simulator.retained_garbage_count() == 3);
        }
    }
}

void test_collisions_garbage_and_corruption_as_miss() {
    auto request = make_request();
    for (size_t index : { size_t(0), size_t(1) }) {
        context_store_publication_root_fence fence;
        context_store_publication_writer writer(fence);
        context_store_publication_simulator simulator(
            request.expected_predecessor, request.next, request.object_count);
        simulator.set_object_destination(index, context_store_publication_simulator_collision::unequal);
        auto result = writer.publish(request, simulator);
        assert(result.status == context_store_publication_status::object_collision);
        simulator.crash({ false, false });
        assert(simulator.recover() == context_store_publication_simulator_recovery::old_generation);
        assert(simulator.retained_garbage_count() == index + 1);
    }
    {
        context_store_publication_root_fence fence;
        context_store_publication_writer writer(fence);
        context_store_publication_simulator simulator(
            request.expected_predecessor, request.next, request.object_count);
        simulator.set_manifest_destination(context_store_publication_simulator_collision::unequal);
        auto result = writer.publish(request, simulator);
        assert(result.status == context_store_publication_status::manifest_collision);
        simulator.crash({ false, false });
        assert(simulator.recover() == context_store_publication_simulator_recovery::old_generation);
        assert(simulator.retained_garbage_count() == 3);
    }
    for (size_t equal_index : { size_t(0), size_t(1), size_t(2) }) {
        context_store_publication_root_fence fence;
        context_store_publication_writer writer(fence);
        context_store_publication_simulator simulator(
            request.expected_predecessor, request.next, request.object_count);
        if (equal_index < 2) {
            simulator.set_object_destination(equal_index, context_store_publication_simulator_collision::equal);
        } else {
            simulator.set_manifest_destination(context_store_publication_simulator_collision::equal);
        }
        auto result = writer.publish(request, simulator);
        assert(result.status == context_store_publication_status::published);
        simulator.crash({ false, false });
        assert(simulator.recover() == context_store_publication_simulator_recovery::new_generation);
        assert(simulator.retained_garbage_count() == 1);
    }
    {
        context_store_publication_root_fence fence;
        context_store_publication_writer writer(fence);
        context_store_publication_simulator simulator(
            request.expected_predecessor, request.next, request.object_count);
        auto result = writer.publish(request, simulator);
        assert(result.status == context_store_publication_status::published);
        simulator.set_manifest_destination(context_store_publication_simulator_collision::absent);
        assert(simulator.recover() == context_store_publication_simulator_recovery::miss);
    }
    {
        context_store_publication_root_fence fence;
        context_store_publication_writer writer(fence);
        context_store_publication_simulator simulator(
            request.expected_predecessor, request.next, request.object_count);
        assert(writer.publish(request, simulator).status == context_store_publication_status::published);
        simulator.set_manifest_destination(context_store_publication_simulator_collision::unequal);
        assert(simulator.recover() == context_store_publication_simulator_recovery::miss);
    }
    for (size_t index : { size_t(0), size_t(1) }) {
        context_store_publication_root_fence fence;
        context_store_publication_writer writer(fence);
        context_store_publication_simulator simulator(
            request.expected_predecessor, request.next, request.object_count);
        auto result = writer.publish(request, simulator);
        assert(result.status == context_store_publication_status::published);
        simulator.set_object_destination(index, context_store_publication_simulator_collision::absent);
        assert(simulator.recover() == context_store_publication_simulator_recovery::miss);
    }
    for (size_t index : { size_t(0), size_t(1) }) {
        context_store_publication_root_fence fence;
        context_store_publication_writer writer(fence);
        context_store_publication_simulator simulator(
            request.expected_predecessor, request.next, request.object_count);
        assert(writer.publish(request, simulator).status == context_store_publication_status::published);
        simulator.set_object_destination(index, context_store_publication_simulator_collision::unequal);
        assert(simulator.recover() == context_store_publication_simulator_recovery::miss);
    }
}

void test_manifest_binding_bounds_and_names() {
    auto request = make_request();
    {
        context_store_publication_root_fence fence;
        context_store_publication_writer writer(fence);
        context_store_publication_simulator simulator(
            request.expected_predecessor, request.next, request.object_count);
        auto wrong = request.next.manifest_digest;
        wrong[0] ^= 1;
        simulator.set_verified_manifest_digest(wrong);
        auto result = writer.publish(request, simulator);
        assert(result.status == context_store_publication_status::manifest_identity_mismatch);
        assert(simulator.recover() == context_store_publication_simulator_recovery::old_generation);
    }
    context_store_publication_simulator invalid(
        request.expected_predecessor, request.next, context_store_publication_max_objects_v1 + 1);
    context_store_publication_anchor anchor;
    assert(invalid.read_anchor(anchor) == context_store_publication_step_result::storage_error);
    assert(invalid.objects().empty());
    assert(invalid.stage_manifest(request.attempt_id) == context_store_publication_step_result::attempt_fenced);
    assert(!invalid.manifest().temp_live);
    assert(std::string(context_store_publication_simulator_operation_name(
        context_store_publication_simulator_operation::replace_anchor)) == "replace_anchor");
    assert(std::string(context_store_publication_simulator_recovery_name(
        context_store_publication_simulator_recovery::miss)) == "miss");
    assert(std::string(context_store_publication_simulator_operation_name(
        static_cast<context_store_publication_simulator_operation>(255))) == "unknown");
    assert(std::string(context_store_publication_simulator_recovery_name(
        static_cast<context_store_publication_simulator_recovery>(255))) == "unknown");
}

void test_sequential_stale_retry_and_maximum_bounds() {
    auto request = make_request();
    for (bool after_crash : { false, true }) {
        context_store_publication_root_fence first_fence;
        context_store_publication_writer first_writer(first_fence);
        context_store_publication_simulator first(
            request.expected_predecessor, request.next, request.object_count);
        auto committed = first_writer.publish(request, first);
        assert(committed.status == context_store_publication_status::published);
        if (after_crash) first.crash({ false, false });
        assert(first.recover() == context_store_publication_simulator_recovery::new_generation);

        auto generation_nine = make_anchor(9);
        generation_nine.predecessor_manifest_digest = request.next.manifest_digest;
        context_store_publication_root_fence retry_fence;
        context_store_publication_writer retry_writer(retry_fence);
        context_store_publication_simulator retry(
            request.next, generation_nine, request.object_count);
        auto stale = retry_writer.publish(request, retry);
        assert(stale.status == context_store_publication_status::stale_predecessor);
        assert(stale.completed_steps == 1);
        assert(retry.trace().size() == 2);
        for (const auto & object : retry.objects()) assert(!object.temp_live);
    }

    auto maximum = request;
    maximum.object_count = context_store_publication_max_objects_v1;
    context_store_publication_root_fence fence;
    context_store_publication_writer writer(fence);
    context_store_publication_simulator simulator(
        maximum.expected_predecessor, maximum.next, maximum.object_count);
    auto result = writer.publish(maximum, simulator);
    assert(result.status == context_store_publication_status::published);
    assert(result.completed_steps == 6 * context_store_publication_max_objects_v1 + 11);
    assert(simulator.trace().size() == 2 * result.completed_steps);
    assert(!simulator.trace_overflowed());
    context_store_publication_anchor anchor;
    assert(simulator.read_anchor(anchor) == context_store_publication_step_result::storage_error);
    assert(simulator.trace_overflowed());
    assert(simulator.trace().size() == 2 * result.completed_steps);
}

void test_predecessor_chain_control() {
    auto request = make_request();
    {
        context_store_publication_root_fence fence;
        context_store_publication_writer writer(fence);
        context_store_publication_simulator simulator(
            request.expected_predecessor, request.next, request.object_count);
        simulator.invalidate_predecessor_chain();
        auto result = writer.publish(request, simulator);
        assert(result.status == context_store_publication_status::storage_error);
        assert(result.completed_steps == 0);
        assert(simulator.recover() == context_store_publication_simulator_recovery::miss);
        for (const auto & object : simulator.objects()) assert(!object.temp_live);
    }
    {
        context_store_publication_root_fence fence;
        context_store_publication_writer writer(fence);
        context_store_publication_simulator simulator(
            request.expected_predecessor, request.next, request.object_count);
        assert(writer.publish(request, simulator).status == context_store_publication_status::published);
        simulator.invalidate_predecessor_chain();
        assert(simulator.recover() == context_store_publication_simulator_recovery::miss);
    }
}

void test_post_linearization_stale_injection_is_uncertain() {
    auto request = make_request();
    context_store_publication_root_fence fence;
    context_store_publication_writer writer(fence);
    context_store_publication_simulator simulator(
        request.expected_predecessor, request.next, request.object_count);
    simulator.set_failpoint({
        true,
        context_store_publication_simulator_operation::replace_anchor,
        context_store_publication_simulator_no_index,
        context_store_publication_simulator_phase::after,
        context_store_publication_step_result::stale_predecessor,
    });
    auto result = writer.publish(request, simulator);
    assert(result.status == context_store_publication_status::anchor_visibility_uncertain);
    assert(!result.anchor_replaced && !result.durability_acknowledged);
    assert(simulator.live_anchor().generation == request.next.generation);
}

void test_attempt_lifecycle_and_late_call_fencing() {
    auto request = make_request();
    auto second_id = request.attempt_id;
    second_id[0] ^= 0x33;
    auto third_id = request.attempt_id;
    third_id[0] ^= 0x55;

    for (auto phase : { context_store_publication_simulator_phase::before,
                        context_store_publication_simulator_phase::after }) {
        context_store_publication_root_fence fence;
        context_store_publication_writer writer(fence);
        context_store_publication_simulator simulator(
            request.expected_predecessor, request.next, request.object_count);
        simulator.set_failpoint({ true,
            context_store_publication_simulator_operation::begin_attempt,
            context_store_publication_simulator_no_index, phase,
            context_store_publication_step_result::interrupted });
        auto result = writer.publish(request, simulator);
        assert(result.status == context_store_publication_status::attempt_fencing_uncertain);
        assert(result.attempt_fence_confirmed);
        simulator.clear_failpoint();
        assert(simulator.begin_attempt(second_id, request.expected_predecessor,
            request.next, request.object_count) == context_store_publication_step_result::attempt_fenced);
    }

    {
        context_store_publication_root_fence fence;
        context_store_publication_writer writer(fence);
        context_store_publication_simulator simulator(
            request.expected_predecessor, request.next, request.object_count);
        simulator.set_abandonment_uncertain(true);
        simulator.set_failpoint({ true,
            context_store_publication_simulator_operation::stage_object, 0,
            context_store_publication_simulator_phase::before,
            context_store_publication_step_result::io_error });
        auto result = writer.publish(request, simulator);
        assert(result.status == context_store_publication_status::attempt_fencing_uncertain);
        assert(result.attempt_fence_confirmed);
        simulator.clear_failpoint();
        assert(simulator.begin_attempt(second_id, request.expected_predecessor,
            request.next, request.object_count) == context_store_publication_step_result::attempt_fenced);
    }

    {
        context_store_publication_simulator simulator(
            request.expected_predecessor, request.next, request.object_count);
        context_store_publication_anchor observed;
        assert(simulator.read_anchor(observed) == context_store_publication_step_result::ok);
        assert(simulator.begin_attempt(request.attempt_id, request.expected_predecessor,
            request.next, request.object_count) == context_store_publication_step_result::ok);
        assert(simulator.stage_object(second_id, 0) == context_store_publication_step_result::attempt_fenced);
        assert(!simulator.objects()[0].temp_live);
        assert(simulator.abandon_attempt(request.attempt_id) == context_store_publication_step_result::ok);
        assert(simulator.stage_object(request.attempt_id, 0) == context_store_publication_step_result::attempt_fenced);
        assert(simulator.begin_attempt(request.attempt_id, request.expected_predecessor,
            request.next, request.object_count) == context_store_publication_step_result::attempt_fenced);
        assert(simulator.begin_attempt(second_id, request.expected_predecessor,
            request.next, request.object_count) == context_store_publication_step_result::ok);
        assert(simulator.fence_attempt_uncertain(second_id) == context_store_publication_step_result::ok);
        assert(simulator.stage_object(second_id, 0) == context_store_publication_step_result::attempt_fenced);
        assert(simulator.begin_attempt(third_id, request.expected_predecessor,
            request.next, request.object_count) == context_store_publication_step_result::attempt_fenced);
    }
    {
        context_store_publication_simulator simulator(
            request.expected_predecessor, request.next, request.object_count);
        assert(simulator.begin_attempt(request.attempt_id, request.expected_predecessor,
            request.next, request.object_count) == context_store_publication_step_result::ok);
        assert(simulator.abandon_attempt(request.attempt_id) == context_store_publication_step_result::ok);
        assert(simulator.begin_attempt(second_id, request.expected_predecessor,
            request.next, request.object_count) == context_store_publication_step_result::ok);
        assert(simulator.abandon_attempt(second_id) == context_store_publication_step_result::ok);
        assert(simulator.begin_attempt(request.attempt_id, request.expected_predecessor,
            request.next, request.object_count) == context_store_publication_step_result::attempt_fenced);
    }

    const std::vector<std::function<void(
        context_store_publication_anchor &,
        context_store_publication_anchor &,
        size_t &)>> binding_mutations = {
        [](auto & expected, auto &, auto &) { expected.store_id[0] ^= 1; },
        [](auto & expected, auto &, auto &) { expected.namespace_id[0] ^= 1; },
        [](auto & expected, auto &, auto &) { expected.checkpoint_lineage_id[0] ^= 1; },
        [](auto & expected, auto &, auto &) { ++expected.policy_epoch; },
        [](auto & expected, auto &, auto &) { ++expected.key_generation; },
        [](auto & expected, auto &, auto &) { ++expected.authority_epoch; },
        [](auto &, auto & next, auto &) { next.manifest_digest[0] ^= 1; },
        [](auto &, auto & next, auto &) { next.predecessor_manifest_digest[0] ^= 1; },
        [](auto &, auto &, auto & count) { ++count; },
    };
    for (const auto & mutate : binding_mutations) {
        context_store_publication_simulator simulator(
            request.expected_predecessor, request.next, request.object_count);
        auto expected = request.expected_predecessor;
        auto next = request.next;
        auto count = request.object_count;
        mutate(expected, next, count);
        const auto status = simulator.begin_attempt(second_id, expected, next, count);
        assert(status == context_store_publication_step_result::attempt_fenced ||
               status == context_store_publication_step_result::stale_predecessor);
        assert(simulator.stage_object(second_id, 0) == context_store_publication_step_result::attempt_fenced);
    }

    {
        context_store_publication_root_fence first_fence;
        context_store_publication_writer first_writer(first_fence);
        context_store_publication_simulator simulator(
            request.expected_predecessor, request.next, request.object_count);
        simulator.set_failpoint({ true,
            context_store_publication_simulator_operation::stage_object, 0,
            context_store_publication_simulator_phase::before,
            context_store_publication_step_result::io_error });
        auto failed = first_writer.publish(request, simulator);
        assert(failed.status == context_store_publication_status::storage_error);
        simulator.clear_failpoint();
        assert(simulator.stage_object(request.attempt_id, 0) == context_store_publication_step_result::attempt_fenced);
        assert(simulator.begin_attempt(second_id, request.expected_predecessor,
            request.next, request.object_count) == context_store_publication_step_result::ok);
        assert(simulator.stage_object(second_id, 0) == context_store_publication_step_result::ok);
        assert(simulator.abandon_attempt(second_id) == context_store_publication_step_result::ok);
        assert(simulator.stage_object(second_id, 0) == context_store_publication_step_result::attempt_fenced);
    }
}

} // namespace

int main() {
    test_success_and_exact_trace();
    test_every_before_after_resource_failure_and_crash_policy();
    test_namespace_live_durable_projection();
    test_collisions_garbage_and_corruption_as_miss();
    test_manifest_binding_bounds_and_names();
    test_sequential_stale_retry_and_maximum_bounds();
    test_predecessor_chain_control();
    test_post_linearization_stale_injection_is_uncertain();
    test_attempt_lifecycle_and_late_call_fencing();
    return 0;
}
