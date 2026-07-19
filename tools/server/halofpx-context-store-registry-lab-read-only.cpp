#include "halofpx-context-store-registry-lab-read-only-internal.h"

#include <algorithm>
#include <climits>
#include <cstring>
#include <type_traits>
#include <utility>

#if defined(_MSC_VER)
#define HALOFPX_NOINLINE __declspec(noinline)
#elif defined(__GNUC__) || defined(__clang__)
#define HALOFPX_NOINLINE __attribute__((noinline))
#else
#define HALOFPX_NOINLINE
#endif

extern "C" HALOFPX_NOINLINE bool halofpx_registry_lab_constant_time_tag_equal_32(
        const uint8_t * a, const uint8_t * b) noexcept;
extern "C" HALOFPX_NOINLINE bool halofpx_registry_lab_constant_time_tag_equal_32(
        const uint8_t * a, const uint8_t * b) noexcept {
    volatile uint8_t difference = 0;
    for (size_t i = 0; i < 32; ++i) difference = static_cast<uint8_t>(difference | (a[i] ^ b[i]));
    return difference == 0;
}

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
        if (byte < 0x21 || byte > 0x7e) return false;
    }
    for (size_t i = owner.key_id.size; i < owner.key_id.bytes.size(); ++i) if (owner.key_id.bytes[i] != '\0') return false;
    return true;
}

void wipe_credential(credential_owner & owner) noexcept {
    owner.clear();
}

template<size_t N> void wipe(std::array<uint8_t, N> & value) noexcept {
    volatile uint8_t * output = value.data();
    for (size_t i = 0; i < N; ++i) output[i] = 0;
}

template<size_t N> bool valid_durable_file(const modeled_file<N> & source) noexcept {
    if (source.durable_length > N) return false;
    for (size_t i = source.durable_length; i < N; ++i) if (source.durable_bytes[i] != 0) return false;
    // File-data synchronization can advance an inode before its directory name
    // is durable. Hidden durable bytes are valid modeled state but are omitted
    // from the canonical durable-only restart image until namespace publication.
    if (!source.durable_present && (source.durable_complete || source.durable_length != 0)) {
        if (!source.live_present || source.durable_length > source.live_length || source.live_length > N) return false;
        for (size_t i = 0; i < source.durable_length; ++i)
            if (source.durable_bytes[i] != source.live_bytes[i]) return false;
        if (source.durable_complete && (!source.live_complete || source.durable_length == 0 || source.durable_length != source.live_length)) return false;
    }
    return true;
}

template<size_t N> bool valid_restart_file(bool present, bool complete, size_t length, const std::array<uint8_t, N> & bytes) noexcept {
    if (length > N || (!present && (complete || length != 0))) return false;
    for (size_t i = length; i < N; ++i) if (bytes[i] != 0) return false;
    return true;
}

template<size_t N> void save_file(const modeled_file<N> & source, bool & present, bool & complete, size_t & length, std::array<uint8_t, N> & bytes) noexcept {
    present = source.durable_present;
    complete = present && source.durable_complete;
    length = present ? source.durable_length : 0;
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
    return file.durable_length <= N && contains_secret(file.durable_bytes.data(), file.durable_length, secret);
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
    if ((op == operation::preflight || op == operation::snapshot_load || op == operation::recovery_validation) &&
        (code == primitive_code::unavailable || code == primitive_code::io_failure)) return status::quarantined_or_unavailable;
    return status::invalid_request_no_mutation;
}

bool same_id(const context_store_registered_id & a, const context_store_registered_id & b) noexcept {
    if (a.size != b.size) return false;
    uint8_t difference = 0;
    for (size_t i = 0; i < a.size; ++i) difference |= static_cast<uint8_t>(a.bytes[i]) ^ static_cast<uint8_t>(b.bytes[i]);
    return difference == 0;
}

template<size_t N> bool nonzero(const std::array<uint8_t, N> & value) noexcept { return !all_zero(value); }

bool exact_bytes(const uint8_t * a, const uint8_t * b, size_t size) noexcept {
    if ((!a || !b) && size) return false;
    uint8_t difference = 0;
    for (size_t i = 0; i < size; ++i) difference |= a[i] ^ b[i];
    return difference == 0;
}

struct cbor_reader {
    const uint8_t * data = nullptr;
    size_t size = 0, offset = 0;
    bool head(uint8_t major, uint64_t & value) noexcept {
        if (!data || offset >= size) return false;
        const uint8_t first = data[offset++];
        if ((first >> 5) != major) return false;
        const uint8_t additional = first & 31;
        if (additional < 24) { value = additional; return true; }
        const size_t width = additional == 24 ? 1 : additional == 25 ? 2 : additional == 26 ? 4 : additional == 27 ? 8 : 0;
        if (!width || width > size - offset) return false;
        value = 0;
        for (size_t i = 0; i < width; ++i) value = (value << 8) | data[offset++];
        return !((width == 1 && value < 24) || (width == 2 && value <= UINT8_MAX) ||
            (width == 4 && value <= UINT16_MAX) || (width == 8 && value <= UINT32_MAX));
    }
    bool exact(uint64_t expected) noexcept { uint64_t value = 0; return head(0, value) && value == expected; }
    bool unsigned_value(uint64_t & value) noexcept { return head(0, value); }
    bool map(size_t expected) noexcept { uint64_t value = 0; return head(5, value) && value == expected; }
    bool bytes(uint8_t major, const uint8_t *& value, size_t & length) noexcept {
        uint64_t encoded = 0;
        if (!head(major, encoded) || encoded > size - offset) return false;
        value = data + offset; length = static_cast<size_t>(encoded); offset += length; return true;
    }
    bool digest(context_store_format_digest & output) noexcept {
        const uint8_t * value = nullptr; size_t length = 0;
        if (!bytes(2, value, length) || length != output.size()) return false;
        std::copy_n(value, length, output.begin()); return true;
    }
    bool id(context_store_registered_id & output) noexcept {
        const uint8_t * value = nullptr; size_t length = 0;
        if (!bytes(3, value, length) || length == 0 || length > context_store_registered_id_max_bytes) return false;
        output = {}; output.size = static_cast<uint8_t>(length);
        std::copy_n(value, length, output.bytes.begin());
        for (size_t i = 0; i < length; ++i) if (value[i] < 0x21 || value[i] > 0x7e) return false;
        return output.bytes[length] == '\0';
    }
    template<size_t N> bool fixed(std::array<uint8_t, N> & output) noexcept {
        const uint8_t * value = nullptr; size_t length = 0;
        if (!bytes(2, value, length) || length != N) return false;
        std::copy_n(value, length, output.begin()); return true;
    }
    template<size_t N> bool bounded(std::array<uint8_t, N> & output, size_t & length, size_t maximum) noexcept {
        const uint8_t * value = nullptr;
        if (!bytes(2, value, length) || length == 0 || length > maximum || length > N) return false;
        output.fill(0); std::copy_n(value, length, output.begin()); return true;
    }
    bool null_value() noexcept { return offset < size && data[offset++] == 0xf6; }
};

struct cbor_writer {
    uint8_t * data = nullptr; size_t capacity = 0, size = 0;
    bool raw(const void * value, size_t length) noexcept {
        if ((!value && length) || !data || length > capacity - size) return false;
        std::memcpy(data + size, value, length); size += length; return true;
    }
    bool head(uint8_t major, uint64_t value) noexcept {
        if (value < 24) { const uint8_t byte = static_cast<uint8_t>((major << 5) | value); return raw(&byte, 1); }
        const size_t width = value <= UINT8_MAX ? 1 : value <= UINT16_MAX ? 2 : value <= UINT32_MAX ? 4 : 8;
        uint8_t bytes[9] = { static_cast<uint8_t>((major << 5) | (width == 1 ? 24 : width == 2 ? 25 : width == 4 ? 26 : 27)) };
        for (size_t i = 0; i < width; ++i) bytes[width - i] = static_cast<uint8_t>(value >> (8 * i));
        return raw(bytes, width + 1);
    }
    bool unsigned_value(uint64_t value) noexcept { return head(0, value); }
    bool map(size_t length) noexcept { return head(5, length); }
    bool bytes(const void * value, size_t length) noexcept { return head(2, length) && raw(value, length); }
    bool text(const context_store_registered_id & value) noexcept { return head(3, value.size) && raw(value.bytes.data(), value.size); }
    bool null_value() noexcept { const uint8_t value = 0xf6; return raw(&value, 1); }
};

struct decoded_body_v1 {
    uint8_t kind = 0xff;
    std::array<uint8_t, 32> root_id {};
    context_store_format_digest path_policy {}, initial_head {}, selected_digest {}, attempt_id {}, operation_commitment {};
    context_store_format_digest predecessor_digest {}, successor_digest {}, previous_head {}, predecessor_head {}, prepare_digest {}, terminal_head {};
    std::array<uint8_t, 16> store_uuid {}, filesystem_uuid {};
    context_store_registered_id registry_id {}, body_key_id {};
    uint64_t registry_epoch = 0, mount_id = 0, owner_uid = 0, body_generation = 0, capacity = 0;
    uint64_t lock_st_dev = 0, lock_st_ino = 0, selected_length = 0, selected_high_water = 0, selector_generation = 0;
    uint64_t slot = 0, predecessor_length = 0, successor_length = 0, phase = 0, terminal_class = 0;
    uint8_t root_state = 0xff;
    std::array<uint8_t, 1024> predecessor {}, successor {};
};

template<context_store_registry_lab_kind Kind> class authenticated_record_v1;
template<context_store_registry_lab_kind Kind>
authenticated_record_v1<Kind> decode_record(const uint8_t *, size_t, const credential_owner &, size_t &) noexcept;

template<context_store_registry_lab_kind Kind> class authenticated_record_v1 {
public:
    static constexpr size_t capacity = Kind == context_store_registry_lab_kind::prepare ? 4096 : 1024;
    authenticated_record_v1(const authenticated_record_v1 &) = delete;
    authenticated_record_v1 & operator=(const authenticated_record_v1 &) = delete;
    authenticated_record_v1(authenticated_record_v1 &&) noexcept = default;
    authenticated_record_v1 & operator=(authenticated_record_v1 &&) noexcept = default;
    bool authenticated() const noexcept { return valid_; }
    const decoded_body_v1 & body() const noexcept { return body_; }
    const context_store_format_digest & content_digest() const noexcept { return content_digest_; }
    const uint8_t * exact_data() const noexcept { return exact_.data(); }
    size_t exact_size() const noexcept { return length_; }

private:
public:
    authenticated_record_v1() noexcept = default;
private:
    std::array<uint8_t, capacity> exact_ {};
    size_t length_ = 0;
    context_store_format_digest content_digest_ {};
    context_store_registered_id key_id_ {};
    uint64_t generation_ = 0;
    decoded_body_v1 body_ {};
    bool valid_ = false;
    template<context_store_registry_lab_kind K>
    friend authenticated_record_v1<K> decode_record(const uint8_t *, size_t, const credential_owner &, size_t &) noexcept;
};

bool parse_root(cbor_reader & reader, decoded_body_v1 & body) noexcept {
    uint64_t value = 0;
    if (!reader.map(18) || !reader.exact(0) || !reader.exact(1) || !reader.exact(1) || !reader.exact(0) ||
        !reader.exact(2) || !reader.exact(0) || !reader.exact(3) || !reader.fixed(body.root_id) ||
        !reader.exact(4) || !reader.fixed(body.store_uuid) || !reader.exact(5) || !reader.id(body.registry_id) ||
        !reader.exact(6) || !reader.unsigned_value(body.registry_epoch) || body.registry_epoch == 0 ||
        !reader.exact(7) || !reader.fixed(body.filesystem_uuid) || !reader.exact(8) || !reader.unsigned_value(body.mount_id) || body.mount_id == 0 ||
        !reader.exact(9) || !reader.unsigned_value(body.owner_uid) || !reader.exact(10) || !reader.id(body.body_key_id) ||
        !reader.exact(11) || !reader.unsigned_value(body.body_generation) || body.body_generation == 0 ||
        !reader.exact(12) || !reader.unsigned_value(body.capacity) || body.capacity != 512 ||
        !reader.exact(13) || !reader.digest(body.path_policy) || !reader.exact(14) || !reader.unsigned_value(value) || value > 1) return false;
    body.root_state = static_cast<uint8_t>(value);
    if (!reader.exact(15)) return false;
    if (body.root_state == 0) { if (!reader.null_value()) return false; }
    else if (!reader.digest(body.initial_head) || !nonzero(body.initial_head)) return false;
    return reader.exact(16) && reader.unsigned_value(body.lock_st_dev) && body.lock_st_dev != 0 &&
        reader.exact(17) && reader.unsigned_value(body.lock_st_ino) && body.lock_st_ino != 0;
}

