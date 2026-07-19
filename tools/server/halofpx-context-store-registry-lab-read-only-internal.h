#pragma once

#include "halofpx-context-store-registry-lab-wire.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace halofpx::registry_lab_read_only_test {

enum class operation : uint8_t {
    guard_acquire = 1, writer_lock_acquire = 2, preflight = 3, snapshot_load = 4, recovery_validation = 5,
    action_mutation_admission = 6,
    successor_file_sync = 33, envelopes_directory_sync = 35, staging_directory_sync_after_successor = 36,
    head_file_sync = 43, root_directory_sync = 45, staging_directory_sync_after_head = 46,
    head_read = 50, successor_read = 51,
    terminal_create = 60, terminal_write = 61, terminal_readback = 62,
    terminal_file_sync = 63, attempts_directory_sync = 64,
    quarantine_event_id_acquire = 69,
    quarantine_staging_create = 70, quarantine_staging_write = 71, quarantine_staging_readback = 72,
    quarantine_file_sync = 73, quarantine_publish_rename = 74,
    quarantine_root_directory_sync = 75, quarantine_staging_directory_sync = 76,
};
enum class storage_effect : uint8_t { none, bounded_partial_bytes, complete_live, bounded_partial_durability_projection, complete_durability_projection };
enum class completion : uint8_t { response_confirmed, response_lost, process_death };
enum class primitive_code : uint8_t { ok, busy, unsupported, invalid_request, capacity_exhausted, reserve_exhausted, unavailable, io_failure };
enum class recovery_classification : uint8_t { none, continue_to_mutation, needs_successor_close, needs_predecessor_abort, needs_sticky_quarantine, blocked_by_existing_quarantine, inadmissible_initialization_artifact, attempt_replayed, capacity_exhausted, requested_slot_occupied, invalid_transition, preexisting_unattributed_material };
enum class status : uint8_t { invalid_request_no_mutation, unsupported_no_mutation, busy_no_mutation, capacity_exhausted_no_mutation, reserve_exhausted_no_mutation, attempt_replayed_no_mutation, slot_occupied_no_mutation, invalid_transition_no_mutation, preexisting_material_no_authority, uncertain_requires_recovery, quarantined_or_unavailable, recovered_not_applied_no_authority, modeled_recovered_successor_closed };
enum class quarantine_reason : uint8_t { unknown=0, marker_invalid=1, layout_or_unexpected=2, existing_quarantine=3, head_invalid=4, selected_envelope_invalid=5, journal_invalid=6, chain_contradiction=7, multiple_unresolved=8, referent_invalid=9, staging_ambiguous=10, durability_unproved=11, scope_or_root_mismatch=12, key_or_auth_mismatch=13, resource_or_io_failure=14, internal_invariant=15 };
enum class quarantine_shape : uint8_t { u0=0, uh=1, prepare=2, successor=3, none=255 };

struct quarantine_diagnosis_view {
    bool valid = false, publishable = false, authenticated_initialized_root = false;
    quarantine_reason reason = quarantine_reason::unknown;
    quarantine_shape shape = quarantine_shape::none;
    bool attributable = false, has_previous_record = false, has_head = false;
    uint8_t phase = 0;
    uint64_t slot = 0, registry_epoch = 0;
    context_store_registered_id registry_id {};
    context_store_format_digest root_id {}, path_policy_commitment {};
    context_store_format_digest attempt_id {}, operation_commitment {}, previous_record_digest {}, head_digest {}, diagnosis_commitment {};
};

constexpr quarantine_reason select_quarantine_reason_for_test(const std::array<bool, 16> & flags) noexcept {
    constexpr std::array<uint8_t, 14> precedence { 13,12,1,15,2,10,4,5,6,7,8,9,11,0 };
    for (uint8_t value : precedence) if (flags[value]) return static_cast<quarantine_reason>(value);
    return quarantine_reason::unknown;
}
bool quarantine_diagnosis_commitment_for_test(uint64_t invocation_id, const quarantine_diagnosis_view & diagnosis,
    context_store_format_digest & output) noexcept;
bool quarantine_action_commitment_for_test(uint64_t invocation_id, const quarantine_diagnosis_view & diagnosis,
    const context_store_format_digest & event_id, const context_store_format_digest & quarantine_content_digest,
    size_t quarantine_encoded_length, context_store_format_digest & output) noexcept;
