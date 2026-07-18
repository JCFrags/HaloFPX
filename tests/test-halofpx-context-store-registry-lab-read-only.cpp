#include "halofpx-context-store-registry-lab-read-only-internal.h"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <new>
#ifdef _MSC_VER
#include <malloc.h>
#endif

using namespace halofpx;
using namespace halofpx::registry_lab_read_only_test;

namespace {
bool fail_allocations = false;
}

void * operator new(std::size_t size) {
    if (fail_allocations) throw std::bad_alloc();
    if (void * value = std::malloc(size)) return value;
    throw std::bad_alloc();
}
void * operator new[](std::size_t size) { return ::operator new(size); }
void * operator new(std::size_t size, const std::nothrow_t &) noexcept { try { return ::operator new(size); } catch (...) { return nullptr; } }
void * operator new[](std::size_t size, const std::nothrow_t &) noexcept { try { return ::operator new[](size); } catch (...) { return nullptr; } }
void * operator new(std::size_t size, std::align_val_t alignment) {
    if (fail_allocations) throw std::bad_alloc();
    void * value = nullptr;
#ifdef _MSC_VER
    value = _aligned_malloc(size, static_cast<std::size_t>(alignment));
#else
    if (posix_memalign(&value, static_cast<std::size_t>(alignment), size) != 0) value = nullptr;
#endif
    if (value) return value;
    throw std::bad_alloc();
}
void * operator new[](std::size_t size, std::align_val_t alignment) { return ::operator new(size, alignment); }
void * operator new(std::size_t size, std::align_val_t alignment, const std::nothrow_t &) noexcept { try { return ::operator new(size, alignment); } catch (...) { return nullptr; } }
void * operator new[](std::size_t size, std::align_val_t alignment, const std::nothrow_t &) noexcept { try { return ::operator new[](size, alignment); } catch (...) { return nullptr; } }
void operator delete(void * value) noexcept { std::free(value); }
void operator delete[](void * value) noexcept { std::free(value); }
void operator delete(void * value, std::size_t) noexcept { std::free(value); }
void operator delete[](void * value, std::size_t) noexcept { std::free(value); }
void operator delete(void * value, const std::nothrow_t &) noexcept { std::free(value); }
void operator delete[](void * value, const std::nothrow_t &) noexcept { std::free(value); }
#ifdef _MSC_VER
static void halofpx_test_aligned_free(void * value) noexcept { _aligned_free(value); }
#else
static void halofpx_test_aligned_free(void * value) noexcept { std::free(value); }
#endif
void operator delete(void * value, std::align_val_t) noexcept { halofpx_test_aligned_free(value); }
void operator delete[](void * value, std::align_val_t) noexcept { halofpx_test_aligned_free(value); }
void operator delete(void * value, std::size_t, std::align_val_t) noexcept { halofpx_test_aligned_free(value); }
void operator delete[](void * value, std::size_t, std::align_val_t) noexcept { halofpx_test_aligned_free(value); }
void operator delete(void * value, std::align_val_t, const std::nothrow_t &) noexcept { halofpx_test_aligned_free(value); }
void operator delete[](void * value, std::align_val_t, const std::nothrow_t &) noexcept { halofpx_test_aligned_free(value); }

