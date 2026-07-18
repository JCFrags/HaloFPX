#pragma once

#include "halofpx-context-store-auth.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace halofpx_test_fixture {

using bytes = std::vector<uint8_t>;

inline void head(bytes & out, uint8_t major, uint64_t value) {
    if (value < 24) out.push_back(static_cast<uint8_t>((major << 5) | value));
    else if (value <= 0xff) { out.push_back(static_cast<uint8_t>((major << 5) | 24)); out.push_back(static_cast<uint8_t>(value)); }
    else if (value <= 0xffff) { out.push_back(static_cast<uint8_t>((major << 5) | 25)); out.push_back(static_cast<uint8_t>(value >> 8)); out.push_back(static_cast<uint8_t>(value)); }
    else if (value <= 0xffffffffULL) { out.push_back(static_cast<uint8_t>((major << 5) | 26)); for (int s = 24; s >= 0; s -= 8) out.push_back(static_cast<uint8_t>(value >> s)); }
    else { out.push_back(static_cast<uint8_t>((major << 5) | 27)); for (int s = 56; s >= 0; s -= 8) out.push_back(static_cast<uint8_t>(value >> s)); }
}

inline void uint(bytes & out, uint64_t value) { head(out, 0, value); }
inline void map(bytes & out, uint64_t count) { head(out, 5, count); }
inline void array(bytes & out, uint64_t count) { head(out, 4, count); }
inline void bstr(bytes & out, const uint8_t * data, size_t size) { head(out, 2, size); out.insert(out.end(), data, data + size); }
inline void fill(bytes & out, size_t size, uint8_t value) { head(out, 2, size); out.insert(out.end(), size, value); }
inline void digest(bytes & out, const halofpx::context_store_format_digest & value) { bstr(out, value.data(), value.size()); }
inline void text(bytes & out, const bytes & value) { head(out, 3, value.size()); out.insert(out.end(), value.begin(), value.end()); }
inline void text(bytes & out, const char * value) { const auto size = std::strlen(value); head(out, 3, size); out.insert(out.end(), value, value + size); }

inline bytes domain(const char * name, const bytes & body) {
    bytes out(reinterpret_cast<const uint8_t *>(name), reinterpret_cast<const uint8_t *>(name) + std::strlen(name) + 1);
    out.insert(out.end(), body.begin(), body.end());
    return out;
}

inline halofpx::context_store_format_digest sha(const bytes & input) {
    halofpx::context_store_format_digest output {};
    assert(halofpx::context_store_sha256(input.data(), input.size(), output));
    return output;
}

inline halofpx::context_store_format_digest hmac(const bytes & key, const bytes & input) {
    halofpx::context_store_format_digest output {};
    assert(halofpx::context_store_hmac_sha256(key.data(), key.size(), input.data(), input.size(), output));
    return output;
}

inline halofpx::context_store_registered_id registered_id(const bytes & value) {
    halofpx::context_store_registered_id output;
    assert(!value.empty() && value.size() <= halofpx::context_store_registered_id_max_bytes);
    output.size = static_cast<uint8_t>(value.size());
    std::copy(value.begin(), value.end(), output.bytes.begin());
    return output;
}

struct signed_manifest {
    bytes encoded;
    bytes master_key;
    halofpx::context_store_manifest_verification_policy policy;

    halofpx::context_store_manifest_verify_result verify() {
        policy.key.master_key = { master_key.data(), master_key.size() };
        return halofpx::context_store_verify_manifest_v1(encoded.data(), encoded.size(), policy);
    }
};