struct quarantine_event_authority_test_audit {
    size_t issued = 0, consumed = 0, wrong_invocation_rejected = 0, wrong_diagnosis_rejected = 0;
    size_t replay_rejected = 0, moved_from_rejected = 0;
    bool nonzero = false, distinct = false, exact_encoding = false;
    bool invalid_binding_rejected = false, move_source_wiped = false;
    bool explicit_wipe_verified = false, destructor_clear_path_exercised = false;
};
bool quarantine_event_authority_for_test(size_t issuance_count,
    quarantine_event_authority_test_audit & audit) noexcept;
struct quarantine_event_concurrency_test_audit {
    size_t retained = 0;
    bool all_workers_completed = false, pairwise_distinct = false, exact_encoding = false;
};
bool quarantine_event_concurrency_begin_for_test(size_t worker_count, size_t issuances_per_worker) noexcept;
bool quarantine_event_concurrency_worker_for_test(size_t worker_index) noexcept;
bool quarantine_event_concurrency_finish_for_test(quarantine_event_concurrency_test_audit & audit) noexcept;
void quarantine_event_fail_next_issuance_for_test() noexcept;
struct quarantine_operation_69_issuance_audit { size_t sequence_values_consumed = 0; };
enum class quarantine_private_fault_for_test : uint8_t {
    diagnosis_commitment, encoded_byte, encoded_length, content_digest,
    action_commitment, cleared_event_witness, maximum_logical_authority,
};
class quarantine_event_id_witness {
public:
    quarantine_event_id_witness() noexcept = default;
    ~quarantine_event_id_witness() noexcept;
    quarantine_event_id_witness(const quarantine_event_id_witness &) = delete;
    quarantine_event_id_witness & operator=(const quarantine_event_id_witness &) = delete;
    quarantine_event_id_witness(quarantine_event_id_witness &&) noexcept;
    quarantine_event_id_witness & operator=(quarantine_event_id_witness &&) noexcept;
    bool consume(uint64_t, const context_store_format_digest &, context_store_format_digest &) noexcept;
    bool empty() const noexcept;
    void clear() noexcept;
private:
    bool authorize(uint64_t, const context_store_format_digest &, context_store_format_digest &) const noexcept;
    quarantine_event_id_witness(const context_store_format_digest &, uint64_t,
        const context_store_format_digest &) noexcept;
    void move_from(quarantine_event_id_witness &) noexcept;
    context_store_format_digest event_id_ {}, diagnosis_commitment_ {};
    uint64_t invocation_id_ = 0;
    bool owns_ = false;
    friend struct fake_quarantine_event_authority;
    friend class fixture;
};
struct fake_quarantine_event_authority {
    static quarantine_event_id_witness acquire(uint64_t,
        const context_store_format_digest &, bool * = nullptr) noexcept;
};
enum class visibility : uint8_t { not_visible, ordinary_result, dead_process_no_result };

struct recovery_precedence_flags {
    bool blocked = false, initializing = false, sticky = false, successor_close = false, predecessor_abort = false;
    bool replay = false, capacity = false, slot = false, invalid = false, preexisting = false;
};

constexpr recovery_classification select_recovery_precedence(const recovery_precedence_flags & value) noexcept {
    return value.blocked ? recovery_classification::blocked_by_existing_quarantine :
        value.initializing ? recovery_classification::inadmissible_initialization_artifact :
        value.sticky ? recovery_classification::needs_sticky_quarantine :
        value.successor_close ? recovery_classification::needs_successor_close :
        value.predecessor_abort ? recovery_classification::needs_predecessor_abort :
        value.replay ? recovery_classification::attempt_replayed :
        value.capacity ? recovery_classification::capacity_exhausted :
        value.slot ? recovery_classification::requested_slot_occupied :
        value.invalid ? recovery_classification::invalid_transition :
        value.preexisting ? recovery_classification::preexisting_unattributed_material :
        recovery_classification::continue_to_mutation;
}

struct primitive_product {
    operation op = operation::guard_acquire;
    storage_effect effect = storage_effect::none;
    completion completed = completion::response_confirmed;
    primitive_code code = primitive_code::ok;
    recovery_classification classification = recovery_classification::none;
};