bool parse_head(cbor_reader & reader, decoded_body_v1 & body) noexcept {
    return reader.map(13) && reader.exact(0) && reader.exact(1) && reader.exact(1) && reader.exact(0) &&
        reader.exact(2) && reader.exact(1) && reader.exact(3) && reader.fixed(body.root_id) &&
        reader.exact(4) && reader.digest(body.path_policy) && reader.exact(5) && reader.digest(body.selected_digest) &&
        reader.exact(6) && reader.unsigned_value(body.selected_length) && body.selected_length >= 1 && body.selected_length <= 1024 &&
        reader.exact(7) && reader.id(body.registry_id) && reader.exact(8) && reader.unsigned_value(body.registry_epoch) && body.registry_epoch != 0 &&
        reader.exact(9) && reader.unsigned_value(body.selected_high_water) && reader.exact(10) && reader.unsigned_value(body.selector_generation) && body.selector_generation != 0 &&
        reader.exact(11) && reader.id(body.body_key_id) && reader.exact(12) && reader.unsigned_value(body.body_generation) && body.body_generation != 0;
}

bool parse_prepare(cbor_reader & reader, decoded_body_v1 & body) noexcept {
    size_t predecessor_size = 0, successor_size = 0;
    return reader.map(17) && reader.exact(0) && reader.exact(1) && reader.exact(1) && reader.exact(0) &&
        reader.exact(2) && reader.exact(2) && reader.exact(3) && reader.fixed(body.root_id) &&
        reader.exact(4) && reader.digest(body.path_policy) && reader.exact(5) && reader.digest(body.attempt_id) && nonzero(body.attempt_id) &&
        reader.exact(6) && reader.unsigned_value(body.slot) && body.slot < 512 && reader.exact(7) && reader.digest(body.operation_commitment) &&
        reader.exact(8) && reader.unsigned_value(body.predecessor_length) && body.predecessor_length >= 1 && body.predecessor_length <= 1024 &&
        reader.exact(9) && reader.bounded(body.predecessor, predecessor_size, 1024) && predecessor_size == body.predecessor_length &&
        reader.exact(10) && reader.digest(body.predecessor_digest) && reader.exact(11) && reader.unsigned_value(body.successor_length) && body.successor_length >= 1 && body.successor_length <= 1024 &&
        reader.exact(12) && reader.bounded(body.successor, successor_size, 1024) && successor_size == body.successor_length &&
        reader.exact(13) && reader.digest(body.successor_digest) && reader.exact(14) && reader.unsigned_value(body.phase) && body.phase == 1 &&
        reader.exact(15) && reader.digest(body.previous_head) && reader.exact(16) && reader.digest(body.predecessor_head);
}

bool parse_terminal(cbor_reader & reader, decoded_body_v1 & body, uint8_t kind) noexcept {
    uint64_t parsed_kind = 0;
    if (!reader.map(14) || !reader.exact(0) || !reader.exact(1) || !reader.exact(1) || !reader.exact(0) ||
        !reader.exact(2) || !reader.unsigned_value(parsed_kind) || parsed_kind != kind || !reader.exact(3) || !reader.fixed(body.root_id) ||
        !reader.exact(4) || !reader.digest(body.path_policy) || !reader.exact(5) || !reader.digest(body.attempt_id) || !nonzero(body.attempt_id) ||
        !reader.exact(6) || !reader.unsigned_value(body.slot) || body.slot >= 512 || !reader.exact(7) || !reader.digest(body.operation_commitment) ||
        !reader.exact(8) || !reader.digest(body.predecessor_digest) || !reader.exact(9) || !reader.digest(body.successor_digest) ||
        !reader.exact(10) || !reader.digest(body.prepare_digest) || !reader.exact(11) || !reader.unsigned_value(body.phase) ||
        !reader.exact(12) || !reader.unsigned_value(body.terminal_class) || body.terminal_class > 1 ||
        !reader.exact(13) || !reader.digest(body.terminal_head)) return false;
    return body.phase == (kind == 3 ? 2U : 1U);
}

const char * auth_domain(context_store_registry_lab_kind kind) noexcept {
    switch (kind) {
        case context_store_registry_lab_kind::root: return "halofpx.registry-lab-root-auth.v1";
        case context_store_registry_lab_kind::head: return "halofpx.registry-lab-head-auth.v1";
        case context_store_registry_lab_kind::prepare: return "halofpx.registry-lab-prepare-auth.v1";
        case context_store_registry_lab_kind::close: return "halofpx.registry-lab-close-auth.v1";
        case context_store_registry_lab_kind::abort_record: return "halofpx.registry-lab-abort-auth.v1";
        case context_store_registry_lab_kind::quarantine: break;
    }
    return nullptr;
}

const char * content_domain(context_store_registry_lab_kind kind) noexcept {
    switch (kind) {
        case context_store_registry_lab_kind::root: return "halofpx.registry-lab-root.v1";
        case context_store_registry_lab_kind::head: return "halofpx.registry-lab-head.v1";
        case context_store_registry_lab_kind::prepare: return "halofpx.registry-lab-prepare.v1";
        case context_store_registry_lab_kind::close: return "halofpx.registry-lab-close.v1";
        case context_store_registry_lab_kind::abort_record: return "halofpx.registry-lab-abort.v1";
        case context_store_registry_lab_kind::quarantine: break;
    }
    return nullptr;
}

bool hash_with_domain(const char * domain, const uint8_t * data, size_t size, context_store_format_digest & output) noexcept {
    std::array<uint8_t, 4160> scratch {};
    const size_t domain_size = domain ? std::strlen(domain) + 1 : 0;
    if (!domain_size || size > scratch.size() - domain_size) return false;
    std::copy_n(reinterpret_cast<const uint8_t *>(domain), domain_size, scratch.begin());
    std::copy_n(data, size, scratch.begin() + domain_size);
    const bool ok = context_store_sha256_bounded(scratch.data(), domain_size + size, scratch.size(), output);
    wipe(scratch); return ok;
}

bool derive_lab_key(const credential_owner & credential, context_store_format_digest & output, size_t & derivations) noexcept {
    constexpr char domain[] = "halofpx.registry-lab-key.v1";
    std::array<uint8_t, 256> scratch {};
    cbor_writer writer { scratch.data(), scratch.size() };
    ++derivations;
    const bool ok = writer.raw(domain, sizeof(domain)) && writer.text(credential.key_id) && writer.unsigned_value(credential.generation) &&
        context_store_hmac_sha256(credential.secret.data(), credential.secret.size(), scratch.data(), writer.size, output);
    wipe(scratch); return ok;
}

template<context_store_registry_lab_kind Kind>
authenticated_record_v1<Kind> decode_record(const uint8_t * data, size_t size, const credential_owner & credential,
        size_t & derivations) noexcept {
    authenticated_record_v1<Kind> output;
    if (!data || size == 0 || size > output.exact_.size()) return output;
    std::copy_n(data, size, output.exact_.begin()); output.length_ = size;
    cbor_reader reader { output.exact_.data(), size };
    if (!reader.map(2) || !reader.exact(0)) return output;
    const size_t auth_start = reader.offset;
    if (!reader.map(4) || !reader.exact(0)) return output;
    output.body_.kind = static_cast<uint8_t>(Kind);
    bool body_ok = false;
    if constexpr (Kind == context_store_registry_lab_kind::root) body_ok = parse_root(reader, output.body_);
    if constexpr (Kind == context_store_registry_lab_kind::head) body_ok = parse_head(reader, output.body_);
    if constexpr (Kind == context_store_registry_lab_kind::prepare) body_ok = parse_prepare(reader, output.body_);
    if constexpr (Kind == context_store_registry_lab_kind::close) body_ok = parse_terminal(reader, output.body_, 3);
    if constexpr (Kind == context_store_registry_lab_kind::abort_record) body_ok = parse_terminal(reader, output.body_, 4);
    if (!body_ok || !reader.exact(1) || !reader.id(output.key_id_) || !reader.exact(2) || !reader.exact(1) ||
        !reader.exact(3) || !reader.unsigned_value(output.generation_) || output.generation_ == 0) return output;
    const size_t auth_end = reader.offset;
    if (!reader.exact(1)) return output;
    const uint8_t * supplied_tag = nullptr; size_t supplied_tag_size = 0;
    if (!reader.bytes(2, supplied_tag, supplied_tag_size) || supplied_tag_size != 32 || reader.offset != size) return output;
    if (!same_id(output.key_id_, credential.key_id) || output.generation_ != credential.generation) return output;
    if constexpr (Kind == context_store_registry_lab_kind::root || Kind == context_store_registry_lab_kind::head) {
        if (!same_id(output.body_.body_key_id, output.key_id_) || output.body_.body_generation != output.generation_) return output;
    }
    context_store_format_digest derived {}, expected {};
    std::array<uint8_t, 4160> authentication_input {};
    const char * domain = auth_domain(Kind); const size_t domain_size = domain ? std::strlen(domain) + 1 : 0;
    const size_t auth_size = auth_end - auth_start;
    bool ok = derive_lab_key(credential, derived, derivations) && domain_size && auth_size <= authentication_input.size() - domain_size;
    if (ok) {
        std::copy_n(reinterpret_cast<const uint8_t *>(domain), domain_size, authentication_input.begin());
        std::copy_n(output.exact_.data() + auth_start, auth_size, authentication_input.begin() + domain_size);
        ok = context_store_hmac_sha256(derived.data(), derived.size(), authentication_input.data(), domain_size + auth_size, expected) &&
            halofpx_registry_lab_constant_time_tag_equal_32(expected.data(), supplied_tag) &&
            hash_with_domain(content_domain(Kind), output.exact_.data(), size, output.content_digest_);
    }
    wipe(derived); wipe(expected); wipe(authentication_input);
    output.valid_ = ok; return output;
}

template<size_t N> bool bounded_snapshot_file(const modeled_file<N> & file) noexcept {
    return file.live_length <= N && file.durable_length <= N;
}

bool bounded_snapshot(const fixed_state & state) noexcept {
#define BOUNDED1024(NAME) if (!bounded_snapshot_file(state.NAME)) return false
    BOUNDED1024(marker); BOUNDED1024(lock_file); BOUNDED1024(head); BOUNDED1024(quarantine); BOUNDED1024(quarantine_staging);
#undef BOUNDED1024
    for (const modeled_slot & slot : state.slots) if (!bounded_snapshot_file(slot.prepare) || !bounded_snapshot_file(slot.close) ||
        !bounded_snapshot_file(slot.abort_record) || !bounded_snapshot_file(slot.successor_staging) || !bounded_snapshot_file(slot.selector_staging)) return false;
    if (!bounded_snapshot_file(state.initial_envelope.object)) return false;
    for (const modeled_envelope & envelope : state.successors) if (!bounded_snapshot_file(envelope.object)) return false;
    for (const modeled_unexpected_entry & entry : state.unexpected) if (entry.live_length > entry.live_name.size() || entry.durable_length > entry.durable_name.size()) return false;
    return true;
}

template<size_t N> bool absent_file(const modeled_file<N> & file) noexcept {
    return !file.live_present && !file.durable_present && !file.live_complete && !file.durable_complete && file.live_length == 0 && file.durable_length == 0;
}

template<size_t N> bool exact_published_file(const modeled_file<N> & file) noexcept {
    return file.live_present && file.durable_present && file.live_complete && file.durable_complete && file.live_length == file.durable_length &&
        file.live_length <= N && exact_bytes(file.live_bytes.data(), file.durable_bytes.data(), file.live_length);
}

template<size_t N> bool exact_live_file(const modeled_file<N> & file) noexcept {
    return file.live_present && file.live_complete && file.live_length <= N;
}

template<size_t N> bool exact_durable_copy(const modeled_file<N> & file) noexcept {
    return exact_live_file(file) && file.durable_present && file.durable_complete && file.live_length == file.durable_length &&
        exact_bytes(file.live_bytes.data(), file.durable_bytes.data(), file.live_length);
}