namespace {

credential_owner credential(uint8_t seed = 1) {
    credential_owner value;
    value.key_id.size = 3; value.key_id.bytes[0] = 'k'; value.key_id.bytes[1] = 'e'; value.key_id.bytes[2] = static_cast<char>('a' + (seed % 26)); value.key_id.bytes[3] = '\0';
    value.generation = seed;
    value.secret.fill(seed);
    value.owns = true;
    return value;
}

primitive_product product(operation op, completion completed = completion::response_confirmed, primitive_code code = primitive_code::ok) {
    return { op, storage_effect::none, completed, code, recovery_classification::none };
}

script normal_script() {
    return { { product(operation::guard_acquire), product(operation::writer_lock_acquire), product(operation::preflight), product(operation::snapshot_load) } };
}

void finish(fixture & f, size_t handle) {
    for (size_t guard = 0; guard < 16 && f.step(handle); ++guard) {}
}

bool credential_is_zero(const credential_owner & value) {
    if (value.owns || value.generation != 0 || value.key_id.size != 0) return false;
    for (char byte : value.key_id.bytes) if (byte != 0) return false;
    for (uint8_t byte : value.secret) if (byte != 0) return false;
    return true;
}

template<size_t N> bool same_file(const std::array<uint8_t, N> & a_bytes, const std::array<uint8_t, N> & b_bytes,
                                  bool a_present, bool b_present, bool a_complete, bool b_complete, size_t a_length, size_t b_length) {
    return a_present == b_present && a_complete == b_complete && a_length == b_length && a_bytes == b_bytes;
}

template<class File> bool same_file(const File & a, const File & b) {
    return same_file(a.bytes, b.bytes, a.present, b.present, a.complete, b.complete, a.length, b.length);
}

bool same_restart(const restart_image & a, const restart_image & b) {
    if (!same_file(a.marker, b.marker) || !same_file(a.lock_file, b.lock_file) || !same_file(a.head, b.head) ||
        !same_file(a.quarantine, b.quarantine) || !same_file(a.quarantine_staging, b.quarantine_staging)) return false;
    for (size_t i = 0; i < a.slots.size(); ++i) {
        if (!same_file(a.slots[i].prepare, b.slots[i].prepare) || !same_file(a.slots[i].close, b.slots[i].close) ||
            !same_file(a.slots[i].abort_record, b.slots[i].abort_record) || !same_file(a.slots[i].successor_staging, b.slots[i].successor_staging) ||
            !same_file(a.slots[i].selector_staging, b.slots[i].selector_staging)) return false;
    }
    const auto same_envelope = [](const restart_envelope & x, const restart_envelope & y) {
        return x.occupied == y.occupied && x.digest == y.digest && same_file(x.object, y.object);
    };
    if (!same_envelope(a.initial_envelope, b.initial_envelope)) return false;
    for (size_t i = 0; i < a.successors.size(); ++i) if (!same_envelope(a.successors[i], b.successors[i])) return false;
    for (size_t i = 0; i < a.unexpected.size(); ++i)
        if (a.unexpected[i].occupied != b.unexpected[i].occupied || a.unexpected[i].length != b.unexpected[i].length ||
            a.unexpected[i].bounded_name != b.unexpected[i].bounded_name) return false;
    return a.root_directory == b.root_directory && a.attempts_directory == b.attempts_directory &&
        a.staging_directory == b.staging_directory && a.envelopes_directory == b.envelopes_directory;
}

status expected_status(const primitive_product & p) {
    if (p.completed == completion::response_lost) return status::quarantined_or_unavailable;
    if (p.code == primitive_code::busy) return status::busy_no_mutation;
    if (p.code == primitive_code::unsupported) return status::unsupported_no_mutation;
    if (p.code == primitive_code::invalid_request) return status::invalid_request_no_mutation;
    if (p.code == primitive_code::capacity_exhausted) return status::capacity_exhausted_no_mutation;
    if (p.code == primitive_code::reserve_exhausted) return status::reserve_exhausted_no_mutation;
    return status::quarantined_or_unavailable;
}

void algebra() {
    size_t admitted_all = 0, admitted_14 = 0, forbidden_14 = 0, losses = 0, deaths = 0;
    for (uint8_t op = 1; op <= 5; ++op) for (uint8_t effect = 0; effect < 5; ++effect)
        for (uint8_t completed = 0; completed < 3; ++completed) for (uint8_t code = 0; code < 8; ++code) {
            const bool admitted = admitted_product(static_cast<operation>(op), static_cast<storage_effect>(effect), static_cast<completion>(completed), static_cast<primitive_code>(code));
            admitted_all += admitted;
            if (op <= 4) { admitted_14 += admitted; forbidden_14 += !admitted; }
            if (op <= 4 && admitted && completed == static_cast<uint8_t>(completion::response_lost)) ++losses;
            if (op <= 4 && admitted && completed == static_cast<uint8_t>(completion::process_death)) ++deaths;
        }
    assert(admitted_all == 55 && admitted_14 == 43 && forbidden_14 == 437 && losses == 11 && deaths == 16);
    primitive_product op5 = product(operation::recovery_validation);
    assert(!admitted_payload(op5)); op5.classification = recovery_classification::continue_to_mutation; assert(admitted_payload(op5));
    op5.completed = completion::response_lost; op5.classification = recovery_classification::none; assert(admitted_payload(op5));
    op5.classification = recovery_classification::needs_sticky_quarantine; assert(!admitted_payload(op5));
}

void forbidden_products_reject_before_entry() {
    size_t rejected = 0;
    auto f = std::make_unique<fixture>();
    for (uint8_t op = 1; op <= 4; ++op) for (uint8_t effect = 0; effect < 5; ++effect)
        for (uint8_t completed = 0; completed < 3; ++completed) for (uint8_t code = 0; code < 8; ++code) {
            primitive_product candidate { static_cast<operation>(op), static_cast<storage_effect>(effect), static_cast<completion>(completed), static_cast<primitive_code>(code), recovery_classification::none };
            if (admitted_product(candidate.op, candidate.effect, candidate.completed, candidate.code)) continue;
            script s = normal_script(); s.entries[op - 1] = candidate;
            const size_t h = f->begin(1000 + rejected, 0, credential(), s);
            assert(f->result(h).state == visibility::ordinary_result && f->result(h).ordinary == status::invalid_request_no_mutation && f->trace_size(h) == 0 && f->rejection_wipe_audited(h));
            ++rejected;
        }
    assert(rejected == 437);
}

void admitted_products_execute() {
    size_t executed = 0;
    auto f = std::make_unique<fixture>();
    for (uint8_t op = 1; op <= 4; ++op) for (uint8_t completed = 0; completed < 3; ++completed) for (uint8_t code = 0; code < 8; ++code) {
        primitive_product candidate { static_cast<operation>(op), storage_effect::none, static_cast<completion>(completed), static_cast<primitive_code>(code), recovery_classification::none };
        if (!admitted_product(candidate.op, candidate.effect, candidate.completed, candidate.code)) continue;
        script s = normal_script(); s.entries[op - 1] = candidate;
        size_t owner = max_invocations;
        if (op == 1 && code == static_cast<uint8_t>(primitive_code::busy)) {
            owner = f->begin(10, 0, credential(2), normal_script());
            assert(f->trace(owner, 0).event == 1);
        }
        if (op == 2 && code == static_cast<uint8_t>(primitive_code::busy)) {
            owner = f->begin(11, 1, credential(2), normal_script()); assert(f->step(owner));
        }
        const size_t h = f->begin(100 + executed, 0, credential(), s);
        finish(*f, h);
        const result_view result = f->result(h);
        if (candidate.completed == completion::process_death) {
            assert(result.state == visibility::dead_process_no_result && f->invocation_dead(h));
            assert(f->teardown_audit_count() >= 1);
            const auto audit = f->teardown_audit(f->teardown_audit_count() - 1);
            assert(audit.credential_zero && audit.scratch_zero && audit.serialized_secret_absent);
            for (size_t i = 0; i < f->trace_size(h); ++i) assert(f->trace(h, i).event < 90);
        } else if (candidate.code == primitive_code::ok && candidate.completed == completion::response_confirmed) {
            assert(result.state == visibility::not_visible);
            assert(f->trace_size(h) == 8 && f->trace(h, 4).event == 200 && f->trace(h, 5).event == 90 && f->trace(h, 6).event == 91 && f->trace(h, 7).event == 92);
        } else {
            assert(result.state == visibility::ordinary_result && result.ordinary == expected_status(candidate));
            const size_t n = f->trace_size(h); assert(n >= 4 && f->trace(h, n - 3).event == 90 && f->trace(h, n - 2).event == 91 && f->trace(h, n - 1).event == 92);
        }
        if (candidate.completed != completion::process_death) assert(f->ordinary_wipe_audited(h));
        if (owner != max_invocations && !f->invocation_dead(owner)) finish(*f, owner);
        ++executed;
    }
    assert(executed == 43);
}

void admission_shape_and_payloads() {
    auto f = std::make_unique<fixture>();
    script wrong_order = normal_script(); wrong_order.entries[2].op = operation::snapshot_load;
    size_t h = f->begin(1, 0, credential(), wrong_order); assert(f->result(h).state == visibility::ordinary_result && f->trace_size(h) == 0);
    script payload = normal_script(); payload.entries[3].classification = recovery_classification::continue_to_mutation;
    h = f->begin(2, 0, credential(), payload); assert(f->result(h).state == visibility::ordinary_result && f->trace_size(h) == 0);
    script bad_enum = normal_script(); bad_enum.entries[2].code = static_cast<primitive_code>(255);
    h = f->begin(3, 0, credential(), bad_enum); assert(f->result(h).state == visibility::ordinary_result && f->trace_size(h) == 0);
    script bad_operation = normal_script(); bad_operation.entries[2].op = static_cast<operation>(255);
    h = f->begin(4, 0, credential(), bad_operation); assert(f->result(h).state == visibility::ordinary_result && f->trace_size(h) == 0);
    script bad_effect = normal_script(); bad_effect.entries[2].effect = static_cast<storage_effect>(255);
    h = f->begin(5, 0, credential(), bad_effect); assert(f->result(h).state == visibility::ordinary_result && f->trace_size(h) == 0);
    script bad_completion = normal_script(); bad_completion.entries[2].completed = static_cast<completion>(255);
    h = f->begin(6, 0, credential(), bad_completion); assert(f->result(h).state == visibility::ordinary_result && f->trace_size(h) == 0);
    script bad_classification = normal_script(); bad_classification.entries[2].classification = static_cast<recovery_classification>(255);
    h = f->begin(7, 0, credential(), bad_classification); assert(f->result(h).state == visibility::ordinary_result && f->trace_size(h) == 0);
    credential_owner invalid;
    h = f->begin(8, 0, std::move(invalid), normal_script()); assert(f->result(h).state == visibility::ordinary_result && f->trace_size(h) == 0 && credential_is_zero(invalid));
    credential_owner non_ascii = credential(); non_ascii.key_id.bytes[1] = static_cast<char>(0x80);
    h = f->begin(9, 0, std::move(non_ascii), normal_script()); assert(f->result(h).state == visibility::ordinary_result && f->trace_size(h) == 0 && credential_is_zero(non_ascii));
    credential_owner embedded_nul = credential(); embedded_nul.key_id.bytes[1] = '\0';
    h = f->begin(10, 0, std::move(embedded_nul), normal_script()); assert(f->result(h).state == visibility::ordinary_result && f->trace_size(h) == 0 && credential_is_zero(embedded_nul));
    for (uint8_t boundary : { uint8_t { 0x01 }, uint8_t { 0x20 }, uint8_t { 0x7f } }) {
        credential_owner rejected = credential(); rejected.key_id.bytes[1] = static_cast<char>(boundary);
        h = f->begin(11 + boundary, 0, std::move(rejected), normal_script());
        assert(f->result(h).state == visibility::ordinary_result && f->trace_size(h) == 0 && credential_is_zero(rejected) && f->rejection_wipe_audited(h));
    }
    for (uint8_t boundary : { uint8_t { 0x21 }, uint8_t { 0x7e } }) {
        credential_owner admitted = credential(); admitted.key_id.bytes[1] = static_cast<char>(boundary);
        h = f->begin(300 + boundary, 0, std::move(admitted), normal_script()); finish(*f, h);
        assert(f->trace(h, 0).event == 1 && credential_is_zero(admitted) && f->ordinary_wipe_audited(h));
    }
}

void credential_move_and_overwrite_wipe() {
    credential_owner first = credential(3);
    credential_owner second = credential(4);
    second = std::move(first);
    assert(credential_is_zero(first) && second.owns && second.secret[0] == 3);
    credential_owner third = credential(5);
    third = std::move(second);
    assert(credential_is_zero(second) && third.owns && third.secret[0] == 3);
    auto f = std::make_unique<fixture>();
    const size_t h = f->begin(50, 0, std::move(third), normal_script());
    assert(credential_is_zero(third)); finish(*f, h);
}

void contention_and_cleanup() {
    {
        auto f = std::make_unique<fixture>();
        size_t first = f->begin(1, 0, credential(), normal_script());
        script busy = normal_script(); busy.entries[0].code = primitive_code::busy;
        size_t second = f->begin(2, 0, credential(2), busy); finish(*f, second);
        assert(f->result(second).ordinary == status::busy_no_mutation);
        finish(*f, first);
        size_t reverse = f->begin(3, 0, credential(3), normal_script()); finish(*f, reverse);
        assert(f->result(reverse).state == visibility::not_visible && f->trace(reverse, 4).event == 200);
    }
    for (int winner = 0; winner < 2; ++winner) {
        auto f = std::make_unique<fixture>();
        const uint8_t wp = static_cast<uint8_t>(winner), lp = static_cast<uint8_t>(1 - winner);
        size_t first = f->begin(10 + winner, wp, credential(1 + winner), normal_script()); assert(f->step(first));
        script busy = normal_script(); busy.entries[1].code = primitive_code::busy;
        size_t second = f->begin(20 + winner, lp, credential(3 + winner), busy); assert(f->step(second)); finish(*f, second);
        assert(f->result(second).ordinary == status::busy_no_mutation);
        finish(*f, first);
    }
    {
        auto f = std::make_unique<fixture>();
        script expects_busy = normal_script(); expects_busy.entries[1].code = primitive_code::busy;
        size_t candidate = f->begin(30, 0, credential(), expects_busy);
        size_t owner = f->begin(31, 1, credential(2), normal_script()); assert(f->step(owner));
        assert(f->step(candidate)); finish(*f, candidate);
        assert(f->result(candidate).ordinary == status::busy_no_mutation);
    }
    {
        auto f = std::make_unique<fixture>();
        size_t candidate = f->begin(40, 0, credential(), normal_script());
        size_t owner = f->begin(41, 1, credential(2), normal_script()); assert(f->step(owner));
        assert(f->step(candidate)); finish(*f, candidate);
        assert(f->result(candidate).ordinary == status::invalid_request_no_mutation);
        assert(f->trace_size(candidate) == 4 && f->trace(candidate, 0).event == 1 && f->trace(candidate, 1).event == 90);
    }
}

void process_wide_death() {
    auto f = std::make_unique<fixture>();
    size_t owner = f->begin(1, 0, credential(), normal_script());
    script dies_busy = normal_script(); dies_busy.entries[0].code = primitive_code::busy; dies_busy.entries[0].completed = completion::process_death;
    size_t killer = f->begin(2, 0, credential(2), dies_busy);
    assert(f->invocation_dead(owner) && f->invocation_dead(killer) && f->teardown_audit_count() == 2);
    size_t survivor = f->begin(3, 1, credential(3), normal_script()); assert(!f->invocation_dead(survivor)); finish(*f, survivor);
}

void restart_round_trip() {
    auto source = std::make_unique<fixture>();
    source->state().marker.durable_present = true; source->state().marker.durable_complete = true; source->state().marker.durable_length = 2; source->state().marker.durable_bytes[0] = 0x11; source->state().marker.durable_bytes[1] = 0x22;
    source->state().marker.live_present = true; source->state().marker.live_length = 1; source->state().marker.live_bytes[0] = 0xee;
    for (size_t i = 0; i < 512; ++i) {
        source->state().slots[i].prepare.durable_present = true; source->state().slots[i].prepare.durable_complete = true; source->state().slots[i].prepare.durable_length = 1; source->state().slots[i].prepare.durable_bytes[0] = static_cast<uint8_t>(i);
        source->state().slots[i].close.durable_present = true; source->state().slots[i].close.durable_length = 1; source->state().slots[i].close.durable_bytes[0] = 1;
        source->state().slots[i].abort_record.durable_present = true; source->state().slots[i].abort_record.durable_length = 1; source->state().slots[i].abort_record.durable_bytes[0] = 2;
        source->state().slots[i].successor_staging.durable_present = true; source->state().slots[i].successor_staging.durable_length = 1; source->state().slots[i].successor_staging.durable_bytes[0] = 3;
        source->state().slots[i].selector_staging.durable_present = true; source->state().slots[i].selector_staging.durable_length = 1; source->state().slots[i].selector_staging.durable_bytes[0] = 4;
        source->state().successors[i].durable_occupied = true; source->state().successors[i].durable_digest[0] = static_cast<uint8_t>(i); source->state().successors[i].object.durable_present = true; source->state().successors[i].object.durable_length = 1; source->state().successors[i].object.durable_bytes[0] = 5;
    }
    source->state().initial_envelope.durable_occupied = true; source->state().initial_envelope.durable_digest[0] = 9; source->state().initial_envelope.object.durable_present = true;
    source->state().unexpected[0].durable_occupied = true; source->state().unexpected[0].durable_length = 1; source->state().unexpected[0].durable_name[0] = 'x';
    source->state().root_directory.durable_projection = true; source->state().attempts_directory.durable_projection = true; source->state().staging_directory.durable_projection = true; source->state().envelopes_directory.durable_projection = true;
    auto image = std::make_unique<restart_image>(); assert(source->serialize_restart(*image));
    auto restored = std::make_unique<fixture>(); assert(restored->restore_restart(*image, 0));
    assert(restored->state().marker.live_bytes[0] == 0x11 && restored->state().marker.durable_bytes[1] == 0x22);
    for (size_t i = 0; i < 512; ++i) {
        assert(restored->state().slots[i].prepare.live_bytes[0] == static_cast<uint8_t>(i));
        assert(restored->state().slots[i].close.live_bytes[0] == 1 && restored->state().slots[i].abort_record.live_bytes[0] == 2);
        assert(restored->state().slots[i].successor_staging.live_bytes[0] == 3 && restored->state().slots[i].selector_staging.live_bytes[0] == 4);
        assert(restored->state().successors[i].live_occupied && restored->state().successors[i].durable_occupied && restored->state().successors[i].object.live_bytes[0] == 5);
    }
    assert(restored->state().initial_envelope.live_occupied && restored->state().unexpected[0].live_occupied);
    assert(restored->state().root_directory.live_projection && restored->state().attempts_directory.live_projection && restored->state().staging_directory.live_projection && restored->state().envelopes_directory.live_projection);
}

void live_only_restart_metadata_is_discarded() {
    auto source = std::make_unique<fixture>();
    source->state().successors[7].live_occupied = true;
    source->state().successors[7].live_digest[0] = 0x71;
    source->state().successors[7].object.live_present = true;
    source->state().successors[7].object.live_length = 1;
    source->state().successors[7].object.live_bytes[0] = 0x72;
    source->state().unexpected[3].live_occupied = true;
    source->state().unexpected[3].live_length = 1;
    source->state().unexpected[3].live_name[0] = 'z';
    auto image = std::make_unique<restart_image>(); assert(source->serialize_restart(*image));
    assert(!image->successors[7].occupied && !image->successors[7].object.present && !image->unexpected[3].occupied);
    auto restored = std::make_unique<fixture>(); assert(restored->restore_restart(*image, 0));
    assert(!restored->state().successors[7].live_occupied && !restored->state().successors[7].durable_occupied);
    assert(!restored->state().unexpected[3].live_occupied && !restored->state().unexpected[3].durable_occupied);
}

void exact_restart_capacity_boundaries() {
    auto source = std::make_unique<fixture>(); auto image = std::make_unique<restart_image>(); auto restored = std::make_unique<fixture>();
    source->state().marker.durable_present = true; source->state().marker.durable_complete = true; source->state().marker.durable_length = 1024;
    source->state().marker.durable_bytes.fill(0x5a);
    source->state().slots[511].prepare.durable_present = true; source->state().slots[511].prepare.durable_complete = true;
    source->state().slots[511].prepare.durable_length = 4096; source->state().slots[511].prepare.durable_bytes.fill(0xa5);
    source->state().unexpected[31].durable_occupied = true; source->state().unexpected[31].durable_length = 64;
    source->state().unexpected[31].durable_name.fill(0x33);
    assert(source->serialize_restart(*image) && restored->restore_restart(*image, 0));
    assert(restored->state().marker.live_length == 1024 && restored->state().marker.live_bytes[1023] == 0x5a);
    assert(restored->state().slots[511].prepare.live_length == 4096 && restored->state().slots[511].prepare.live_bytes[4095] == 0xa5);
    assert(restored->state().unexpected[31].live_length == 64 && restored->state().unexpected[31].live_name[63] == 0x33);
}

void invalid_restart_is_rejected_atomically() {
    auto image = std::make_unique<restart_image>();
    auto f = std::make_unique<fixture>();
    f->state().marker.live_present = true; f->state().marker.live_length = 1; f->state().marker.live_bytes[0] = 0xa5;
    const size_t paused = f->begin(81, 0, credential(), normal_script());
    image->marker.length = 1025;
    assert(!f->restore_restart(*image, 0) && f->state().marker.live_bytes[0] == 0xa5 && !f->invocation_dead(paused));
    image->marker.length = 0; image->marker.complete = true;
    assert(!f->restore_restart(*image, 0) && !f->invocation_dead(paused));
    image->marker.complete = false; image->marker.bytes[4] = 1;
    assert(!f->restore_restart(*image, 0) && !f->invocation_dead(paused));
    image->marker.bytes[4] = 0; image->initial_envelope.occupied = true;
    assert(!f->restore_restart(*image, 0) && !f->invocation_dead(paused));
    image->initial_envelope.occupied = false; image->unexpected[0].length = 65; image->unexpected[0].occupied = true;
    assert(!f->restore_restart(*image, 0) && !f->invocation_dead(paused));
    finish(*f, paused);

    auto bad_state = std::make_unique<fixture>(); auto output = std::make_unique<restart_image>();
    bad_state->state().head.durable_length = 1025;
    assert(!bad_state->serialize_restart(*output));
    bad_state->state().head.durable_length = 0; bad_state->state().head.durable_complete = true;
    assert(!bad_state->serialize_restart(*output));
    bad_state->state().head.durable_complete = false; bad_state->state().successors[0].durable_occupied = true;
    assert(!bad_state->serialize_restart(*output));
}

void restart_invalidates_same_process_invocations() {
    auto f = std::make_unique<fixture>(); auto image = std::make_unique<restart_image>();
    const size_t same_process = f->begin(91, 0, credential(1), normal_script());
    const size_t other_process = f->begin(92, 1, credential(2), normal_script());
    assert(f->serialize_restart(*image));
    assert(f->restore_restart(*image, 0));
    assert(f->invocation_dead(same_process) && !f->invocation_dead(other_process) && !f->step(same_process));
    assert(f->teardown_audit_count() == 1);
    const auto audit = f->teardown_audit(0);
    assert(audit.invocation_id == 91 && audit.credential_zero && audit.scratch_zero && audit.serialized_secret_absent);
    finish(*f, other_process);
    const size_t fresh = f->begin(93, 0, credential(3), normal_script()); finish(*f, fresh);
    assert(f->trace(fresh, 0).event == 1);
}

void differential_secret_exclusion() {
    auto first = std::make_unique<fixture>(); auto second = std::make_unique<fixture>();
    auto first_image = std::make_unique<restart_image>(); auto second_image = std::make_unique<restart_image>();
    script death = normal_script(); death.entries[2].completed = completion::process_death;
    const size_t a = first->begin(101, 0, credential(0x31), death); assert(first->step(a)); assert(first->step(a));
    const size_t b = second->begin(102, 0, credential(0x52), death); assert(second->step(b)); assert(second->step(b));
    assert(first->invocation_dead(a) && second->invocation_dead(b));
    assert(first->serialize_restart(*first_image) && second->serialize_restart(*second_image));
    assert(same_restart(*first_image, *second_image));
    assert(first->teardown_audit(0).serialized_secret_absent && second->teardown_audit(0).serialized_secret_absent);
}

void teardown_audit_capacity_is_lossless() {
    auto f = std::make_unique<fixture>();
    script death = normal_script(); death.entries[0].completed = completion::process_death;
    for (size_t i = 0; i < max_invocations; ++i) {
        const size_t h = f->begin(1000 + i, 0, credential(static_cast<uint8_t>(1 + i)), death);
        assert(h != max_invocations && f->invocation_dead(h));
    }
    assert(f->teardown_audit_count() == max_invocations);
    for (size_t i = 0; i < max_invocations; ++i) {
        const auto audit = f->teardown_audit(i);
        assert(audit.invocation_id == 1000 + i && audit.credential_zero && audit.scratch_zero && audit.serialized_secret_absent);
    }
    credential_owner rejected = credential(99);
    assert(f->begin(9999, 0, std::move(rejected), death) == max_invocations && credential_is_zero(rejected));
    assert(f->teardown_audit_count() == max_invocations);
}

void allocation_free_after_construction() {
    auto f = std::make_unique<fixture>(); script s = normal_script(); credential_owner c = credential();
    auto image = std::make_unique<restart_image>();
    fail_allocations = true;
    size_t h = f->begin(77, 0, std::move(c), s); finish(*f, h); assert(f->serialize_restart(*image)); assert(f->restore_restart(*image, 0));
    fail_allocations = false;
    assert(f->result(h).state == visibility::not_visible && f->trace(h, 4).event == 200);
}

} // namespace

int main() {
    algebra();
    forbidden_products_reject_before_entry();
    admitted_products_execute();
    admission_shape_and_payloads();
    credential_move_and_overwrite_wipe();
    contention_and_cleanup();
    process_wide_death();
    restart_round_trip();
    live_only_restart_metadata_is_discarded();
    exact_restart_capacity_boundaries();
    invalid_restart_is_rejected_atomically();
    restart_invalidates_same_process_invocations();
    differential_secret_exclusion();
    teardown_audit_capacity_is_lossless();
    allocation_free_after_construction();
}
