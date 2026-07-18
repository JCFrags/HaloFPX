#pragma once

#include "halofpx-context-store-registry-lab-wire.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace halofpx::registry_lab_read_only_test {

enum class operation : uint8_t { guard_acquire = 1, writer_lock_acquire = 2, preflight = 3, snapshot_load = 4, recovery_validation = 5 };
enum class storage_effect : uint8_t { none, bounded_partial_bytes, complete_live, bounded_partial_durability_projection, complete_durability_projection };
enum class completion : uint8_t { response_confirmed, response_lost, process_death };
enum class primitive_code : uint8_t { ok, busy, unsupported, invalid_request, capacity_exhausted, reserve_exhausted, unavailable, io_failure };
enum class recovery_classification : uint8_t { none, continue_to_mutation, needs_successor_close, needs_predecessor_abort, needs_sticky_quarantine, blocked_by_existing_quarantine, inadmissible_initialization_artifact, attempt_replayed, capacity_exhausted, requested_slot_occupied, invalid_transition, preexisting_unattributed_material };
enum class status : uint8_t { invalid_request_no_mutation, unsupported_no_mutation, busy_no_mutation, capacity_exhausted_no_mutation, reserve_exhausted_no_mutation, quarantined_or_unavailable };
enum class visibility : uint8_t { not_visible, ordinary_result, dead_process_no_result };

struct primitive_product {
    operation op = operation::guard_acquire;
    storage_effect effect = storage_effect::none;
    completion completed = completion::response_confirmed;
    primitive_code code = primitive_code::ok;
    recovery_classification classification = recovery_classification::none;
};

constexpr bool valid_operation(operation value) noexcept { return static_cast<uint8_t>(value) >= 1 && static_cast<uint8_t>(value) <= 5; }
constexpr bool valid_effect(storage_effect value) noexcept { return static_cast<uint8_t>(value) <= 4; }
constexpr bool valid_completion(completion value) noexcept { return static_cast<uint8_t>(value) <= 2; }
constexpr bool valid_code(primitive_code value) noexcept { return static_cast<uint8_t>(value) <= 7; }
constexpr bool valid_classification(recovery_classification value) noexcept { return static_cast<uint8_t>(value) <= 11; }

constexpr bool admitted_product(operation op, storage_effect effect, completion completed, primitive_code code) noexcept {
    if (!valid_operation(op) || !valid_effect(effect) || !valid_completion(completed) || !valid_code(code) || effect != storage_effect::none) return false;
    const bool confirmed = completed == completion::response_confirmed;
    const bool lost = completed == completion::response_lost;
    const bool death = completed == completion::process_death;
    switch (op) {
        case operation::guard_acquire:
            return (confirmed || death) && (code == primitive_code::ok || code == primitive_code::busy);
        case operation::writer_lock_acquire:
            return (confirmed || death) && (code == primitive_code::ok || code == primitive_code::busy || code == primitive_code::unsupported);
        case operation::preflight:
            return (confirmed || lost || death) && code != primitive_code::busy;
        case operation::snapshot_load:
        case operation::recovery_validation:
            return (confirmed || lost || death) && (code == primitive_code::ok || code == primitive_code::unsupported || code == primitive_code::unavailable || code == primitive_code::io_failure);
    }
    return false;
}

constexpr bool admitted_payload(const primitive_product & product) noexcept {
    if (!admitted_product(product.op, product.effect, product.completed, product.code) || !valid_classification(product.classification)) return false;
    if (product.op != operation::recovery_validation) return product.classification == recovery_classification::none;
    return product.completed == completion::response_confirmed && product.code == primitive_code::ok
        ? product.classification != recovery_classification::none
        : product.classification == recovery_classification::none;
}

constexpr size_t admitted_algebra_count() noexcept {
    size_t count = 0;
    for (uint8_t op = 1; op <= 5; ++op) for (uint8_t effect = 0; effect < 5; ++effect)
        for (uint8_t completed = 0; completed < 3; ++completed) for (uint8_t code = 0; code < 8; ++code)
            count += admitted_product(static_cast<operation>(op), static_cast<storage_effect>(effect), static_cast<completion>(completed), static_cast<primitive_code>(code)) ? 1U : 0U;
    return count;
}
static_assert(admitted_algebra_count() == 55);

template<size_t Capacity> struct modeled_file {
    bool live_present = false, durable_present = false, live_complete = false, durable_complete = false;
    size_t live_length = 0, durable_length = 0;
    std::array<uint8_t, Capacity> live_bytes {}, durable_bytes {};
};
struct modeled_directory { bool live_projection = false, durable_projection = false; };
struct modeled_slot { modeled_file<4096> prepare; modeled_file<1024> close, abort_record, successor_staging, selector_staging; };
struct modeled_envelope {
    bool live_occupied = false, durable_occupied = false;
    context_store_format_digest live_digest {}, durable_digest {};
    modeled_file<1024> object;
};
struct modeled_unexpected_entry {
    bool live_occupied = false, durable_occupied = false;
    std::array<uint8_t, 64> live_name {}, durable_name {};
    size_t live_length = 0, durable_length = 0;
};