template<size_t N> bool any_name(const modeled_file<N> & file) noexcept { return file.live_present || file.durable_present; }

bool valid_registered_id(const context_store_registered_id & id) noexcept {
    if (!id.size || id.size > context_store_registered_id_max_bytes) return false;
    for (size_t i = 0; i < id.size; ++i) {
        const uint8_t byte = static_cast<uint8_t>(id.bytes[i]);
        if (byte < 0x21 || byte > 0x7e) return false;
    }
    for (size_t i = id.size; i < id.bytes.size(); ++i) if (id.bytes[i] != '\0') return false;
    return true;
}

bool valid_preflight(const preflight_context_v1 & value, const credential_owner & credential) noexcept {
    return nonzero(value.store_uuid) && nonzero(value.filesystem_uuid) && nonzero(value.subvolume_uuid) && value.mount_id && value.st_dev &&
        value.root_mode == 448 && value.authority_file_mode == 384 && value.lock_st_dev && value.lock_st_ino && nonzero(value.path_policy_commitment) &&
        valid_registered_id(value.registry_id) && valid_registered_id(value.credential_key_id) && value.registry_epoch &&
        nonzero(value.authority_base_scope_commitment) && nonzero(value.registry_policy_commitment) &&
        value.inner_key_disposition == context_store_key_disposition::active && same_id(value.credential_key_id, credential.key_id) &&
        value.credential_generation == credential.generation && value.attempt_capacity == 512 && value.maximum_logical_authority_bytes == 16777216;
}

bool valid_request(const request_transition_v1 & value) noexcept {
    const auto zero_tail = [](const auto & bytes, size_t used) noexcept {
        if (used > bytes.size()) return false;
        for (size_t i = used; i < bytes.size(); ++i) if (bytes[i] != 0) return false;
        return true;
    };
    return nonzero(value.attempt_id) && nonzero(value.operation_commitment) && value.requested_slot < 512 &&
        value.predecessor_length >= 1 && value.predecessor_length <= 1024 && value.successor_length >= 1 && value.successor_length <= 1024 &&
        value.expected_current_head_length >= 1 && value.expected_current_head_length <= 1024 && nonzero(value.predecessor_digest) &&
        nonzero(value.successor_digest) && nonzero(value.expected_current_head_digest) &&
        zero_tail(value.predecessor, value.predecessor_length) && zero_tail(value.successor, value.successor_length) &&
        zero_tail(value.expected_current_head, value.expected_current_head_length);
}

bool same_scope(const decoded_body_v1 & body, const decoded_body_v1 & root) noexcept {
    return body.root_id == root.root_id && body.path_policy == root.path_policy;
}

bool bind_root(const decoded_body_v1 & root, const preflight_context_v1 & preflight, const credential_owner & credential) noexcept {
    return root.store_uuid == preflight.store_uuid && root.filesystem_uuid == preflight.filesystem_uuid && root.mount_id == preflight.mount_id &&
        root.owner_uid == preflight.owner_uid && root.lock_st_dev == preflight.lock_st_dev && root.lock_st_ino == preflight.lock_st_ino &&
        root.path_policy == preflight.path_policy_commitment && same_id(root.registry_id, preflight.registry_id) && root.registry_epoch == preflight.registry_epoch &&
        same_id(root.body_key_id, credential.key_id) && root.body_generation == credential.generation && root.capacity == 512 && nonzero(root.root_id);
}

bool registry_envelope_digest(const uint8_t * data, size_t size, context_store_format_digest & output) noexcept {
    return hash_with_domain("halofpx.registry-lab-registry-envelope.v1", data, size, output);
}

bool operation_commitment(const context_store_format_digest & root_id, const context_store_format_digest & path,
        const context_store_format_digest & attempt, uint64_t slot, const context_store_format_digest & predecessor,
        const context_store_format_digest & successor, size_t predecessor_size, size_t successor_size,
        context_store_format_digest & output) noexcept {
    std::array<uint8_t, 320> encoded {};
    cbor_writer writer { encoded.data(), encoded.size() };
    const bool written = writer.map(8) && writer.unsigned_value(0) && writer.bytes(root_id.data(), root_id.size()) &&
        writer.unsigned_value(1) && writer.bytes(path.data(), path.size()) && writer.unsigned_value(2) && writer.bytes(attempt.data(), attempt.size()) &&
        writer.unsigned_value(3) && writer.unsigned_value(slot) && writer.unsigned_value(4) && writer.bytes(predecessor.data(), predecessor.size()) &&
        writer.unsigned_value(5) && writer.bytes(successor.data(), successor.size()) && writer.unsigned_value(6) && writer.unsigned_value(predecessor_size) &&
        writer.unsigned_value(7) && writer.unsigned_value(successor_size);
    const bool ok = written && hash_with_domain("halofpx.registry-lab-operation.v1", encoded.data(), writer.size, output);
    wipe(encoded); return ok;
}

bool bind_predecessor(const uint8_t * data, size_t size, const preflight_context_v1 & preflight, const credential_owner & credential,
        context_store_authenticated_protected_registry * carrier = nullptr) noexcept {
    context_store_protected_registry_key_record key { preflight.inner_key_disposition, credential.key_id, credential.generation,
        { credential.secret.data(), credential.secret.size() } };
    auto result = context_store_verify_protected_registry_v1(data, size, key);
    const auto * authenticated = result.authenticated_carrier();
    if (!authenticated || !authenticated->body() || !authenticated->key_id() || !authenticated->authority_binding() || !authenticated->key_continuity_commitment()) return false;
    const auto & body = *authenticated->body();
    const bool ok = same_id(body.registry_id, preflight.registry_id) && body.registry_epoch == preflight.registry_epoch &&
        body.authority_base_scope_commitment == preflight.authority_base_scope_commitment && body.policy_commitment == preflight.registry_policy_commitment &&
        same_id(*authenticated->key_id(), credential.key_id) && authenticated->key_generation() == credential.generation;
    if (ok && carrier) *carrier = *authenticated;
    return ok;
}

bool bind_successor(const uint8_t * data, size_t size, const preflight_context_v1 & preflight, const credential_owner & credential,
        const context_store_authenticated_protected_registry & predecessor,
        context_store_authenticated_protected_registry_successor * carrier = nullptr) noexcept {
    context_store_protected_registry_key_record key { preflight.inner_key_disposition, credential.key_id, credential.generation,
        { credential.secret.data(), credential.secret.size() } };
    auto result = context_store_verify_protected_registry_successor_v1(data, size, key);
    const auto * authenticated = result.authenticated_carrier();
    if (!authenticated || !authenticated->body() || !authenticated->key_id() || !authenticated->authority_binding() ||
        !authenticated->key_continuity_commitment() || !predecessor.body() || !predecessor.envelope_digest() ||
        !predecessor.authority_binding() || !predecessor.key_continuity_commitment()) return false;
    const auto & body = *authenticated->body(); const auto & before = *predecessor.body();
    const auto & receipt = body.receipt;
    const bool receipt_ok = receipt.authorization_sequence == body.consumed_authorization_high_water && nonzero(receipt.command_id) &&
        nonzero(receipt.authorization_token_digest) && nonzero(receipt.plan_commitment) && nonzero(receipt.selected_manifest_digest) &&
        nonzero(receipt.proposed_anchor_envelope_digest);
    const bool ok = same_id(body.registry_id, before.registry_id) && body.registry_epoch == before.registry_epoch &&
        body.authority_base_scope_commitment == before.authority_base_scope_commitment && body.policy_commitment == before.policy_commitment &&
        before.last_consumed_sequence != UINT64_MAX && body.consumed_authorization_high_water == before.last_consumed_sequence + 1 &&
        body.predecessor_snapshot_envelope_digest == *predecessor.envelope_digest() &&
        *authenticated->key_continuity_commitment() == *predecessor.key_continuity_commitment() && receipt_ok &&
        same_id(*authenticated->key_id(), credential.key_id) && authenticated->key_generation() == credential.generation;
    if (ok && carrier) *carrier = *authenticated;
    return ok;
}

bool bind_prepare_body(const decoded_body_v1 & body, const decoded_body_v1 & root, const preflight_context_v1 & preflight,
        const credential_owner & credential, const uint8_t * initial, size_t initial_size, const context_store_format_digest & initial_digest) noexcept {
    if (!same_scope(body, root) || body.previous_head != root.initial_head || body.predecessor_head != root.initial_head ||
        body.predecessor_digest != initial_digest || body.predecessor_length != initial_size ||
        !exact_bytes(body.predecessor.data(), initial, initial_size)) return false;
    context_store_format_digest predecessor_digest {}, successor_digest {}, commitment {};
    context_store_authenticated_protected_registry predecessor;
    const bool ok = registry_envelope_digest(body.predecessor.data(), body.predecessor_length, predecessor_digest) &&
        registry_envelope_digest(body.successor.data(), body.successor_length, successor_digest) && predecessor_digest == body.predecessor_digest &&
        successor_digest == body.successor_digest && bind_predecessor(body.predecessor.data(), body.predecessor_length, preflight, credential, &predecessor) &&
        bind_successor(body.successor.data(), body.successor_length, preflight, credential, predecessor) &&
        operation_commitment(root.root_id, root.path_policy, body.attempt_id, body.slot, body.predecessor_digest, body.successor_digest,
            body.predecessor_length, body.successor_length, commitment) && commitment == body.operation_commitment;
    return ok;
}

bool reason_admits_shape(quarantine_reason reason, quarantine_shape shape) noexcept {
    switch (reason) {
        case quarantine_reason::layout_or_unexpected:
        case quarantine_reason::chain_contradiction:
        case quarantine_reason::staging_ambiguous:
        case quarantine_reason::durability_unproved: return shape != quarantine_shape::none;
        case quarantine_reason::head_invalid: return shape == quarantine_shape::u0;
        case quarantine_reason::selected_envelope_invalid: return shape == quarantine_shape::uh || shape == quarantine_shape::successor;
        case quarantine_reason::journal_invalid:
        case quarantine_reason::multiple_unresolved: return shape == quarantine_shape::u0 || shape == quarantine_shape::uh;
        case quarantine_reason::referent_invalid: return shape == quarantine_shape::uh || shape == quarantine_shape::prepare;
        default: return false;
    }
}

bool derive_quarantine_diagnosis_commitment(uint64_t invocation_id, const quarantine_diagnosis_view & diagnosis,
        context_store_format_digest & output) noexcept {
    output = {};
    if (!invocation_id || !diagnosis.valid || !diagnosis.publishable || !diagnosis.authenticated_initialized_root ||
        !nonzero(diagnosis.root_id) || !nonzero(diagnosis.path_policy_commitment) ||
        !valid_registered_id(diagnosis.registry_id) || !diagnosis.registry_epoch ||
        !reason_admits_shape(diagnosis.reason, diagnosis.shape)) return false;
    const bool attributable = diagnosis.shape == quarantine_shape::prepare || diagnosis.shape == quarantine_shape::successor;
    const bool head = diagnosis.shape != quarantine_shape::u0;
    const uint8_t phase = diagnosis.shape == quarantine_shape::successor ? 2 : diagnosis.shape == quarantine_shape::prepare ? 1 : 0;
    if (diagnosis.attributable != attributable || diagnosis.has_previous_record != attributable || diagnosis.has_head != head ||
        diagnosis.phase != phase || (attributable && (diagnosis.slot >= 512 || !nonzero(diagnosis.attempt_id) ||
            !nonzero(diagnosis.operation_commitment) || !nonzero(diagnosis.previous_record_digest))) ||
        (!attributable && (diagnosis.slot != 0 || nonzero(diagnosis.attempt_id) || nonzero(diagnosis.operation_commitment) ||
            nonzero(diagnosis.previous_record_digest))) || (head != nonzero(diagnosis.head_digest))) return false;
    std::array<uint8_t, 384> encoded {};
    cbor_writer writer { encoded.data(), encoded.size() };
    const auto optional_digest = [&](bool present, const context_store_format_digest & digest) noexcept {
        return present ? writer.bytes(digest.data(), digest.size()) : writer.null_value();
    };
    const bool written = writer.map(12) && writer.unsigned_value(0) && writer.unsigned_value(1) &&
        writer.unsigned_value(1) && writer.unsigned_value(invocation_id) &&
        writer.unsigned_value(2) && writer.bytes(diagnosis.root_id.data(), diagnosis.root_id.size()) &&
        writer.unsigned_value(3) && writer.bytes(diagnosis.path_policy_commitment.data(), diagnosis.path_policy_commitment.size()) &&
        writer.unsigned_value(4) && writer.unsigned_value(static_cast<uint8_t>(diagnosis.reason)) &&
        writer.unsigned_value(5) && writer.unsigned_value(diagnosis.phase) &&
        writer.unsigned_value(6) && writer.unsigned_value(static_cast<uint8_t>(diagnosis.shape)) &&
        writer.unsigned_value(7) && optional_digest(attributable, diagnosis.attempt_id) &&
        writer.unsigned_value(8) && (attributable ? writer.unsigned_value(diagnosis.slot) : writer.null_value()) &&
        writer.unsigned_value(9) && optional_digest(attributable, diagnosis.operation_commitment) &&
        writer.unsigned_value(10) && optional_digest(attributable, diagnosis.previous_record_digest) &&
        writer.unsigned_value(11) && optional_digest(head, diagnosis.head_digest);
    const bool ok = written && hash_with_domain("halofpx.registry-lab-quarantine-diagnosis.v1", encoded.data(), writer.size, output);
    wipe(encoded); if (!ok) output = {}; return ok;
}

