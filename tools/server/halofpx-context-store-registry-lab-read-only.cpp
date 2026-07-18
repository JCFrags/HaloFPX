#include "halofpx-context-store-registry-lab-read-only-internal.h"

#include <algorithm>
#include <type_traits>
#include <utility>

namespace halofpx::registry_lab_read_only_test {
namespace {

template<class T, size_t N> bool all_zero(const std::array<T, N> & value) noexcept {
    uint8_t aggregate = 0;
    for (T byte : value) aggregate |= static_cast<uint8_t>(byte);
    return aggregate == 0;
}

bool valid_credential(const credential_owner & owner) noexcept {
    if (!owner.owns || owner.key_id.size == 0 || owner.key_id.size > context_store_registered_id_max_bytes || owner.generation == 0 || all_zero(owner.secret)) return false;
    for (size_t i = 0; i < owner.key_id.size; ++i) {
        const uint8_t byte = static_cast<uint8_t>(owner.key_id.bytes[i]);
        if (byte == 0 || byte > 0x7f) return false;
    }
    return owner.key_id.bytes[owner.key_id.size] == '\0';
}

void wipe_credential(credential_owner & owner) noexcept {
    owner.clear();
}

template<size_t N> void wipe(std::array<uint8_t, N> & value) noexcept {
    volatile uint8_t * output = value.data();
    for (size_t i = 0; i < N; ++i) output[i] = 0;
}

template<size_t N> bool valid_durable_file(const modeled_file<N> & source) noexcept {
    return source.durable_length <= N && (source.durable_present || (!source.durable_complete && source.durable_length == 0));
}

template<size_t N> bool valid_restart_file(bool present, bool complete, size_t length, const std::array<uint8_t, N> & bytes) noexcept {
    if (length > N || (!present && (complete || length != 0))) return false;
    for (size_t i = length; i < N; ++i) if (bytes[i] != 0) return false;
    return true;
}

template<size_t N> void save_file(const modeled_file<N> & source, bool & present, bool & complete, size_t & length, std::array<uint8_t, N> & bytes) noexcept {
    present = source.durable_present;
    complete = source.durable_complete;
    length = source.durable_length;
    bytes.fill(0);
    for (size_t i = 0; i < length; ++i) bytes[i] = source.durable_bytes[i];
}

template<size_t N> void load_file(modeled_file<N> & target, bool present, bool complete, size_t length, const std::array<uint8_t, N> & bytes) noexcept {
    target = {};
    target.live_present = target.durable_present = present;
    target.live_complete = target.durable_complete = complete;
    target.live_length = target.durable_length = length;
    for (size_t i = 0; i < length; ++i) target.live_bytes[i] = target.durable_bytes[i] = bytes[i];
}

bool digest_zero(const context_store_format_digest & digest) noexcept { return all_zero(digest); }

bool valid_durable_projection(const fixed_state & state) noexcept {
#define VALID1024(NAME) if (!valid_durable_file(state.NAME)) return false
    VALID1024(marker); VALID1024(lock_file); VALID1024(head); VALID1024(quarantine); VALID1024(quarantine_staging);
#undef VALID1024
    for (const modeled_slot & slot : state.slots) {
        if (!valid_durable_file(slot.prepare) || !valid_durable_file(slot.close) || !valid_durable_file(slot.abort_record) ||
            !valid_durable_file(slot.successor_staging) || !valid_durable_file(slot.selector_staging)) return false;
    }
    const auto valid_envelope = [](const modeled_envelope & envelope) noexcept {
        return valid_durable_file(envelope.object) && envelope.durable_occupied == envelope.object.durable_present &&
            (envelope.durable_occupied || digest_zero(envelope.durable_digest));
    };
    if (!valid_envelope(state.initial_envelope)) return false;
    for (const modeled_envelope & envelope : state.successors) if (!valid_envelope(envelope)) return false;
    for (const modeled_unexpected_entry & entry : state.unexpected) {
        if (entry.durable_length > entry.durable_name.size() ||
            (!entry.durable_occupied && entry.durable_length != 0)) return false;
    }
    return true;
}

bool valid_restart_projection(const restart_image & image) noexcept {
#define VALID_RESTART_FILE(FILE) if (!valid_restart_file(FILE.present, FILE.complete, FILE.length, FILE.bytes)) return false
    VALID_RESTART_FILE(image.marker); VALID_RESTART_FILE(image.lock_file); VALID_RESTART_FILE(image.head);
    VALID_RESTART_FILE(image.quarantine); VALID_RESTART_FILE(image.quarantine_staging);
    for (const restart_slot & slot : image.slots) {
        VALID_RESTART_FILE(slot.prepare); VALID_RESTART_FILE(slot.close); VALID_RESTART_FILE(slot.abort_record);
        VALID_RESTART_FILE(slot.successor_staging); VALID_RESTART_FILE(slot.selector_staging);
    }
    const auto valid_envelope = [](const restart_envelope & envelope) noexcept {
        return valid_restart_file(envelope.object.present, envelope.object.complete, envelope.object.length, envelope.object.bytes) &&
            envelope.occupied == envelope.object.present && (envelope.occupied || digest_zero(envelope.digest));
    };
    if (!valid_envelope(image.initial_envelope)) return false;
    for (const restart_envelope & envelope : image.successors) if (!valid_envelope(envelope)) return false;
    for (const restart_unexpected_entry & entry : image.unexpected) {
        if (entry.length > entry.bounded_name.size() || (!entry.occupied && entry.length != 0)) return false;
        for (size_t i = entry.length; i < entry.bounded_name.size(); ++i) if (entry.bounded_name[i] != 0) return false;
    }
#undef VALID_RESTART_FILE
    return true;
}

bool contains_secret(const uint8_t * bytes, size_t length, const std::array<uint8_t, 32> & secret) noexcept {
    if (length < secret.size()) return false;
    for (size_t i = 0; i + secret.size() <= length; ++i) {
        bool equal = true;
        for (size_t j = 0; j < secret.size(); ++j) equal = equal && bytes[i + j] == secret[j];
        if (equal) return true;
    }
    return false;
}

template<size_t N> bool durable_file_contains_secret(const modeled_file<N> & file, const std::array<uint8_t, 32> & secret) noexcept {
    return file.durable_present && file.durable_length <= N && contains_secret(file.durable_bytes.data(), file.durable_length, secret);
}

bool durable_projection_contains_secret(const fixed_state & state, const std::array<uint8_t, 32> & secret) noexcept {
#define HAS1024(NAME) if (durable_file_contains_secret(state.NAME, secret)) return true
    HAS1024(marker); HAS1024(lock_file); HAS1024(head); HAS1024(quarantine); HAS1024(quarantine_staging);
#undef HAS1024
    for (const modeled_slot & slot : state.slots) {
        if (durable_file_contains_secret(slot.prepare, secret) || durable_file_contains_secret(slot.close, secret) ||
            durable_file_contains_secret(slot.abort_record, secret) || durable_file_contains_secret(slot.successor_staging, secret) ||
            durable_file_contains_secret(slot.selector_staging, secret)) return true;
    }
    const auto envelope_has = [&secret](const modeled_envelope & envelope) noexcept {
        return (envelope.durable_occupied && contains_secret(envelope.durable_digest.data(), envelope.durable_digest.size(), secret)) ||
            durable_file_contains_secret(envelope.object, secret);
    };
    if (envelope_has(state.initial_envelope)) return true;
    for (const modeled_envelope & envelope : state.successors) if (envelope_has(envelope)) return true;
    for (const modeled_unexpected_entry & entry : state.unexpected)
        if (entry.durable_occupied && contains_secret(entry.durable_name.data(), entry.durable_length, secret)) return true;
    return false;
}

bool restart_projection_contains_secret(const restart_image & image, const std::array<uint8_t, 32> & secret) noexcept {
    const auto file_has = [&secret](const auto & file) noexcept {
        return file.present && contains_secret(file.bytes.data(), file.length, secret);
    };
    if (file_has(image.marker) || file_has(image.lock_file) || file_has(image.head) || file_has(image.quarantine) || file_has(image.quarantine_staging)) return true;
    for (const restart_slot & slot : image.slots)
        if (file_has(slot.prepare) || file_has(slot.close) || file_has(slot.abort_record) || file_has(slot.successor_staging) || file_has(slot.selector_staging)) return true;
    if ((image.initial_envelope.occupied && contains_secret(image.initial_envelope.digest.data(), image.initial_envelope.digest.size(), secret)) || file_has(image.initial_envelope.object)) return true;
    for (const restart_envelope & envelope : image.successors)
        if ((envelope.occupied && contains_secret(envelope.digest.data(), envelope.digest.size(), secret)) || file_has(envelope.object)) return true;
    for (const restart_unexpected_entry & entry : image.unexpected)
        if (entry.occupied && contains_secret(entry.bounded_name.data(), entry.length, secret)) return true;
    return false;
}

bool credential_zero(const credential_owner & owner) noexcept {
    return !owner.owns && owner.generation == 0 && owner.key_id.size == 0 && all_zero(owner.key_id.bytes) && all_zero(owner.secret);
}

status map_confirmed(operation op, primitive_code code) noexcept {
    if (code == primitive_code::busy) return status::busy_no_mutation;
    if (code == primitive_code::unsupported) return status::unsupported_no_mutation;
    if (code == primitive_code::invalid_request) return status::invalid_request_no_mutation;
    if (code == primitive_code::capacity_exhausted) return status::capacity_exhausted_no_mutation;
    if (code == primitive_code::reserve_exhausted) return status::reserve_exhausted_no_mutation;
    if ((op == operation::preflight || op == operation::snapshot_load) && (code == primitive_code::unavailable || code == primitive_code::io_failure)) return status::quarantined_or_unavailable;
    return status::invalid_request_no_mutation;
}

static_assert(std::is_nothrow_default_constructible_v<fixed_state>);
static_assert(std::is_nothrow_copy_constructible_v<restart_image>);

} // namespace

fixture::fixture() noexcept = default;
void credential_owner::clear() noexcept {
    key_id = {}; generation = 0;
    volatile uint8_t * output = secret.data();
    for (size_t i = 0; i < secret.size(); ++i) output[i] = 0;
    owns = false;
}
credential_owner::~credential_owner() noexcept { clear(); }
credential_owner::credential_owner(credential_owner && other) noexcept
    : key_id(other.key_id), generation(other.generation), secret(other.secret), owns(other.owns) { other.clear(); }
credential_owner & credential_owner::operator=(credential_owner && other) noexcept {
    if (this != &other) { clear(); key_id = other.key_id; generation = other.generation; secret = other.secret; owns = other.owns; other.clear(); }
    return *this;
}
fixed_state & fixture::state() noexcept { return state_; }
const fixed_state & fixture::state() const noexcept { return state_; }

size_t fixture::begin(uint64_t invocation_id, uint8_t process_slot, credential_owner && credential, const script & immutable_script) noexcept {
    size_t handle = max_invocations;
    for (size_t i = 0; i < invocations_.size(); ++i) {
        if (!invocations_[i].occupied || invocations_[i].current == phase::complete) {
            handle = i;
            break;
        }
    }
    if (handle == max_invocations) { wipe_credential(credential); return max_invocations; }
    invocation & current = invocations_[handle];
    current = invocation {};
    current.occupied = true;
    current.id = invocation_id;
    current.process = process_slot;
    current.credential = std::move(credential);
    current.current = phase::operations;
    current.immutable_script = immutable_script;
    current.derived.fill(0xa1);
    current.tag.fill(0xb2);
    current.scratch.fill(0xc3);
    current.witness.fill(0xd4);

    bool valid = invocation_id != 0 && process_slot < max_processes && valid_credential(current.credential);
    for (const invocation & other : invocations_) if (&other != &current && other.occupied && other.current != phase::complete && other.current != phase::dead && other.id == invocation_id) valid = false;
    for (size_t i = 0; i < 4; ++i) {
        valid = valid && immutable_script.entries[i].op == static_cast<operation>(i + 1) && admitted_payload(immutable_script.entries[i]);
    }
    if (valid) {
        const primitive_code derived = guard_owner_[process_slot] == 0 ? primitive_code::ok : primitive_code::busy;
        valid = immutable_script.entries[0].code == derived;
    }
    if (!valid) {
        wipe_credential(current.credential); wipe(current.derived); wipe(current.tag); wipe(current.scratch); wipe(current.witness);
        current.rejection_wipe_verified = credential_zero(current.credential) && all_zero(current.derived) && all_zero(current.tag) && all_zero(current.scratch) && all_zero(current.witness);
        current.pending = { visibility::ordinary_result, status::invalid_request_no_mutation };
        current.current = phase::complete;
        return handle;
    }
    (void) step(handle); // Entry executes operation 1 atomically, then pauses at the first boundary.
    return handle;
}

void fixture::finish_ordinary(invocation & current, status value) noexcept {
    current.pending = { visibility::not_visible, value };
    current.current = phase::cleanup_wipe;
}

void fixture::kill_process(uint8_t process, const restart_image * projected_restart) noexcept {
    for (invocation & current : invocations_) {
        if (!current.occupied || current.process != process || current.current == phase::complete || current.current == phase::dead) continue;
        const bool secret_absent = projected_restart != nullptr
            ? !restart_projection_contains_secret(*projected_restart, current.credential.secret)
            : !durable_projection_contains_secret(state_, current.credential.secret);
        wipe_credential(current.credential); wipe(current.derived); wipe(current.tag); wipe(current.scratch); wipe(current.witness);
        current.pending.state = visibility::dead_process_no_result;
        current.current = phase::dead;
        current.teardown = { current.id, process, credential_zero(current.credential),
            all_zero(current.derived) && all_zero(current.tag) && all_zero(current.scratch) && all_zero(current.witness), secret_absent };
    }
    if (process < guard_owner_.size()) guard_owner_[process] = 0;
    if (writer_process_ == process) { writer_owner_ = 0; writer_process_ = 0xff; }
}

bool fixture::step(size_t handle) noexcept {
    if (handle >= invocations_.size()) return false;
    invocation & current = invocations_[handle];
    if (!current.occupied || current.current == phase::complete || current.current == phase::dead || current.current == phase::free) return false;
    if (current.current == phase::cleanup_wipe) {
        wipe_credential(current.credential); wipe(current.derived); wipe(current.tag); wipe(current.scratch); wipe(current.witness);
        current.ordinary_wipe_verified = credential_zero(current.credential) && all_zero(current.derived) && all_zero(current.tag) && all_zero(current.scratch) && all_zero(current.witness);
        if (!current.ordinary_wipe_verified) return false;
        current.events[current.event_count++] = { 90 }; current.current = phase::cleanup_lock; return true;
    }
    if (current.current == phase::cleanup_lock) {
        if (!current.ordinary_wipe_verified) return false;
        if (writer_owner_ == current.id && writer_process_ == current.process) { writer_owner_ = 0; writer_process_ = 0xff; }
        current.events[current.event_count++] = { 91 }; current.current = phase::cleanup_guard; return true;
    }
    if (current.current == phase::cleanup_guard) {
        if (guard_owner_[current.process] == current.id) guard_owner_[current.process] = 0;
        current.events[current.event_count++] = { 92 };
        if (!current.boundary) current.pending.state = visibility::ordinary_result;
        current.current = phase::complete; return true;
    }

    if (current.cursor >= current.immutable_script.entries.size()) return false;
    const primitive_product product = current.immutable_script.entries[current.cursor];
    primitive_code derived = product.code;
    if (product.op == operation::guard_acquire) derived = guard_owner_[current.process] == 0 ? primitive_code::ok : primitive_code::busy;
    if (product.op == operation::writer_lock_acquire && product.code != primitive_code::unsupported)
        derived = writer_owner_ == 0 ? primitive_code::ok : primitive_code::busy;
    if (derived != product.code) {
        finish_ordinary(current, status::invalid_request_no_mutation);
        return true;
    }

    current.events[current.event_count++] = { static_cast<uint16_t>(product.op) };
    ++current.cursor;
    if (product.op == operation::guard_acquire && product.code == primitive_code::ok) guard_owner_[current.process] = current.id;
    if (product.op == operation::writer_lock_acquire && product.code == primitive_code::ok) { writer_owner_ = current.id; writer_process_ = current.process; }
    if (product.completed == completion::process_death) { kill_process(current.process); return true; }
    if (product.completed == completion::response_lost) { finish_ordinary(current, status::quarantined_or_unavailable); return true; }
    if (product.code != primitive_code::ok) { finish_ordinary(current, map_confirmed(product.op, product.code)); return true; }
    if (product.op == operation::snapshot_load) {
        current.events[current.event_count++] = { 200 };
        current.boundary = true;
        finish_ordinary(current, status::invalid_request_no_mutation);
    }
    return true;
}

result_view fixture::result(size_t handle) const noexcept {
    if (handle >= invocations_.size() || !invocations_[handle].occupied) return {};
    const invocation & current = invocations_[handle];
    if (current.current == phase::dead) return { visibility::dead_process_no_result, status::invalid_request_no_mutation };
    if (current.current != phase::complete) return {};
    return current.pending;
}

size_t fixture::trace_size(size_t handle) const noexcept { return handle < invocations_.size() && invocations_[handle].occupied ? invocations_[handle].event_count : 0; }
trace_entry fixture::trace(size_t handle, size_t index) const noexcept { return handle < invocations_.size() && index < invocations_[handle].event_count ? invocations_[handle].events[index] : trace_entry {}; }
bool fixture::invocation_dead(size_t handle) const noexcept { return handle < invocations_.size() && invocations_[handle].current == phase::dead; }
bool fixture::rejection_wipe_audited(size_t handle) const noexcept {
    return handle < invocations_.size() && invocations_[handle].occupied && invocations_[handle].rejection_wipe_verified;
}
bool fixture::ordinary_wipe_audited(size_t handle) const noexcept {
    return handle < invocations_.size() && invocations_[handle].occupied && invocations_[handle].ordinary_wipe_verified;
}
size_t fixture::teardown_audit_count() const noexcept {
    size_t count = 0;
    for (const invocation & current : invocations_) count += current.current == phase::dead && current.teardown.invocation_id != 0 ? 1U : 0U;
    return count;
}
restart_teardown_audit fixture::teardown_audit(size_t index) const noexcept {
    for (const invocation & current : invocations_) {
        if (current.current != phase::dead || current.teardown.invocation_id == 0) continue;
        if (index == 0) return current.teardown;
        --index;
    }
    return {};
}

bool fixture::serialize_restart(restart_image & image) const noexcept {
    if (!valid_durable_projection(state_)) return false;
#define SAVE1024(NAME) save_file(state_.NAME, image.NAME.present, image.NAME.complete, image.NAME.length, image.NAME.bytes)
    SAVE1024(marker); SAVE1024(lock_file); SAVE1024(head); SAVE1024(quarantine); SAVE1024(quarantine_staging);
#undef SAVE1024
    for (size_t i = 0; i < state_.slots.size(); ++i) {
        save_file(state_.slots[i].prepare, image.slots[i].prepare.present, image.slots[i].prepare.complete, image.slots[i].prepare.length, image.slots[i].prepare.bytes);
#define SAVE_SLOT(NAME) save_file(state_.slots[i].NAME, image.slots[i].NAME.present, image.slots[i].NAME.complete, image.slots[i].NAME.length, image.slots[i].NAME.bytes)
        SAVE_SLOT(close); SAVE_SLOT(abort_record); SAVE_SLOT(successor_staging); SAVE_SLOT(selector_staging);
#undef SAVE_SLOT
    }
    image.initial_envelope.occupied = state_.initial_envelope.durable_occupied;
    image.initial_envelope.digest = state_.initial_envelope.durable_digest;
    save_file(state_.initial_envelope.object, image.initial_envelope.object.present, image.initial_envelope.object.complete, image.initial_envelope.object.length, image.initial_envelope.object.bytes);
    for (size_t i = 0; i < state_.successors.size(); ++i) {
        image.successors[i].occupied = state_.successors[i].durable_occupied; image.successors[i].digest = state_.successors[i].durable_digest;
        save_file(state_.successors[i].object, image.successors[i].object.present, image.successors[i].object.complete, image.successors[i].object.length, image.successors[i].object.bytes);
    }
    for (size_t i = 0; i < state_.unexpected.size(); ++i) {
        image.unexpected[i] = {};
        image.unexpected[i].occupied = state_.unexpected[i].durable_occupied;
        image.unexpected[i].length = state_.unexpected[i].durable_length;
        for (size_t j = 0; j < image.unexpected[i].length; ++j) image.unexpected[i].bounded_name[j] = state_.unexpected[i].durable_name[j];
    }
    image.root_directory = state_.root_directory.durable_projection; image.attempts_directory = state_.attempts_directory.durable_projection;
    image.staging_directory = state_.staging_directory.durable_projection; image.envelopes_directory = state_.envelopes_directory.durable_projection;
    return true;
}

bool fixture::restore_restart(const restart_image & image, uint8_t restarted_process_slot) noexcept {
    if (restarted_process_slot >= max_processes || !valid_restart_projection(image)) return false;
    kill_process(restarted_process_slot, &image);
#define LOAD1024(NAME) load_file(state_.NAME, image.NAME.present, image.NAME.complete, image.NAME.length, image.NAME.bytes)
    LOAD1024(marker); LOAD1024(lock_file); LOAD1024(head); LOAD1024(quarantine); LOAD1024(quarantine_staging);
#undef LOAD1024
    for (size_t i = 0; i < state_.slots.size(); ++i) {
        load_file(state_.slots[i].prepare, image.slots[i].prepare.present, image.slots[i].prepare.complete, image.slots[i].prepare.length, image.slots[i].prepare.bytes);
#define LOAD_SLOT(NAME) load_file(state_.slots[i].NAME, image.slots[i].NAME.present, image.slots[i].NAME.complete, image.slots[i].NAME.length, image.slots[i].NAME.bytes)
        LOAD_SLOT(close); LOAD_SLOT(abort_record); LOAD_SLOT(successor_staging); LOAD_SLOT(selector_staging);
#undef LOAD_SLOT
    }
    state_.initial_envelope.live_occupied = state_.initial_envelope.durable_occupied = image.initial_envelope.occupied;
    state_.initial_envelope.live_digest = state_.initial_envelope.durable_digest = image.initial_envelope.digest;
    load_file(state_.initial_envelope.object, image.initial_envelope.object.present, image.initial_envelope.object.complete, image.initial_envelope.object.length, image.initial_envelope.object.bytes);
    for (size_t i = 0; i < state_.successors.size(); ++i) {
        state_.successors[i].live_occupied = state_.successors[i].durable_occupied = image.successors[i].occupied;
        state_.successors[i].live_digest = state_.successors[i].durable_digest = image.successors[i].digest;
        load_file(state_.successors[i].object, image.successors[i].object.present, image.successors[i].object.complete, image.successors[i].object.length, image.successors[i].object.bytes);
    }
    for (size_t i = 0; i < state_.unexpected.size(); ++i) {
        state_.unexpected[i] = {};
        state_.unexpected[i].live_occupied = state_.unexpected[i].durable_occupied = image.unexpected[i].occupied;
        state_.unexpected[i].live_length = state_.unexpected[i].durable_length = image.unexpected[i].length;
        for (size_t j = 0; j < image.unexpected[i].length; ++j)
            state_.unexpected[i].live_name[j] = state_.unexpected[i].durable_name[j] = image.unexpected[i].bounded_name[j];
    }
    state_.root_directory = { image.root_directory, image.root_directory }; state_.attempts_directory = { image.attempts_directory, image.attempts_directory };
    state_.staging_directory = { image.staging_directory, image.staging_directory }; state_.envelopes_directory = { image.envelopes_directory, image.envelopes_directory };
    return true;
}

} // namespace halofpx::registry_lab_read_only_test