constexpr bool valid_operation(operation value) noexcept {
    switch (value) {
        case operation::guard_acquire: case operation::writer_lock_acquire: case operation::preflight:
        case operation::snapshot_load: case operation::recovery_validation: case operation::action_mutation_admission:
        case operation::successor_file_sync: case operation::envelopes_directory_sync: case operation::staging_directory_sync_after_successor:
        case operation::head_file_sync: case operation::root_directory_sync: case operation::staging_directory_sync_after_head:
        case operation::head_read: case operation::successor_read: case operation::terminal_create: case operation::terminal_write:
        case operation::terminal_readback: case operation::terminal_file_sync: case operation::attempts_directory_sync: return true;
        case operation::quarantine_event_id_acquire: case operation::quarantine_staging_create:
        case operation::quarantine_staging_write: case operation::quarantine_staging_readback:
        case operation::quarantine_file_sync: case operation::quarantine_publish_rename:
        case operation::quarantine_root_directory_sync: case operation::quarantine_staging_directory_sync: return true;
    }
    return false;
}
constexpr bool valid_effect(storage_effect value) noexcept { return static_cast<uint8_t>(value) <= 4; }
constexpr bool valid_completion(completion value) noexcept { return static_cast<uint8_t>(value) <= 2; }
constexpr bool valid_code(primitive_code value) noexcept { return static_cast<uint8_t>(value) <= 7; }
constexpr bool valid_classification(recovery_classification value) noexcept { return static_cast<uint8_t>(value) <= 11; }