recovery_classification classify_operation_5(const fixed_state & snapshot, const preflight_context_v1 & preflight,
        const request_transition_v1 & request, const credential_owner & credential, size_t & scanned, size_t & derivations,
        uint64_t invocation_id, quarantine_diagnosis_view & diagnosis) noexcept {
    diagnosis = {};
    std::array<bool, 16> diagnosis_flags {};
    const auto fault = [&](quarantine_reason reason) noexcept { diagnosis_flags[static_cast<size_t>(reason)] = true; };
    if (any_name(snapshot.quarantine) || any_name(snapshot.quarantine_staging)) {
        diagnosis.valid = true; diagnosis.reason = quarantine_reason::existing_quarantine;
        return select_recovery_precedence({ true });
    }

    bool sticky = false;
    auto root = decode_record<context_store_registry_lab_kind::root>(
        snapshot.marker.live_bytes.data(), snapshot.marker.live_length, credential, derivations);
    const bool root_live_exact = exact_live_file(snapshot.marker);
    const bool root_authenticated = root_live_exact && root.authenticated();
    const bool root_compatible = root_authenticated && bind_root(root.body(), preflight, credential);
    const bool root_valid = root_compatible && exact_durable_copy(snapshot.marker);
    if (!root_valid) {
        sticky = true;
        if (!root_live_exact) fault(quarantine_reason::marker_invalid);
        else if (!root.authenticated()) fault(quarantine_reason::key_or_auth_mismatch);
        else if (!root_compatible) fault(quarantine_reason::scope_or_root_mismatch);
        else fault(quarantine_reason::durability_unproved);
    }
    if (root_valid && root.body().root_state == 0) {
        recovery_precedence_flags flags; flags.initializing = true; return select_recovery_precedence(flags);
    }

    sticky = sticky || !snapshot.root_directory.live_projection || !snapshot.root_directory.durable_projection ||
        !snapshot.attempts_directory.live_projection || !snapshot.attempts_directory.durable_projection ||
        !snapshot.staging_directory.live_projection || !snapshot.staging_directory.durable_projection ||
        !snapshot.envelopes_directory.live_projection || !snapshot.envelopes_directory.durable_projection;
    for (const modeled_directory * directory : { &snapshot.root_directory, &snapshot.attempts_directory,
            &snapshot.staging_directory, &snapshot.envelopes_directory }) {
        if (!directory->live_projection) fault(quarantine_reason::layout_or_unexpected);
        else if (!directory->durable_projection) fault(quarantine_reason::durability_unproved);
    }
    sticky = sticky || !absent_file(snapshot.quarantine) || !absent_file(snapshot.quarantine_staging) ||
        !exact_published_file(snapshot.lock_file) || snapshot.lock_file.live_length != 0;
    if (!absent_file(snapshot.quarantine) || !absent_file(snapshot.quarantine_staging)) fault(quarantine_reason::layout_or_unexpected);
    const bool lock_live_exact = exact_live_file(snapshot.lock_file) && snapshot.lock_file.live_length == 0;
    if (!lock_live_exact) fault(quarantine_reason::layout_or_unexpected);
    else if (!exact_durable_copy(snapshot.lock_file)) fault(quarantine_reason::durability_unproved);
    sticky = sticky || !exact_published_file(snapshot.head);
    for (const modeled_unexpected_entry & entry : snapshot.unexpected) {
        sticky = sticky || entry.live_occupied || entry.durable_occupied || entry.live_length || entry.durable_length;
        bool unexpected = entry.live_occupied || entry.durable_occupied || entry.live_length || entry.durable_length;
        for (uint8_t byte : entry.live_name) { sticky = sticky || byte != 0; unexpected = unexpected || byte != 0; }
        for (uint8_t byte : entry.durable_name) { sticky = sticky || byte != 0; unexpected = unexpected || byte != 0; }
        if (unexpected) fault(quarantine_reason::layout_or_unexpected);
    }

    auto head = decode_record<context_store_registry_lab_kind::head>(
        snapshot.head.live_bytes.data(), snapshot.head.live_length, credential, derivations);
    const bool head_live_bound = exact_live_file(snapshot.head) && head.authenticated() && root_compatible &&
        same_scope(head.body(), root.body()) && same_id(head.body().registry_id, preflight.registry_id) && head.body().registry_epoch == preflight.registry_epoch;
    const bool head_valid = head_live_bound && exact_durable_copy(snapshot.head);
    if (!head_valid) {
        sticky = true;
        if (!head_live_bound) fault(quarantine_reason::head_invalid);
        else fault(quarantine_reason::durability_unproved);
    }

    const uint8_t * initial_bytes = nullptr; size_t initial_size = 0;
    context_store_format_digest initial_digest {};
    context_store_authenticated_protected_registry initial_carrier;
    std::array<context_store_format_digest, 513> seen_envelopes {}; size_t seen_envelope_count = 0;
    if (!snapshot.initial_envelope.live_occupied || !snapshot.initial_envelope.durable_occupied ||
        snapshot.initial_envelope.live_digest != snapshot.initial_envelope.durable_digest || !exact_published_file(snapshot.initial_envelope.object)) sticky = true;
    bool initial_valid = false;
    const bool initial_live_candidate = snapshot.initial_envelope.live_occupied && exact_live_file(snapshot.initial_envelope.object);
    if (initial_live_candidate) {
        initial_bytes = snapshot.initial_envelope.object.live_bytes.data(); initial_size = snapshot.initial_envelope.object.live_length;
        if (!registry_envelope_digest(initial_bytes, initial_size, initial_digest) || initial_digest != snapshot.initial_envelope.live_digest ||
            !bind_predecessor(initial_bytes, initial_size, preflight, credential, &initial_carrier)) {
            sticky = true; fault(quarantine_reason::selected_envelope_invalid);
        }
        else { initial_valid = true; seen_envelopes[seen_envelope_count++] = initial_digest; }
    } else {
        fault(quarantine_reason::selected_envelope_invalid);
    }
    if (initial_valid && (!snapshot.initial_envelope.durable_occupied ||
            snapshot.initial_envelope.live_digest != snapshot.initial_envelope.durable_digest ||
            !exact_durable_copy(snapshot.initial_envelope.object))) fault(quarantine_reason::durability_unproved);
    if (!initial_valid) sticky = true;

    const auto envelope_selected_by_head = [&](const modeled_envelope & envelope) noexcept {
        return head_live_bound && (head.body().selected_digest == envelope.live_digest || head.body().selected_digest == envelope.durable_digest);
    };

    const uint8_t * selected_bytes = nullptr; size_t selected_size = 0; uint64_t selected_high_water = 0;
    bool request_successor_exists = false;
    for (const modeled_envelope & envelope : snapshot.successors) {
        if (!envelope.live_occupied && !envelope.durable_occupied && absent_file(envelope.object) && digest_zero(envelope.live_digest) && digest_zero(envelope.durable_digest)) continue;
        if (!envelope.live_occupied || !exact_live_file(envelope.object)) {
            sticky = true; fault(envelope_selected_by_head(envelope) ? quarantine_reason::selected_envelope_invalid : quarantine_reason::layout_or_unexpected); continue;
        }
        context_store_format_digest digest {};
        if (!registry_envelope_digest(envelope.object.live_bytes.data(), envelope.object.live_length, digest) || digest != envelope.live_digest) {
            sticky = true; fault(envelope_selected_by_head(envelope) ? quarantine_reason::selected_envelope_invalid : quarantine_reason::layout_or_unexpected); continue;
        }
        for (size_t i = 0; i < seen_envelope_count; ++i) if (seen_envelopes[i] == digest) { sticky = true; fault(quarantine_reason::chain_contradiction); }
        if (seen_envelope_count < seen_envelopes.size()) seen_envelopes[seen_envelope_count++] = digest;
        else { sticky = true; fault(quarantine_reason::internal_invariant); }
        context_store_authenticated_protected_registry_successor successor_carrier;
        if (!bind_successor(envelope.object.live_bytes.data(), envelope.object.live_length, preflight, credential, initial_carrier, &successor_carrier)) {
            sticky = true; fault(envelope_selected_by_head(envelope) ? quarantine_reason::selected_envelope_invalid : quarantine_reason::chain_contradiction);
        }
        if (!envelope.durable_occupied || envelope.live_digest != envelope.durable_digest || !exact_durable_copy(envelope.object)) {
            sticky = true; fault(quarantine_reason::durability_unproved);
        }
        if (head_live_bound && digest == head.body().selected_digest && successor_carrier.authenticated()) {
            selected_bytes = envelope.object.live_bytes.data(); selected_size = envelope.object.live_length;
            selected_high_water = successor_carrier.body()->consumed_authorization_high_water;
        }
        if (digest == request.successor_digest && envelope.object.live_length == request.successor_length &&
            exact_bytes(envelope.object.live_bytes.data(), request.successor.data(), request.successor_length)) request_successor_exists = true;
    }
    if (head_live_bound && initial_valid && initial_digest == head.body().selected_digest) {
        selected_bytes = initial_bytes; selected_size = initial_size; selected_high_water = initial_carrier.body()->last_consumed_sequence;
    }
    if (head_valid && (!selected_bytes || selected_size != head.body().selected_length || selected_high_water != head.body().selected_high_water)) sticky = true;
    const bool head_evidence_valid = head_live_bound && selected_bytes && selected_size == head.body().selected_length &&
        selected_high_water == head.body().selected_high_water;
    const bool predecessor_head_evidence = head_evidence_valid && initial_valid &&
        head.content_digest() == root.body().initial_head && head.body().selected_digest == initial_digest &&
        selected_size == initial_size && exact_bytes(selected_bytes, initial_bytes, initial_size);
    if (head_live_bound && !head_evidence_valid) fault(quarantine_reason::selected_envelope_invalid);

    std::array<context_store_format_digest, 512> attempts {}; size_t attempt_count = 0, occupied_count = 0, close_count = 0, unresolved_count = 0;
    size_t valid_prepare_count = 0;
    decoded_body_v1 attributable_prepare {}; context_store_format_digest attributable_prepare_digest {};
    decoded_body_v1 unresolved_body {};
    decoded_body_v1 close_prepare {}; context_store_format_digest close_head {};
    for (size_t index = 0; index < snapshot.slots.size(); ++index) {
        ++scanned;
        const modeled_slot & slot = snapshot.slots[index];
        if (!absent_file(slot.successor_staging) || !absent_file(slot.selector_staging)) { sticky = true; fault(quarantine_reason::staging_ambiguous); }
        const bool prepare_present = any_name(slot.prepare), close_present = any_name(slot.close), abort_present = any_name(slot.abort_record);
        if (!prepare_present && !absent_file(slot.prepare)) { sticky = true; fault(quarantine_reason::journal_invalid); }
        if (!close_present && !absent_file(slot.close)) { sticky = true; fault(quarantine_reason::journal_invalid); }
        if (!abort_present && !absent_file(slot.abort_record)) { sticky = true; fault(quarantine_reason::journal_invalid); }
        if (!prepare_present && !close_present && !abort_present) continue;
        ++occupied_count;
        if (!prepare_present) { sticky = true; fault(quarantine_reason::journal_invalid); }
        if (close_present && abort_present) { sticky = true; fault(quarantine_reason::chain_contradiction); }
        auto prepare = decode_record<context_store_registry_lab_kind::prepare>(
            slot.prepare.live_bytes.data(), slot.prepare.live_length, credential, derivations);
        const bool prepare_live_authenticated = prepare_present && exact_live_file(slot.prepare) && prepare.authenticated();
        const bool prepare_authenticated = prepare_live_authenticated && exact_durable_copy(slot.prepare);
        if (prepare_present && !prepare_live_authenticated) fault(quarantine_reason::journal_invalid);
        if (prepare_live_authenticated && !prepare_authenticated) fault(quarantine_reason::durability_unproved);
        const bool prepare_local = prepare_live_authenticated && root_compatible && same_scope(prepare.body(), root.body()) && prepare.body().slot == index;
        const bool prepare_valid = prepare_local && initial_valid &&
            bind_prepare_body(prepare.body(), root.body(), preflight, credential, initial_bytes, initial_size, initial_digest);
        if (!prepare_valid) {
            sticky = true;
            fault(prepare_live_authenticated && prepare_local ? quarantine_reason::chain_contradiction : quarantine_reason::journal_invalid);
        } else {
            ++valid_prepare_count;
            if (valid_prepare_count == 1) { attributable_prepare = prepare.body(); attributable_prepare_digest = prepare.content_digest(); }
        }
        if (prepare_live_authenticated) {
            for (size_t i = 0; i < attempt_count; ++i) if (attempts[i] == prepare.body().attempt_id) { sticky = true; fault(quarantine_reason::chain_contradiction); }
            attempts[attempt_count++] = prepare.body().attempt_id;
        }
        if (!close_present && !abort_present) {
            if (prepare_valid) { ++unresolved_count; if (unresolved_count == 1) unresolved_body = prepare.body(); }
            continue;
        }
        if (close_present) {
            auto terminal = decode_record<context_store_registry_lab_kind::close>(
                slot.close.live_bytes.data(), slot.close.live_length, credential, derivations);
            const bool terminal_live_authenticated = exact_live_file(slot.close) && terminal.authenticated();
            const bool terminal_authenticated = terminal_live_authenticated && exact_durable_copy(slot.close);
            if (!terminal_live_authenticated) fault(quarantine_reason::journal_invalid);
            if (terminal_live_authenticated && !terminal_authenticated) fault(quarantine_reason::durability_unproved);
            const bool terminal_local = terminal_live_authenticated && root_compatible && same_scope(terminal.body(), root.body()) && terminal.body().slot == index;
            if (!terminal_live_authenticated || !prepare_valid || !terminal_local ||
                terminal.body().attempt_id != prepare.body().attempt_id || terminal.body().operation_commitment != prepare.body().operation_commitment ||
                terminal.body().predecessor_digest != prepare.body().predecessor_digest || terminal.body().successor_digest != prepare.body().successor_digest ||
                terminal.body().prepare_digest != prepare.content_digest()) {
                sticky = true; fault(terminal_live_authenticated && terminal_local && prepare_valid ? quarantine_reason::chain_contradiction : quarantine_reason::journal_invalid);
            }
            else { ++close_count; close_prepare = prepare.body(); close_head = terminal.body().terminal_head; }
        }
        if (abort_present) {
            auto terminal = decode_record<context_store_registry_lab_kind::abort_record>(
                slot.abort_record.live_bytes.data(), slot.abort_record.live_length, credential, derivations);
            const bool terminal_live_authenticated = exact_live_file(slot.abort_record) && terminal.authenticated();
            const bool terminal_authenticated = terminal_live_authenticated && exact_durable_copy(slot.abort_record);
            if (!terminal_live_authenticated) fault(quarantine_reason::journal_invalid);
            if (terminal_live_authenticated && !terminal_authenticated) fault(quarantine_reason::durability_unproved);
            const bool terminal_local = terminal_live_authenticated && root_compatible && same_scope(terminal.body(), root.body()) && terminal.body().slot == index;
            if (!terminal_live_authenticated || !prepare_valid || !terminal_local ||
                terminal.body().attempt_id != prepare.body().attempt_id || terminal.body().operation_commitment != prepare.body().operation_commitment ||
                terminal.body().predecessor_digest != prepare.body().predecessor_digest || terminal.body().successor_digest != prepare.body().successor_digest ||
                terminal.body().prepare_digest != prepare.content_digest() || terminal.body().terminal_head != root.body().initial_head) {
                sticky = true; fault(terminal_live_authenticated && terminal_local && prepare_valid ? quarantine_reason::chain_contradiction : quarantine_reason::journal_invalid);
            }
        }
    }

    if (close_count > 1) { sticky = true; fault(quarantine_reason::chain_contradiction); }
    if (unresolved_count > 1) { sticky = true; fault(quarantine_reason::multiple_unresolved); }
    if (!sticky && unresolved_count == 0) {
        if (close_count == 0) {
            if (head.content_digest() != root.body().initial_head || head.body().selected_digest != initial_digest || selected_size != initial_size ||
                !exact_bytes(selected_bytes, initial_bytes, initial_size)) { sticky = true; fault(quarantine_reason::chain_contradiction); }
        } else {
            if (head.content_digest() != close_head || head.body().selected_digest != close_prepare.successor_digest || selected_size != close_prepare.successor_length ||
                !exact_bytes(selected_bytes, close_prepare.successor.data(), selected_size)) { sticky = true; fault(quarantine_reason::chain_contradiction); }
        }
    }
    if (close_count && unresolved_count) { sticky = true; fault(quarantine_reason::chain_contradiction); }
    if (sticky) {
        diagnosis.valid = true;
        diagnosis.authenticated_initialized_root = root_valid && root.body().root_state == 1;
        if (diagnosis.authenticated_initialized_root) {
            diagnosis.root_id = root.body().root_id;
            diagnosis.path_policy_commitment = root.body().path_policy;
            diagnosis.registry_id = root.body().registry_id;
            diagnosis.registry_epoch = root.body().registry_epoch;
        }
        diagnosis.reason = select_quarantine_reason_for_test(diagnosis_flags);
        if (diagnosis.authenticated_initialized_root) {
            if (valid_prepare_count == 1) {
                const bool predecessor_head = head_evidence_valid &&
                    head.content_digest() == attributable_prepare.previous_head &&
                    attributable_prepare.previous_head == attributable_prepare.predecessor_head &&
                    selected_size == attributable_prepare.predecessor_length &&
                    exact_bytes(selected_bytes, attributable_prepare.predecessor.data(), selected_size);
                if (predecessor_head) {
                    diagnosis.shape = quarantine_shape::prepare;
                    diagnosis.attributable = true; diagnosis.has_previous_record = true; diagnosis.has_head = true;
                    diagnosis.phase = 1; diagnosis.slot = attributable_prepare.slot;
                    diagnosis.attempt_id = attributable_prepare.attempt_id;
                    diagnosis.operation_commitment = attributable_prepare.operation_commitment;
                    diagnosis.previous_record_digest = attributable_prepare_digest;
                    diagnosis.head_digest = head.content_digest();
                }
            } else if (predecessor_head_evidence) {
                diagnosis.shape = quarantine_shape::uh;
                diagnosis.has_head = true; diagnosis.head_digest = head.content_digest();
            } else if (!head_evidence_valid) {
                diagnosis.shape = quarantine_shape::u0;
            }
        }
        diagnosis.publishable = diagnosis.authenticated_initialized_root && diagnosis.shape != quarantine_shape::none &&
            reason_admits_shape(diagnosis.reason, diagnosis.shape);
        if (diagnosis.publishable && !derive_quarantine_diagnosis_commitment(invocation_id, diagnosis, diagnosis.diagnosis_commitment))
            diagnosis.publishable = false;
        recovery_precedence_flags flags; flags.sticky = true; return select_recovery_precedence(flags);
    }
    if (unresolved_count == 1) {
        recovery_precedence_flags flags;
        flags.successor_close = selected_size == unresolved_body.successor_length && exact_bytes(selected_bytes, unresolved_body.successor.data(), selected_size);
        flags.predecessor_abort = selected_size == unresolved_body.predecessor_length && exact_bytes(selected_bytes, unresolved_body.predecessor.data(), selected_size);
        flags.sticky = !flags.successor_close && !flags.predecessor_abort;
        return select_recovery_precedence(flags);
    }
    bool replay = false;
    for (size_t i = 0; i < attempt_count; ++i) replay = replay || attempts[i] == request.attempt_id;
    const bool capacity = occupied_count == 512;
    const bool requested_slot_occupied = any_name(snapshot.slots[request.requested_slot].prepare) || any_name(snapshot.slots[request.requested_slot].close) ||
        any_name(snapshot.slots[request.requested_slot].abort_record);

    context_store_format_digest request_predecessor_digest {}, request_successor_digest {}, request_head_digest {}, request_commitment {};
    context_store_authenticated_protected_registry request_predecessor;
    const bool transition_ok = registry_envelope_digest(request.predecessor.data(), request.predecessor_length, request_predecessor_digest) &&
        registry_envelope_digest(request.successor.data(), request.successor_length, request_successor_digest) &&
        hash_with_domain("halofpx.registry-lab-head.v1", request.expected_current_head.data(), request.expected_current_head_length, request_head_digest) &&
        request_predecessor_digest == request.predecessor_digest && request_successor_digest == request.successor_digest && request_head_digest == request.expected_current_head_digest &&
        request.expected_current_head_digest == head.content_digest() && request.expected_current_head_length == snapshot.head.live_length &&
        exact_bytes(request.expected_current_head.data(), snapshot.head.live_bytes.data(), request.expected_current_head_length) &&
        request.predecessor_length == selected_size && exact_bytes(request.predecessor.data(), selected_bytes, selected_size) &&
        bind_predecessor(request.predecessor.data(), request.predecessor_length, preflight, credential, &request_predecessor) &&
        bind_successor(request.successor.data(), request.successor_length, preflight, credential, request_predecessor) &&
        operation_commitment(root.body().root_id, root.body().path_policy, request.attempt_id, request.requested_slot,
            request.predecessor_digest, request.successor_digest, request.predecessor_length, request.successor_length, request_commitment) &&
        request_commitment == request.operation_commitment;
    recovery_precedence_flags flags;
    flags.replay = replay; flags.capacity = capacity; flags.slot = requested_slot_occupied;
    flags.invalid = !transition_ok; flags.preexisting = request_successor_exists;
    return select_recovery_precedence(flags);
}