inline signed_manifest make_signed_object_manifest(
        const halofpx::context_store_format_digest & object_id,
        const bytes & stream_type,
        uint64_t frame_bytes) {
    signed_manifest fixture;
    fixture.master_key.assign(32, 0x11);
    for (size_t index = 0; index < fixture.policy.compatibility.components.size(); ++index) {
        fixture.policy.compatibility.components[index].fill(static_cast<uint8_t>(0x20 + index));
    }

    bytes compatibility;
    map(compatibility, 16);
    for (size_t index = 0; index < fixture.policy.compatibility.components.size(); ++index) {
        uint(compatibility, index); digest(compatibility, fixture.policy.compatibility.components[index]);
    }
    fixture.policy.compatibility.root = sha(domain("halofpx.compat.v1", compatibility));

    bytes body;
    map(body, 15);
    uint(body, 0); uint(body, 1); uint(body, 1); uint(body, 0);
    uint(body, 2); fill(body, 16, 0x02); uint(body, 3); fill(body, 32, 0x03);
    uint(body, 4); uint(body, 1); uint(body, 5); body.push_back(0xf6);
    uint(body, 6); body.insert(body.end(), compatibility.begin(), compatibility.end());
    uint(body, 7); digest(body, fixture.policy.compatibility.root);
    uint(body, 8); fill(body, 32, 0x80); uint(body, 9); uint(body, 7);
    uint(body, 10); map(body, 6);
    uint(body, 0); text(body, "plan.synthetic.v1"); uint(body, 1); text(body, "single");
    uint(body, 2); uint(body, 1); uint(body, 3); fill(body, 32, 0xa3);
    uint(body, 4); uint(body, 9); uint(body, 5); array(body, 1);
    map(body, 3); uint(body, 0); uint(body, 0); uint(body, 1); fill(body, 32, 0xb0); uint(body, 2); fill(body, 32, 0xc0);
    uint(body, 11); text(body, "profile.synthetic.v1"); uint(body, 12); array(body, 1);
    map(body, 13);
    uint(body, 0); digest(body, object_id); uint(body, 1); text(body, stream_type);
    uint(body, 2); text(body, "codec.synthetic.v1"); uint(body, 3); uint(body, 1); uint(body, 4); uint(body, 0);
    uint(body, 5); body.push_back(0xf5); uint(body, 6); uint(body, frame_bytes);
    uint(body, 7); fill(body, 32, 0xd7); uint(body, 8); uint(body, 16); uint(body, 9); uint(body, 8);
    uint(body, 10); uint(body, 0); uint(body, 11); fill(body, 32, 0xb0);
    uint(body, 12); digest(body, fixture.policy.compatibility.root);
    uint(body, 13); fill(body, 32, 0xe3); uint(body, 14); uint(body, 0);

    const bytes key_id = { 't','e','s','t','-','k','e','y','-','v','1' };
    bytes auth_input;
    map(auth_input, 4); uint(auth_input, 0); auth_input.insert(auth_input.end(), body.begin(), body.end());
    uint(auth_input, 1); text(auth_input, key_id); uint(auth_input, 2); uint(auth_input, 1); uint(auth_input, 3); uint(auth_input, 1);

    bytes kdf = domain("halofpx.manifest-key.v1", {});
    kdf.push_back(0); kdf.push_back(static_cast<uint8_t>(key_id.size()));
    kdf.insert(kdf.end(), key_id.begin(), key_id.end());
    kdf.insert(kdf.end(), 16, 0x02); kdf.insert(kdf.end(), 32, 0x80);
    kdf.insert(kdf.end(), 7, 0); kdf.push_back(1);
    const auto derived_key = hmac(fixture.master_key, kdf);
    const auto tag = hmac(bytes(derived_key.begin(), derived_key.end()), domain("halofpx.manifest-auth.v1", auth_input));

    map(fixture.encoded, 2); uint(fixture.encoded, 0);
    fixture.encoded.insert(fixture.encoded.end(), auth_input.begin(), auth_input.end());
    uint(fixture.encoded, 1); digest(fixture.encoded, tag);

    fixture.policy.key.disposition = halofpx::context_store_key_disposition::active;
    fixture.policy.key.key_id = registered_id(key_id); fixture.policy.key.generation = 1;
    fixture.policy.anchor.store_uuid.fill(0x02); fixture.policy.anchor.checkpoint_lineage_id.fill(0x03);
    fixture.policy.anchor.namespace_id.fill(0x80); fixture.policy.anchor.policy_epoch = 7;
    fixture.policy.anchor.key_generation = 1; fixture.policy.anchor.generation = 1;
    fixture.policy.anchor.selected_manifest_digest = sha(domain("halofpx.manifest.v1", fixture.encoded));
    return fixture;
}

} // namespace halofpx_test_fixture