constexpr bool admitted_product(operation op, storage_effect effect, completion completed, primitive_code code) noexcept {
    if (!valid_operation(op) || !valid_effect(effect) || !valid_completion(completed) || !valid_code(code)) return false;
    const bool confirmed = completed == completion::response_confirmed;
    const bool lost = completed == completion::response_lost;
    const bool death = completed == completion::process_death;
    switch (op) {
        case operation::guard_acquire:
            return effect == storage_effect::none && (confirmed || death) && (code == primitive_code::ok || code == primitive_code::busy);
        case operation::writer_lock_acquire:
            return effect == storage_effect::none && (confirmed || death) && (code == primitive_code::ok || code == primitive_code::busy || code == primitive_code::unsupported);
        case operation::preflight:
            return effect == storage_effect::none && (confirmed || lost || death) && code != primitive_code::busy;
        case operation::snapshot_load:
        case operation::recovery_validation:
        case operation::quarantine_event_id_acquire:
            return effect == storage_effect::none && (confirmed || lost || death) && (code == primitive_code::ok || code == primitive_code::unsupported || code == primitive_code::unavailable || code == primitive_code::io_failure);
        case operation::action_mutation_admission:
            return effect == storage_effect::none && (confirmed || lost || death) &&
                (code == primitive_code::ok || code == primitive_code::capacity_exhausted || code == primitive_code::reserve_exhausted ||
                 code == primitive_code::unsupported || code == primitive_code::unavailable || code == primitive_code::io_failure);
        case operation::successor_file_sync: case operation::envelopes_directory_sync: case operation::staging_directory_sync_after_successor:
        case operation::head_file_sync: case operation::root_directory_sync: case operation::staging_directory_sync_after_head:
        case operation::terminal_file_sync: case operation::attempts_directory_sync:
        case operation::quarantine_file_sync: case operation::quarantine_root_directory_sync:
        case operation::quarantine_staging_directory_sync: {
            const bool allowed_effect = effect == storage_effect::none || effect == storage_effect::bounded_partial_durability_projection || effect == storage_effect::complete_durability_projection;
            return allowed_effect && (code == primitive_code::ok || code == primitive_code::unavailable || code == primitive_code::io_failure) &&
                !(confirmed && code == primitive_code::ok && effect != storage_effect::complete_durability_projection);
        }
        case operation::head_read: case operation::successor_read: case operation::terminal_readback:
        case operation::quarantine_staging_readback:
            return effect == storage_effect::none && (code == primitive_code::ok || code == primitive_code::unavailable || code == primitive_code::io_failure);
        case operation::terminal_create: case operation::quarantine_staging_create:
        case operation::quarantine_publish_rename: {
            const bool allowed_effect = effect == storage_effect::none || effect == storage_effect::complete_live;
            return allowed_effect && (code == primitive_code::ok || code == primitive_code::unavailable || code == primitive_code::io_failure) &&
                !(confirmed && code == primitive_code::ok && effect != storage_effect::complete_live);
        }
        case operation::terminal_write: case operation::quarantine_staging_write: {
            const bool allowed_effect = effect == storage_effect::none || effect == storage_effect::bounded_partial_bytes || effect == storage_effect::complete_live;
            return allowed_effect && (code == primitive_code::ok || code == primitive_code::unavailable || code == primitive_code::io_failure) &&
                !(confirmed && code == primitive_code::ok && effect != storage_effect::complete_live);
        }
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
    constexpr std::array<operation, 27> operations { operation::guard_acquire, operation::writer_lock_acquire, operation::preflight,
        operation::snapshot_load, operation::recovery_validation, operation::action_mutation_admission, operation::successor_file_sync,
        operation::envelopes_directory_sync, operation::staging_directory_sync_after_successor, operation::head_file_sync,
        operation::root_directory_sync, operation::staging_directory_sync_after_head, operation::head_read, operation::successor_read,
        operation::terminal_create, operation::terminal_write, operation::terminal_readback, operation::terminal_file_sync, operation::attempts_directory_sync,
        operation::quarantine_event_id_acquire, operation::quarantine_staging_create, operation::quarantine_staging_write,
        operation::quarantine_staging_readback, operation::quarantine_file_sync, operation::quarantine_publish_rename,
        operation::quarantine_root_directory_sync, operation::quarantine_staging_directory_sync };
    for (operation op : operations) for (uint8_t effect = 0; effect < 5; ++effect)
        for (uint8_t completed = 0; completed < 3; ++completed) for (uint8_t code = 0; code < 8; ++code)
            count += admitted_product(op, static_cast<storage_effect>(effect), static_cast<completion>(completed), static_cast<primitive_code>(code)) ? 1U : 0U;
    return count;
}
static_assert(admitted_algebra_count() == 497);

constexpr uint64_t registry_logical_budget_bytes = 16777216;
constexpr uint64_t registry_terminal_reservation_bytes = 1024;
constexpr uint64_t registry_minimum_reserve_bytes = 268435456;
constexpr bool admitted_logical_budget(uint64_t current) noexcept {
    return current <= UINT64_MAX - registry_terminal_reservation_bytes &&
        current + registry_terminal_reservation_bytes <= registry_logical_budget_bytes;
}

template<size_t Capacity> struct modeled_file {
    // `durable_present` is durable namespace visibility. Durable inode bytes may
    // advance before it becomes true and remain restart-hidden until dir sync.
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
    uint64_t modeled_available_bytes = 268435456;
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
    uint64_t modeled_available_bytes = 268435456;
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

struct preflight_context_v1 {
    std::array<uint8_t, 16> store_uuid {}, filesystem_uuid {}, subvolume_uuid {};
    uint64_t mount_id = 0, st_dev = 0, owner_uid = 0;
    uint32_t root_mode = 0, authority_file_mode = 0;
    uint64_t lock_st_dev = 0, lock_st_ino = 0;
    context_store_format_digest path_policy_commitment {};
    context_store_registered_id registry_id {};
    uint64_t registry_epoch = 0;
    context_store_format_digest authority_base_scope_commitment {}, registry_policy_commitment {};
    context_store_registered_id credential_key_id {};
    uint64_t credential_generation = 0;
    context_store_key_disposition inner_key_disposition = context_store_key_disposition::unknown;
    uint64_t attempt_capacity = 0, maximum_logical_authority_bytes = 0;
};

struct request_transition_v1 {
    context_store_format_digest attempt_id {}, operation_commitment {};
    uint64_t requested_slot = 0;
    std::array<uint8_t, 1024> predecessor {}, successor {}, expected_current_head {};
    size_t predecessor_length = 0, successor_length = 0, expected_current_head_length = 0;
    context_store_format_digest predecessor_digest {}, successor_digest {}, expected_current_head_digest {};
};

struct quarantine_encoding_inputs_test_audit {
    bool prepared = false, scope_exact = false, value_exact = false;
    bool transition_present = false, standalone_head_present = false, explicit_wipe_verified = false;
    quarantine_shape shape = quarantine_shape::none;
    size_t predecessor_head_size = 0, successor_head_size = 0, prepare_size = 0;
    context_store_format_digest predecessor_head_digest {}, successor_head_digest {}, prepare_digest {};
};
bool quarantine_encoding_inputs_for_test(const fixed_state & snapshot, const preflight_context_v1 & preflight,
    const credential_owner & credential, uint64_t invocation_id, const quarantine_diagnosis_view & diagnosis,
    quarantine_encoding_inputs_test_audit & audit) noexcept;

// One extra entry is retained solely so the fake can prove an exact 19-step
// successor script plus any additional operation rejects before engine entry.
struct script { std::array<primitive_product, 20> entries {}; size_t size = 5; };
struct trace_entry { uint16_t event = 0; };
struct result_view { visibility state = visibility::not_visible; status ordinary = status::invalid_request_no_mutation; };
struct restart_teardown_audit { uint64_t invocation_id = 0; uint8_t process_slot = 0; bool credential_zero = false, scratch_zero = false, serialized_secret_absent = false; };
struct restart_projection_audit {
    size_t count = 0, ordinal = 0, retained_length = 0;
    bool terminal_name_retained = false;
    bool quarantine_projection = false, quarantine_staging_retained = false;
    bool quarantine_final_retained = false, quarantine_exact_bytes = false;
};

constexpr size_t max_invocations = 64;
constexpr size_t max_processes = 4;
constexpr size_t max_trace = 22;

class fixture final {
public:
    fixture() noexcept;
    fixture(const fixture &) = delete;
    fixture & operator=(const fixture &) = delete;
    fixed_state & state() noexcept;
    const fixed_state & state() const noexcept;
    size_t begin(uint64_t invocation_id, uint8_t process_slot, credential_owner && credential,
        const preflight_context_v1 &, const request_transition_v1 &, const script & immutable_script) noexcept;
    bool step(size_t handle) noexcept;
    result_view result(size_t handle) const noexcept;
    size_t trace_size(size_t handle) const noexcept;
    trace_entry trace(size_t handle, size_t index) const noexcept;
    bool invocation_dead(size_t handle) const noexcept;
    bool rejection_wipe_audited(size_t handle) const noexcept;
    bool ordinary_wipe_audited(size_t handle) const noexcept;
    size_t teardown_audit_count() const noexcept;
    restart_teardown_audit teardown_audit(size_t index) const noexcept;
    recovery_classification derived_classification(size_t handle) const noexcept;
    quarantine_diagnosis_view quarantine_diagnosis(size_t handle) const noexcept;
    size_t scanned_slots(size_t handle) const noexcept;
    size_t kdf_calls(size_t handle) const noexcept;
    quarantine_operation_69_issuance_audit quarantine_operation_69_issuance(size_t handle) const noexcept;
    bool inject_quarantine_private_fault_for_test(size_t handle,
        quarantine_private_fault_for_test fault) noexcept;
    bool inject_quarantine_retagged_readback_for_test(size_t handle) noexcept;
    bool serialize_restart(restart_image & caller_preallocated) const noexcept;
    size_t restart_projection_count(size_t handle) const noexcept;
    bool project_restart(size_t handle, size_t ordinal, restart_image & caller_preallocated,
        restart_projection_audit & audit) const noexcept;
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
        preflight_context_v1 preflight {};
        request_transition_v1 request {};
        credential_owner credential {};
        std::array<uint8_t, 64> derived {}, tag {}, scratch {}, witness {};
        std::array<uint8_t, 1024> terminal_scratch {};
        size_t terminal_size = 0;
        uint64_t recovery_slot = 0;
        uint8_t recovery_action = 0;
        context_store_format_digest prepare_digest {}, current_head_digest {}, action_commitment {};
        context_store_format_digest action_attempt {}, action_operation {};
        context_store_format_digest action_predecessor {}, action_successor {};
        bool action_latched = false;
        context_store_format_digest quarantine_consumed_event_id {}, quarantine_content_digest {}, quarantine_action_commitment {};
        std::array<uint8_t, 1024> quarantine_scratch {};
        size_t quarantine_size = 0;
        quarantine_event_id_witness quarantine_event_authority {};
        bool quarantine_event_confirmed = false, quarantine_action_latched = false;
        size_t quarantine_sequence_values_consumed = 0;
        std::array<trace_entry, max_trace> events {};
        size_t event_count = 0;
        result_view pending {};
        bool boundary = false;
        bool rejection_wipe_verified = false;
        bool ordinary_wipe_verified = false;
        recovery_classification derived_class = recovery_classification::none;
        quarantine_diagnosis_view quarantine_plan {};
        size_t scanned = 0, derivations = 0;
        restart_teardown_audit teardown {};
    };
    fixed_state state_ {};
    fixed_state snapshot_ {};
    uint64_t snapshot_owner_ = 0;
    std::array<invocation, max_invocations> invocations_ {};
    std::array<uint64_t, max_processes> guard_owner_ {};
    uint64_t writer_owner_ = 0;
    uint8_t writer_process_ = 0xff;
    void finish_ordinary(invocation &, status) noexcept;
    void kill_process(uint8_t, const restart_image * = nullptr) noexcept;
};

} // namespace halofpx::registry_lab_read_only_test