template<size_t N> bool same_modeled_file(const modeled_file<N> & a, const modeled_file<N> & b) noexcept {
    return a.live_present == b.live_present && a.durable_present == b.durable_present &&
        a.live_complete == b.live_complete && a.durable_complete == b.durable_complete &&
        a.live_length == b.live_length && a.durable_length == b.durable_length &&
        a.live_bytes == b.live_bytes && a.durable_bytes == b.durable_bytes;
}

bool same_action_state(const fixed_state & a, const fixed_state & b) noexcept {
    if (!same_modeled_file(a.marker, b.marker) || !same_modeled_file(a.lock_file, b.lock_file) || !same_modeled_file(a.head, b.head) ||
        !same_modeled_file(a.quarantine, b.quarantine) || !same_modeled_file(a.quarantine_staging, b.quarantine_staging) ||
        a.root_directory.live_projection != b.root_directory.live_projection || a.root_directory.durable_projection != b.root_directory.durable_projection ||
        a.attempts_directory.live_projection != b.attempts_directory.live_projection || a.attempts_directory.durable_projection != b.attempts_directory.durable_projection ||
        a.staging_directory.live_projection != b.staging_directory.live_projection || a.staging_directory.durable_projection != b.staging_directory.durable_projection ||
        a.envelopes_directory.live_projection != b.envelopes_directory.live_projection || a.envelopes_directory.durable_projection != b.envelopes_directory.durable_projection) return false;
    for (size_t i = 0; i < a.slots.size(); ++i) {
        const auto & x = a.slots[i]; const auto & y = b.slots[i];
        if (!same_modeled_file(x.prepare, y.prepare) || !same_modeled_file(x.close, y.close) || !same_modeled_file(x.abort_record, y.abort_record) ||
            !same_modeled_file(x.successor_staging, y.successor_staging) || !same_modeled_file(x.selector_staging, y.selector_staging)) return false;
    }
    const auto same_envelope = [](const modeled_envelope & x, const modeled_envelope & y) noexcept {
        return x.live_occupied == y.live_occupied && x.durable_occupied == y.durable_occupied && x.live_digest == y.live_digest &&
            x.durable_digest == y.durable_digest && same_modeled_file(x.object, y.object);
    };
    if (!same_envelope(a.initial_envelope, b.initial_envelope)) return false;
    for (size_t i = 0; i < a.successors.size(); ++i) if (!same_envelope(a.successors[i], b.successors[i])) return false;
    for (size_t i = 0; i < a.unexpected.size(); ++i) {
        const auto & x = a.unexpected[i]; const auto & y = b.unexpected[i];
        if (x.live_occupied != y.live_occupied || x.durable_occupied != y.durable_occupied || x.live_name != y.live_name ||
            x.durable_name != y.durable_name || x.live_length != y.live_length || x.durable_length != y.durable_length) return false;
    }
    // modeled_available_bytes is the dynamic operation-6 observation and is checked separately.
    return true;
}

