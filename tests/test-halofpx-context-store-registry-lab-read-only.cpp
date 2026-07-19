#include "halofpx-context-store-registry-lab-read-only-internal.h"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <memory>
#include <new>
#include <string>
#include <vector>
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
    return { { product(operation::guard_acquire), product(operation::writer_lock_acquire), product(operation::preflight),
        product(operation::snapshot_load), product(operation::recovery_validation, completion::response_confirmed, primitive_code::unsupported) } };
}

preflight_context_v1 default_preflight(const credential_owner & c) {
    preflight_context_v1 value;
    value.store_uuid.fill(1); value.filesystem_uuid.fill(2); value.subvolume_uuid.fill(3);
    value.mount_id = 1; value.st_dev = 2; value.owner_uid = 3; value.root_mode = 448; value.authority_file_mode = 384;
    value.lock_st_dev = 4; value.lock_st_ino = 5; value.path_policy_commitment.fill(6);
    value.registry_id.size = 3; value.registry_id.bytes[0] = 'r'; value.registry_id.bytes[1] = 'e'; value.registry_id.bytes[2] = 'g';
    value.registry_epoch = 1; value.authority_base_scope_commitment.fill(7); value.registry_policy_commitment.fill(8);
    value.credential_key_id = c.key_id; value.credential_generation = c.generation;
    value.inner_key_disposition = context_store_key_disposition::active;
    value.attempt_capacity = 512; value.maximum_logical_authority_bytes = 16777216;
    return value;
}

request_transition_v1 default_request() {
    request_transition_v1 value;
    value.attempt_id.fill(1); value.operation_commitment.fill(2); value.requested_slot = 0;
    value.predecessor_length = value.successor_length = value.expected_current_head_length = 1;
    value.predecessor[0] = 1; value.successor[0] = 2; value.expected_current_head[0] = 3;
    value.predecessor_digest.fill(4); value.successor_digest.fill(5); value.expected_current_head_digest.fill(6);
    return value;
}

size_t begin_case(fixture & f, uint64_t id, uint8_t process, credential_owner && c, const script & s) {
    const preflight_context_v1 preflight = default_preflight(c);
    const request_transition_v1 request = default_request();
    return f.begin(id, process, std::move(c), preflight, request, s);
}

std::string read_text(const char * path) {
    std::ifstream input(path, std::ios::binary);
    assert(input.good());
    return { std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>() };
}

std::string json_value(const std::string & json, const std::string & key, size_t start = 0) {
    const size_t position = json.find("\"" + key + "\"", start); assert(position != std::string::npos);
    const size_t colon = json.find(':', position), first = json.find('"', colon), last = json.find('"', first + 1);
    assert(colon != std::string::npos && first != std::string::npos && last != std::string::npos);
    return json.substr(first + 1, last - first - 1);
}

std::vector<uint8_t> hex_bytes(const std::string & text) {
    assert((text.size() & 1U) == 0); std::vector<uint8_t> output(text.size() / 2);
    const auto digit = [](char value) { if (value >= '0' && value <= '9') return value - '0'; if (value >= 'a' && value <= 'f') return value - 'a' + 10; assert(false); return 0; };
    for (size_t i = 0; i < output.size(); ++i) output[i] = static_cast<uint8_t>((digit(text[2 * i]) << 4) | digit(text[2 * i + 1]));
    return output;
}

context_store_format_digest hex_digest(const std::string & text) {
    const auto bytes = hex_bytes(text); assert(bytes.size() == 32); context_store_format_digest output {}; std::copy(bytes.begin(), bytes.end(), output.begin()); return output;
}

struct test_cbor_writer {
    std::array<uint8_t, 320> bytes {}; size_t size = 0;
    bool raw(const void * data, size_t length) { if (length > bytes.size() - size) return false; std::memcpy(bytes.data() + size, data, length); size += length; return true; }
    bool head(uint8_t major, uint64_t value) {
        if (value < 24) { const uint8_t byte = static_cast<uint8_t>((major << 5) | value); return raw(&byte, 1); }
        const size_t width = value <= UINT8_MAX ? 1 : value <= UINT16_MAX ? 2 : value <= UINT32_MAX ? 4 : 8;
        uint8_t encoded[9] = { static_cast<uint8_t>((major << 5) | (width == 1 ? 24 : width == 2 ? 25 : width == 4 ? 26 : 27)) };
        for (size_t i = 0; i < width; ++i) encoded[width - i] = static_cast<uint8_t>(value >> (8 * i));
        return raw(encoded, width + 1);
    }
    bool u64(uint64_t value) { return head(0, value); }
    bool bstr(const context_store_format_digest & value) { return head(2, value.size()) && raw(value.data(), value.size()); }
};

context_store_format_digest test_operation_commitment(const context_store_format_digest & root, const context_store_format_digest & path,
        const context_store_format_digest & attempt, uint64_t slot, const context_store_format_digest & predecessor,
        const context_store_format_digest & successor, size_t predecessor_size, size_t successor_size) {
    test_cbor_writer writer; assert(writer.head(5, 8) && writer.u64(0) && writer.bstr(root) && writer.u64(1) && writer.bstr(path) &&
        writer.u64(2) && writer.bstr(attempt) && writer.u64(3) && writer.u64(slot) && writer.u64(4) && writer.bstr(predecessor) &&
        writer.u64(5) && writer.bstr(successor) && writer.u64(6) && writer.u64(predecessor_size) && writer.u64(7) && writer.u64(successor_size));
    constexpr char domain[] = "halofpx.registry-lab-operation.v1"; std::array<uint8_t, 384> input {};
    std::copy_n(reinterpret_cast<const uint8_t *>(domain), sizeof(domain), input.begin()); std::copy_n(writer.bytes.data(), writer.size, input.begin() + sizeof(domain));
    context_store_format_digest output {}; assert(context_store_sha256_bounded(input.data(), sizeof(domain) + writer.size, input.size(), output)); return output;
}

std::vector<uint8_t> golden_fixture(const std::string & json, const char * name) {
    const size_t position = json.find("\"" + std::string(name) + "\""); assert(position != std::string::npos);
    return hex_bytes(json_value(json, "envelope_hex", position));
}

void replace_unique_byte(std::vector<uint8_t> & bytes, std::initializer_list<uint8_t> pattern,
        size_t byte_in_pattern, uint8_t replacement) {
    const auto first = std::search(bytes.begin(), bytes.end(), pattern.begin(), pattern.end());
    assert(first != bytes.end());
    assert(std::search(first + 1, bytes.end(), pattern.begin(), pattern.end()) == bytes.end());
    assert(byte_in_pattern < pattern.size());
    first[static_cast<std::ptrdiff_t>(byte_in_pattern)] = replacement;
}

void xor_unique_byte(std::vector<uint8_t> & bytes, std::initializer_list<uint8_t> pattern,
        size_t byte_after_pattern, uint8_t mask = 1) {
    const auto first = std::search(bytes.begin(), bytes.end(), pattern.begin(), pattern.end());
    assert(first != bytes.end());
    assert(std::search(first + 1, bytes.end(), pattern.begin(), pattern.end()) == bytes.end());
    assert(static_cast<size_t>(bytes.end() - first) > pattern.size() + byte_after_pattern);
    first[static_cast<std::ptrdiff_t>(pattern.size() + byte_after_pattern)] ^= mask;
}

void replace_unique_digest(std::vector<uint8_t> & bytes, std::initializer_list<uint8_t> pattern,
        const context_store_format_digest & digest) {
    const auto first = std::search(bytes.begin(), bytes.end(), pattern.begin(), pattern.end());
    assert(first != bytes.end());
    assert(std::search(first + 1, bytes.end(), pattern.begin(), pattern.end()) == bytes.end());
    assert(static_cast<size_t>(bytes.end() - first) >= pattern.size() + digest.size());
    std::copy(digest.begin(), digest.end(), first + static_cast<std::ptrdiff_t>(pattern.size()));
}

context_store_format_digest test_hash_domain(const char * domain, const std::vector<uint8_t> & bytes) {
    const size_t domain_size = std::strlen(domain) + 1; std::vector<uint8_t> input(domain_size + bytes.size());
    std::copy_n(reinterpret_cast<const uint8_t *>(domain), domain_size, input.begin());
    std::copy(bytes.begin(), bytes.end(), input.begin() + static_cast<std::ptrdiff_t>(domain_size));
    context_store_format_digest digest {};
    assert(context_store_sha256_bounded(input.data(), input.size(), input.size(), digest)); return digest;
}

void retag_record(std::vector<uint8_t> & bytes, const char * authentication_domain) {
    assert(bytes.size() > 37 && (bytes[0] & 0xe0) == 0xa0 && bytes[1] == 0x00);
    constexpr char key_domain[] = "halofpx.registry-lab-key.v1";
    constexpr char key_id[] = "registry-auth-v1";
    std::array<uint8_t, 128> derivation_input {};
    size_t derivation_size = 0;
    std::copy_n(reinterpret_cast<const uint8_t *>(key_domain), sizeof(key_domain), derivation_input.begin());
    derivation_size += sizeof(key_domain);
    derivation_input[derivation_size++] = 0x70;
    std::copy_n(reinterpret_cast<const uint8_t *>(key_id), sizeof(key_id) - 1, derivation_input.begin() + derivation_size);
    derivation_size += sizeof(key_id) - 1;
    derivation_input[derivation_size++] = 0x0d;
    std::array<uint8_t, 32> secret {}, derived {}, tag {}; secret.fill(0x44);
    assert(context_store_hmac_sha256(secret.data(), secret.size(), derivation_input.data(), derivation_size, derived));

    const size_t authentication_end = bytes.size() - 35;
    assert(bytes[authentication_end] == 0x01 && bytes[authentication_end + 1] == 0x58 && bytes[authentication_end + 2] == 0x20);
    const size_t domain_size = std::strlen(authentication_domain) + 1;
    std::vector<uint8_t> authentication_input(domain_size + authentication_end - 2);
    std::copy_n(reinterpret_cast<const uint8_t *>(authentication_domain), domain_size, authentication_input.begin());
    std::copy(bytes.begin() + 2, bytes.begin() + authentication_end, authentication_input.begin() + domain_size);
    assert(context_store_hmac_sha256(derived.data(), derived.size(), authentication_input.data(), authentication_input.size(), tag));
    std::copy(tag.begin(), tag.end(), bytes.end() - static_cast<std::ptrdiff_t>(tag.size()));
}

context_store_registered_id registered_id(const char * text) {
    context_store_registered_id output; const size_t size = std::char_traits<char>::length(text); assert(size && size <= 128);
    output.size = static_cast<uint8_t>(size); std::copy_n(text, size, output.bytes.begin()); return output;
}

struct golden_data {
    std::vector<uint8_t> root_initialized, root_initializing, head_initial, head_successor, prepare, close_record, abort_record;
    std::array<uint8_t, 1024> predecessor {}, successor {};
    size_t predecessor_size = 0, successor_size = 0;
    preflight_context_v1 preflight;
    request_transition_v1 request;

