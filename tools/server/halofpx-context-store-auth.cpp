#include "halofpx-context-store-auth.h"

extern "C" {
#include "sha256/sha256.h"
}

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>

namespace halofpx {
namespace {

struct byte_span {
    const uint8_t * data = nullptr;
    size_t size = 0;
};

constexpr size_t sha256_block_bytes = 64;
constexpr size_t crypto_input_max_bytes = context_store_manifest_max_bytes + 4096;
constexpr char manifest_key_domain[] = "halofpx.manifest-key.v1";
constexpr char manifest_auth_domain[] = "halofpx.manifest-auth.v1";
constexpr char manifest_domain[] = "halofpx.manifest.v1";
constexpr char compatibility_domain[] = "halofpx.compat.v1";

void secure_wipe(void * memory, size_t size) noexcept {
    volatile uint8_t * bytes = static_cast<volatile uint8_t *>(memory);
    while (size-- > 0) {
        *bytes++ = 0;
    }
}

bool valid_span(const byte_span & span) noexcept {
    return span.size == 0 || span.data != nullptr;
}

bool spans_bounded(const byte_span * spans, size_t span_count) noexcept {
    if (spans == nullptr && span_count != 0) {
        return false;
    }
    size_t total = 0;
    for (size_t index = 0; index < span_count; ++index) {
        if (!valid_span(spans[index]) || spans[index].size > crypto_input_max_bytes - total) {
            return false;
        }
        total += spans[index].size;
    }
    return true;
}

bool sha256_spans(
        const byte_span * spans,
        size_t span_count,
        context_store_format_digest & digest) noexcept {
    if (!spans_bounded(spans, span_count)) {
        return false;
    }
    sha256_t context;
    sha256_init(&context);
    for (size_t index = 0; index < span_count; ++index) {
        if (!valid_span(spans[index])) {
            secure_wipe(&context, sizeof(context));
            return false;
        }
        if (spans[index].size != 0) {
            sha256_update(&context, spans[index].data, spans[index].size);
        }
    }
    sha256_final(&context, digest.data());
    secure_wipe(&context, sizeof(context));
    return true;
}

bool hmac_sha256_spans(
        const uint8_t * key,
        size_t key_size,
        const byte_span * spans,
        size_t span_count,
        context_store_format_digest & tag) noexcept {
    if ((key == nullptr && key_size != 0) ||
        key_size > context_store_master_key_max_bytes ||
        !spans_bounded(spans, span_count)) {
        return false;
    }

    std::array<uint8_t, sha256_block_bytes> key_block {};
    context_store_format_digest hashed_key {};
    if (key_size > sha256_block_bytes) {
        const byte_span key_span { key, key_size };
        if (!sha256_spans(&key_span, 1, hashed_key)) {
            return false;
        }
        std::copy(hashed_key.begin(), hashed_key.end(), key_block.begin());
    } else if (key_size != 0) {
        std::copy_n(key, key_size, key_block.begin());
    }

    std::array<uint8_t, sha256_block_bytes> inner_pad {};
    std::array<uint8_t, sha256_block_bytes> outer_pad {};
    for (size_t index = 0; index < sha256_block_bytes; ++index) {
        inner_pad[index] = static_cast<uint8_t>(key_block[index] ^ 0x36U);
        outer_pad[index] = static_cast<uint8_t>(key_block[index] ^ 0x5cU);
    }

    sha256_t context;
    context_store_format_digest inner_digest {};
    sha256_init(&context);
    sha256_update(&context, inner_pad.data(), inner_pad.size());
    for (size_t index = 0; index < span_count; ++index) {
        if (!valid_span(spans[index])) {
            secure_wipe(&context, sizeof(context));
            secure_wipe(key_block.data(), key_block.size());
            secure_wipe(hashed_key.data(), hashed_key.size());
            secure_wipe(inner_pad.data(), inner_pad.size());
            secure_wipe(outer_pad.data(), outer_pad.size());
            return false;
        }
        if (spans[index].size != 0) {
            sha256_update(&context, spans[index].data, spans[index].size);
        }
    }
    sha256_final(&context, inner_digest.data());

    sha256_init(&context);
    sha256_update(&context, outer_pad.data(), outer_pad.size());
    sha256_update(&context, inner_digest.data(), inner_digest.size());
    sha256_final(&context, tag.data());

    secure_wipe(&context, sizeof(context));
    secure_wipe(key_block.data(), key_block.size());
    secure_wipe(hashed_key.data(), hashed_key.size());
    secure_wipe(inner_pad.data(), inner_pad.size());
    secure_wipe(outer_pad.data(), outer_pad.size());
    secure_wipe(inner_digest.data(), inner_digest.size());
    return true;
}

bool constant_time_equal(
        const context_store_format_digest & left,
        const context_store_format_digest & right) noexcept {
    volatile uint8_t difference = 0;
    for (size_t index = 0; index < left.size(); ++index) {
        difference = static_cast<uint8_t>(difference | (left[index] ^ right[index]));
    }
    return difference == 0;
}

bool registered_id_equal(
        const context_store_registered_id & left,
        const context_store_registered_id & right) noexcept {
    if (left.size != right.size) {
        return false;
    }
    uint8_t difference = 0;
    for (size_t index = 0; index < left.size; ++index) {
        difference = static_cast<uint8_t>(difference | (left.bytes[index] ^ right.bytes[index]));
    }
    return difference == 0;
}

std::array<uint8_t, 8> uint64_be(uint64_t value) noexcept {
    std::array<uint8_t, 8> output {};
    for (size_t index = 0; index < output.size(); ++index) {
        output[output.size() - 1 - index] = static_cast<uint8_t>(value >> (index * 8));
    }
    return output;
}

bool derive_manifest_key(
        const context_store_parsed_manifest & manifest,
        const context_store_manifest_key_record & record,
        context_store_format_digest & derived_key) noexcept {
    std::array<uint8_t, 2> key_id_length = {
        static_cast<uint8_t>(static_cast<uint16_t>(manifest.authentication_key_id.size) >> 8),
        manifest.authentication_key_id.size
    };
    const auto generation = uint64_be(manifest.authentication_key_generation);
    const byte_span spans[] = {
        { reinterpret_cast<const uint8_t *>(manifest_key_domain), sizeof(manifest_key_domain) },
        { key_id_length.data(), key_id_length.size() },
        { reinterpret_cast<const uint8_t *>(manifest.authentication_key_id.bytes.data()), manifest.authentication_key_id.size },
        { manifest.store_uuid.data(), manifest.store_uuid.size() },
        { manifest.scope_namespace.data(), manifest.scope_namespace.size() },
        { generation.data(), generation.size() },
    };
    return hmac_sha256_spans(record.master_key.data, record.master_key.size,
        spans, sizeof(spans) / sizeof(spans[0]), derived_key);
}

bool compatibility_root_from_components(
        const std::array<context_store_format_digest, context_store_compatibility_component_count> & components,
        context_store_format_digest & root) noexcept {
    constexpr uint8_t map_header = 0xb0;
    constexpr std::array<uint8_t, 2> digest_header = { 0x58, 0x20 };
    sha256_t context;
    sha256_init(&context);
    sha256_update(&context, reinterpret_cast<const uint8_t *>(compatibility_domain), sizeof(compatibility_domain));
    sha256_update(&context, &map_header, 1);
    for (size_t index = 0; index < components.size(); ++index) {
        const uint8_t key = static_cast<uint8_t>(index);
        sha256_update(&context, &key, 1);
        sha256_update(&context, digest_header.data(), digest_header.size());
        sha256_update(&context, components[index].data(), components[index].size());
    }
    sha256_final(&context, root.data());
    secure_wipe(&context, sizeof(context));
    return true;
}

bool predecessor_equal(
        const context_store_parsed_manifest & manifest,
        const context_store_replay_anchor & anchor) noexcept {
    return manifest.has_predecessor == anchor.has_predecessor &&
        (!manifest.has_predecessor ||
         manifest.predecessor_manifest_digest == anchor.predecessor_manifest_digest);
}

} // namespace

bool context_store_sha256(
        const uint8_t * data,
        size_t size,
        context_store_format_digest & digest) noexcept {
    const byte_span span { data, size };
    return sha256_spans(&span, 1, digest);
}

bool context_store_hmac_sha256(
        const uint8_t * key,
        size_t key_size,
        const uint8_t * data,
        size_t size,
        context_store_format_digest & tag) noexcept {
    const byte_span span { data, size };
    return hmac_sha256_spans(key, key_size, &span, 1, tag);
}

context_store_manifest_verify_result context_store_verify_manifest_v1(
        const uint8_t * data,
        size_t size,
        const context_store_manifest_verification_policy & policy) noexcept {
    context_store_manifest_verify_result verification;
    const auto parsed = context_store_parse_manifest_v1(data, size);
    verification.parse_status = parsed.status;
    if (parsed.status != context_store_manifest_parse_status::structural_only) {
        return verification;
    }
    const auto & manifest = parsed.manifest;

    if (!registered_id_equal(manifest.authentication_key_id, policy.key.key_id) ||
        policy.key.disposition == context_store_key_disposition::unknown) {
        verification.status = context_store_manifest_verify_status::unknown_key;
        return verification;
    }
    if (policy.key.disposition == context_store_key_disposition::revoked) {
        verification.status = context_store_manifest_verify_status::revoked_key;
        return verification;
    }
    if (policy.key.disposition == context_store_key_disposition::read_disabled) {
        verification.status = context_store_manifest_verify_status::read_disabled_key;
        return verification;
    }
    if (policy.key.disposition != context_store_key_disposition::active) {
        verification.status = context_store_manifest_verify_status::invalid_policy;
        return verification;
    }
    if (manifest.authentication_key_generation != policy.key.generation) {
        verification.status = context_store_manifest_verify_status::key_generation_mismatch;
        return verification;
    }
    if (policy.key.master_key.data == nullptr || policy.key.master_key.size == 0 ||
        policy.key.master_key.size > context_store_master_key_max_bytes) {
        verification.status = context_store_manifest_verify_status::invalid_policy;
        return verification;
    }

    context_store_format_digest derived_key {};
    context_store_format_digest expected_tag {};
    if (!derive_manifest_key(manifest, policy.key, derived_key)) {
        verification.status = context_store_manifest_verify_status::invalid_policy;
        return verification;
    }
    const byte_span authentication_spans[] = {
        { reinterpret_cast<const uint8_t *>(manifest_auth_domain), sizeof(manifest_auth_domain) },
        { data + manifest.authentication_input_offset, manifest.authentication_input_size },
    };
    const bool hmac_ok = hmac_sha256_spans(derived_key.data(), derived_key.size(),
        authentication_spans, sizeof(authentication_spans) / sizeof(authentication_spans[0]), expected_tag);
    secure_wipe(derived_key.data(), derived_key.size());
    if (!hmac_ok || !constant_time_equal(expected_tag, manifest.authentication_tag)) {
        secure_wipe(expected_tag.data(), expected_tag.size());
        verification.status = context_store_manifest_verify_status::authentication_failed;
        return verification;
    }
    secure_wipe(expected_tag.data(), expected_tag.size());

    const auto & anchor = policy.anchor;
    if (manifest.store_uuid != anchor.store_uuid ||
        manifest.scope_namespace != anchor.namespace_id ||
        manifest.policy_epoch != anchor.policy_epoch) {
        verification.status = context_store_manifest_verify_status::authority_mismatch;
        return verification;
    }
    if (manifest.checkpoint_lineage_id != anchor.checkpoint_lineage_id ||
        manifest.authentication_key_generation != anchor.key_generation ||
        manifest.generation != anchor.generation ||
        !predecessor_equal(manifest, anchor)) {
        verification.status = context_store_manifest_verify_status::replay_mismatch;
        return verification;
    }

    const byte_span manifest_spans[] = {
        { reinterpret_cast<const uint8_t *>(manifest_domain), sizeof(manifest_domain) },
        { data, size },
    };
    if (!sha256_spans(manifest_spans, sizeof(manifest_spans) / sizeof(manifest_spans[0]),
            verification.manifest_digest) ||
        verification.manifest_digest != anchor.selected_manifest_digest) {
        verification.status = context_store_manifest_verify_status::replay_mismatch;
        return verification;
    }

    context_store_format_digest computed_compatibility_root {};
    const byte_span compatibility_spans[] = {
        { reinterpret_cast<const uint8_t *>(compatibility_domain), sizeof(compatibility_domain) },
        { data + manifest.compatibility_manifest_offset, manifest.compatibility_manifest_size },
    };
    if (!sha256_spans(compatibility_spans,
            sizeof(compatibility_spans) / sizeof(compatibility_spans[0]),
            computed_compatibility_root) ||
        computed_compatibility_root != manifest.compatibility_root) {
        verification.status = context_store_manifest_verify_status::compatibility_corrupt;
        return verification;
    }
    context_store_format_digest expected_compatibility_root {};
    compatibility_root_from_components(policy.compatibility.components, expected_compatibility_root);
    if (expected_compatibility_root != policy.compatibility.root ||
        manifest.compatibility_root != policy.compatibility.root ||
        manifest.compatibility_components != policy.compatibility.components) {
        verification.status = context_store_manifest_verify_status::compatibility_mismatch;
        return verification;
    }

    verification.status = context_store_manifest_verify_status::authenticated_unadmitted;
    return verification;
}

const char * context_store_manifest_verify_status_name(
        context_store_manifest_verify_status status) noexcept {
    switch (status) {
        case context_store_manifest_verify_status::authenticated_unadmitted: return "authenticated-unadmitted";
        case context_store_manifest_verify_status::structural_rejection:     return "structural-rejection";
        case context_store_manifest_verify_status::invalid_policy:           return "invalid-policy";
        case context_store_manifest_verify_status::unknown_key:              return "unknown-key";
        case context_store_manifest_verify_status::revoked_key:              return "revoked-key";
        case context_store_manifest_verify_status::read_disabled_key:        return "read-disabled-key";
        case context_store_manifest_verify_status::key_generation_mismatch:  return "key-generation-mismatch";
        case context_store_manifest_verify_status::authentication_failed:    return "authentication-failed";
        case context_store_manifest_verify_status::authority_mismatch:       return "authority-mismatch";
        case context_store_manifest_verify_status::replay_mismatch:          return "replay-mismatch";
        case context_store_manifest_verify_status::compatibility_corrupt:    return "compatibility-corrupt";
        case context_store_manifest_verify_status::compatibility_mismatch:   return "compatibility-mismatch";
    }
    return "unknown";
}

} // namespace halofpx