bool logical_bytes(const fixed_state & state, uint64_t & total) noexcept {
    total = 0;
    const auto add = [&total](const auto & file) noexcept {
        if (!file.live_present) return true;
        if (file.live_length > file.live_bytes.size() || total > UINT64_MAX - file.live_length) return false;
        total += file.live_length; return true;
    };
    if (!add(state.marker) || !add(state.lock_file) || !add(state.head) || !add(state.quarantine) || !add(state.quarantine_staging)) return false;
    for (const modeled_slot & slot : state.slots)
        if (!add(slot.prepare) || !add(slot.close) || !add(slot.abort_record) || !add(slot.successor_staging) || !add(slot.selector_staging)) return false;
    if (!add(state.initial_envelope.object)) return false;
    for (const modeled_envelope & envelope : state.successors) if (!add(envelope.object)) return false;
    return true;
}

bool recovery_action_commitment(uint8_t action, uint64_t slot, const context_store_format_digest & attempt,
        const context_store_format_digest & prepare, const context_store_format_digest & head,
        const context_store_format_digest & operation, context_store_format_digest & output) noexcept {
    if ((action != 1 && action != 2) || slot >= 512) return false;
    std::array<uint8_t, 137> body {};
    size_t offset = 0; body[offset++] = action;
    for (size_t i = 0; i < 8; ++i) body[offset++] = static_cast<uint8_t>(slot >> (56 - 8 * i));
    for (const auto * digest : { &attempt, &prepare, &head, &operation }) {
        std::copy(digest->begin(), digest->end(), body.begin() + static_cast<std::ptrdiff_t>(offset)); offset += digest->size();
    }
    const bool ok = offset == body.size() && hash_with_domain("halofpx.registry-lab-recovery-action.v1", body.data(), body.size(), output);
    wipe(body); return ok;
}

struct scoped_wire_credential {
    explicit scoped_wire_credential(const credential_owner & owner) noexcept {
        value.key_id = owner.key_id; value.generation = owner.generation; value.secret = owner.secret;
    }
    ~scoped_wire_credential() noexcept {
        value.key_id = {}; value.generation = 0; volatile uint8_t * bytes = value.secret.data();
        for (size_t i = 0; i < value.secret.size(); ++i) bytes[i] = 0;
    }
    context_store_registry_lab_credential value;
};

bool derive_recovery_terminal(const fixed_state & snapshot, const preflight_context_v1 & preflight,
        const request_transition_v1 &, const credential_owner & credential, recovery_classification classification,
        std::array<uint8_t, 1024> & output, size_t & output_size, uint64_t & slot,
        context_store_format_digest & prepare_digest, context_store_format_digest & head_digest,
        context_store_format_digest & action_commitment, size_t & derivations) noexcept {
    output.fill(0); output_size = 0; slot = 0; prepare_digest = {}; head_digest = {}; action_commitment = {};
    if (classification != recovery_classification::needs_predecessor_abort && classification != recovery_classification::needs_successor_close) return false;
    auto root = decode_record<context_store_registry_lab_kind::root>(snapshot.marker.live_bytes.data(), snapshot.marker.live_length, credential, derivations);
    auto head = decode_record<context_store_registry_lab_kind::head>(snapshot.head.live_bytes.data(), snapshot.head.live_length, credential, derivations);
    if (!root.authenticated() || !head.authenticated()) return false;
    authenticated_record_v1<context_store_registry_lab_kind::prepare> prepare;
    bool found = false;
    for (size_t i = 0; i < snapshot.slots.size(); ++i) {
        if (!any_name(snapshot.slots[i].prepare) || any_name(snapshot.slots[i].close) || any_name(snapshot.slots[i].abort_record)) continue;
        auto candidate = decode_record<context_store_registry_lab_kind::prepare>(snapshot.slots[i].prepare.live_bytes.data(), snapshot.slots[i].prepare.live_length, credential, derivations);
        if (!candidate.authenticated() || found) return false;
        prepare = std::move(candidate); slot = i; found = true;
    }
    if (!found || prepare.body().slot != slot) return false;

    context_store_authenticated_protected_registry predecessor;
    if (!bind_predecessor(prepare.body().predecessor.data(), prepare.body().predecessor_length, preflight, credential, &predecessor) || !predecessor.body()) return false;
    if (head.body().selector_generation == 0) return false;
    const uint64_t predecessor_generation = classification == recovery_classification::needs_successor_close
        ? head.body().selector_generation - 1 : head.body().selector_generation;
    if (predecessor_generation == 0 || predecessor.body()->last_consumed_sequence == UINT64_MAX) return false;

    context_store_registry_lab_prepare_value_v1 transition;
    transition.scope.root_id = root.body().root_id; transition.scope.path_policy_commitment = root.body().path_policy;
    transition.scope.registry_id = preflight.registry_id; transition.scope.registry_epoch = preflight.registry_epoch;
    transition.attempt_id = prepare.body().attempt_id; transition.operation_commitment = prepare.body().operation_commitment;
    transition.predecessor_envelope_digest = prepare.body().predecessor_digest; transition.successor_envelope_digest = prepare.body().successor_digest;
    transition.initial_head_digest = prepare.body().previous_head; transition.slot = slot;
    transition.predecessor_high_water = predecessor.body()->last_consumed_sequence;
    transition.predecessor_selector_generation = predecessor_generation; transition.successor_selector_generation = predecessor_generation + 1;
    transition.predecessor.size = prepare.body().predecessor_length; transition.successor.size = prepare.body().successor_length;
    std::copy_n(prepare.body().predecessor.data(), transition.predecessor.size, transition.predecessor.bytes.begin());
    std::copy_n(prepare.body().successor.data(), transition.successor.size, transition.successor.bytes.begin());
    prepare_digest = prepare.content_digest(); head_digest = head.content_digest();
    scoped_wire_credential wire_owner(credential); const auto & wc = wire_owner.value;
    std::array<uint8_t, 1024> predecessor_head_bytes {}; size_t predecessor_head_size = 0;
    context_store_registry_lab_head_value_v1 predecessor_head_value; predecessor_head_value.scope = transition.scope;
    predecessor_head_value.selection = context_store_registry_lab_head_selection_v1::predecessor;
    predecessor_head_value.selected_envelope = transition.predecessor; predecessor_head_value.selected_envelope_digest = transition.predecessor_envelope_digest;
    predecessor_head_value.selected_high_water = transition.predecessor_high_water; predecessor_head_value.selector_generation = transition.predecessor_selector_generation;
    predecessor_head_value.expected_head_digest = transition.initial_head_digest;
    context_store_registry_lab_head_witness predecessor_head_witness;
    if (!context_store_registry_lab_admit_head_v1(predecessor_head_value, wc, predecessor_head_witness)) { wipe(predecessor_head_bytes); return false; }
    const auto predecessor_head_result = context_store_registry_lab_encode_head_v1(predecessor_head_value, wc, predecessor_head_witness,
        predecessor_head_bytes.data(), predecessor_head_bytes.size(), predecessor_head_size);
    if (!predecessor_head_result.authenticated() || predecessor_head_result.content_digest != prepare.body().previous_head) { wipe(predecessor_head_bytes); return false; }
    context_store_registry_lab_wire_result encoded;
    if (classification == recovery_classification::needs_predecessor_abort) {
        context_store_registry_lab_abort_value_v1 value; value.scope = transition.scope; value.terminal_class = context_store_registry_lab_terminal_class_v1::recovered;
        value.attempt_id = transition.attempt_id; value.operation_commitment = transition.operation_commitment; value.predecessor_envelope_digest = transition.predecessor_envelope_digest;
        value.successor_envelope_digest = transition.successor_envelope_digest; value.prepare_digest = prepare_digest; value.head_digest = prepare.body().previous_head; value.slot = slot;
        context_store_registry_lab_abort_evidence_v1 evidence; evidence.transition = transition; evidence.predecessor_head = predecessor_head_bytes.data();
        evidence.predecessor_head_size = predecessor_head_size; evidence.prepare = prepare.exact_data(); evidence.prepare_size = prepare.exact_size();
        context_store_registry_lab_abort_witness witness;
        if (!context_store_registry_lab_admit_abort_v1(value, wc, evidence, witness)) { wipe(predecessor_head_bytes); wipe(output); output_size = 0; return false; }
        encoded = context_store_registry_lab_encode_abort_v1(value, wc, witness, output.data(), output.size(), output_size);
    } else {
        context_store_registry_lab_close_value_v1 value; value.scope = transition.scope; value.terminal_class = context_store_registry_lab_terminal_class_v1::recovered;
        value.attempt_id = transition.attempt_id; value.operation_commitment = transition.operation_commitment; value.predecessor_envelope_digest = transition.predecessor_envelope_digest;
        value.successor_envelope_digest = transition.successor_envelope_digest; value.prepare_digest = prepare_digest; value.head_digest = head_digest; value.slot = slot;
        context_store_registry_lab_close_evidence_v1 evidence; evidence.transition = transition; evidence.predecessor_head = predecessor_head_bytes.data();
        evidence.predecessor_head_size = predecessor_head_size; evidence.successor_head = head.exact_data(); evidence.successor_head_size = head.exact_size();
        evidence.prepare = prepare.exact_data(); evidence.prepare_size = prepare.exact_size(); context_store_registry_lab_close_witness witness;
        if (!context_store_registry_lab_admit_close_v1(value, wc, evidence, witness)) { wipe(predecessor_head_bytes); wipe(output); output_size = 0; return false; }
        encoded = context_store_registry_lab_encode_close_v1(value, wc, witness, output.data(), output.size(), output_size);
    }
    const uint8_t action = classification == recovery_classification::needs_predecessor_abort ? 1 : 2;
    const bool ok = encoded.authenticated() && output_size > 0 && output_size <= output.size() &&
        recovery_action_commitment(action, slot, prepare.body().attempt_id, prepare_digest, head_digest, prepare.body().operation_commitment, action_commitment);
    wipe(predecessor_head_bytes); if (!ok) { wipe(output); output_size = 0; } return ok;
}

bool valid_script_shape(const script & value) noexcept {
    if (value.size != 5 && value.size != 11 && value.size != 19) return false;
    constexpr std::array<operation, 5> prefix { operation::guard_acquire, operation::writer_lock_acquire, operation::preflight,
        operation::snapshot_load, operation::recovery_validation };
    for (size_t i = 0; i < prefix.size(); ++i) if (value.entries[i].op != prefix[i]) return false;
    if (value.size == 5) return true;
    constexpr std::array<operation, 6> predecessor { operation::action_mutation_admission, operation::terminal_create,
        operation::terminal_write, operation::terminal_readback, operation::terminal_file_sync, operation::attempts_directory_sync };
    if (value.size == 11) {
        for (size_t i = 0; i < predecessor.size(); ++i) if (value.entries[5 + i].op != predecessor[i]) return false;
        return true;
    }
    constexpr std::array<operation, 14> successor { operation::action_mutation_admission, operation::successor_file_sync,
        operation::envelopes_directory_sync, operation::staging_directory_sync_after_successor, operation::head_file_sync,
        operation::root_directory_sync, operation::staging_directory_sync_after_head, operation::head_read, operation::successor_read,
        operation::terminal_create, operation::terminal_write, operation::terminal_readback, operation::terminal_file_sync,
        operation::attempts_directory_sync };
    for (size_t i = 0; i < successor.size(); ++i) if (value.entries[5 + i].op != successor[i]) return false;
    return true;
}