    explicit golden_data(const char * path) {
        const std::string json = read_text(path);
        root_initialized = golden_fixture(json, "root_initialized"); root_initializing = golden_fixture(json, "root_initializing");
        head_initial = golden_fixture(json, "head_initial"); head_successor = golden_fixture(json, "head_successor");
        prepare = golden_fixture(json, "prepare"); close_record = golden_fixture(json, "close"); abort_record = golden_fixture(json, "abort");

        credential_owner c = golden_credential();
        context_store_protected_registry_key_record key { context_store_key_disposition::active, c.key_id, c.generation, { c.secret.data(), c.secret.size() } };
        context_store_protected_registry_body before; before.registry_id = registered_id("registry-v1"); before.registry_epoch = 9;
        before.authority_base_scope_commitment.fill(0xaa); before.policy_commitment.fill(0xbb); before.last_consumed_sequence = 40;
        auto predecessor_result = context_store_encode_protected_registry_v1(before, key, predecessor.data(), predecessor.size());
        assert(predecessor_result.authenticated_carrier()); predecessor_size = predecessor_result.encoded_size;
        context_store_protected_registry_successor_body after; after.registry_id = before.registry_id; after.registry_epoch = before.registry_epoch;
        after.authority_base_scope_commitment = before.authority_base_scope_commitment; after.policy_commitment = before.policy_commitment;
        after.consumed_authorization_high_water = 41; after.predecessor_snapshot_envelope_digest = *predecessor_result.authenticated_carrier()->envelope_digest();
        after.receipt.authorization_sequence = 41; after.receipt.command_id.fill(0xcc); after.receipt.authorization_token_digest.fill(0xdd);
        after.receipt.plan_commitment.fill(0xee); after.receipt.selected_manifest_digest.fill(0x11); after.receipt.proposed_anchor_envelope_digest.fill(0x22);
        auto successor_result = context_store_encode_protected_registry_successor_v1(after, key, successor.data(), successor.size());
        assert(successor_result.authenticated_carrier()); successor_size = successor_result.encoded_size;

        preflight.store_uuid.fill(0x22); preflight.filesystem_uuid.fill(0x44); preflight.subvolume_uuid.fill(0x45);
        preflight.mount_id = 731; preflight.st_dev = 2049; preflight.owner_uid = 1000; preflight.root_mode = 448; preflight.authority_file_mode = 384;
        preflight.lock_st_dev = 2049; preflight.lock_st_ino = 987654;
        preflight.path_policy_commitment = hex_digest("162c7c9272841ccdec5dbf024d1d8dd511dbb4e2f538d8cdbab830e06abc9c0d");
        preflight.registry_id = before.registry_id; preflight.registry_epoch = 9; preflight.authority_base_scope_commitment = before.authority_base_scope_commitment;
        preflight.registry_policy_commitment = before.policy_commitment; preflight.credential_key_id = c.key_id; preflight.credential_generation = c.generation;
        preflight.inner_key_disposition = context_store_key_disposition::active; preflight.attempt_capacity = 512; preflight.maximum_logical_authority_bytes = 16777216;

        request.attempt_id.fill(0x66); request.requested_slot = 17;
        request.operation_commitment = hex_digest("f28cb18e21d74d63346bddacbd482e28a55989cfa2f9e18cea382792a89b7fb1");
        request.predecessor_length = predecessor_size; request.successor_length = successor_size; request.expected_current_head_length = head_initial.size();
        std::copy_n(predecessor.data(), predecessor_size, request.predecessor.begin()); std::copy_n(successor.data(), successor_size, request.successor.begin());
        std::copy(head_initial.begin(), head_initial.end(), request.expected_current_head.begin());
        request.predecessor_digest = hex_digest(json_value(json, "predecessor_registry_envelope_digest_hex"));
        request.successor_digest = hex_digest(json_value(json, "successor_registry_envelope_digest_hex"));
        const size_t head_position = json.find("\"head_initial\""); request.expected_current_head_digest = hex_digest(json_value(json, "content_digest_sha256_hex", head_position));
    }

    static credential_owner golden_credential() {
        credential_owner value; value.key_id = registered_id("registry-auth-v1"); value.generation = 13; value.secret.fill(0x44); value.owns = true; return value;
    }
};

struct generated_journal {
    std::vector<uint8_t> prepare, terminal;
    context_store_format_digest attempt_id {}, operation_commitment {};
};

context_store_registry_lab_credential wire_credential() {
    context_store_registry_lab_credential value; value.key_id = registered_id("registry-auth-v1"); value.generation = 13; value.secret.fill(0x44); return value;
}

context_store_registry_lab_prepare_value_v1 prepare_value(const golden_data & g, size_t slot, const context_store_format_digest & attempt) {
    context_store_registry_lab_prepare_value_v1 value; value.scope.root_id.fill(0x11); value.scope.path_policy_commitment = g.preflight.path_policy_commitment;
    value.scope.registry_id = g.preflight.registry_id; value.scope.registry_epoch = g.preflight.registry_epoch; value.attempt_id = attempt; value.slot = slot;
    value.predecessor_envelope_digest = g.request.predecessor_digest; value.successor_envelope_digest = g.request.successor_digest;
    value.initial_head_digest = g.request.expected_current_head_digest; value.predecessor_high_water = 40; value.predecessor_selector_generation = 1; value.successor_selector_generation = 2;
    value.predecessor.size = g.predecessor_size; value.successor.size = g.successor_size;
    std::copy_n(g.predecessor.data(), g.predecessor_size, value.predecessor.bytes.begin()); std::copy_n(g.successor.data(), g.successor_size, value.successor.bytes.begin());
    value.operation_commitment = test_operation_commitment(value.scope.root_id, value.scope.path_policy_commitment, attempt, slot,
        value.predecessor_envelope_digest, value.successor_envelope_digest, value.predecessor.size, value.successor.size);
    return value;
}

generated_journal generate_journal(const golden_data & g, size_t slot, uint64_t identity, bool close) {
    generated_journal output; output.attempt_id.fill(0); output.attempt_id[0] = 0x80;
    for (size_t i = 0; i < 8; ++i) output.attempt_id[31 - i] = static_cast<uint8_t>(identity >> (8 * i));
    auto transition = prepare_value(g, slot, output.attempt_id); output.operation_commitment = transition.operation_commitment;
    auto credential = wire_credential(); context_store_registry_lab_prepare_evidence_v1 prepare_evidence { g.head_initial.data(), g.head_initial.size() };
    context_store_registry_lab_prepare_witness prepare_witness; assert(context_store_registry_lab_admit_prepare_v1(transition, credential, prepare_evidence, prepare_witness));
    std::array<uint8_t, 4096> prepare_bytes {}; size_t prepare_size = 0;
    auto prepare_result = context_store_registry_lab_encode_prepare_v1(transition, credential, prepare_witness, prepare_bytes.data(), prepare_bytes.size(), prepare_size);
    assert(prepare_result.authenticated()); output.prepare.assign(prepare_bytes.begin(), prepare_bytes.begin() + prepare_size);
    std::array<uint8_t, 1024> terminal_bytes {}; size_t terminal_size = 0;
    if (close) {
        context_store_registry_lab_close_value_v1 value; value.scope = transition.scope; value.attempt_id = transition.attempt_id; value.slot = slot;
        value.operation_commitment = transition.operation_commitment; value.predecessor_envelope_digest = transition.predecessor_envelope_digest;
        value.successor_envelope_digest = transition.successor_envelope_digest; value.prepare_digest = prepare_result.content_digest;
        value.head_digest = hex_digest("d3d372ef42c5254a2baca52ba35cae46899cdf0e464396e3fa272ffc238a309b");
        context_store_registry_lab_close_evidence_v1 evidence; evidence.transition = transition; evidence.predecessor_head = g.head_initial.data();
        evidence.predecessor_head_size = g.head_initial.size(); evidence.successor_head = g.head_successor.data(); evidence.successor_head_size = g.head_successor.size();
        evidence.prepare = output.prepare.data(); evidence.prepare_size = output.prepare.size(); context_store_registry_lab_close_witness witness;
        assert(context_store_registry_lab_admit_close_v1(value, credential, evidence, witness));
        auto result = context_store_registry_lab_encode_close_v1(value, credential, witness, terminal_bytes.data(), terminal_bytes.size(), terminal_size); assert(result.authenticated());
    } else {
        context_store_registry_lab_abort_value_v1 value; value.scope = transition.scope; value.attempt_id = transition.attempt_id; value.slot = slot;
        value.operation_commitment = transition.operation_commitment; value.predecessor_envelope_digest = transition.predecessor_envelope_digest;
        value.successor_envelope_digest = transition.successor_envelope_digest; value.prepare_digest = prepare_result.content_digest;
        value.head_digest = g.request.expected_current_head_digest; value.terminal_class = context_store_registry_lab_terminal_class_v1::recovered;
        context_store_registry_lab_abort_evidence_v1 evidence; evidence.transition = transition; evidence.predecessor_head = g.head_initial.data();
        evidence.predecessor_head_size = g.head_initial.size(); evidence.prepare = output.prepare.data(); evidence.prepare_size = output.prepare.size();
        context_store_registry_lab_abort_witness witness; assert(context_store_registry_lab_admit_abort_v1(value, credential, evidence, witness));
        auto result = context_store_registry_lab_encode_abort_v1(value, credential, witness, terminal_bytes.data(), terminal_bytes.size(), terminal_size); assert(result.authenticated());
    }
    output.terminal.assign(terminal_bytes.begin(), terminal_bytes.begin() + terminal_size); return output;
}

template<size_t N> void publish(modeled_file<N> & file, const uint8_t * data, size_t size) {
    assert(size <= N); file = {}; file.live_present = file.durable_present = true; file.live_complete = file.durable_complete = true;
    file.live_length = file.durable_length = size;
    if (size) { assert(data); std::copy_n(data, size, file.live_bytes.begin()); std::copy_n(data, size, file.durable_bytes.begin()); }
}

template<size_t N> void publish(modeled_file<N> & file, const std::vector<uint8_t> & data) { publish(file, data.data(), data.size()); }

void populate_clean(fixture & f, const golden_data & g, bool initializing = false) {
    fixed_state & state = f.state();
    publish(state.marker, initializing ? g.root_initializing : g.root_initialized); publish(state.head, g.head_initial);
    publish(state.lock_file, static_cast<const uint8_t *>(nullptr), 0);
    state.initial_envelope.live_occupied = state.initial_envelope.durable_occupied = true;
    state.initial_envelope.live_digest = state.initial_envelope.durable_digest = g.request.predecessor_digest;
    publish(state.initial_envelope.object, g.predecessor.data(), g.predecessor_size);
    state.root_directory = { true, true }; state.attempts_directory = { true, true };
    state.staging_directory = { true, true }; state.envelopes_directory = { true, true };
}

script operation_5_script(recovery_classification classification) {
    script value { { product(operation::guard_acquire), product(operation::writer_lock_acquire), product(operation::preflight),
        product(operation::snapshot_load), product(operation::recovery_validation) } };
    value.entries[4].classification = classification; return value;
}

void finish(fixture & f, size_t handle);

