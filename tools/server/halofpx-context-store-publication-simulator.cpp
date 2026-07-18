#include "halofpx-context-store-publication-simulator.h"

namespace halofpx {
namespace {

bool exact_anchor(
        const context_store_publication_anchor & left,
        const context_store_publication_anchor & right) noexcept {
    return left.store_id == right.store_id &&
        left.namespace_id == right.namespace_id &&
        left.checkpoint_lineage_id == right.checkpoint_lineage_id &&
        left.policy_epoch == right.policy_epoch &&
        left.key_generation == right.key_generation &&
        left.authority_epoch == right.authority_epoch &&
        left.generation == right.generation &&
        left.manifest_digest == right.manifest_digest &&
        left.predecessor_manifest_digest == right.predecessor_manifest_digest;
}

} // namespace

context_store_publication_simulator::context_store_publication_simulator(
        const context_store_publication_anchor & predecessor,
        const context_store_publication_anchor & next,
        size_t object_count) :
    predecessor_(predecessor),
    next_(next),
    live_anchor_(predecessor),
    durable_anchor_(predecessor),
    verified_manifest_digest_(next.manifest_digest),
    objects_(object_count <= context_store_publication_max_objects_v1 ? object_count : 0),
    trace_limit_(2 * (1 + 6 * objects_.size() + 6 + 2)),
    valid_(object_count > 0 && object_count <= context_store_publication_max_objects_v1) {
    trace_.reserve(trace_limit_);
}

void context_store_publication_simulator::set_failpoint(
        const context_store_publication_simulator_failpoint & failpoint) {
    failpoint_ = failpoint;
}

void context_store_publication_simulator::clear_failpoint() noexcept {
    failpoint_.enabled = false;
}

void context_store_publication_simulator::set_object_destination(
        size_t index,
        context_store_publication_simulator_collision collision) {
    if (!valid_object(index)) return;
    auto & state = objects_[index];
    state.destination = collision;
    state.published_live = collision != context_store_publication_simulator_collision::absent;
    state.published_durable = state.published_live;
    state.published_by_attempt = false;
}

void context_store_publication_simulator::set_manifest_destination(
        context_store_publication_simulator_collision collision) noexcept {
    manifest_.destination = collision;
    manifest_.published_live = collision != context_store_publication_simulator_collision::absent;
    manifest_.published_durable = manifest_.published_live;
    manifest_.published_by_attempt = false;
}

void context_store_publication_simulator::set_verified_manifest_digest(
        const context_store_digest & digest) noexcept {
    verified_manifest_digest_ = digest;
}

void context_store_publication_simulator::invalidate_predecessor_chain() noexcept {
    predecessor_chain_valid_ = false;
}

bool context_store_publication_simulator::before(
        context_store_publication_simulator_operation operation,
        size_t index,
        context_store_publication_step_result & result) {
    if (trace_.size() >= trace_limit_) {
        trace_overflowed_ = true;
        result = context_store_publication_step_result::storage_error;
        return false;
    }
    if (failpoint_.enabled &&
        failpoint_.operation == operation &&
        failpoint_.index == index &&
        failpoint_.phase == context_store_publication_simulator_phase::before) {
        result = failpoint_.result;
        trace_.push_back({ operation, index, context_store_publication_simulator_phase::before, result, true });
        return false;
    }
    trace_.push_back({ operation, index, context_store_publication_simulator_phase::before,
                       context_store_publication_step_result::ok, false });
    return true;
}

context_store_publication_step_result context_store_publication_simulator::after(
        context_store_publication_simulator_operation operation,
        size_t index,
        context_store_publication_step_result result) {
    if (trace_.size() >= trace_limit_) {
        trace_overflowed_ = true;
        return context_store_publication_step_result::storage_error;
    }
    if (failpoint_.enabled &&
        failpoint_.operation == operation &&
        failpoint_.index == index &&
        failpoint_.phase == context_store_publication_simulator_phase::after) {
        result = failpoint_.result;
        trace_.push_back({ operation, index, context_store_publication_simulator_phase::after, result, true });
        return result;
    }
    trace_.push_back({ operation, index, context_store_publication_simulator_phase::after, result, false });
    return result;
}

bool context_store_publication_simulator::valid_object(size_t index) const noexcept {
    return valid_ && index < objects_.size();
}

context_store_publication_step_result context_store_publication_simulator::read_anchor(
        context_store_publication_anchor & anchor) {
    context_store_publication_step_result result;
    const auto op = context_store_publication_simulator_operation::read_anchor;
    if (!before(op, context_store_publication_simulator_no_index, result)) return result;
    if (!valid_ || !predecessor_chain_valid_) {
        return after(op, context_store_publication_simulator_no_index,
                     context_store_publication_step_result::storage_error);
    }
    anchor = live_anchor_;
    return after(op, context_store_publication_simulator_no_index,
                 context_store_publication_step_result::ok);
}

context_store_publication_step_result context_store_publication_simulator::stage_object(size_t index) {
    context_store_publication_step_result result;
    const auto op = context_store_publication_simulator_operation::stage_object;
    if (!before(op, index, result)) return result;
    if (!valid_object(index) || objects_[index].temp_live) {
        return after(op, index, context_store_publication_step_result::storage_error);
    }
    objects_[index].temp_live = true;
    return after(op, index, context_store_publication_step_result::ok);
}

context_store_publication_step_result context_store_publication_simulator::write_object(size_t index) {
    context_store_publication_step_result result;
    const auto op = context_store_publication_simulator_operation::write_object;
    if (!before(op, index, result)) return result;
    if (!valid_object(index) || !objects_[index].temp_live) {
        return after(op, index, context_store_publication_step_result::storage_error);
    }
    objects_[index].temp_written = true;
    return after(op, index, context_store_publication_step_result::ok);
}

context_store_publication_step_result context_store_publication_simulator::verify_object(size_t index) {
    context_store_publication_step_result result;
    const auto op = context_store_publication_simulator_operation::verify_object;
    if (!before(op, index, result)) return result;
    if (!valid_object(index) || !objects_[index].temp_written) {
        return after(op, index, context_store_publication_step_result::storage_error);
    }
    objects_[index].temp_verified = true;
    return after(op, index, context_store_publication_step_result::ok);
}

context_store_publication_step_result context_store_publication_simulator::sync_object_file(size_t index) {
    context_store_publication_step_result result;
    const auto op = context_store_publication_simulator_operation::sync_object_file;
    if (!before(op, index, result)) return result;
    if (!valid_object(index) || !objects_[index].temp_verified) {
        return after(op, index, context_store_publication_step_result::storage_error);
    }
    objects_[index].temp_durable = true;
    return after(op, index, context_store_publication_step_result::ok);
}

context_store_publication_step_result context_store_publication_simulator::publish_object_no_replace(size_t index) {
    context_store_publication_step_result result;
    const auto op = context_store_publication_simulator_operation::publish_object;
    if (!before(op, index, result)) return result;
    if (!valid_object(index) || !objects_[index].temp_durable) {
        return after(op, index, context_store_publication_step_result::storage_error);
    }
    auto & state = objects_[index];
    if (state.destination == context_store_publication_simulator_collision::unequal) {
        return after(op, index, context_store_publication_step_result::conflict);
    }
    if (state.destination == context_store_publication_simulator_collision::equal) {
        return after(op, index, context_store_publication_step_result::already_equal);
    }
    state.published_live = true;
    state.published_by_attempt = true;
    state.temp_live = false;
    state.temp_written = false;
    state.temp_verified = false;
    state.temp_durable = false;
    return after(op, index, context_store_publication_step_result::ok);
}

context_store_publication_step_result context_store_publication_simulator::sync_object_directory(size_t index) {
    context_store_publication_step_result result;
    const auto op = context_store_publication_simulator_operation::sync_object_directory;
    if (!before(op, index, result)) return result;
    if (!valid_object(index) || !objects_[index].published_live) {
        return after(op, index, context_store_publication_step_result::storage_error);
    }
    objects_[index].published_durable = true;
    return after(op, index, context_store_publication_step_result::ok);
}

context_store_publication_step_result context_store_publication_simulator::stage_manifest() {
    context_store_publication_step_result result;
    const auto op = context_store_publication_simulator_operation::stage_manifest;
    if (!before(op, context_store_publication_simulator_no_index, result)) return result;
    if (!valid_ || manifest_.temp_live) {
        return after(op, context_store_publication_simulator_no_index,
                     context_store_publication_step_result::storage_error);
    }
    manifest_.temp_live = true;
    return after(op, context_store_publication_simulator_no_index,
                 context_store_publication_step_result::ok);
}

context_store_publication_step_result context_store_publication_simulator::write_manifest() {
    context_store_publication_step_result result;
    const auto op = context_store_publication_simulator_operation::write_manifest;
    if (!before(op, context_store_publication_simulator_no_index, result)) return result;
    if (!valid_ || !manifest_.temp_live) {
        return after(op, context_store_publication_simulator_no_index,
                     context_store_publication_step_result::storage_error);
    }
    manifest_.temp_written = true;
    return after(op, context_store_publication_simulator_no_index,
                 context_store_publication_step_result::ok);
}

context_store_publication_step_result context_store_publication_simulator::verify_manifest(
        context_store_digest & digest) {
    context_store_publication_step_result result;
    const auto op = context_store_publication_simulator_operation::verify_manifest;
    if (!before(op, context_store_publication_simulator_no_index, result)) return result;
    if (!valid_ || !manifest_.temp_written) {
        return after(op, context_store_publication_simulator_no_index,
                     context_store_publication_step_result::storage_error);
    }
    manifest_.temp_verified = true;
    digest = verified_manifest_digest_;
    return after(op, context_store_publication_simulator_no_index,
                 context_store_publication_step_result::ok);
}

context_store_publication_step_result context_store_publication_simulator::sync_manifest_file() {
    context_store_publication_step_result result;
    const auto op = context_store_publication_simulator_operation::sync_manifest_file;
    if (!before(op, context_store_publication_simulator_no_index, result)) return result;
    if (!valid_ || !manifest_.temp_verified) {
        return after(op, context_store_publication_simulator_no_index,
                     context_store_publication_step_result::storage_error);
    }
    manifest_.temp_durable = true;
    return after(op, context_store_publication_simulator_no_index,
                 context_store_publication_step_result::ok);
}

context_store_publication_step_result context_store_publication_simulator::publish_manifest_no_replace() {
    context_store_publication_step_result result;
    const auto op = context_store_publication_simulator_operation::publish_manifest;
    if (!before(op, context_store_publication_simulator_no_index, result)) return result;
    if (!valid_ || !manifest_.temp_durable) {
        return after(op, context_store_publication_simulator_no_index,
                     context_store_publication_step_result::storage_error);
    }
    if (manifest_.destination == context_store_publication_simulator_collision::unequal) {
        return after(op, context_store_publication_simulator_no_index,
                     context_store_publication_step_result::conflict);
    }
    if (manifest_.destination == context_store_publication_simulator_collision::equal) {
        return after(op, context_store_publication_simulator_no_index,
                     context_store_publication_step_result::already_equal);
    }
    manifest_.published_live = true;
    manifest_.published_by_attempt = true;
    manifest_.temp_live = false;
    manifest_.temp_written = false;
    manifest_.temp_verified = false;
    manifest_.temp_durable = false;
    return after(op, context_store_publication_simulator_no_index,
                 context_store_publication_step_result::ok);
}

context_store_publication_step_result context_store_publication_simulator::sync_manifest_directory() {
    context_store_publication_step_result result;
    const auto op = context_store_publication_simulator_operation::sync_manifest_directory;
    if (!before(op, context_store_publication_simulator_no_index, result)) return result;
    if (!valid_ || !manifest_.published_live) {
        return after(op, context_store_publication_simulator_no_index,
                     context_store_publication_step_result::storage_error);
    }
    manifest_.published_durable = true;
    return after(op, context_store_publication_simulator_no_index,
                 context_store_publication_step_result::ok);
}

context_store_publication_step_result context_store_publication_simulator::replace_anchor_atomically(
        const context_store_publication_anchor & next) {
    context_store_publication_step_result result;
    const auto op = context_store_publication_simulator_operation::replace_anchor;
    if (!before(op, context_store_publication_simulator_no_index, result)) return result;
    if (!valid_ || !manifest_.published_durable || !exact_anchor(next, next_)) {
        return after(op, context_store_publication_simulator_no_index,
                     context_store_publication_step_result::storage_error);
    }
    live_anchor_ = next;
    anchor_unsynced_ = true;
    return after(op, context_store_publication_simulator_no_index,
                 context_store_publication_step_result::ok);
}

context_store_publication_step_result context_store_publication_simulator::sync_anchor() {
    context_store_publication_step_result result;
    const auto op = context_store_publication_simulator_operation::sync_anchor;
    if (!before(op, context_store_publication_simulator_no_index, result)) return result;
    if (!valid_ || !anchor_unsynced_ || !exact_anchor(live_anchor_, next_)) {
        return after(op, context_store_publication_simulator_no_index,
                     context_store_publication_step_result::sync_error);
    }
    durable_anchor_ = live_anchor_;
    anchor_unsynced_ = false;
    return after(op, context_store_publication_simulator_no_index,
                 context_store_publication_step_result::ok);
}

void context_store_publication_simulator::discard_temp(
        context_store_publication_simulator_entry_state & state,
        size_t & garbage_count) noexcept {
    if (state.temp_live) ++garbage_count;
    state.temp_live = false;
    state.temp_written = false;
    state.temp_verified = false;
    state.temp_durable = false;
}

void context_store_publication_simulator::crash(
        const context_store_publication_simulator_crash_policy & policy) noexcept {
    for (auto & object : objects_) {
        discard_temp(object, retained_garbage_count_);
        if (!object.published_durable) {
            object.published_live = policy.retain_unsynced_namespace && object.published_live;
            object.published_durable = object.published_live;
        } else {
            object.published_live = true;
        }
    }
    discard_temp(manifest_, retained_garbage_count_);
    if (!manifest_.published_durable) {
        manifest_.published_live = policy.retain_unsynced_namespace && manifest_.published_live;
        manifest_.published_durable = manifest_.published_live;
    } else {
        manifest_.published_live = true;
    }

    if (anchor_unsynced_ && policy.retain_unsynced_anchor) {
        durable_anchor_ = live_anchor_;
    } else {
        live_anchor_ = durable_anchor_;
    }
    anchor_unsynced_ = false;
}

context_store_publication_simulator_recovery context_store_publication_simulator::recover() const noexcept {
    if (!predecessor_chain_valid_) {
        return context_store_publication_simulator_recovery::miss;
    }
    if (exact_anchor(live_anchor_, predecessor_)) {
        return context_store_publication_simulator_recovery::old_generation;
    }
    if (!exact_anchor(live_anchor_, next_) || !manifest_.published_live ||
        manifest_.destination == context_store_publication_simulator_collision::unequal) {
        return context_store_publication_simulator_recovery::miss;
    }
    for (const auto & object : objects_) {
        if (!object.published_live ||
            object.destination == context_store_publication_simulator_collision::unequal) {
            return context_store_publication_simulator_recovery::miss;
        }
    }
    return context_store_publication_simulator_recovery::new_generation;
}

const std::vector<context_store_publication_simulator_trace_entry> &
context_store_publication_simulator::trace() const noexcept {
    return trace_;
}

const std::vector<context_store_publication_simulator_entry_state> &
context_store_publication_simulator::objects() const noexcept {
    return objects_;
}

const context_store_publication_simulator_entry_state &
context_store_publication_simulator::manifest() const noexcept {
    return manifest_;
}

const context_store_publication_anchor &
context_store_publication_simulator::live_anchor() const noexcept {
    return live_anchor_;
}

const context_store_publication_anchor &
context_store_publication_simulator::durable_anchor() const noexcept {
    return durable_anchor_;
}

size_t context_store_publication_simulator::retained_garbage_count() const noexcept {
    size_t count = retained_garbage_count_;
    if (recover() != context_store_publication_simulator_recovery::new_generation) {
        for (const auto & object : objects_) {
            if (object.published_live && object.published_by_attempt) ++count;
        }
        if (manifest_.published_live && manifest_.published_by_attempt) ++count;
    }
    return count;
}

bool context_store_publication_simulator::trace_overflowed() const noexcept {
    return trace_overflowed_;
}

const char * context_store_publication_simulator_operation_name(
        context_store_publication_simulator_operation operation) noexcept {
    switch (operation) {
        case context_store_publication_simulator_operation::read_anchor: return "read_anchor";
        case context_store_publication_simulator_operation::stage_object: return "stage_object";
        case context_store_publication_simulator_operation::write_object: return "write_object";
        case context_store_publication_simulator_operation::verify_object: return "verify_object";
        case context_store_publication_simulator_operation::sync_object_file: return "sync_object_file";
        case context_store_publication_simulator_operation::publish_object: return "publish_object";
        case context_store_publication_simulator_operation::sync_object_directory: return "sync_object_directory";
        case context_store_publication_simulator_operation::stage_manifest: return "stage_manifest";
        case context_store_publication_simulator_operation::write_manifest: return "write_manifest";
        case context_store_publication_simulator_operation::verify_manifest: return "verify_manifest";
        case context_store_publication_simulator_operation::sync_manifest_file: return "sync_manifest_file";
        case context_store_publication_simulator_operation::publish_manifest: return "publish_manifest";
        case context_store_publication_simulator_operation::sync_manifest_directory: return "sync_manifest_directory";
        case context_store_publication_simulator_operation::replace_anchor: return "replace_anchor";
        case context_store_publication_simulator_operation::sync_anchor: return "sync_anchor";
    }
    return "unknown";
}

const char * context_store_publication_simulator_recovery_name(
        context_store_publication_simulator_recovery recovery) noexcept {
    switch (recovery) {
        case context_store_publication_simulator_recovery::old_generation: return "old_generation";
        case context_store_publication_simulator_recovery::new_generation: return "new_generation";
        case context_store_publication_simulator_recovery::miss: return "miss";
    }
    return "unknown";
}

} // namespace halofpx
