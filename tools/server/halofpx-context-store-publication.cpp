#include "halofpx-context-store-publication.h"

#include <limits>
#include <algorithm>

namespace halofpx {

context_store_publication_backend::~context_store_publication_backend() = default;

namespace {

bool fixed_digest_equal(const context_store_format_digest & left,
        const context_store_format_digest & right) noexcept {
    volatile uint8_t difference = 0;
    for (size_t index = 0; index < left.size(); ++index) {
        difference = static_cast<uint8_t>(difference | static_cast<uint8_t>(left[index] ^ right[index]));
    }
    return difference == 0;
}

bool same_lineage_domain(
        const context_store_publication_anchor & left,
        const context_store_publication_anchor & right) noexcept {
    const auto * l = left.body();
    const auto * r = right.body();
    const auto * lk = left.authentication_key_id();
    const auto * rk = right.authentication_key_id();
    const auto * lb = left.authority_binding();
    const auto * rb = right.authority_binding();
    return l != nullptr && r != nullptr && lk != nullptr && rk != nullptr && lb != nullptr && rb != nullptr &&
        l->store_uuid == r->store_uuid && l->namespace_id == r->namespace_id &&
        l->checkpoint_lineage_id == r->checkpoint_lineage_id && l->policy_epoch == r->policy_epoch &&
        l->manifest_key_generation == r->manifest_key_generation && l->authority_epoch == r->authority_epoch &&
        lk->size == rk->size && std::equal(lk->bytes.begin(), lk->bytes.begin() + lk->size, rk->bytes.begin()) &&
        left.authentication_key_generation() == right.authentication_key_generation() && fixed_digest_equal(*lb, *rb);
}

bool valid_transition(const context_store_publication_request & request) noexcept {
    const auto & predecessor = request.expected_predecessor;
    const auto & next = request.next;
    const auto * predecessor_body = predecessor.body();
    const auto * next_body = next.body();
    const bool nonzero_attempt = request.attempt_id != context_store_publication_id {};
    return predecessor_body != nullptr && next_body != nullptr && nonzero_attempt &&
        request.object_count > 0 &&
        request.object_count <= context_store_publication_max_objects_v1 &&
        predecessor_body->generation >= 1 &&
        predecessor_body->generation != std::numeric_limits<uint64_t>::max() &&
        same_lineage_domain(predecessor, next) &&
        next_body->generation == predecessor_body->generation + 1 &&
        next_body->has_predecessor &&
        next_body->predecessor_manifest_digest == predecessor_body->selected_manifest_digest;
}

bool exact_anchor(
        const context_store_publication_anchor & left,
        const context_store_publication_anchor & right) noexcept {
    if (!left.authenticated() || !right.authenticated() ||
        left.envelope_size() != right.envelope_size() ||
        left.envelope_digest() == nullptr || right.envelope_digest() == nullptr ||
        *left.envelope_digest() != *right.envelope_digest()) return false;
    return std::equal(left.envelope_data(), left.envelope_data() + left.envelope_size(), right.envelope_data());
}

class active_guard {
public:
    explicit active_guard(std::atomic_flag & flag) noexcept : flag_(flag) {}
    ~active_guard() { flag_.clear(std::memory_order_release); }
private:
    std::atomic_flag & flag_;
};

context_store_publication_status ordinary_failure(
        context_store_publication_step_result step,
        context_store_publication_status collision) noexcept {
    if (step == context_store_publication_step_result::conflict) return collision;
    if (step == context_store_publication_step_result::attempt_fenced) {
        return context_store_publication_status::attempt_fenced;
    }
    if (step == context_store_publication_step_result::sync_error) {
        return context_store_publication_status::sync_error;
    }
    return context_store_publication_status::storage_error;
}

bool completed(context_store_publication_step_result step, bool allow_equal = false) noexcept {
    return step == context_store_publication_step_result::ok ||
        (allow_equal && step == context_store_publication_step_result::already_equal);
}

} // namespace

context_store_publication_writer::context_store_publication_writer(
        context_store_publication_root_fence & root_fence) noexcept :
    root_fence_(root_fence) {
}

context_store_publication_result context_store_publication_writer::publish(
        const context_store_publication_request & request,
        context_store_publication_backend & backend) noexcept {
    context_store_publication_result result;
    if (root_fence_.active_.test_and_set(std::memory_order_acquire)) {
        result.status = context_store_publication_status::writer_busy;
        return result;
    }
    active_guard guard(root_fence_.active_);

    if (!valid_transition(request)) {
        result.status = context_store_publication_status::invalid_request;
        return result;
    }

    bool attempt_started = false;
    bool anchor_attempted = false;
    const auto abandon = [&]() noexcept {
        if (!attempt_started) return true;
        try {
            const auto abandoned = backend.abandon_attempt(request.attempt_id);
            if (!completed(abandoned)) return false;
            attempt_started = false;
            return true;
        } catch (...) {
            return false;
        }
    };
    const auto fence_uncertain = [&]() noexcept {
        try {
            const auto fenced = backend.fence_attempt_uncertain(request.attempt_id);
            result.attempt_fence_confirmed = completed(fenced);
        } catch (...) {
            result.attempt_fence_confirmed = false;
        }
        return result.attempt_fence_confirmed;
    };
    const auto fail_after_abandon = [&]() noexcept {
        if (!abandon()) {
            result.status = context_store_publication_status::attempt_fencing_uncertain;
            fence_uncertain();
        }
        result.durability_acknowledged = false;
        return result;
    };
    try {
        context_store_publication_anchor current;
        auto step = backend.read_anchor(current);
        if (step == context_store_publication_step_result::anchor_absent) {
            result.status = context_store_publication_status::bootstrap_required;
            return result;
        }
        if (!completed(step)) {
            result.status = ordinary_failure(step, context_store_publication_status::storage_error);
            return result;
        }
        ++result.completed_steps;
        if (!current.authenticated()) {
            result.status = context_store_publication_status::storage_error;
            return result;
        }
        if (!exact_anchor(current, request.expected_predecessor)) {
            result.status = context_store_publication_status::stale_predecessor;
            return result;
        }

        step = backend.begin_attempt(
            request.attempt_id, request.expected_predecessor, request.next,
            request.object_count);
        if (step == context_store_publication_step_result::stale_predecessor) {
            result.status = context_store_publication_status::stale_predecessor;
            return result;
        }
        if (step == context_store_publication_step_result::attempt_fenced) {
            result.status = context_store_publication_status::attempt_fenced;
            return result;
        }
        if (!completed(step)) {
            result.status = context_store_publication_status::attempt_fencing_uncertain;
            fence_uncertain();
            return result;
        }
        ++result.completed_steps;
        attempt_started = true;

        for (size_t index = 0; index < request.object_count; ++index) {
            const auto run = [&](auto operation, context_store_publication_status collision, bool allow_equal = false) {
                step = (backend.*operation)(request.attempt_id, index);
                if (completed(step, allow_equal)) {
                    ++result.completed_steps;
                    return true;
                }
                result.status = ordinary_failure(step, collision);
                return false;
            };
            if (!run(&context_store_publication_backend::stage_object, context_store_publication_status::object_collision) ||
                !run(&context_store_publication_backend::write_object, context_store_publication_status::object_collision) ||
                !run(&context_store_publication_backend::verify_object, context_store_publication_status::object_collision) ||
                !run(&context_store_publication_backend::sync_object_file, context_store_publication_status::object_collision) ||
                !run(&context_store_publication_backend::publish_object_no_replace, context_store_publication_status::object_collision, true) ||
                !run(&context_store_publication_backend::sync_object_directory, context_store_publication_status::object_collision)) {
                return fail_after_abandon();
            }
        }

        const auto run_manifest = [&](auto operation, bool allow_equal = false) {
            step = (backend.*operation)(request.attempt_id);
            if (completed(step, allow_equal)) {
                ++result.completed_steps;
                return true;
            }
            result.status = ordinary_failure(step, context_store_publication_status::manifest_collision);
            return false;
        };
        if (!run_manifest(&context_store_publication_backend::stage_manifest) ||
            !run_manifest(&context_store_publication_backend::write_manifest)) {
            return fail_after_abandon();
        }

        context_store_digest verified_manifest_digest {};
        step = backend.verify_manifest(request.attempt_id, verified_manifest_digest);
        if (!completed(step)) {
            result.status = ordinary_failure(step, context_store_publication_status::manifest_collision);
            return fail_after_abandon();
        }
        if (request.next.body() == nullptr || verified_manifest_digest != request.next.body()->selected_manifest_digest) {
            result.status = context_store_publication_status::manifest_identity_mismatch;
            return fail_after_abandon();
        }
        ++result.completed_steps;

        if (!run_manifest(&context_store_publication_backend::sync_manifest_file) ||
            !run_manifest(&context_store_publication_backend::publish_manifest_no_replace, true) ||
            !run_manifest(&context_store_publication_backend::sync_manifest_directory)) {
            return fail_after_abandon();
        }

        anchor_attempted = true;
        step = backend.replace_anchor_atomically(
            request.attempt_id, request.expected_predecessor, request.next);
        if (step == context_store_publication_step_result::stale_predecessor) {
            // This typed result is admitted only when the backend guarantees
            // that the atomic replacement did not take effect.
            result.status = context_store_publication_status::stale_predecessor;
            anchor_attempted = false;
            return fail_after_abandon();
        }
        if (step == context_store_publication_step_result::attempt_fenced) {
            result.status = context_store_publication_status::attempt_fenced;
            anchor_attempted = false;
            return fail_after_abandon();
        }
        if (!completed(step)) {
            // The abstract seam cannot prove that a failed or interrupted
            // replacement did not complete late.
            result.status = context_store_publication_status::anchor_visibility_uncertain;
            fence_uncertain();
            return result;
        }
        ++result.completed_steps;
        result.anchor_replaced = true;

        step = backend.sync_anchor(request.attempt_id, request.next);
        if (!completed(step)) {
            result.status = context_store_publication_status::anchor_visibility_uncertain;
            fence_uncertain();
            return result;
        }
        ++result.completed_steps;

        step = backend.close_durable_attempt(request.attempt_id, request.next);
        if (!completed(step)) {
            result.status = context_store_publication_status::anchor_visibility_uncertain;
            fence_uncertain();
            return result;
        }
        ++result.completed_steps;
        attempt_started = false;
        result.status = context_store_publication_status::published;
        result.durability_acknowledged = true;
        return result;
    } catch (...) {
        if (anchor_attempted) {
            result.status = context_store_publication_status::anchor_visibility_uncertain;
            fence_uncertain();
        } else {
            result.status = context_store_publication_status::storage_error;
            if (!abandon()) {
                result.status = context_store_publication_status::attempt_fencing_uncertain;
                fence_uncertain();
            }
        }
        result.durability_acknowledged = false;
        return result;
    }
}

const char * context_store_publication_status_name(
        context_store_publication_status status) noexcept {
    switch (status) {
        case context_store_publication_status::published: return "published";
        case context_store_publication_status::invalid_request: return "invalid_request";
        case context_store_publication_status::bootstrap_required: return "bootstrap_required";
        case context_store_publication_status::stale_predecessor: return "stale_predecessor";
        case context_store_publication_status::attempt_fenced: return "attempt_fenced";
        case context_store_publication_status::writer_busy: return "writer_busy";
        case context_store_publication_status::object_collision: return "object_collision";
        case context_store_publication_status::manifest_collision: return "manifest_collision";
        case context_store_publication_status::manifest_identity_mismatch: return "manifest_identity_mismatch";
        case context_store_publication_status::storage_error: return "storage_error";
        case context_store_publication_status::sync_error: return "sync_error";
        case context_store_publication_status::anchor_visibility_uncertain: return "anchor_visibility_uncertain";
        case context_store_publication_status::attempt_fencing_uncertain: return "attempt_fencing_uncertain";
    }
    return "unknown";
}

} // namespace halofpx
