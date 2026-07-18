#include "halofpx-context-store-anchor.h"

#include <algorithm>
#include <array>
#include <cstring>

namespace halofpx {
namespace {

constexpr char anchor_key_domain[] = "halofpx.anchor-key.v1";
constexpr char anchor_auth_domain[] = "halofpx.anchor-auth.v1";
constexpr char anchor_digest_domain[] = "halofpx.anchor.v1";
constexpr char anchor_authority_binding_domain[] = "halofpx.anchor-authority-binding.v1";

struct buffer {
    std::array<uint8_t, context_store_anchor_max_bytes> bytes {};
    size_t size = 0;
    bool append(uint8_t value) noexcept {
        if (size == bytes.size()) return false;
        bytes[size++] = value;
        return true;
    }
    bool append(const uint8_t * data, size_t count) noexcept {
        if ((data == nullptr && count != 0) || count > bytes.size() - size) return false;
        std::copy_n(data, count, bytes.begin() + size);
        size += count;
        return true;
    }
};

void wipe(void * pointer, size_t size) noexcept {
    volatile uint8_t * bytes = static_cast<volatile uint8_t *>(pointer);
    while (size-- != 0) *bytes++ = 0;
}

bool anchor_id_valid(const context_store_registered_id & id) noexcept {
    if (id.size == 0 || id.size > context_store_registered_id_max_bytes) return false;
    const auto * data = reinterpret_cast<const uint8_t *>(id.bytes.data());
    for (size_t i = 0; i < id.size; ++i) if (data[i] == 0 || data[i] > 0x7f) return false;
    return true;
}

bool head(buffer & out, uint8_t major, uint64_t value) noexcept {
    if (value < 24) return out.append(static_cast<uint8_t>((major << 5) | value));
    size_t count = value <= 0xff ? 1 : value <= 0xffff ? 2 : value <= 0xffffffffULL ? 4 : 8;
    const uint8_t ai = count == 1 ? 24 : count == 2 ? 25 : count == 4 ? 26 : 27;
    if (!out.append(static_cast<uint8_t>((major << 5) | ai))) return false;
    for (size_t n = count; n != 0; --n) if (!out.append(static_cast<uint8_t>(value >> ((n - 1) * 8)))) return false;
    return true;
}
bool uint(buffer & out, uint64_t value) noexcept { return head(out, 0, value); }
bool map(buffer & out, uint64_t count) noexcept { return head(out, 5, count); }
template <size_t N> bool bstr(buffer & out, const std::array<uint8_t, N> & value) noexcept {
    return head(out, 2, N) && out.append(value.data(), N);
}
bool text(buffer & out, const context_store_registered_id & id) noexcept {
    return anchor_id_valid(id) && head(out, 3, id.size) && out.append(reinterpret_cast<const uint8_t *>(id.bytes.data()), id.size);
}

bool encode_body(buffer & out, const context_store_anchor_body & a) noexcept {
    if (a.generation == 0 || (a.generation == 1 && a.has_predecessor) || (a.generation > 1 && !a.has_predecessor)) return false;
    return map(out, 11) && uint(out, 0) && uint(out, 1) && uint(out, 1) && uint(out, 0) &&
        uint(out, 2) && bstr(out, a.store_uuid) && uint(out, 3) && bstr(out, a.namespace_id) &&
        uint(out, 4) && uint(out, a.policy_epoch) && uint(out, 5) && bstr(out, a.checkpoint_lineage_id) &&
        uint(out, 6) && uint(out, a.manifest_key_generation) && uint(out, 7) && uint(out, a.authority_epoch) &&
        uint(out, 8) && uint(out, a.generation) && uint(out, 9) && bstr(out, a.selected_manifest_digest) &&
        uint(out, 10) && (a.has_predecessor ? bstr(out, a.predecessor_manifest_digest) : out.append(0xf6));
}

bool encode_auth_input(buffer & out, const context_store_anchor_body & anchor, const context_store_anchor_key_record & key) noexcept {
    return map(out, 4) && uint(out, 0) && encode_body(out, anchor) && uint(out, 1) && text(out, key.key_id) &&
        uint(out, 2) && uint(out, 1) && uint(out, 3) && uint(out, key.generation);
}

bool derive_key(const context_store_anchor_body & anchor, const context_store_anchor_key_record & key,
        context_store_format_digest & derived) noexcept {
    buffer input;
    const uint8_t length[2] = { 0, key.key_id.size };
    uint8_t generation[8] {};
    for (size_t i = 0; i < 8; ++i) generation[7 - i] = static_cast<uint8_t>(key.generation >> (i * 8));
    return input.append(reinterpret_cast<const uint8_t *>(anchor_key_domain), sizeof(anchor_key_domain)) &&
        input.append(length, sizeof(length)) &&
        input.append(reinterpret_cast<const uint8_t *>(key.key_id.bytes.data()), key.key_id.size) &&
        input.append(anchor.store_uuid.data(), anchor.store_uuid.size()) &&
        input.append(anchor.namespace_id.data(), anchor.namespace_id.size()) && input.append(generation, sizeof(generation)) &&
        context_store_hmac_sha256(key.master_key.data, key.master_key.size, input.bytes.data(), input.size, derived);
}

bool digest_envelope(const uint8_t * data, size_t size, context_store_format_digest & digest) noexcept {
    buffer input;
    return input.append(reinterpret_cast<const uint8_t *>(anchor_digest_domain), sizeof(anchor_digest_domain)) &&
        input.append(data, size) && context_store_sha256(input.bytes.data(), input.size, digest);
}

bool authority_binding(const context_store_format_digest & derived_key,
        context_store_format_digest & binding) noexcept {
    return context_store_hmac_sha256(
        derived_key.data(), derived_key.size(),
        reinterpret_cast<const uint8_t *>(anchor_authority_binding_domain),
        sizeof(anchor_authority_binding_domain), binding);
}

class cursor {
public:
    cursor(const uint8_t * data, size_t size) noexcept : data_(data), size_(size) {}
    size_t pos() const noexcept { return pos_; }
    bool end() const noexcept { return pos_ == size_; }
    bool map_n(uint64_t n) noexcept { uint64_t v; return read_head(5, v) && v == n; }
    bool key(uint64_t n) noexcept { uint64_t v; return number(v) && v == n; }
    bool number(uint64_t & v) noexcept { return read_head(0, v); }
    template <size_t N> bool bytes(std::array<uint8_t, N> & out) noexcept {
        uint64_t n; if (!read_head(2, n) || n != N || !have(N)) return false;
        std::copy_n(data_ + pos_, N, out.begin()); pos_ += N; return true;
    }
    bool id(context_store_registered_id & out) noexcept {
        uint64_t n; if (!read_head(3, n) || n == 0 || n > context_store_registered_id_max_bytes || !have(static_cast<size_t>(n))) return false;
        out.size = static_cast<uint8_t>(n); std::copy_n(data_ + pos_, out.size, out.bytes.begin()); pos_ += out.size;
        return anchor_id_valid(out);
    }
    bool predecessor(bool & present, context_store_format_digest & out) noexcept {
        if (!have(1)) return false;
        if (data_[pos_] == 0xf6) { ++pos_; present = false; out.fill(0); return true; }
        present = true; return bytes(out);
    }
private:
    bool have(size_t n) const noexcept { return n <= size_ - pos_; }
    bool read_head(uint8_t major, uint64_t & value) noexcept {
        if (!have(1)) return false;
        const uint8_t initial = data_[pos_++];
        if ((initial >> 5) != major) return false;
        const uint8_t ai = initial & 31;
        if (ai < 24) { value = ai; return true; }
        if (ai > 27) return false;
        const size_t n = size_t(1) << (ai - 24); if (!have(n)) return false;
        value = 0; for (size_t i = 0; i < n; ++i) value = (value << 8) | data_[pos_++];
        const uint64_t minimum = ai == 24 ? 24 : ai == 25 ? 0x100 : ai == 26 ? 0x10000 : 0x100000000ULL;
        return value >= minimum;
    }
    const uint8_t * data_; size_t size_; size_t pos_ = 0;
};

struct parsed_anchor {
    context_store_anchor_body body;
    context_store_registered_id key_id;
    uint64_t key_generation = 0;
    context_store_format_digest tag {};
    size_t auth_offset = 0, auth_size = 0;
};

bool parse(const uint8_t * data, size_t size, parsed_anchor & parsed) noexcept {
    if (data == nullptr || size == 0 || size > context_store_anchor_max_bytes) return false;
    cursor c(data, size);
    uint64_t value = 0;
    if (!c.map_n(2) || !c.key(0)) return false;
    parsed.auth_offset = c.pos();
    if (!c.map_n(4) || !c.key(0) || !c.map_n(11) ||
        !c.key(0) || !c.number(value) || value != 1 || !c.key(1) || !c.number(value) || value != 0 ||
        !c.key(2) || !c.bytes(parsed.body.store_uuid) || !c.key(3) || !c.bytes(parsed.body.namespace_id) ||
        !c.key(4) || !c.number(parsed.body.policy_epoch) || !c.key(5) || !c.bytes(parsed.body.checkpoint_lineage_id) ||
        !c.key(6) || !c.number(parsed.body.manifest_key_generation) || !c.key(7) || !c.number(parsed.body.authority_epoch) ||
        !c.key(8) || !c.number(parsed.body.generation) || parsed.body.generation == 0 ||
        !c.key(9) || !c.bytes(parsed.body.selected_manifest_digest) || !c.key(10) ||
        !c.predecessor(parsed.body.has_predecessor, parsed.body.predecessor_manifest_digest)) return false;
    if ((parsed.body.generation == 1) != !parsed.body.has_predecessor) return false;
    if (!c.key(1) || !c.id(parsed.key_id) || !c.key(2) || !c.number(value) || value != 1 ||
        !c.key(3) || !c.number(parsed.key_generation)) return false;
    parsed.auth_size = c.pos() - parsed.auth_offset;
    return c.key(1) && c.bytes(parsed.tag) && c.end();
}

bool id_equal(const context_store_registered_id & a, const context_store_registered_id & b) noexcept {
    if (a.size != b.size) return false;
    volatile uint8_t difference = 0; for (size_t i = 0; i < a.size; ++i) difference = static_cast<uint8_t>(difference | static_cast<uint8_t>(a.bytes[i] ^ b.bytes[i]));
    return difference == 0;
}
bool digest_equal(const context_store_format_digest & a, const context_store_format_digest & b) noexcept {
    volatile uint8_t difference = 0; for (size_t i = 0; i < a.size(); ++i) difference = static_cast<uint8_t>(difference | static_cast<uint8_t>(a[i] ^ b[i]));
    return difference == 0;
}
bool same_authority(const context_store_anchor_body & a, const context_store_anchor_body & b) noexcept {
    return a.store_uuid == b.store_uuid && a.namespace_id == b.namespace_id && a.policy_epoch == b.policy_epoch &&
        a.checkpoint_lineage_id == b.checkpoint_lineage_id && a.authority_epoch == b.authority_epoch;
}
bool same_replay(const context_store_anchor_body & a, const context_store_anchor_body & b) noexcept {
    return a.manifest_key_generation == b.manifest_key_generation && a.generation == b.generation &&
        a.selected_manifest_digest == b.selected_manifest_digest && a.has_predecessor == b.has_predecessor &&
        (!a.has_predecessor || a.predecessor_manifest_digest == b.predecessor_manifest_digest);
}

} // namespace

context_store_anchor_result context_store_encode_anchor_v1(const context_store_anchor_body & anchor,
        const context_store_anchor_key_record & key, uint8_t * output, size_t capacity) noexcept {
    context_store_anchor_result result;
    if (key.disposition != context_store_key_disposition::active || !anchor_id_valid(key.key_id) ||
        key.master_key.data == nullptr || key.master_key.size == 0 || key.master_key.size > context_store_master_key_max_bytes) {
        result.status = context_store_anchor_status::invalid_policy; return result;
    }
    buffer auth, envelope, message; context_store_format_digest derived {}, tag {}, binding {};
    if (!encode_auth_input(auth, anchor, key) || !derive_key(anchor, key, derived) ||
        !message.append(reinterpret_cast<const uint8_t *>(anchor_auth_domain), sizeof(anchor_auth_domain)) ||
        !message.append(auth.bytes.data(), auth.size) ||
        !context_store_hmac_sha256(derived.data(), derived.size(), message.bytes.data(), message.size, tag) ||
        !authority_binding(derived, binding)) {
        wipe(derived.data(), derived.size());
        wipe(tag.data(), tag.size());
        wipe(binding.data(), binding.size());
        result.status = context_store_anchor_status::invalid_policy;
        return result;
    }
    wipe(derived.data(), derived.size());
    if (!map(envelope, 2) || !uint(envelope, 0) || !envelope.append(auth.bytes.data(), auth.size) ||
        !uint(envelope, 1) || !bstr(envelope, tag)) {
        wipe(tag.data(), tag.size());
        wipe(binding.data(), binding.size());
        result.status = context_store_anchor_status::invalid_policy;
        return result;
    }
    wipe(tag.data(), tag.size()); result.encoded_size = envelope.size;
    if (output == nullptr || capacity < envelope.size) {
        wipe(binding.data(), binding.size());
        result.status = context_store_anchor_status::output_too_small;
        return result;
    }
    std::copy_n(envelope.bytes.data(), envelope.size, output);
    if (!digest_envelope(output, envelope.size, result.envelope_digest)) {
        wipe(binding.data(), binding.size());
        result.status = context_store_anchor_status::invalid_policy;
        return result;
    }
    result.carrier_.body_ = anchor;
    result.carrier_.key_id_ = key.key_id;
    result.carrier_.key_generation_ = key.generation;
    result.carrier_.authority_binding_ = binding;
    result.carrier_.digest_ = result.envelope_digest;
    std::copy_n(output, envelope.size, result.carrier_.envelope_.begin());
    result.carrier_.envelope_size_ = envelope.size;
    result.carrier_.authenticated_ = true;
    wipe(binding.data(), binding.size());
    result.status = context_store_anchor_status::authenticated_unadmitted; return result;
}

context_store_anchor_result context_store_verify_anchor_v1(const uint8_t * data, size_t size,
        const context_store_anchor_policy & policy) noexcept {
    context_store_anchor_result result; parsed_anchor parsed;
    if (!parse(data, size, parsed)) return result;
    result.encoded_size = size;
    if (!anchor_id_valid(policy.key.key_id) || policy.expected.generation == 0 ||
        (policy.expected.generation == 1 && policy.expected.has_predecessor) ||
        (policy.expected.generation > 1 && !policy.expected.has_predecessor)) {
        result.status = context_store_anchor_status::invalid_policy; return result;
    }
    if (!id_equal(parsed.key_id, policy.key.key_id) || policy.key.disposition == context_store_key_disposition::unknown) {
        result.status = context_store_anchor_status::unknown_key; return result;
    }
    if (policy.key.disposition == context_store_key_disposition::revoked) { result.status = context_store_anchor_status::revoked_key; return result; }
    if (policy.key.disposition == context_store_key_disposition::read_disabled) { result.status = context_store_anchor_status::read_disabled_key; return result; }
    if (policy.key.disposition != context_store_key_disposition::active || policy.key.master_key.data == nullptr ||
        policy.key.master_key.size == 0 || policy.key.master_key.size > context_store_master_key_max_bytes) {
        result.status = context_store_anchor_status::invalid_policy; return result;
    }
    if (parsed.key_generation != policy.key.generation) { result.status = context_store_anchor_status::key_generation_mismatch; return result; }
    context_store_format_digest derived {}, expected_tag {}, binding {}; buffer message;
    const bool crypto_ok = derive_key(parsed.body, policy.key, derived) &&
        message.append(reinterpret_cast<const uint8_t *>(anchor_auth_domain), sizeof(anchor_auth_domain)) &&
        message.append(data + parsed.auth_offset, parsed.auth_size) &&
        context_store_hmac_sha256(derived.data(), derived.size(), message.bytes.data(), message.size, expected_tag) &&
        authority_binding(derived, binding);
    wipe(derived.data(), derived.size());
    const bool tag_ok = crypto_ok && digest_equal(expected_tag, parsed.tag); wipe(expected_tag.data(), expected_tag.size());
    if (!tag_ok) {
        wipe(binding.data(), binding.size());
        result.status = context_store_anchor_status::authentication_failed;
        return result;
    }
    if (!same_authority(parsed.body, policy.expected)) {
        wipe(binding.data(), binding.size());
        result.status = context_store_anchor_status::authority_mismatch;
        return result;
    }
    if (parsed.body.generation < policy.expected.generation) {
        wipe(binding.data(), binding.size());
        result.status = context_store_anchor_status::rollback_detected;
        return result;
    }
    if (!same_replay(parsed.body, policy.expected)) {
        wipe(binding.data(), binding.size());
        result.status = context_store_anchor_status::replay_mismatch;
        return result;
    }
    if (!digest_envelope(data, size, result.envelope_digest)) {
        wipe(binding.data(), binding.size());
        result.status = context_store_anchor_status::invalid_policy;
        return result;
    }
    result.carrier_.body_ = parsed.body;
    result.carrier_.key_id_ = parsed.key_id;
    result.carrier_.key_generation_ = parsed.key_generation;
    result.carrier_.authority_binding_ = binding;
    result.carrier_.digest_ = result.envelope_digest;
    std::copy_n(data, size, result.carrier_.envelope_.begin());
    result.carrier_.envelope_size_ = size;
    result.carrier_.authenticated_ = true;
    wipe(binding.data(), binding.size());
    result.status = context_store_anchor_status::authenticated_unadmitted; return result;
}

const char * context_store_anchor_status_name(context_store_anchor_status status) noexcept {
    switch (status) {
        case context_store_anchor_status::authenticated_unadmitted: return "authenticated_unadmitted";
        case context_store_anchor_status::structural_rejection: return "structural_rejection";
        case context_store_anchor_status::invalid_policy: return "invalid_policy";
        case context_store_anchor_status::unknown_key: return "unknown_key";
        case context_store_anchor_status::revoked_key: return "revoked_key";
        case context_store_anchor_status::read_disabled_key: return "read_disabled_key";
        case context_store_anchor_status::key_generation_mismatch: return "key_generation_mismatch";
        case context_store_anchor_status::authentication_failed: return "authentication_failed";
        case context_store_anchor_status::authority_mismatch: return "authority_mismatch";
        case context_store_anchor_status::rollback_detected: return "rollback_detected";
        case context_store_anchor_status::replay_mismatch: return "replay_mismatch";
        case context_store_anchor_status::output_too_small: return "output_too_small";
    }
    return "unknown";
}

} // namespace halofpx