bool script_matches_recovery(const script & value, recovery_classification classification) noexcept {
    return (classification == recovery_classification::needs_predecessor_abort && value.size == 11) ||
        (classification == recovery_classification::needs_successor_close && value.size == 19);
}

template<size_t N> void apply_complete_live(modeled_file<N> & file, const uint8_t * bytes, size_t size) noexcept {
    file.live_present = true; file.live_complete = true; file.live_length = size; file.live_bytes.fill(0);
    if (size && size <= N) std::copy_n(bytes, size, file.live_bytes.begin());
}

template<size_t N> void apply_partial_live(modeled_file<N> & file, const uint8_t * bytes, size_t size) noexcept {
    const size_t prefix = size > 1 ? size / 2 : 0;
    file.live_present = true; file.live_complete = false; file.live_length = prefix; file.live_bytes.fill(0);
    if (prefix) std::copy_n(bytes, prefix, file.live_bytes.begin());
}

template<size_t N> void apply_complete_durable(modeled_file<N> & file) noexcept {
    file.durable_present = file.live_present; file.durable_complete = file.live_complete; file.durable_length = file.live_length;
    file.durable_bytes.fill(0); if (file.live_length <= N) std::copy_n(file.live_bytes.data(), file.live_length, file.durable_bytes.begin());
}

template<size_t N> void apply_complete_durable_bytes(modeled_file<N> & file) noexcept {
    file.durable_complete = file.live_complete; file.durable_length = file.live_length;
    file.durable_bytes.fill(0); if (file.live_length <= N) std::copy_n(file.live_bytes.data(), file.live_length, file.durable_bytes.begin());
}

template<size_t N> bool exact_durable_bytes(const modeled_file<N> & file) noexcept {
    return file.live_present && file.live_complete && file.durable_complete &&
        file.live_length == file.durable_length && file.live_length <= N &&
        exact_bytes(file.live_bytes.data(), file.durable_bytes.data(), file.live_length);
}

template<size_t N> void apply_partial_durable(modeled_file<N> & file) noexcept {
    const size_t advanced = file.live_length > 1 ? file.live_length / 2 : 0;
    const size_t prefix = std::max(file.durable_length, advanced);
    const bool already_complete = file.durable_complete && file.durable_length == file.live_length;
    file.durable_present = file.durable_present || file.live_present;
    file.durable_complete = already_complete;
    file.durable_length = prefix;
    file.durable_bytes.fill(0); if (prefix <= N) std::copy_n(file.live_bytes.data(), prefix, file.durable_bytes.begin());
}

template<size_t N> void apply_partial_durable_bytes(modeled_file<N> & file) noexcept {
    const bool namespace_durable = file.durable_present;
    apply_partial_durable(file);
    file.durable_present = namespace_durable;
}

status map_classification(recovery_classification value) noexcept {
    switch (value) {
        case recovery_classification::needs_successor_close:
        case recovery_classification::needs_predecessor_abort: return status::uncertain_requires_recovery;
        case recovery_classification::needs_sticky_quarantine:
        case recovery_classification::blocked_by_existing_quarantine: return status::quarantined_or_unavailable;
        case recovery_classification::inadmissible_initialization_artifact:
        case recovery_classification::preexisting_unattributed_material: return status::preexisting_material_no_authority;
        case recovery_classification::attempt_replayed: return status::attempt_replayed_no_mutation;
        case recovery_classification::capacity_exhausted: return status::capacity_exhausted_no_mutation;
        case recovery_classification::requested_slot_occupied: return status::slot_occupied_no_mutation;
        case recovery_classification::invalid_transition: return status::invalid_transition_no_mutation;
        case recovery_classification::continue_to_mutation:
        case recovery_classification::none: return status::invalid_request_no_mutation;
    }
    return status::invalid_request_no_mutation;
}

static_assert(std::is_nothrow_default_constructible_v<fixed_state>);
static_assert(std::is_nothrow_copy_constructible_v<restart_image>);

} // namespace

bool quarantine_diagnosis_commitment_for_test(uint64_t invocation_id, const quarantine_diagnosis_view & diagnosis,
        context_store_format_digest & output) noexcept {
    return derive_quarantine_diagnosis_commitment(invocation_id, diagnosis, output);
}

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

size_t fixture::begin(uint64_t invocation_id, uint8_t process_slot, credential_owner && credential,
        const preflight_context_v1 & preflight, const request_transition_v1 & request, const script & immutable_script) noexcept {
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
    current.preflight = preflight;
    current.request = request;
    current.current = phase::operations;
    current.immutable_script = immutable_script;
    current.derived.fill(0xa1);
    current.tag.fill(0xb2);
    current.scratch.fill(0xc3);
    current.witness.fill(0xd4);

    bool valid = invocation_id != 0 && process_slot < max_processes && valid_credential(current.credential) &&
        valid_preflight(current.preflight, current.credential) && valid_request(current.request);
    for (const invocation & other : invocations_) if (&other != &current && other.occupied && other.current != phase::complete && other.current != phase::dead && other.id == invocation_id) valid = false;
    valid = valid && immutable_script.size <= immutable_script.entries.size() && valid_script_shape(immutable_script);
    if (valid) for (size_t i = 0; i < immutable_script.size; ++i) valid = valid && admitted_payload(immutable_script.entries[i]);
    if (valid) {
        const primitive_code derived = guard_owner_[process_slot] == 0 ? primitive_code::ok : primitive_code::busy;
        valid = immutable_script.entries[0].code == derived;
    }
    if (!valid) {
        wipe_credential(current.credential); wipe(current.derived); wipe(current.tag); wipe(current.scratch); wipe(current.witness); wipe(current.terminal_scratch);
        current.rejection_wipe_verified = credential_zero(current.credential) && all_zero(current.derived) && all_zero(current.tag) && all_zero(current.scratch) && all_zero(current.witness) && all_zero(current.terminal_scratch);
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
        wipe_credential(current.credential); wipe(current.derived); wipe(current.tag); wipe(current.scratch); wipe(current.witness); wipe(current.terminal_scratch);
        current.pending.state = visibility::dead_process_no_result;
        current.current = phase::dead;
        current.teardown = { current.id, process, credential_zero(current.credential),
            all_zero(current.derived) && all_zero(current.tag) && all_zero(current.scratch) && all_zero(current.witness) && all_zero(current.terminal_scratch), secret_absent };
    }
    if (process < guard_owner_.size()) guard_owner_[process] = 0;
    if (writer_process_ == process) { writer_owner_ = 0; writer_process_ = 0xff; }
    for (const invocation & current : invocations_) if (current.process == process && current.id == snapshot_owner_) { snapshot_owner_ = 0; break; }
}