size_t run_operation_5(fixture & f, const golden_data & g, uint64_t id, recovery_classification classification,
        request_transition_v1 request) {
    script s = operation_5_script(classification); credential_owner c = golden_data::golden_credential();
    const size_t handle = f.begin(id, 0, std::move(c), g.preflight, request, s); finish(f, handle); return handle;
}

void install_journal(fixed_state & state, size_t slot, const std::vector<uint8_t> & prepare_bytes,
        const std::vector<uint8_t> * terminal_bytes = nullptr, bool close = false) {
    publish(state.slots[slot].prepare, prepare_bytes);
    if (terminal_bytes) publish(close ? state.slots[slot].close : state.slots[slot].abort_record, *terminal_bytes);
}

void install_successor_head(fixed_state & state, const golden_data & g) {
    publish(state.head, g.head_successor); auto & envelope = state.successors[0];
    envelope.live_occupied = envelope.durable_occupied = true; envelope.live_digest = envelope.durable_digest = g.request.successor_digest;
    publish(envelope.object, g.successor.data(), g.successor_size);
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

void operation_5_pairwise_precedence_matrix() {
    constexpr std::array<recovery_classification, 11> order {
        recovery_classification::blocked_by_existing_quarantine,
        recovery_classification::inadmissible_initialization_artifact,
        recovery_classification::needs_sticky_quarantine,
        recovery_classification::needs_successor_close,
        recovery_classification::needs_predecessor_abort,
        recovery_classification::attempt_replayed,
        recovery_classification::capacity_exhausted,
        recovery_classification::requested_slot_occupied,
        recovery_classification::invalid_transition,
        recovery_classification::preexisting_unattributed_material,
        recovery_classification::continue_to_mutation,
    };
    const auto set = [](recovery_precedence_flags & flags, recovery_classification value) {
        if (value == recovery_classification::blocked_by_existing_quarantine) flags.blocked = true;
        if (value == recovery_classification::inadmissible_initialization_artifact) flags.initializing = true;
        if (value == recovery_classification::needs_sticky_quarantine) flags.sticky = true;
        if (value == recovery_classification::needs_successor_close) flags.successor_close = true;
        if (value == recovery_classification::needs_predecessor_abort) flags.predecessor_abort = true;
        if (value == recovery_classification::attempt_replayed) flags.replay = true;
        if (value == recovery_classification::capacity_exhausted) flags.capacity = true;
        if (value == recovery_classification::requested_slot_occupied) flags.slot = true;
        if (value == recovery_classification::invalid_transition) flags.invalid = true;
        if (value == recovery_classification::preexisting_unattributed_material) flags.preexisting = true;
    };
    size_t pairwise_precedence_combinations = 0;
    for (size_t higher = 0; higher < order.size(); ++higher) for (size_t lower = higher + 1; lower < order.size(); ++lower) {
        recovery_precedence_flags flags; set(flags, order[higher]); set(flags, order[lower]);
        assert(select_recovery_precedence(flags) == order[higher]); ++pairwise_precedence_combinations;
    }
    assert(pairwise_precedence_combinations == 55);
}

void forbidden_products_reject_before_entry() {
    size_t rejected = 0;
    auto f = std::make_unique<fixture>();
    for (uint8_t op = 1; op <= 4; ++op) for (uint8_t effect = 0; effect < 5; ++effect)
        for (uint8_t completed = 0; completed < 3; ++completed) for (uint8_t code = 0; code < 8; ++code) {
            primitive_product candidate { static_cast<operation>(op), static_cast<storage_effect>(effect), static_cast<completion>(completed), static_cast<primitive_code>(code), recovery_classification::none };
            if (admitted_product(candidate.op, candidate.effect, candidate.completed, candidate.code)) continue;
            script s = normal_script(); s.entries[op - 1] = candidate;
            const size_t h = begin_case(*f, 1000 + rejected, 0, credential(), s);
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
            owner = begin_case(*f, 10, 0, credential(2), normal_script());
            assert(f->trace(owner, 0).event == 1);
        }
        if (op == 2 && code == static_cast<uint8_t>(primitive_code::busy)) {
            owner = begin_case(*f, 11, 1, credential(2), normal_script()); assert(f->step(owner));
        }
        const size_t h = begin_case(*f, 100 + executed, 0, credential(), s);
        finish(*f, h);
        const result_view result = f->result(h);
        if (candidate.completed == completion::process_death) {
            assert(result.state == visibility::dead_process_no_result && f->invocation_dead(h));
            assert(f->teardown_audit_count() >= 1);
            const auto audit = f->teardown_audit(f->teardown_audit_count() - 1);
            assert(audit.credential_zero && audit.scratch_zero && audit.serialized_secret_absent);
            for (size_t i = 0; i < f->trace_size(h); ++i) assert(f->trace(h, i).event < 90);
        } else if (candidate.code == primitive_code::ok && candidate.completed == completion::response_confirmed) {
            assert(result.state == visibility::ordinary_result && result.ordinary == status::unsupported_no_mutation);
            assert(f->trace_size(h) == 8 && f->trace(h, 4).event == 5 && f->trace(h, 5).event == 90 && f->trace(h, 6).event == 91 && f->trace(h, 7).event == 92);
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
    size_t h = begin_case(*f, 1, 0, credential(), wrong_order); assert(f->result(h).state == visibility::ordinary_result && f->trace_size(h) == 0);
    script payload = normal_script(); payload.entries[3].classification = recovery_classification::continue_to_mutation;
    h = begin_case(*f, 2, 0, credential(), payload); assert(f->result(h).state == visibility::ordinary_result && f->trace_size(h) == 0);
    script bad_enum = normal_script(); bad_enum.entries[2].code = static_cast<primitive_code>(255);
    h = begin_case(*f, 3, 0, credential(), bad_enum); assert(f->result(h).state == visibility::ordinary_result && f->trace_size(h) == 0);
    script bad_operation = normal_script(); bad_operation.entries[2].op = static_cast<operation>(255);
    h = begin_case(*f, 4, 0, credential(), bad_operation); assert(f->result(h).state == visibility::ordinary_result && f->trace_size(h) == 0);
    script bad_effect = normal_script(); bad_effect.entries[2].effect = static_cast<storage_effect>(255);
    h = begin_case(*f, 5, 0, credential(), bad_effect); assert(f->result(h).state == visibility::ordinary_result && f->trace_size(h) == 0);
    script bad_completion = normal_script(); bad_completion.entries[2].completed = static_cast<completion>(255);
    h = begin_case(*f, 6, 0, credential(), bad_completion); assert(f->result(h).state == visibility::ordinary_result && f->trace_size(h) == 0);
    script bad_classification = normal_script(); bad_classification.entries[2].classification = static_cast<recovery_classification>(255);
    h = begin_case(*f, 7, 0, credential(), bad_classification); assert(f->result(h).state == visibility::ordinary_result && f->trace_size(h) == 0);
    credential_owner invalid;
    h = begin_case(*f, 8, 0, std::move(invalid), normal_script()); assert(f->result(h).state == visibility::ordinary_result && f->trace_size(h) == 0 && credential_is_zero(invalid));
    credential_owner non_ascii = credential(); non_ascii.key_id.bytes[1] = static_cast<char>(0x80);
    h = begin_case(*f, 9, 0, std::move(non_ascii), normal_script()); assert(f->result(h).state == visibility::ordinary_result && f->trace_size(h) == 0 && credential_is_zero(non_ascii));
    credential_owner embedded_nul = credential(); embedded_nul.key_id.bytes[1] = '\0';
    h = begin_case(*f, 10, 0, std::move(embedded_nul), normal_script()); assert(f->result(h).state == visibility::ordinary_result && f->trace_size(h) == 0 && credential_is_zero(embedded_nul));
    for (uint8_t boundary : { uint8_t { 0x01 }, uint8_t { 0x20 }, uint8_t { 0x7f } }) {
        credential_owner rejected = credential(); rejected.key_id.bytes[1] = static_cast<char>(boundary);
        h = begin_case(*f, 11 + boundary, 0, std::move(rejected), normal_script());
        assert(f->result(h).state == visibility::ordinary_result && f->trace_size(h) == 0 && credential_is_zero(rejected) && f->rejection_wipe_audited(h));
    }
    for (uint8_t boundary : { uint8_t { 0x21 }, uint8_t { 0x7e } }) {
        credential_owner admitted = credential(); admitted.key_id.bytes[1] = static_cast<char>(boundary);
        h = begin_case(*f, 300 + boundary, 0, std::move(admitted), normal_script()); finish(*f, h);
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
    const size_t h = begin_case(*f, 50, 0, std::move(third), normal_script());
    assert(credential_is_zero(third)); finish(*f, h);
}

void contention_and_cleanup() {
    {
        auto f = std::make_unique<fixture>();
        size_t first = begin_case(*f, 1, 0, credential(), normal_script());
        script busy = normal_script(); busy.entries[0].code = primitive_code::busy;
        size_t second = begin_case(*f, 2, 0, credential(2), busy); finish(*f, second);
        assert(f->result(second).ordinary == status::busy_no_mutation);
        finish(*f, first);
        size_t reverse = begin_case(*f, 3, 0, credential(3), normal_script()); finish(*f, reverse);
        assert(f->result(reverse).state == visibility::ordinary_result && f->result(reverse).ordinary == status::unsupported_no_mutation && f->trace(reverse, 4).event == 5);
    }
    for (int winner = 0; winner < 2; ++winner) {
        auto f = std::make_unique<fixture>();
        const uint8_t wp = static_cast<uint8_t>(winner), lp = static_cast<uint8_t>(1 - winner);
        size_t first = begin_case(*f, 10 + winner, wp, credential(1 + winner), normal_script()); assert(f->step(first));
        script busy = normal_script(); busy.entries[1].code = primitive_code::busy;
        size_t second = begin_case(*f, 20 + winner, lp, credential(3 + winner), busy); assert(f->step(second)); finish(*f, second);
        assert(f->result(second).ordinary == status::busy_no_mutation);
        finish(*f, first);
    }
    {
        auto f = std::make_unique<fixture>();
        script expects_busy = normal_script(); expects_busy.entries[1].code = primitive_code::busy;
        size_t candidate = begin_case(*f, 30, 0, credential(), expects_busy);
        size_t owner = begin_case(*f, 31, 1, credential(2), normal_script()); assert(f->step(owner));
        assert(f->step(candidate)); finish(*f, candidate);
        assert(f->result(candidate).ordinary == status::busy_no_mutation);
    }
    {
        auto f = std::make_unique<fixture>();
        size_t candidate = begin_case(*f, 40, 0, credential(), normal_script());
        size_t owner = begin_case(*f, 41, 1, credential(2), normal_script()); assert(f->step(owner));
        assert(f->step(candidate)); finish(*f, candidate);
        assert(f->result(candidate).ordinary == status::invalid_request_no_mutation);
        assert(f->trace_size(candidate) == 4 && f->trace(candidate, 0).event == 1 && f->trace(candidate, 1).event == 90);
    }
}

void process_wide_death() {
    auto f = std::make_unique<fixture>();
    size_t owner = begin_case(*f, 1, 0, credential(), normal_script());
    script dies_busy = normal_script(); dies_busy.entries[0].code = primitive_code::busy; dies_busy.entries[0].completed = completion::process_death;
    size_t killer = begin_case(*f, 2, 0, credential(2), dies_busy);
    assert(f->invocation_dead(owner) && f->invocation_dead(killer) && f->teardown_audit_count() == 2);
    size_t survivor = begin_case(*f, 3, 1, credential(3), normal_script()); assert(!f->invocation_dead(survivor)); finish(*f, survivor);
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
    const size_t paused = begin_case(*f, 81, 0, credential(), normal_script());
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
    const size_t same_process = begin_case(*f, 91, 0, credential(1), normal_script());
    const size_t other_process = begin_case(*f, 92, 1, credential(2), normal_script());
    assert(f->serialize_restart(*image));
    assert(f->restore_restart(*image, 0));
    assert(f->invocation_dead(same_process) && !f->invocation_dead(other_process) && !f->step(same_process));
    assert(f->teardown_audit_count() == 1);
    const auto audit = f->teardown_audit(0);
    assert(audit.invocation_id == 91 && audit.credential_zero && audit.scratch_zero && audit.serialized_secret_absent);
    finish(*f, other_process);
    const size_t fresh = begin_case(*f, 93, 0, credential(3), normal_script()); finish(*f, fresh);
    assert(f->trace(fresh, 0).event == 1);
}

void differential_secret_exclusion() {
    auto first = std::make_unique<fixture>(); auto second = std::make_unique<fixture>();
    auto first_image = std::make_unique<restart_image>(); auto second_image = std::make_unique<restart_image>();
    script death = normal_script(); death.entries[2].completed = completion::process_death;
    const size_t a = begin_case(*first, 101, 0, credential(0x31), death); assert(first->step(a)); assert(first->step(a));
    const size_t b = begin_case(*second, 102, 0, credential(0x52), death); assert(second->step(b)); assert(second->step(b));
    assert(first->invocation_dead(a) && second->invocation_dead(b));
    assert(first->serialize_restart(*first_image) && second->serialize_restart(*second_image));
    assert(same_restart(*first_image, *second_image));
    assert(first->teardown_audit(0).serialized_secret_absent && second->teardown_audit(0).serialized_secret_absent);
}

void teardown_audit_capacity_is_lossless() {
    auto f = std::make_unique<fixture>();
    script death = normal_script(); death.entries[0].completed = completion::process_death;
    for (size_t i = 0; i < max_invocations; ++i) {
        const size_t h = begin_case(*f, 1000 + i, 0, credential(static_cast<uint8_t>(1 + i)), death);
        assert(h != max_invocations && f->invocation_dead(h));
    }
    assert(f->teardown_audit_count() == max_invocations);
    for (size_t i = 0; i < max_invocations; ++i) {
        const auto audit = f->teardown_audit(i);
        assert(audit.invocation_id == 1000 + i && audit.credential_zero && audit.scratch_zero && audit.serialized_secret_absent);
    }
    credential_owner rejected = credential(99);
    assert(begin_case(*f, 9999, 0, std::move(rejected), death) == max_invocations && credential_is_zero(rejected));
    assert(f->teardown_audit_count() == max_invocations);
}

void allocation_free_after_construction() {
    auto f = std::make_unique<fixture>(); script s = normal_script(); credential_owner c = credential();
    auto image = std::make_unique<restart_image>();
    fail_allocations = true;
    size_t h = begin_case(*f, 77, 0, std::move(c), s); finish(*f, h); assert(f->serialize_restart(*image)); assert(f->restore_restart(*image, 0));
    fail_allocations = false;
    assert(f->result(h).state == visibility::ordinary_result && f->trace(h, 4).event == 5);
}

void operation_5_allocation_free_after_construction(const golden_data & g) {
    auto f = std::make_unique<fixture>(); populate_clean(*f, g);
    script s = operation_5_script(recovery_classification::continue_to_mutation);
    credential_owner c = golden_data::golden_credential(); request_transition_v1 request = g.request;
    fail_allocations = true;
    const size_t h = f->begin(1999, 0, std::move(c), g.preflight, request, s); finish(*f, h);
    fail_allocations = false;
    assert(f->derived_classification(h) == recovery_classification::continue_to_mutation &&
        f->result(h).state == visibility::not_visible && f->ordinary_wipe_audited(h));
}

void operation_5_core(const golden_data & g) {
    {
        auto f = std::make_unique<fixture>(); populate_clean(*f, g);
        const size_t h = run_operation_5(*f, g, 2001, recovery_classification::continue_to_mutation, g.request);
        assert(f->derived_classification(h) == recovery_classification::continue_to_mutation && f->scanned_slots(h) == 512);
        assert(f->result(h).state == visibility::not_visible && f->trace_size(h) == 9 && f->trace(h, 5).event == 201);
    }
    {
        auto f = std::make_unique<fixture>(); populate_clean(*f, g); f->state().quarantine.live_present = true;
        const size_t h = run_operation_5(*f, g, 2002, recovery_classification::blocked_by_existing_quarantine, g.request);
        assert(f->result(h).ordinary == status::quarantined_or_unavailable && f->scanned_slots(h) == 0 && f->kdf_calls(h) == 0);
    }
    {
        auto f = std::make_unique<fixture>(); populate_clean(*f, g, true);
        const size_t h = run_operation_5(*f, g, 2003, recovery_classification::inadmissible_initialization_artifact, g.request);
        assert(f->result(h).ordinary == status::preexisting_material_no_authority && f->scanned_slots(h) == 0);
    }
    {
        auto f = std::make_unique<fixture>(); populate_clean(*f, g); f->state().marker.live_bytes[10] ^= 1; f->state().marker.durable_bytes[10] ^= 1;
        const size_t h = run_operation_5(*f, g, 2004, recovery_classification::needs_sticky_quarantine, g.request);
        assert(f->result(h).ordinary == status::quarantined_or_unavailable);
    }
    {
        auto f = std::make_unique<fixture>(); populate_clean(*f, g); request_transition_v1 request = g.request; request.operation_commitment[0] ^= 1;
        const size_t h = run_operation_5(*f, g, 2005, recovery_classification::invalid_transition, request);
        assert(f->result(h).ordinary == status::invalid_transition_no_mutation);
    }
    {
        auto f = std::make_unique<fixture>(); populate_clean(*f, g);
        auto & envelope = f->state().successors[0]; envelope.live_occupied = envelope.durable_occupied = true;
        envelope.live_digest = envelope.durable_digest = g.request.successor_digest; publish(envelope.object, g.successor.data(), g.successor_size);
        const size_t h = run_operation_5(*f, g, 2006, recovery_classification::preexisting_unattributed_material, g.request);
        assert(f->result(h).ordinary == status::preexisting_material_no_authority);
    }
    {
        auto f = std::make_unique<fixture>(); populate_clean(*f, g);
        const size_t h = run_operation_5(*f, g, 2007, recovery_classification::invalid_transition, g.request);
        assert(f->derived_classification(h) == recovery_classification::continue_to_mutation &&
            f->result(h).ordinary == status::invalid_request_no_mutation);
    }
}

void operation_5_recovery_and_request_precedence(const golden_data & g) {
    {
        auto f = std::make_unique<fixture>(); populate_clean(*f, g); install_journal(f->state(), 17, g.prepare);
        const size_t h = run_operation_5(*f, g, 2101, recovery_classification::needs_predecessor_abort, g.request);
        assert(f->result(h).ordinary == status::uncertain_requires_recovery && f->scanned_slots(h) == 512);
    }
    {
        auto f = std::make_unique<fixture>(); populate_clean(*f, g); install_journal(f->state(), 17, g.prepare); install_successor_head(f->state(), g);
        const size_t h = run_operation_5(*f, g, 2102, recovery_classification::needs_successor_close, g.request);
        assert(f->result(h).ordinary == status::uncertain_requires_recovery);
    }
    {
        auto f = std::make_unique<fixture>(); populate_clean(*f, g); install_journal(f->state(), 17, g.prepare, &g.abort_record, false);
        const size_t h = run_operation_5(*f, g, 2103, recovery_classification::attempt_replayed, g.request);
        assert(f->result(h).ordinary == status::attempt_replayed_no_mutation);
    }
    {
        auto f = std::make_unique<fixture>(); populate_clean(*f, g); install_journal(f->state(), 17, g.prepare, &g.abort_record, false);
        request_transition_v1 request = g.request; request.attempt_id.fill(0x67);
        const size_t h = run_operation_5(*f, g, 2104, recovery_classification::requested_slot_occupied, request);
        assert(f->result(h).ordinary == status::slot_occupied_no_mutation);
    }
    {
        auto f = std::make_unique<fixture>(); populate_clean(*f, g);
        for (size_t slot = 0; slot < 512; ++slot) {
            generated_journal journal = generate_journal(g, slot, 10000 + slot, false);
            install_journal(f->state(), slot, journal.prepare, &journal.terminal, false);
        }
        request_transition_v1 request = g.request; request.attempt_id.fill(0x6a); request.requested_slot = 17;
        const size_t h = run_operation_5(*f, g, 2105, recovery_classification::capacity_exhausted, request);
        assert(f->result(h).ordinary == status::capacity_exhausted_no_mutation && f->scanned_slots(h) == 512);
    }
}

void operation_5_integrated_precedence_overlaps(const golden_data & g) {
    size_t overlaps = 0; uint64_t invocation = 120000;
    const auto run = [&](fixture & f, recovery_classification expected, request_transition_v1 request) {
        const size_t h = run_operation_5(f, g, invocation++, expected, request);
        assert(f.derived_classification(h) == expected); ++overlaps;
    };
    {
        auto f = std::make_unique<fixture>(); populate_clean(*f, g, true); f->state().quarantine.live_present = true;
        run(*f, recovery_classification::blocked_by_existing_quarantine, g.request);
    }
    {
        auto f = std::make_unique<fixture>(); populate_clean(*f, g, true); f->state().unexpected[0].live_name[0] = 'x';
        run(*f, recovery_classification::inadmissible_initialization_artifact, g.request);
    }
    {
        auto f = std::make_unique<fixture>(); populate_clean(*f, g); install_journal(f->state(), 17, g.prepare); install_successor_head(f->state(), g);
        f->state().unexpected[0].live_name[0] = 'x'; run(*f, recovery_classification::needs_sticky_quarantine, g.request);
    }
    {
        auto f = std::make_unique<fixture>(); populate_clean(*f, g); install_journal(f->state(), 17, g.prepare);
        f->state().unexpected[0].live_name[0] = 'x'; run(*f, recovery_classification::needs_sticky_quarantine, g.request);
    }
    {
        auto f = std::make_unique<fixture>(); populate_clean(*f, g); install_journal(f->state(), 17, g.prepare); install_successor_head(f->state(), g);
        run(*f, recovery_classification::needs_successor_close, g.request); // also request-successor material
    }
    {
        auto f = std::make_unique<fixture>(); populate_clean(*f, g); install_journal(f->state(), 17, g.prepare);
        run(*f, recovery_classification::needs_predecessor_abort, g.request); // also replay candidate
    }
    {
        auto f = std::make_unique<fixture>(); populate_clean(*f, g);
        for (size_t slot = 0; slot < 512; ++slot) {
            generated_journal journal = generate_journal(g, slot, 130000 + slot, false);
            install_journal(f->state(), slot, journal.prepare, &journal.terminal, false);
        }
        f->state().slots[17] = {}; install_journal(f->state(), 17, g.prepare, &g.abort_record, false);
        run(*f, recovery_classification::attempt_replayed, g.request); // replay outranks capacity and occupied slot
    }
    {
        auto f = std::make_unique<fixture>(); populate_clean(*f, g);
        for (size_t slot = 0; slot < 512; ++slot) {
            generated_journal journal = generate_journal(g, slot, 131000 + slot, false);
            install_journal(f->state(), slot, journal.prepare, &journal.terminal, false);
        }
        generated_journal requested = generate_journal(g, 17, 140000, false); request_transition_v1 request = g.request;
        request.attempt_id = requested.attempt_id; request.operation_commitment = requested.operation_commitment;
        run(*f, recovery_classification::capacity_exhausted, request); // capacity outranks occupied requested slot
    }
    {
        auto f = std::make_unique<fixture>(); populate_clean(*f, g); generated_journal occupied = generate_journal(g, 17, 141000, false);
        install_journal(f->state(), 17, occupied.prepare, &occupied.terminal, false); auto request = g.request; request.operation_commitment[0] ^= 1;
        run(*f, recovery_classification::requested_slot_occupied, request); // slot outranks invalid transition
    }
    {
        auto f = std::make_unique<fixture>(); populate_clean(*f, g); auto & envelope = f->state().successors[0];
        envelope.live_occupied = envelope.durable_occupied = true; envelope.live_digest = envelope.durable_digest = g.request.successor_digest;
        publish(envelope.object, g.successor.data(), g.successor_size); auto request = g.request; request.operation_commitment[0] ^= 1;
        run(*f, recovery_classification::invalid_transition, request); // invalid transition outranks preexisting material
    }
    {
        auto f = std::make_unique<fixture>(); populate_clean(*f, g);
        for (size_t slot = 0; slot < 512; ++slot) {
            generated_journal journal = generate_journal(g, slot, 150000 + slot, false);
            install_journal(f->state(), slot, journal.prepare, &journal.terminal, false);
        }
        f->state().slots[17] = {}; install_journal(f->state(), 17, g.prepare);
        run(*f, recovery_classification::needs_predecessor_abort, g.request); // recovery outranks replay/capacity/slot
    }
    assert(overlaps == 11);
}

void operation_5_retry_after_abort(const golden_data & g) {
    const generated_journal retry = generate_journal(g, 18, 2200, false);
    request_transition_v1 request = g.request; request.attempt_id = retry.attempt_id; request.requested_slot = 18; request.operation_commitment = retry.operation_commitment;
    {
        auto f = std::make_unique<fixture>(); populate_clean(*f, g); install_journal(f->state(), 17, g.prepare, &g.abort_record, false);
        const size_t h = run_operation_5(*f, g, 2201, recovery_classification::continue_to_mutation, request);
        assert(f->derived_classification(h) == recovery_classification::continue_to_mutation);
    }
    {
        auto f = std::make_unique<fixture>(); populate_clean(*f, g); install_journal(f->state(), 17, g.prepare, &g.abort_record, false);
        install_journal(f->state(), 18, retry.prepare);
        const size_t h = run_operation_5(*f, g, 2202, recovery_classification::needs_predecessor_abort, request);
        assert(f->result(h).ordinary == status::uncertain_requires_recovery);
    }
    {
        auto f = std::make_unique<fixture>(); populate_clean(*f, g); install_journal(f->state(), 17, g.prepare, &g.abort_record, false);
        install_journal(f->state(), 18, retry.prepare); install_successor_head(f->state(), g);
        const size_t h = run_operation_5(*f, g, 2203, recovery_classification::needs_successor_close, request);
        assert(f->result(h).ordinary == status::uncertain_requires_recovery);
    }
    {
        const generated_journal closed_retry = generate_journal(g, 18, 2200, true);
        auto f = std::make_unique<fixture>(); populate_clean(*f, g); install_journal(f->state(), 17, g.prepare, &g.abort_record, false);
        install_journal(f->state(), 18, closed_retry.prepare, &closed_retry.terminal, true); install_successor_head(f->state(), g);
        const size_t h = run_operation_5(*f, g, 2204, recovery_classification::attempt_replayed, request);
        assert(f->result(h).ordinary == status::attempt_replayed_no_mutation);
    }
}

void operation_5_journal_topology_faults(const golden_data & g) {
    size_t topology_faults = 0; uint64_t id = 2250;
    const auto sticky = [&](auto install) {
        auto f = std::make_unique<fixture>(); populate_clean(*f, g); install(f->state());
        const size_t h = run_operation_5(*f, g, id++, recovery_classification::needs_sticky_quarantine, g.request);
        assert(f->derived_classification(h) == recovery_classification::needs_sticky_quarantine && f->scanned_slots(h) == 512); ++topology_faults;
    };
    sticky([&](fixed_state & s) { publish(s.slots[17].abort_record, g.abort_record); });
    sticky([&](fixed_state & s) { install_journal(s, 17, g.prepare, &g.close_record, true); publish(s.slots[17].abort_record, g.abort_record); install_successor_head(s, g); });
    sticky([&](fixed_state & s) { install_journal(s, 17, g.prepare); const auto other = generate_journal(g, 18, 2251, false); install_journal(s, 18, other.prepare); });
    sticky([&](fixed_state & s) {
        const auto first = generate_journal(g, 18, 2252, false), second = generate_journal(g, 19, 2252, false);
        install_journal(s, 18, first.prepare, &first.terminal, false); install_journal(s, 19, second.prepare, &second.terminal, false);
    });
    sticky([&](fixed_state & s) {
        install_journal(s, 17, g.prepare, &g.close_record, true); const auto unresolved = generate_journal(g, 18, 2253, false);
        install_journal(s, 18, unresolved.prepare); install_successor_head(s, g);
    });
    sticky([&](fixed_state & s) { install_journal(s, 17, g.prepare, &g.close_record, true); });
    sticky([&](fixed_state & s) { install_successor_head(s, g); });
    assert(topology_faults == 7);
}

void operation_5_primitive_products(const golden_data & g) {
    size_t executed = 0;
    for (primitive_code code : { primitive_code::unsupported, primitive_code::unavailable, primitive_code::io_failure }) {
        auto f = std::make_unique<fixture>(); populate_clean(*f, g); script s = operation_5_script(recovery_classification::continue_to_mutation);
        s.entries[4].code = code; s.entries[4].classification = recovery_classification::none; credential_owner c = golden_data::golden_credential();
        const size_t h = f->begin(2300 + executed, 0, std::move(c), g.preflight, g.request, s); finish(*f, h);
        assert(f->result(h).state == visibility::ordinary_result && f->result(h).ordinary == (code == primitive_code::unsupported ? status::unsupported_no_mutation : status::quarantined_or_unavailable)); ++executed;
    }
    for (completion completed : { completion::response_lost, completion::process_death }) for (primitive_code code : {
            primitive_code::ok, primitive_code::unsupported, primitive_code::unavailable, primitive_code::io_failure }) {
        auto f = std::make_unique<fixture>(); populate_clean(*f, g); script s = operation_5_script(recovery_classification::continue_to_mutation);
        s.entries[4].completed = completed; s.entries[4].code = code; s.entries[4].classification = recovery_classification::none;
        credential_owner c = golden_data::golden_credential(); const size_t h = f->begin(2310 + executed, 0, std::move(c), g.preflight, g.request, s); finish(*f, h);
        if (completed == completion::response_lost) assert(f->result(h).ordinary == status::quarantined_or_unavailable);
        else {
            assert(f->invocation_dead(h) && f->result(h).state == visibility::dead_process_no_result && f->teardown_audit_count() == 1);
            const auto audit = f->teardown_audit(0); assert(audit.credential_zero && audit.scratch_zero && audit.serialized_secret_absent);
        }
        if (completed == completion::response_lost) assert(f->ordinary_wipe_audited(h));
        ++executed;
    }
    assert(executed == 11);
}

void operation_5_snapshot_is_immutable(const golden_data & g) {
    auto f = std::make_unique<fixture>(); populate_clean(*f, g); script s = operation_5_script(recovery_classification::continue_to_mutation);
    credential_owner c = golden_data::golden_credential(); const size_t h = f->begin(2401, 0, std::move(c), g.preflight, g.request, s);
    assert(f->step(h) && f->step(h) && f->step(h));
    f->state().quarantine.live_present = true; f->state().quarantine.live_complete = true;
    assert(f->step(h)); finish(*f, h);
    assert(f->derived_classification(h) == recovery_classification::continue_to_mutation && f->state().quarantine.live_present);
}

void operation_5_preentry_contract(const golden_data & g) {
    size_t rejected = 0;
    const auto reject = [&](preflight_context_v1 preflight, request_transition_v1 request, uint64_t id) {
        auto f = std::make_unique<fixture>(); populate_clean(*f, g); script s = operation_5_script(recovery_classification::continue_to_mutation);
        credential_owner c = golden_data::golden_credential(); const size_t h = f->begin(id, 0, std::move(c), preflight, request, s);
        assert(f->result(h).state == visibility::ordinary_result && f->result(h).ordinary == status::invalid_request_no_mutation &&
            f->trace_size(h) == 0 && f->rejection_wipe_audited(h)); ++rejected;
    };
    uint64_t id = 2600;
    { auto p = g.preflight; p.store_uuid = {}; reject(p, g.request, id++); }
    { auto p = g.preflight; p.filesystem_uuid = {}; reject(p, g.request, id++); }
    { auto p = g.preflight; p.subvolume_uuid = {}; reject(p, g.request, id++); }
    { auto p = g.preflight; p.mount_id = 0; reject(p, g.request, id++); }
    { auto p = g.preflight; p.root_mode = 0777; reject(p, g.request, id++); }
    { auto p = g.preflight; p.authority_file_mode = 0600 + 1; reject(p, g.request, id++); }
    { auto p = g.preflight; p.registry_id.bytes[0] = '\0'; reject(p, g.request, id++); }
    { auto p = g.preflight; p.registry_id.bytes[0] = static_cast<char>(0x01); reject(p, g.request, id++); }
    { auto p = g.preflight; p.registry_id.bytes[0] = static_cast<char>(0x20); reject(p, g.request, id++); }
    { auto p = g.preflight; p.registry_id.bytes[0] = static_cast<char>(0x7f); reject(p, g.request, id++); }
    { auto p = g.preflight; p.registry_id.bytes[0] = static_cast<char>(0x80); reject(p, g.request, id++); }
    { auto p = g.preflight; p.registry_id.bytes[p.registry_id.size + 1] = 'x'; reject(p, g.request, id++); }
    { auto p = g.preflight; p.registry_id.size = 129; p.registry_id.bytes.fill('x'); reject(p, g.request, id++); }
    { auto p = g.preflight; p.credential_key_id.bytes[0] = '\0'; reject(p, g.request, id++); }
    { auto p = g.preflight; p.credential_generation++; reject(p, g.request, id++); }
    { auto p = g.preflight; p.inner_key_disposition = context_store_key_disposition::read_disabled; reject(p, g.request, id++); }
    { auto p = g.preflight; p.attempt_capacity = 511; reject(p, g.request, id++); }
    { auto p = g.preflight; p.maximum_logical_authority_bytes--; reject(p, g.request, id++); }
    { auto r = g.request; r.attempt_id = {}; reject(g.preflight, r, id++); }
    { auto r = g.request; r.operation_commitment = {}; reject(g.preflight, r, id++); }
    { auto r = g.request; r.requested_slot = 512; reject(g.preflight, r, id++); }
    { auto r = g.request; r.predecessor_length = 0; reject(g.preflight, r, id++); }
    { auto r = g.request; r.successor_length = 1025; reject(g.preflight, r, id++); }
    { auto r = g.request; r.expected_current_head_length = 0; reject(g.preflight, r, id++); }
    { auto r = g.request; r.predecessor_digest = {}; reject(g.preflight, r, id++); }
    { auto r = g.request; r.successor_digest = {}; reject(g.preflight, r, id++); }
    { auto r = g.request; r.expected_current_head_digest = {}; reject(g.preflight, r, id++); }
    { auto r = g.request; r.predecessor[r.predecessor_length] = 1; reject(g.preflight, r, id++); }
    { auto r = g.request; r.successor[r.successor_length] = 1; reject(g.preflight, r, id++); }
    { auto r = g.request; r.expected_current_head[r.expected_current_head_length] = 1; reject(g.preflight, r, id++); }
    assert(rejected == 30);

    // The maximum admitted identifier length is exactly 128 bytes. It reaches
    // Operation 5 and then safely becomes a compatibility miss against the
    // authenticated root rather than being rejected as malformed pre-entry.
    {
        auto p = g.preflight; p.registry_id = {}; p.registry_id.size = 128;
        std::fill_n(p.registry_id.bytes.begin(), 128, 'x');
        auto f = std::make_unique<fixture>(); populate_clean(*f, g);
        credential_owner c = golden_data::golden_credential(); script s = operation_5_script(recovery_classification::needs_sticky_quarantine);
        const size_t h = f->begin(id++, 0, std::move(c), p, g.request, s); finish(*f, h);
        assert(f->trace_size(h) != 0 && f->derived_classification(h) == recovery_classification::needs_sticky_quarantine);
    }
    {
        auto f = std::make_unique<fixture>(); populate_clean(*f, g); script s = operation_5_script(recovery_classification::continue_to_mutation);
        credential_owner c = golden_data::golden_credential(); c.key_id.bytes[c.key_id.size + 1] = 'x';
        const size_t h = f->begin(id++, 0, std::move(c), g.preflight, g.request, s);
        assert(f->result(h).state == visibility::ordinary_result && f->result(h).ordinary == status::invalid_request_no_mutation &&
            f->trace_size(h) == 0 && f->rejection_wipe_audited(h));
    }
}

void operation_5_external_binding_and_request_matrix(const golden_data & g) {
    size_t binding_faults = 0, request_faults = 0; uint64_t id = 2800;
    const auto binding_fault = [&](preflight_context_v1 preflight) {
        auto f = std::make_unique<fixture>(); populate_clean(*f, g); script s = operation_5_script(recovery_classification::needs_sticky_quarantine);
        credential_owner c = golden_data::golden_credential(); const size_t h = f->begin(id++, 0, std::move(c), preflight, g.request, s); finish(*f, h);
        assert(f->derived_classification(h) == recovery_classification::needs_sticky_quarantine && f->scanned_slots(h) == 512); ++binding_faults;
    };
    { auto p = g.preflight; p.store_uuid[0] ^= 1; binding_fault(p); }
    { auto p = g.preflight; p.filesystem_uuid[0] ^= 1; binding_fault(p); }
    { auto p = g.preflight; p.mount_id++; binding_fault(p); }
    { auto p = g.preflight; p.owner_uid++; binding_fault(p); }
    { auto p = g.preflight; p.lock_st_dev++; binding_fault(p); }
    { auto p = g.preflight; p.lock_st_ino++; binding_fault(p); }
    { auto p = g.preflight; p.path_policy_commitment[0] ^= 1; binding_fault(p); }
    { auto p = g.preflight; p.registry_id.bytes[0] = 's'; binding_fault(p); }
    { auto p = g.preflight; p.registry_epoch++; binding_fault(p); }
    { auto p = g.preflight; p.authority_base_scope_commitment[0] ^= 1; binding_fault(p); }
    { auto p = g.preflight; p.registry_policy_commitment[0] ^= 1; binding_fault(p); }
    assert(binding_faults == 11);

    const auto request_fault = [&](request_transition_v1 request) {
        auto f = std::make_unique<fixture>(); populate_clean(*f, g);
        const size_t h = run_operation_5(*f, g, id++, recovery_classification::invalid_transition, request);
        assert(f->derived_classification(h) == recovery_classification::invalid_transition && f->scanned_slots(h) == 512); ++request_faults;
    };
    { auto r = g.request; r.operation_commitment[0] ^= 1; request_fault(r); }
    { auto r = g.request; r.predecessor[0] ^= 1; request_fault(r); }
    { auto r = g.request; r.successor[0] ^= 1; request_fault(r); }
    { auto r = g.request; r.expected_current_head[0] ^= 1; request_fault(r); }
    { auto r = g.request; r.predecessor_digest[0] ^= 1; request_fault(r); }
    { auto r = g.request; r.successor_digest[0] ^= 1; request_fault(r); }
    { auto r = g.request; r.expected_current_head_digest[0] ^= 1; request_fault(r); }
    assert(request_faults == 7);
}

void operation_5_all_slot_positions(const golden_data & g) {
    size_t replay_positions = 0, corrupt_positions = 0;
    for (size_t slot = 0; slot < 512; ++slot) {
        const generated_journal journal = generate_journal(g, slot, 50000 + slot, false);
        request_transition_v1 request = g.request; request.attempt_id = journal.attempt_id;
        {
            auto f = std::make_unique<fixture>(); populate_clean(*f, g); install_journal(f->state(), slot, journal.prepare, &journal.terminal, false);
            const size_t h = run_operation_5(*f, g, 60000 + slot, recovery_classification::attempt_replayed, request);
            assert(f->derived_classification(h) == recovery_classification::attempt_replayed && f->scanned_slots(h) == 512); ++replay_positions;
        }
        {
            auto corrupted = journal.terminal; corrupted[corrupted.size() / 2] ^= 1;
            auto f = std::make_unique<fixture>(); populate_clean(*f, g); install_journal(f->state(), slot, journal.prepare, &corrupted, false);
            const size_t h = run_operation_5(*f, g, 61000 + slot, recovery_classification::needs_sticky_quarantine, g.request);
            assert(f->derived_classification(h) == recovery_classification::needs_sticky_quarantine && f->scanned_slots(h) == 512); ++corrupt_positions;
        }
    }
    assert(replay_positions == 512 && corrupt_positions == 512);
}

void operation_5_all_staging_and_envelope_positions(const golden_data & g) {
    size_t successor_staging_positions = 0, selector_staging_positions = 0, cross_staging_positions = 0;
    size_t valid_envelope_positions = 0, selected_envelope_positions = 0, malformed_envelope_positions = 0;
    for (size_t slot = 0; slot < 512; ++slot) {
        {
            auto f = std::make_unique<fixture>(); populate_clean(*f, g); f->state().slots[slot].successor_staging.live_present = true;
            const size_t h = run_operation_5(*f, g, 100000 + slot, recovery_classification::needs_sticky_quarantine, g.request);
            assert(f->derived_classification(h) == recovery_classification::needs_sticky_quarantine && f->scanned_slots(h) == 512);
            ++successor_staging_positions;
        }
        {
            auto f = std::make_unique<fixture>(); populate_clean(*f, g); f->state().slots[slot].selector_staging.durable_present = true;
            const size_t h = run_operation_5(*f, g, 101000 + slot, recovery_classification::needs_sticky_quarantine, g.request);
            assert(f->derived_classification(h) == recovery_classification::needs_sticky_quarantine && f->scanned_slots(h) == 512);
            ++selector_staging_positions;
        }
        {
            auto f = std::make_unique<fixture>(); populate_clean(*f, g); f->state().slots[slot].successor_staging.durable_present = true;
            const size_t h = run_operation_5(*f, g, 101600 + slot, recovery_classification::needs_sticky_quarantine, g.request);
            assert(f->derived_classification(h) == recovery_classification::needs_sticky_quarantine && f->scanned_slots(h) == 512);
            ++cross_staging_positions;
        }
        {
            auto f = std::make_unique<fixture>(); populate_clean(*f, g); f->state().slots[slot].selector_staging.live_present = true;
            const size_t h = run_operation_5(*f, g, 102200 + slot, recovery_classification::needs_sticky_quarantine, g.request);
            assert(f->derived_classification(h) == recovery_classification::needs_sticky_quarantine && f->scanned_slots(h) == 512);
            ++cross_staging_positions;
        }
        {
            auto f = std::make_unique<fixture>(); populate_clean(*f, g); auto & envelope = f->state().successors[slot];
            envelope.live_occupied = envelope.durable_occupied = true;
            envelope.live_digest = envelope.durable_digest = g.request.successor_digest;
            publish(envelope.object, g.successor.data(), g.successor_size);
            const size_t h = run_operation_5(*f, g, 102000 + slot, recovery_classification::preexisting_unattributed_material, g.request);
            assert(f->derived_classification(h) == recovery_classification::preexisting_unattributed_material && f->scanned_slots(h) == 512);
            ++valid_envelope_positions;
        }
        {
            auto f = std::make_unique<fixture>(); populate_clean(*f, g); install_journal(f->state(), 17, g.prepare, &g.close_record, true);
            publish(f->state().head, g.head_successor); auto & envelope = f->state().successors[slot];
            envelope.live_occupied = envelope.durable_occupied = true;
            envelope.live_digest = envelope.durable_digest = g.request.successor_digest;
            publish(envelope.object, g.successor.data(), g.successor_size);
            const size_t h = run_operation_5(*f, g, 102600 + slot, recovery_classification::attempt_replayed, g.request);
            assert(f->derived_classification(h) == recovery_classification::attempt_replayed && f->scanned_slots(h) == 512);
            ++selected_envelope_positions;
        }
        {
            auto f = std::make_unique<fixture>(); populate_clean(*f, g); auto & envelope = f->state().successors[slot];
            envelope.live_occupied = envelope.durable_occupied = true;
            envelope.live_digest = envelope.durable_digest = g.request.successor_digest;
            publish(envelope.object, g.successor.data(), g.successor_size); envelope.object.live_bytes[0] ^= 1; envelope.object.durable_bytes[0] ^= 1;
            const size_t h = run_operation_5(*f, g, 103000 + slot, recovery_classification::needs_sticky_quarantine, g.request);
            assert(f->derived_classification(h) == recovery_classification::needs_sticky_quarantine && f->scanned_slots(h) == 512);
            ++malformed_envelope_positions;
        }
    }
    assert(successor_staging_positions == 512 && selector_staging_positions == 512 && cross_staging_positions == 1024 &&
        valid_envelope_positions == 512 && selected_envelope_positions == 512 && malformed_envelope_positions == 512);
}

void operation_5_projection_and_namespace_matrix(const golden_data & g) {
    size_t cases = 0; uint64_t id = 70000;
    const auto sticky = [&](auto mutate) {
        auto f = std::make_unique<fixture>(); populate_clean(*f, g); mutate(f->state());
        const size_t h = run_operation_5(*f, g, id++, recovery_classification::needs_sticky_quarantine, g.request);
        assert(f->derived_classification(h) == recovery_classification::needs_sticky_quarantine && f->result(h).ordinary == status::quarantined_or_unavailable);
        ++cases;
    };
    sticky([](fixed_state & s) { s.root_directory.live_projection = false; });
    sticky([](fixed_state & s) { s.attempts_directory.durable_projection = false; });
    sticky([](fixed_state & s) { s.staging_directory.live_projection = false; });
    sticky([](fixed_state & s) { s.envelopes_directory.durable_projection = false; });
    sticky([](fixed_state & s) { s.marker.durable_bytes[4] ^= 1; });
    sticky([](fixed_state & s) { s.head.live_bytes[4] ^= 1; });
    sticky([](fixed_state & s) { s.lock_file.live_complete = false; });
    sticky([](fixed_state & s) { s.initial_envelope.durable_digest[0] ^= 1; });
    sticky([](fixed_state & s) { s.initial_envelope.object.durable_complete = false; });
    sticky([](fixed_state & s) { s.quarantine.live_complete = true; });
    sticky([](fixed_state & s) { s.quarantine_staging.durable_length = 1; });
    sticky([](fixed_state & s) { s.slots[510].prepare.live_complete = true; });
    sticky([](fixed_state & s) { s.slots[510].close.durable_length = 1; });
    sticky([](fixed_state & s) { s.slots[510].abort_record.live_complete = true; });
    sticky([](fixed_state & s) { s.slots[510].successor_staging.durable_length = 1; });
    sticky([](fixed_state & s) { s.slots[510].selector_staging.live_complete = true; });
    sticky([](fixed_state & s) { s.slots[511].successor_staging.live_present = true; });
    sticky([](fixed_state & s) { s.slots[511].selector_staging.durable_present = true; });
    for (size_t index = 0; index < 32; ++index) {
        sticky([index](fixed_state & s) { s.unexpected[index].live_occupied = true; s.unexpected[index].live_length = 1; s.unexpected[index].live_name[0] = 'x'; });
        sticky([index](fixed_state & s) { s.unexpected[index].durable_occupied = true; s.unexpected[index].durable_length = 1; s.unexpected[index].durable_name[0] = 'x'; });
        sticky([index](fixed_state & s) { s.unexpected[index].live_name[0] = 'x'; });
        sticky([index](fixed_state & s) {
            s.unexpected[index].live_occupied = s.unexpected[index].durable_occupied = true;
            s.unexpected[index].live_length = s.unexpected[index].durable_length = 1;
            s.unexpected[index].live_name[0] = 'x'; s.unexpected[index].durable_name[0] = 'y';
        });
        sticky([index](fixed_state & s) {
            s.unexpected[index].live_occupied = s.unexpected[index].durable_occupied = true;
            s.unexpected[index].live_length = s.unexpected[index].durable_length = 1;
            s.unexpected[index].live_name[0] = s.unexpected[index].durable_name[0] = 'x';
        });
    }
    assert(cases == 178);
}

enum class hostile_target { root, head, prepare, close_record, abort_record };

modeled_file<1024> * hostile_file_1024(fixed_state & state, hostile_target target) {
    if (target == hostile_target::root) return &state.marker;
    if (target == hostile_target::head) return &state.head;
    if (target == hostile_target::close_record) return &state.slots[17].close;
    if (target == hostile_target::abort_record) return &state.slots[17].abort_record;
    return nullptr;
}

void prepare_hostile_state(fixture & f, const golden_data & g, hostile_target target) {
    populate_clean(f, g);
    if (target == hostile_target::prepare) install_journal(f.state(), 17, g.prepare);
    if (target == hostile_target::abort_record) install_journal(f.state(), 17, g.prepare, &g.abort_record, false);
    if (target == hostile_target::close_record) { install_journal(f.state(), 17, g.prepare, &g.close_record, true); install_successor_head(f.state(), g); }
}

void set_hostile_bytes(fixed_state & state, hostile_target target, const uint8_t * data, size_t size) {
    if (target == hostile_target::prepare) publish(state.slots[17].prepare, data, size);
    else publish(*hostile_file_1024(state, target), data, size);
}

void operation_5_hostile_bytes(const golden_data & g) {
    struct case_row { hostile_target target; const std::vector<uint8_t> * bytes; };
    const case_row rows[] = { { hostile_target::root, &g.root_initialized }, { hostile_target::head, &g.head_initial },
        { hostile_target::prepare, &g.prepare }, { hostile_target::close_record, &g.close_record }, { hostile_target::abort_record, &g.abort_record } };
    uint64_t invocation = 3000; size_t truncations = 0, bit_mutations = 0;
    for (const case_row & row : rows) {
        auto f = std::make_unique<fixture>(); prepare_hostile_state(*f, g, row.target);
        for (size_t cut = 0; cut < row.bytes->size(); ++cut) {
            set_hostile_bytes(f->state(), row.target, row.bytes->data(), cut);
            const size_t h = run_operation_5(*f, g, invocation++, recovery_classification::needs_sticky_quarantine, g.request);
            assert(f->derived_classification(h) == recovery_classification::needs_sticky_quarantine && f->result(h).ordinary == status::quarantined_or_unavailable);
            ++truncations;
        }
        for (size_t index = 0; index < row.bytes->size(); ++index) for (uint8_t bit = 0; bit < 8; ++bit) {
            std::vector<uint8_t> mutated = *row.bytes; mutated[index] ^= static_cast<uint8_t>(1U << bit);
            set_hostile_bytes(f->state(), row.target, mutated.data(), mutated.size());
            const size_t h = run_operation_5(*f, g, invocation++, recovery_classification::needs_sticky_quarantine, g.request);
            assert(f->derived_classification(h) == recovery_classification::needs_sticky_quarantine && f->result(h).ordinary == status::quarantined_or_unavailable);
            ++bit_mutations;
        }
    }
    assert(truncations == g.root_initialized.size() + g.head_initial.size() + g.prepare.size() + g.close_record.size() + g.abort_record.size());
    assert(bit_mutations == truncations * 8);
}

void operation_5_authenticated_semantic_rejection(const golden_data & g) {
    size_t rejected = 0; uint64_t invocation = 90000;
    const auto reject = [&](hostile_target target, std::vector<uint8_t> bytes, const char * domain) {
        retag_record(bytes, domain);
        auto f = std::make_unique<fixture>(); prepare_hostile_state(*f, g, target);
        set_hostile_bytes(f->state(), target, bytes.data(), bytes.size());
        const size_t h = run_operation_5(*f, g, invocation++, recovery_classification::needs_sticky_quarantine, g.request);
        assert(f->derived_classification(h) == recovery_classification::needs_sticky_quarantine &&
            f->result(h).ordinary == status::quarantined_or_unavailable && f->scanned_slots(h) == 512);
        ++rejected;
    };
    {
        auto bytes = g.root_initialized; xor_unique_byte(bytes, { 0x04, 0x50 }, 0);
        reject(hostile_target::root, std::move(bytes), "halofpx.registry-lab-root-auth.v1");
    }
    {
        auto bytes = g.root_initialized; xor_unique_byte(bytes, { 0x0f, 0x58, 0x20 }, 0);
        reject(hostile_target::root, std::move(bytes), "halofpx.registry-lab-root-auth.v1");
    }
    {
        auto bytes = g.head_initial; replace_unique_byte(bytes, { 0x09, 0x18, 0x28 }, 2, 0x29);
        reject(hostile_target::head, std::move(bytes), "halofpx.registry-lab-head-auth.v1");
    }
    {
        auto bytes = g.prepare; replace_unique_byte(bytes, { 0x0e, 0x01, 0x0f, 0x58, 0x20 }, 1, 0x00);
        reject(hostile_target::prepare, std::move(bytes), "halofpx.registry-lab-prepare-auth.v1");
    }
    {
        auto bytes = g.prepare; replace_unique_byte(bytes, { 0x06, 0x11, 0x07, 0x58, 0x20 }, 1, 0x12);
        reject(hostile_target::prepare, std::move(bytes), "halofpx.registry-lab-prepare-auth.v1");
    }
    {
        auto bytes = g.prepare; xor_unique_byte(bytes, { 0x0f, 0x58, 0x20 }, 0);
        reject(hostile_target::prepare, std::move(bytes), "halofpx.registry-lab-prepare-auth.v1");
    }
    {
        auto bytes = g.prepare; xor_unique_byte(bytes, { 0x10, 0x58, 0x20 }, 0);
        reject(hostile_target::prepare, std::move(bytes), "halofpx.registry-lab-prepare-auth.v1");
    }
    {
        auto bytes = g.close_record; replace_unique_byte(bytes, { 0x0b, 0x02, 0x0c, 0x00 }, 1, 0x01);
        reject(hostile_target::close_record, std::move(bytes), "halofpx.registry-lab-close-auth.v1");
    }
    {
        auto bytes = g.close_record; replace_unique_byte(bytes, { 0x0b, 0x02, 0x0c, 0x00 }, 1, 0x00);
        reject(hostile_target::close_record, std::move(bytes), "halofpx.registry-lab-close-auth.v1");
    }
    {
        auto bytes = g.close_record; xor_unique_byte(bytes, { 0x0d, 0x58, 0x20 }, 0);
        reject(hostile_target::close_record, std::move(bytes), "halofpx.registry-lab-close-auth.v1");
    }
    {
        auto bytes = g.close_record; xor_unique_byte(bytes, { 0x0a, 0x58, 0x20 }, 0);
        reject(hostile_target::close_record, std::move(bytes), "halofpx.registry-lab-close-auth.v1");
    }
    {
        auto bytes = g.abort_record; replace_unique_byte(bytes, { 0x0b, 0x01, 0x0c, 0x01 }, 1, 0x02);
        reject(hostile_target::abort_record, std::move(bytes), "halofpx.registry-lab-abort-auth.v1");
    }
    {
        auto bytes = g.abort_record; replace_unique_byte(bytes, { 0x0b, 0x01, 0x0c, 0x01 }, 1, 0x00);
        reject(hostile_target::abort_record, std::move(bytes), "halofpx.registry-lab-abort-auth.v1");
    }
    {
        auto bytes = g.abort_record; xor_unique_byte(bytes, { 0x0d, 0x58, 0x20 }, 0);
        reject(hostile_target::abort_record, std::move(bytes), "halofpx.registry-lab-abort-auth.v1");
    }
    {
        auto bytes = g.abort_record; xor_unique_byte(bytes, { 0x0a, 0x58, 0x20 }, 0);
        reject(hostile_target::abort_record, std::move(bytes), "halofpx.registry-lab-abort-auth.v1");
    }
    {
        auto head = g.head_successor; replace_unique_byte(head, { 0x09, 0x18, 0x29 }, 2, 0x2a);
        retag_record(head, "halofpx.registry-lab-head-auth.v1");
        auto f = std::make_unique<fixture>(); populate_clean(*f, g); install_journal(f->state(), 17, g.prepare); install_successor_head(f->state(), g);
        publish(f->state().head, head);
        const size_t h = run_operation_5(*f, g, invocation++, recovery_classification::needs_sticky_quarantine, g.request);
        assert(f->derived_classification(h) == recovery_classification::needs_sticky_quarantine && f->scanned_slots(h) == 512);
        ++rejected;
    }
    assert(rejected == 16);
}

void operation_5_authenticated_terminal_class_matrix(const golden_data & g) {
    // Both record kinds admit both target-native terminal classes. The class
    // describes how the terminal was reached; record kind and fixed phase
    // still determine whether the transition advanced or remained unchanged.
    {
        auto recovered_close = g.close_record;
        replace_unique_byte(recovered_close, { 0x0b, 0x02, 0x0c, 0x00 }, 3, 0x01);
        retag_record(recovered_close, "halofpx.registry-lab-close-auth.v1");
        auto f = std::make_unique<fixture>(); populate_clean(*f, g);
        install_journal(f->state(), 17, g.prepare, &recovered_close, true); install_successor_head(f->state(), g);
        const size_t h = run_operation_5(*f, g, 155000, recovery_classification::attempt_replayed, g.request);
        assert(f->derived_classification(h) == recovery_classification::attempt_replayed && f->scanned_slots(h) == 512);
    }
    {
        auto normal_abort = g.abort_record;
        replace_unique_byte(normal_abort, { 0x0b, 0x01, 0x0c, 0x01 }, 3, 0x00);
        retag_record(normal_abort, "halofpx.registry-lab-abort-auth.v1");
        auto f = std::make_unique<fixture>(); populate_clean(*f, g);
        install_journal(f->state(), 17, g.prepare, &normal_abort, false);
        const size_t h = run_operation_5(*f, g, 155001, recovery_classification::attempt_replayed, g.request);
        assert(f->derived_classification(h) == recovery_classification::attempt_replayed && f->scanned_slots(h) == 512);
    }
}

void operation_5_authenticated_structural_rejection(const golden_data & g) {
    struct row { hostile_target target; const std::vector<uint8_t> * source; const char * domain; uint8_t body_map; };
    const row rows[] = {
        { hostile_target::root, &g.root_initialized, "halofpx.registry-lab-root-auth.v1", 0xb2 },
        { hostile_target::head, &g.head_initial, "halofpx.registry-lab-head-auth.v1", 0xad },
        { hostile_target::prepare, &g.prepare, "halofpx.registry-lab-prepare-auth.v1", 0xb1 },
        { hostile_target::close_record, &g.close_record, "halofpx.registry-lab-close-auth.v1", 0xae },
        { hostile_target::abort_record, &g.abort_record, "halofpx.registry-lab-abort-auth.v1", 0xae },
    };
    size_t rejected = 0; uint64_t invocation = 160000;
    const auto reject = [&](const row & item, std::vector<uint8_t> bytes) {
        retag_record(bytes, item.domain); auto f = std::make_unique<fixture>(); prepare_hostile_state(*f, g, item.target);
        set_hostile_bytes(f->state(), item.target, bytes.data(), bytes.size());
        const size_t h = run_operation_5(*f, g, invocation++, recovery_classification::needs_sticky_quarantine, g.request);
        assert(f->derived_classification(h) == recovery_classification::needs_sticky_quarantine && f->scanned_slots(h) == 512);
        ++rejected;
    };
    constexpr std::array<uint8_t, 16> key_id { 'r','e','g','i','s','t','r','y','-','a','u','t','h','-','v','1' };
    for (const row & item : rows) {
        assert(item.source->size() > 40 && (*item.source)[0] == 0xa2 && (*item.source)[1] == 0x00 &&
            (*item.source)[2] == 0xa4 && (*item.source)[3] == 0x00 && (*item.source)[4] == item.body_map &&
            (*item.source)[5] == 0x00 && (*item.source)[6] == 0x01 && (*item.source)[7] == 0x01 && (*item.source)[8] == 0x00);
        { auto bytes = *item.source; bytes[6] = 0x02; reject(item, std::move(bytes)); } // wrong major version
        { auto bytes = *item.source; --bytes[4]; reject(item, std::move(bytes)); } // wrong body field count
        { auto bytes = *item.source; bytes[5] = 0x01; bytes[7] = 0x00; reject(item, std::move(bytes)); } // non-increasing keys
        {
            auto bytes = *item.source; assert(bytes[11] == 0x03 && bytes[12] == 0x58 && bytes[13] == 0x20);
            bytes[13] = 0x21; reject(item, std::move(bytes));
        } // malformed declared root-ID length
        {
            const auto & source = *item.source; const size_t authentication_end = source.size() - 35;
            std::vector<uint8_t> bytes; bytes.reserve(source.size() + 1);
            for (size_t i = 0; i < authentication_end; ++i) bytes.push_back(source[i]);
            bytes.push_back(0x00);
            for (size_t i = authentication_end; i < source.size(); ++i) bytes.push_back(source[i]);
            reject(item, std::move(bytes));
        }
        { auto bytes = *item.source; bytes[0] = 0xa3; reject(item, std::move(bytes)); } // wrong outer field count
        {
            auto bytes = *item.source; const auto position = std::find_end(bytes.begin(), bytes.end(), key_id.begin(), key_id.end());
            assert(position != bytes.end()); *position = 's'; reject(item, std::move(bytes));
        }
    }
    assert(rejected == 35);
}

void operation_5_authenticated_one_close_history_rejection(const golden_data & g) {
    size_t rejected = 0; uint64_t invocation = 170000;
    {
        // Mutate PREPARE's previous-HEAD anchor, re-authenticate it, and update
        // the dependent CLOSE's exact PREPARE digest before re-authenticating
        // CLOSE. Rejection therefore proves the marker/history anchor rule.
        auto prepare = g.prepare; xor_unique_byte(prepare, { 0x0f, 0x58, 0x20 }, 0);
        retag_record(prepare, "halofpx.registry-lab-prepare-auth.v1");
        const auto prepare_digest = test_hash_domain("halofpx.registry-lab-prepare.v1", prepare);
        auto close = g.close_record; replace_unique_digest(close, { 0x0a, 0x58, 0x20 }, prepare_digest);
        retag_record(close, "halofpx.registry-lab-close-auth.v1");
        auto f = std::make_unique<fixture>(); populate_clean(*f, g); install_journal(f->state(), 17, prepare, &close, true); install_successor_head(f->state(), g);
        const size_t h = run_operation_5(*f, g, invocation++, recovery_classification::needs_sticky_quarantine, g.request);
        assert(f->derived_classification(h) == recovery_classification::needs_sticky_quarantine && f->scanned_slots(h) == 512); ++rejected;
    }
    {
        // Make CLOSE consistently name the authenticated predecessor HEAD while
        // leaving its claimed successor transition intact. All tags and record
        // links authenticate, but the one-CLOSE resolved successor is wrong.
        auto close = g.close_record; replace_unique_digest(close, { 0x0d, 0x58, 0x20 }, g.request.expected_current_head_digest);
        retag_record(close, "halofpx.registry-lab-close-auth.v1");
        auto f = std::make_unique<fixture>(); populate_clean(*f, g); install_journal(f->state(), 17, g.prepare, &close, true);
        const size_t h = run_operation_5(*f, g, invocation++, recovery_classification::needs_sticky_quarantine, g.request);
        assert(f->derived_classification(h) == recovery_classification::needs_sticky_quarantine && f->scanned_slots(h) == 512); ++rejected;
    }
    assert(rejected == 2);
}

void operation_5_key_selection_before_kdf(const golden_data & g) {
    auto f = std::make_unique<fixture>(); populate_clean(*f, g);
    const std::array<uint8_t, 16> key_text { 'r','e','g','i','s','t','r','y','-','a','u','t','h','-','v','1' };
    auto & bytes = f->state().marker.live_bytes; auto first = std::search(bytes.begin(), bytes.begin() + f->state().marker.live_length, key_text.begin(), key_text.end());
    assert(first != bytes.begin() + f->state().marker.live_length); auto second = std::search(first + key_text.size(), bytes.begin() + f->state().marker.live_length, key_text.begin(), key_text.end());
    assert(second != bytes.begin() + f->state().marker.live_length); const size_t offset = static_cast<size_t>(second - bytes.begin());
    f->state().marker.live_bytes[offset] = f->state().marker.durable_bytes[offset] = 's';
    f->state().head = {}; // Isolate the root decoder's pre-KDF selector decision.
    const size_t h = run_operation_5(*f, g, 2501, recovery_classification::needs_sticky_quarantine, g.request);
    assert(f->derived_classification(h) == recovery_classification::needs_sticky_quarantine && f->kdf_calls(h) == 0);
}

} // namespace

int main(int argc, char ** argv) {
    assert(argc == 2 || argc == 3); const golden_data golden(argv[1]);
    if (argc == 3) {
        assert(std::strcmp(argv[2], "--repeat-core") == 0);
        algebra(); operation_5_pairwise_precedence_matrix(); operation_5_allocation_free_after_construction(golden);
        operation_5_core(golden); operation_5_recovery_and_request_precedence(golden); operation_5_retry_after_abort(golden);
        operation_5_journal_topology_faults(golden); operation_5_primitive_products(golden); operation_5_snapshot_is_immutable(golden);
        operation_5_preentry_contract(golden); operation_5_external_binding_and_request_matrix(golden);
        operation_5_projection_and_namespace_matrix(golden); operation_5_key_selection_before_kdf(golden);
        operation_5_authenticated_semantic_rejection(golden); operation_5_authenticated_terminal_class_matrix(golden);
        operation_5_authenticated_structural_rejection(golden);
        operation_5_authenticated_one_close_history_rejection(golden);
        return 0;
    }
    algebra();
    operation_5_pairwise_precedence_matrix();
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
    operation_5_allocation_free_after_construction(golden);
    operation_5_core(golden);
    operation_5_recovery_and_request_precedence(golden);
    operation_5_integrated_precedence_overlaps(golden);
    operation_5_retry_after_abort(golden);
    operation_5_journal_topology_faults(golden);
    operation_5_primitive_products(golden);
    operation_5_snapshot_is_immutable(golden);
    operation_5_preentry_contract(golden);
    operation_5_external_binding_and_request_matrix(golden);
    operation_5_all_slot_positions(golden);
    operation_5_all_staging_and_envelope_positions(golden);
    operation_5_projection_and_namespace_matrix(golden);
    operation_5_key_selection_before_kdf(golden);
    operation_5_authenticated_semantic_rejection(golden);
    operation_5_authenticated_terminal_class_matrix(golden);
    operation_5_authenticated_structural_rejection(golden);
    operation_5_authenticated_one_close_history_rejection(golden);
    operation_5_hostile_bytes(golden);
}