struct fixed_state {
    modeled_file<1024> marker, lock_file, head, quarantine, quarantine_staging;
    std::array<modeled_slot, 512> slots {};
    modeled_envelope initial_envelope;
    std::array<modeled_envelope, 512> successors {};
    std::array<modeled_unexpected_entry, 32> unexpected {};
    modeled_directory root_directory, attempts_directory, staging_directory, envelopes_directory;
};

struct restart_file_1024 { bool present = false, complete = false; size_t length = 0; std::array<uint8_t, 1024> bytes {}; };
struct restart_file_4096 { bool present = false, complete = false; size_t length = 0; std::array<uint8_t, 4096> bytes {}; };
struct restart_slot { restart_file_4096 prepare; restart_file_1024 close, abort_record, successor_staging, selector_staging; };
struct restart_envelope { bool occupied = false; context_store_format_digest digest {}; restart_file_1024 object; };
struct restart_unexpected_entry { bool occupied = false; std::array<uint8_t, 64> bounded_name {}; size_t length = 0; };
struct restart_image {
    restart_file_1024 marker, lock_file, head, quarantine, quarantine_staging;
    std::array<restart_slot, 512> slots {};
    restart_envelope initial_envelope;
    std::array<restart_envelope, 512> successors {};
    std::array<restart_unexpected_entry, 32> unexpected {};
    bool root_directory = false, attempts_directory = false, staging_directory = false, envelopes_directory = false;
};

struct credential_owner {
    credential_owner() noexcept = default;
    ~credential_owner() noexcept;
    credential_owner(const credential_owner &) = delete;
    credential_owner & operator=(const credential_owner &) = delete;
    credential_owner(credential_owner && other) noexcept;
    credential_owner & operator=(credential_owner && other) noexcept;
    void clear() noexcept;
    context_store_registered_id key_id {};
    uint64_t generation = 0;
    std::array<uint8_t, 32> secret {};
    bool owns = false;
};
struct script { std::array<primitive_product, 4> entries {}; };
struct trace_entry { uint16_t event = 0; };
struct result_view { visibility state = visibility::not_visible; status ordinary = status::invalid_request_no_mutation; };
struct restart_teardown_audit { uint64_t invocation_id = 0; uint8_t process_slot = 0; bool credential_zero = false, scratch_zero = false, serialized_secret_absent = false; };

constexpr size_t max_invocations = 64;
constexpr size_t max_processes = 4;
constexpr size_t max_trace = 8;

class fixture final {
public:
    fixture() noexcept;
    fixture(const fixture &) = delete;
    fixture & operator=(const fixture &) = delete;
    fixed_state & state() noexcept;
    const fixed_state & state() const noexcept;
    size_t begin(uint64_t invocation_id, uint8_t process_slot, credential_owner && credential, const script & immutable_script) noexcept;
    bool step(size_t handle) noexcept;
    result_view result(size_t handle) const noexcept;
    size_t trace_size(size_t handle) const noexcept;
    trace_entry trace(size_t handle, size_t index) const noexcept;
    bool invocation_dead(size_t handle) const noexcept;
    bool rejection_wipe_audited(size_t handle) const noexcept;
    bool ordinary_wipe_audited(size_t handle) const noexcept;
    size_t teardown_audit_count() const noexcept;
    restart_teardown_audit teardown_audit(size_t index) const noexcept;
    bool serialize_restart(restart_image & caller_preallocated) const noexcept;
    bool restore_restart(const restart_image & image, uint8_t restarted_process_slot) noexcept;

private:
    enum class phase : uint8_t { free, operations, cleanup_wipe, cleanup_lock, cleanup_guard, complete, dead };
    struct invocation {
        bool occupied = false;
        uint64_t id = 0;
        uint8_t process = 0;
        phase current = phase::free;
        size_t cursor = 0;
        script immutable_script {};
        credential_owner credential {};
        std::array<uint8_t, 64> derived {}, tag {}, scratch {}, witness {};
        std::array<trace_entry, max_trace> events {};
        size_t event_count = 0;
        result_view pending {};
        bool boundary = false;
        bool rejection_wipe_verified = false;
        bool ordinary_wipe_verified = false;
        restart_teardown_audit teardown {};
    };
    fixed_state state_ {};
    std::array<invocation, max_invocations> invocations_ {};
    std::array<uint64_t, max_processes> guard_owner_ {};
    uint64_t writer_owner_ = 0;
    uint8_t writer_process_ = 0xff;
    void finish_ordinary(invocation &, status) noexcept;
    void kill_process(uint8_t, const restart_image * = nullptr) noexcept;
};

} // namespace halofpx::registry_lab_read_only_test