bool fixture::step(size_t handle) noexcept {
    if (handle >= invocations_.size()) return false;
    invocation & current = invocations_[handle];
    if (!current.occupied || current.current == phase::complete || current.current == phase::dead || current.current == phase::free) return false;
    if (current.current == phase::cleanup_wipe) {
        wipe_credential(current.credential); wipe(current.derived); wipe(current.tag); wipe(current.scratch); wipe(current.witness); wipe(current.terminal_scratch);
        current.ordinary_wipe_verified = credential_zero(current.credential) && all_zero(current.derived) && all_zero(current.tag) && all_zero(current.scratch) && all_zero(current.witness) && all_zero(current.terminal_scratch);
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

    if (current.cursor >= current.immutable_script.size) return false;
    const primitive_product product = current.immutable_script.entries[current.cursor];
    if (current.action_latched && state_.modeled_available_bytes < registry_minimum_reserve_bytes) {
        finish_ordinary(current, status::uncertain_requires_recovery);
        return true;
    }
    primitive_code derived = product.code;
    if (product.op == operation::guard_acquire) derived = guard_owner_[current.process] == 0 ? primitive_code::ok : primitive_code::busy;
    if (product.op == operation::writer_lock_acquire && product.code != primitive_code::unsupported)
        derived = writer_owner_ == 0 ? primitive_code::ok : primitive_code::busy;
    if (product.op == operation::action_mutation_admission) {
        if (product.code == primitive_code::unavailable) derived = primitive_code::ok;
        context_store_format_digest recomputed {}; uint64_t bytes = 0;
        const bool state_ok = snapshot_owner_ == current.id && same_action_state(state_, snapshot_) &&
            recovery_action_commitment(current.recovery_action, current.recovery_slot, current.action_attempt, current.prepare_digest,
                current.current_head_digest, current.action_operation, recomputed) && recomputed == current.action_commitment;
        if (!state_ok) derived = primitive_code::unavailable;
        else if (!logical_bytes(state_, bytes) || current.preflight.maximum_logical_authority_bytes != registry_logical_budget_bytes ||
                 !admitted_logical_budget(bytes)) derived = primitive_code::capacity_exhausted;
        else if (state_.modeled_available_bytes < registry_minimum_reserve_bytes) derived = primitive_code::reserve_exhausted;
    }
    if (static_cast<uint8_t>(product.op) >= 33 && !current.action_latched) derived = primitive_code::unavailable;
    modeled_file<1024> * terminal = current.recovery_action == 1 ? &state_.slots[current.recovery_slot].abort_record : &state_.slots[current.recovery_slot].close;
    modeled_envelope * action_successor = nullptr;
    for (modeled_envelope & envelope : state_.successors)
        if (envelope.live_occupied && envelope.live_digest == current.action_successor) {
            if (action_successor != nullptr) { action_successor = nullptr; break; }
            action_successor = &envelope;
        }
    if (product.op == operation::successor_file_sync || product.op == operation::successor_read) {
        context_store_format_digest digest {}; context_store_authenticated_protected_registry predecessor;
        const modeled_envelope * snapshot_successor = nullptr;
        for (const modeled_envelope & envelope : snapshot_.successors)
            if (envelope.live_occupied && envelope.live_digest == current.action_successor) snapshot_successor = snapshot_successor == nullptr ? &envelope : nullptr;
        const bool exact = action_successor != nullptr && snapshot_successor != nullptr &&
            same_modeled_file(action_successor->object, snapshot_successor->object) &&
            registry_envelope_digest(action_successor->object.live_bytes.data(), action_successor->object.live_length, digest) && digest == current.action_successor &&
            bind_predecessor(snapshot_.initial_envelope.object.live_bytes.data(), snapshot_.initial_envelope.object.live_length, current.preflight, current.credential, &predecessor) &&
            bind_successor(action_successor->object.live_bytes.data(), action_successor->object.live_length, current.preflight, current.credential, predecessor);
        if (!exact) derived = primitive_code::unavailable;
    }
    if (product.op == operation::envelopes_directory_sync && !state_.envelopes_directory.live_projection) derived = primitive_code::unavailable;
    if ((product.op == operation::staging_directory_sync_after_successor || product.op == operation::staging_directory_sync_after_head) && !state_.staging_directory.live_projection)
        derived = primitive_code::unavailable;
    if (product.op == operation::root_directory_sync && !state_.root_directory.live_projection) derived = primitive_code::unavailable;
    if (product.op == operation::head_file_sync && !exact_published_file(state_.head)) derived = primitive_code::unavailable;
    if (product.op == operation::head_read) {
        size_t calls = 0; auto record = decode_record<context_store_registry_lab_kind::head>(state_.head.live_bytes.data(), state_.head.live_length, current.credential, calls);
        current.derivations += calls;
        if (!exact_published_file(state_.head) || !same_modeled_file(state_.head, snapshot_.head) || !record.authenticated() || record.content_digest() != current.current_head_digest ||
            record.body().selected_digest != current.action_successor) derived = primitive_code::unavailable;
    }
    if (product.op == operation::terminal_create && !absent_file(*terminal)) derived = primitive_code::unavailable;
    if ((product.op == operation::terminal_write || product.op == operation::terminal_readback || product.op == operation::terminal_file_sync) && !terminal->live_present)
        derived = primitive_code::unavailable;
    if (product.op == operation::terminal_file_sync &&
        (!terminal->live_complete || terminal->live_length != current.terminal_size ||
         !exact_bytes(terminal->live_bytes.data(), current.terminal_scratch.data(), current.terminal_size)))
        derived = primitive_code::unavailable;
    if (product.op == operation::attempts_directory_sync &&
        (!exact_durable_bytes(*terminal) || !state_.attempts_directory.live_projection))
        derived = primitive_code::unavailable;
    if (product.op == operation::terminal_readback) {
        size_t calls = 0; bool authenticated = false;
        if (terminal->live_complete && terminal->live_length == current.terminal_size && exact_bytes(terminal->live_bytes.data(), current.terminal_scratch.data(), current.terminal_size)) {
            if (current.recovery_action == 1) {
                auto record = decode_record<context_store_registry_lab_kind::abort_record>(terminal->live_bytes.data(), terminal->live_length, current.credential, calls);
                authenticated = record.authenticated() && record.body().terminal_class == 1 && record.body().phase == 1 && record.body().slot == current.recovery_slot &&
                    record.body().attempt_id == current.action_attempt && record.body().operation_commitment == current.action_operation && record.body().prepare_digest == current.prepare_digest &&
                    record.body().predecessor_digest == current.action_predecessor && record.body().successor_digest == current.action_successor &&
                    record.body().terminal_head == current.current_head_digest;
            } else {
                auto record = decode_record<context_store_registry_lab_kind::close>(terminal->live_bytes.data(), terminal->live_length, current.credential, calls);
                authenticated = record.authenticated() && record.body().terminal_class == 1 && record.body().phase == 2 && record.body().slot == current.recovery_slot &&
                    record.body().attempt_id == current.action_attempt && record.body().operation_commitment == current.action_operation && record.body().prepare_digest == current.prepare_digest &&
                    record.body().predecessor_digest == current.action_predecessor && record.body().successor_digest == current.action_successor &&
                    record.body().terminal_head == current.current_head_digest;
            }
        }
        current.derivations += calls; if (!authenticated) derived = primitive_code::unavailable;
    }
    if (derived != product.code) {
        if (current.action_latched) {
            const bool authenticated_readback_contradiction = derived == primitive_code::unavailable &&
                (product.op == operation::head_read || product.op == operation::successor_read || product.op == operation::terminal_readback);
            finish_ordinary(current, authenticated_readback_contradiction ? status::quarantined_or_unavailable : status::uncertain_requires_recovery);
        } else {
            finish_ordinary(current, status::invalid_request_no_mutation);
        }
        return true;
    }

    current.events[current.event_count++] = { static_cast<uint16_t>(product.op) };
    ++current.cursor;
    if (product.op == operation::guard_acquire && product.code == primitive_code::ok) guard_owner_[current.process] = current.id;
    if (product.op == operation::writer_lock_acquire && product.code == primitive_code::ok) { writer_owner_ = current.id; writer_process_ = current.process; }
    if (product.op == operation::terminal_create && product.effect == storage_effect::complete_live) apply_complete_live(*terminal, nullptr, 0);
    if (product.op == operation::terminal_write) {
        if (product.effect == storage_effect::bounded_partial_bytes) apply_partial_live(*terminal, current.terminal_scratch.data(), current.terminal_size);
        if (product.effect == storage_effect::complete_live) apply_complete_live(*terminal, current.terminal_scratch.data(), current.terminal_size);
    }
    if (product.op == operation::successor_file_sync && action_successor != nullptr) {
        if (product.effect == storage_effect::bounded_partial_durability_projection) apply_partial_durable(action_successor->object);
        if (product.effect == storage_effect::complete_durability_projection) {
            apply_complete_durable(action_successor->object); action_successor->durable_occupied = action_successor->live_occupied;
            action_successor->durable_digest = action_successor->live_digest;
        }
    }
    if (product.op == operation::head_file_sync) {
        if (product.effect == storage_effect::bounded_partial_durability_projection) apply_partial_durable(state_.head);
        if (product.effect == storage_effect::complete_durability_projection) apply_complete_durable(state_.head);
    }
    if (product.op == operation::envelopes_directory_sync && product.effect == storage_effect::complete_durability_projection) state_.envelopes_directory.durable_projection = state_.envelopes_directory.live_projection;
    if ((product.op == operation::staging_directory_sync_after_successor || product.op == operation::staging_directory_sync_after_head) &&
        product.effect == storage_effect::complete_durability_projection) state_.staging_directory.durable_projection = state_.staging_directory.live_projection;
    if (product.op == operation::root_directory_sync && product.effect == storage_effect::complete_durability_projection) state_.root_directory.durable_projection = state_.root_directory.live_projection;
    if (product.op == operation::terminal_file_sync) {
        if (product.effect == storage_effect::bounded_partial_durability_projection) apply_partial_durable_bytes(*terminal);
        if (product.effect == storage_effect::complete_durability_projection) apply_complete_durable_bytes(*terminal);
    }
    if (product.op == operation::attempts_directory_sync && product.effect == storage_effect::complete_durability_projection) {
        terminal->durable_present = terminal->live_present;
        state_.attempts_directory.durable_projection = true;
    }
    if (product.completed == completion::process_death) { kill_process(current.process); return true; }
    if (product.completed == completion::response_lost) { finish_ordinary(current, static_cast<uint8_t>(product.op) >= 6 ? status::uncertain_requires_recovery : status::quarantined_or_unavailable); return true; }
    if (product.code != primitive_code::ok) {
        if (product.op == operation::action_mutation_admission)
            finish_ordinary(current, product.code == primitive_code::unavailable ? status::quarantined_or_unavailable : status::uncertain_requires_recovery);
        else if (current.action_latched)
            finish_ordinary(current, (product.op == operation::head_read || product.op == operation::successor_read || product.op == operation::terminal_readback) &&
                product.code == primitive_code::unavailable ? status::quarantined_or_unavailable : status::uncertain_requires_recovery);
        else finish_ordinary(current, map_confirmed(product.op, product.code));
        return true;
    }
    if (product.op == operation::snapshot_load) {
        if (!bounded_snapshot(state_)) { finish_ordinary(current, status::invalid_request_no_mutation); return true; }
        snapshot_ = state_;
        snapshot_owner_ = current.id;
    }
    if (product.op == operation::recovery_validation) {
        if (snapshot_owner_ != current.id) { finish_ordinary(current, status::invalid_request_no_mutation); return true; }
        current.derived_class = classify_operation_5(snapshot_, current.preflight, current.request, current.credential,
            current.scanned, current.derivations, current.id, current.quarantine_plan);
        if (current.derived_class != product.classification) { finish_ordinary(current, status::invalid_request_no_mutation); return true; }
        if (script_matches_recovery(current.immutable_script, current.derived_class)) {
            if (!derive_recovery_terminal(snapshot_, current.preflight, current.request, current.credential, current.derived_class,
                    current.terminal_scratch, current.terminal_size, current.recovery_slot, current.prepare_digest,
                    current.current_head_digest, current.action_commitment, current.derivations)) {
                snapshot_owner_ = 0; finish_ordinary(current, status::quarantined_or_unavailable); return true;
            }
            current.recovery_action = current.derived_class == recovery_classification::needs_predecessor_abort ? 1 : 2;
            size_t calls = 0;
            if (current.recovery_action == 1) {
                auto record = decode_record<context_store_registry_lab_kind::abort_record>(current.terminal_scratch.data(), current.terminal_size, current.credential, calls);
                if (!record.authenticated()) { snapshot_owner_ = 0; finish_ordinary(current, status::quarantined_or_unavailable); return true; }
                current.action_attempt = record.body().attempt_id; current.action_operation = record.body().operation_commitment;
                current.action_predecessor = record.body().predecessor_digest; current.action_successor = record.body().successor_digest;
            } else {
                auto record = decode_record<context_store_registry_lab_kind::close>(current.terminal_scratch.data(), current.terminal_size, current.credential, calls);
                if (!record.authenticated()) { snapshot_owner_ = 0; finish_ordinary(current, status::quarantined_or_unavailable); return true; }
                current.action_attempt = record.body().attempt_id; current.action_operation = record.body().operation_commitment;
                current.action_predecessor = record.body().predecessor_digest; current.action_successor = record.body().successor_digest;
            }
            current.derivations += calls;
        } else if (current.immutable_script.size != 5) {
            snapshot_owner_ = 0; finish_ordinary(current, status::invalid_request_no_mutation);
        } else if (current.derived_class == recovery_classification::continue_to_mutation) {
            snapshot_owner_ = 0;
            current.events[current.event_count++] = { 201 };
            current.boundary = true;
            finish_ordinary(current, status::invalid_request_no_mutation);
        } else {
            snapshot_owner_ = 0;
            finish_ordinary(current, map_classification(current.derived_class));
        }
    }
    if (product.op == operation::action_mutation_admission) {
        current.action_latched = true;
        return step(handle); // ADR-0023: no injectable definite-state gap before the first action operation.
    }
    if (product.op == operation::attempts_directory_sync) {
        snapshot_owner_ = 0;
        finish_ordinary(current, current.recovery_action == 1 ? status::recovered_not_applied_no_authority : status::modeled_recovered_successor_closed);
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

recovery_classification fixture::derived_classification(size_t handle) const noexcept {
    return handle < invocations_.size() && invocations_[handle].occupied ? invocations_[handle].derived_class : recovery_classification::none;
}
quarantine_diagnosis_view fixture::quarantine_diagnosis(size_t handle) const noexcept {
    return handle < invocations_.size() && invocations_[handle].occupied ? invocations_[handle].quarantine_plan : quarantine_diagnosis_view {};
}
size_t fixture::scanned_slots(size_t handle) const noexcept {
    return handle < invocations_.size() && invocations_[handle].occupied ? invocations_[handle].scanned : 0;
}
size_t fixture::kdf_calls(size_t handle) const noexcept {
    return handle < invocations_.size() && invocations_[handle].occupied ? invocations_[handle].derivations : 0;
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
    image.modeled_available_bytes = state_.modeled_available_bytes;
    return true;
}

size_t fixture::restart_projection_count(size_t handle) const noexcept {
    if (handle >= invocations_.size()) return 0;
    const invocation & current = invocations_[handle];
    if (!current.occupied || current.recovery_action == 0 || current.recovery_slot >= state_.slots.size()) return 1;
    const modeled_file<1024> & terminal = current.recovery_action == 1
        ? state_.slots[current.recovery_slot].abort_record : state_.slots[current.recovery_slot].close;
    if (terminal.durable_present || !terminal.live_present) return 1;
    if (current.cursor == 0 || current.cursor > current.immutable_script.size) return 0;
    const primitive_product & last = current.immutable_script.entries[current.cursor - 1];
    if (last.op == operation::attempts_directory_sync ||
        (last.op == operation::terminal_file_sync && last.effect != storage_effect::none)) return 2;
    return terminal.live_length <= terminal.live_bytes.size() ? terminal.live_length + 2 : 0;
}

bool fixture::project_restart(size_t handle, size_t ordinal, restart_image & image,
        restart_projection_audit & audit) const noexcept {
    audit = {};
    const size_t count = restart_projection_count(handle);
    if (count == 0 || ordinal >= count || !serialize_restart(image)) return false;
    audit.count = count; audit.ordinal = ordinal;
    const invocation & current = invocations_[handle];
    if (current.recovery_action == 0 || current.recovery_slot >= state_.slots.size()) return count == 1;
    const modeled_file<1024> & terminal = current.recovery_action == 1
        ? state_.slots[current.recovery_slot].abort_record : state_.slots[current.recovery_slot].close;
    if (count == 1) {
        audit.terminal_name_retained = terminal.durable_present;
        audit.retained_length = terminal.durable_present ? terminal.durable_length : 0;
        return true;
    }
    if (ordinal == 0) return true;

    restart_file_1024 & projected = current.recovery_action == 1
        ? image.slots[current.recovery_slot].abort_record : image.slots[current.recovery_slot].close;
    size_t retained = 0;
    if (count == 2) {
        retained = terminal.durable_length;
        if (current.cursor != 0 && current.immutable_script.entries[current.cursor - 1].op == operation::attempts_directory_sync)
            retained = terminal.live_length;
    } else {
        retained = ordinal - 1;
    }
    if (retained > terminal.live_length || retained > projected.bytes.size()) return false;
    projected = {};
    projected.present = true;
    projected.complete = terminal.live_complete && retained == terminal.live_length;
    projected.length = retained;
    std::copy_n(terminal.live_bytes.data(), retained, projected.bytes.begin());
    audit.terminal_name_retained = true;
    audit.retained_length = retained;
    return valid_restart_projection(image);
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
    state_.modeled_available_bytes = image.modeled_available_bytes;
    return true;
}

} // namespace halofpx::registry_lab_read_only_test
