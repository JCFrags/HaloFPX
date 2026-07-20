#include "halofpx-context-store-v1-transformer-codec.h"

#include <algorithm>
#include <cstring>
#include <limits>
#include <new>
#include <string_view>

namespace halofpx {
namespace {

using bytes = std::vector<uint8_t>;

constexpr std::string_view object_domain = "halofpx.object.v1";
constexpr std::string_view token_stream = "halofpx.tokens.i32be.v1";
constexpr std::string_view state_stream = "halofpx.target-seq-state.opaque.v1";
constexpr std::string_view token_codec = "halofpx.tokens.int32be.v1";
constexpr std::string_view state_codec = "halofpx.llama-state-seq-ext.opaque.v1";
constexpr std::string_view state_profile =
    "halofpx.transformer.world1-rank0.memoryless-greedy.v1";
constexpr std::string_view topology_schema = "halofpx.topology.single-rank.v1";
constexpr std::string_view execution_mode = "single-rank";
constexpr char token_digest_domain[] = "halofpx.token-sequence.int32be.v1";
constexpr char manifest_key_domain[] = "halofpx.manifest-key.v1";
constexpr char manifest_auth_domain[] = "halofpx.manifest-auth.v1";

bool nonzero(const context_store_format_digest & digest) noexcept {
    uint8_t combined = 0;
    for (uint8_t value : digest) combined |= value;
    return combined != 0;
}

bool nonzero(const std::array<uint8_t, 16> & value) noexcept {
    uint8_t combined = 0;
    for (uint8_t byte : value) combined |= byte;
    return combined != 0;
}

bool complete(const context_store_identity & identity) noexcept {
    return nonzero(identity.compatibility_root) && nonzero(identity.scope_namespace) &&
        nonzero(identity.checkpoint_lineage_id);
}

bool same_identity(const context_store_identity & left, const context_store_identity & right) noexcept {
    return left.compatibility_root == right.compatibility_root &&
        left.scope_namespace == right.scope_namespace &&
        left.checkpoint_lineage_id == right.checkpoint_lineage_id &&
        left.policy_epoch == right.policy_epoch;
}

bool valid_id(const context_store_registered_id & id) noexcept {
    if (id.size == 0 || id.size > context_store_registered_id_max_bytes) return false;
    for (size_t i = 0; i < id.size; ++i) {
        const auto byte = static_cast<unsigned char>(id.bytes[i]);
        if (byte == 0 || byte >= 0x80) return false;
    }
    return true;
}

bool same_id(const context_store_registered_id & id, std::string_view expected) noexcept {
    return id.size == expected.size() &&
        std::equal(expected.begin(), expected.end(), id.bytes.begin());
}

bool append(bytes & output, const void * data, size_t size) {
    const auto * first = static_cast<const uint8_t *>(data);
    output.insert(output.end(), first, first + size);
    return true;
}

void append_u16(bytes & output, uint16_t value) {
    output.push_back(static_cast<uint8_t>(value >> 8));
    output.push_back(static_cast<uint8_t>(value));
}

void append_u32(bytes & output, uint32_t value) {
    for (int shift = 24; shift >= 0; shift -= 8) {
        output.push_back(static_cast<uint8_t>(value >> shift));
    }
}

void append_u64(bytes & output, uint64_t value) {
    for (int shift = 56; shift >= 0; shift -= 8) {
        output.push_back(static_cast<uint8_t>(value >> shift));
    }
}

void cbor_head(bytes & output, uint8_t major, uint64_t value) {
    if (value < 24) {
        output.push_back(static_cast<uint8_t>((major << 5) | value));
    } else if (value <= UINT8_MAX) {
        output.push_back(static_cast<uint8_t>((major << 5) | 24));
        output.push_back(static_cast<uint8_t>(value));
    } else if (value <= UINT16_MAX) {
        output.push_back(static_cast<uint8_t>((major << 5) | 25));
        append_u16(output, static_cast<uint16_t>(value));
    } else if (value <= UINT32_MAX) {
        output.push_back(static_cast<uint8_t>((major << 5) | 26));
        append_u32(output, static_cast<uint32_t>(value));
    } else {
        output.push_back(static_cast<uint8_t>((major << 5) | 27));
        append_u64(output, value);
    }
}

void cbor_uint(bytes & output, uint64_t value) { cbor_head(output, 0, value); }
void cbor_map(bytes & output, uint64_t count) { cbor_head(output, 5, count); }
void cbor_array(bytes & output, uint64_t count) { cbor_head(output, 4, count); }

void cbor_bytes(bytes & output, const uint8_t * data, size_t size) {
    cbor_head(output, 2, size);
    append(output, data, size);
}

template <size_t N>
void cbor_bytes(bytes & output, const std::array<uint8_t, N> & value) {
    cbor_bytes(output, value.data(), value.size());
}

void cbor_text(bytes & output, std::string_view value) {
    cbor_head(output, 3, value.size());
    append(output, value.data(), value.size());
}

void cbor_text(bytes & output, const context_store_registered_id & value) {
    cbor_text(output, std::string_view(value.bytes.data(), value.size));
}

context_store_format_digest domain_hash(const char * domain, size_t domain_size, const bytes & body) {
    bytes input;
    input.reserve(domain_size + body.size());
    append(input, domain, domain_size);
    append(input, body.data(), body.size());
    context_store_format_digest output {};
    if (!context_store_sha256(input.data(), input.size(), output)) return {};
    return output;
}

context_store_registered_id make_id(std::string_view value) {
    context_store_registered_id output;
    output.size = static_cast<uint8_t>(value.size());
    std::copy(value.begin(), value.end(), output.bytes.begin());
    return output;
}

bytes make_frame(std::string_view type, const bytes & payload) {
    bytes output;
    output.reserve(8 + 2 + object_domain.size() + 2 + type.size() + 8 + payload.size());
    const uint8_t magic[] = { 0x48, 0x41, 0x4c, 0x4f, 0x4f, 0x42, 0x4a, 0x01 };
    append(output, magic, sizeof(magic));
    append_u16(output, static_cast<uint16_t>(object_domain.size()));
    append(output, object_domain.data(), object_domain.size());
    append_u16(output, static_cast<uint16_t>(type.size()));
    append(output, type.data(), type.size());
    append_u64(output, payload.size());
    append(output, payload.data(), payload.size());
    return output;
}

uint64_t expected_frame_size(std::string_view type, size_t payload_size) noexcept {
    constexpr uint64_t fixed = 8 + 2 + 2 + 8;
    const uint64_t variable = object_domain.size() + type.size();
    if (payload_size > UINT64_MAX - fixed - variable) return 0;
    return fixed + variable + payload_size;
}

bool derive_manifest_key(
        const context_store_v1_transformer_manifest_parameters & parameters,
        const context_store_identity & identity,
        context_store_format_digest & output) {
    bytes input;
    input.reserve(sizeof(manifest_key_domain) + 2 + parameters.signing_key.key_id.size +
        parameters.store_uuid.size() + identity.scope_namespace.size() + 8);
    append(input, manifest_key_domain, sizeof(manifest_key_domain));
    append_u16(input, parameters.signing_key.key_id.size);
    append(input, parameters.signing_key.key_id.bytes.data(), parameters.signing_key.key_id.size);
    append(input, parameters.store_uuid.data(), parameters.store_uuid.size());
    append(input, identity.scope_namespace.data(), identity.scope_namespace.size());
    append_u64(input, parameters.signing_key.generation);
    return context_store_hmac_sha256(
        parameters.signing_key.master_key.data,
        parameters.signing_key.master_key.size,
        input.data(), input.size(), output);
}

void emit_descriptor(
        bytes & body,
        const context_store_format_digest & object_id,
        std::string_view stream,
        std::string_view codec,
        uint64_t frame_bytes,
        const context_store_format_digest & token_digest,
        const context_store_v1_transformer_manifest_parameters & parameters,
        const context_store_identity & identity) {
    cbor_map(body, 13);
    cbor_uint(body, 0); cbor_bytes(body, object_id);
    cbor_uint(body, 1); cbor_text(body, stream);
    cbor_uint(body, 2); cbor_text(body, codec);
    cbor_uint(body, 3); cbor_uint(body, 1);
    cbor_uint(body, 4); cbor_uint(body, 0);
    cbor_uint(body, 5); body.push_back(0xf5);
    cbor_uint(body, 6); cbor_uint(body, frame_bytes);
    cbor_uint(body, 7); cbor_bytes(body, token_digest);
    cbor_uint(body, 8); cbor_uint(body, parameters.logical_position);
    cbor_uint(body, 9); cbor_uint(body, parameters.output_boundary);
    cbor_uint(body, 10); cbor_uint(body, 0);
    cbor_uint(body, 11); cbor_bytes(body, parameters.rank_ownership_digest);
    cbor_uint(body, 12); cbor_bytes(body, identity.compatibility_root);
}

void clear(context_store_v1_transformer_encoded_snapshot & encoded) noexcept {
    encoded.manifest.clear();
    for (auto & frame : encoded.frames) frame.clear();
    encoded.manifest_digest = {};
    encoded.admission_metadata = {};
    encoded.admission_objects = {};
}

} // namespace

context_store_v1_transformer_encode_result context_store_encode_transformer_snapshot_v1(
        const context_store_transformer_snapshot_v1 & snapshot,
        const context_store_v1_transformer_manifest_parameters & parameters,
        const context_store_v1_transformer_codec_limits & limits) noexcept {
    static_assert(sizeof(llama_token) == sizeof(int32_t), "v1 token codec requires int32 llama_token");
    context_store_v1_transformer_encode_result result;
    try {
        if (limits.snapshot.max_tokens == 0 || limits.snapshot.max_state_bytes == 0 ||
                limits.max_frame_bytes == 0 || limits.max_manifest_bytes == 0 ||
                limits.max_manifest_bytes > context_store_manifest_max_bytes ||
                parameters.signing_key.disposition != context_store_key_disposition::active ||
                !valid_id(parameters.signing_key.key_id) || parameters.signing_key.generation == 0 ||
                parameters.signing_key.master_key.data == nullptr ||
                parameters.signing_key.master_key.size == 0 ||
                parameters.signing_key.master_key.size > context_store_master_key_max_bytes ||
                parameters.generation == 0 || parameters.durability_mode > 2 ||
                !nonzero(parameters.store_uuid) || !nonzero(parameters.producer_identity) ||
                !nonzero(parameters.global_plan_digest) ||
                !nonzero(parameters.rank_ownership_digest) ||
                !nonzero(parameters.rank_placement_digest)) {
            result.status = context_store_v1_transformer_codec_status::invalid_argument;
            return result;
        }
        if (!complete(snapshot.compatibility_identity)) {
            result.status = context_store_v1_transformer_codec_status::incomplete_identity;
            return result;
        }
        if (!context_store_transformer_profile_v1_is_admitted(snapshot.profile)) {
            result.status = context_store_v1_transformer_codec_status::unsupported_profile;
            return result;
        }
        if (snapshot.tokens.empty() || snapshot.tokens.size() > limits.snapshot.max_tokens ||
                snapshot.tokens.size() > UINT64_MAX / sizeof(int32_t)) {
            result.status = context_store_v1_transformer_codec_status::token_limit_exceeded;
            return result;
        }
        if (snapshot.state.empty() || snapshot.state.size() > limits.snapshot.max_state_bytes) {
            result.status = context_store_v1_transformer_codec_status::state_limit_exceeded;
            return result;
        }
        if (parameters.logical_position != snapshot.tokens.size() ||
                parameters.output_boundary > parameters.logical_position) {
            result.status = context_store_v1_transformer_codec_status::wrong_boundary;
            return result;
        }
        if (parameters.has_predecessor != nonzero(parameters.predecessor_manifest_digest)) {
            result.status = context_store_v1_transformer_codec_status::invalid_argument;
            return result;
        }
        for (const auto & component : parameters.compatibility_components) {
            if (!nonzero(component)) {
                result.status = context_store_v1_transformer_codec_status::incomplete_identity;
                return result;
            }
        }

        const size_t token_payload_bytes = snapshot.tokens.size() * sizeof(int32_t);
        const uint64_t token_frame_bytes = expected_frame_size(token_stream, token_payload_bytes);
        const uint64_t state_frame_bytes = expected_frame_size(state_stream, snapshot.state.size());
        if (token_frame_bytes == 0 || state_frame_bytes == 0 ||
                token_frame_bytes > limits.max_frame_bytes ||
                state_frame_bytes > limits.max_frame_bytes) {
            result.status = context_store_v1_transformer_codec_status::limit_exceeded;
            return result;
        }

        bytes compatibility;
        cbor_map(compatibility, context_store_compatibility_component_count);
        for (size_t i = 0; i < parameters.compatibility_components.size(); ++i) {
            cbor_uint(compatibility, i);
            cbor_bytes(compatibility, parameters.compatibility_components[i]);
        }
        const auto computed_compatibility =
            domain_hash("halofpx.compat.v1", sizeof("halofpx.compat.v1"), compatibility);
        if (!nonzero(computed_compatibility) ||
                computed_compatibility != snapshot.compatibility_identity.compatibility_root) {
            result.status = context_store_v1_transformer_codec_status::incompatible_identity;
            return result;
        }

        bytes token_payload;
        token_payload.reserve(snapshot.tokens.size() * sizeof(int32_t));
        for (llama_token token : snapshot.tokens) {
            append_u32(token_payload, static_cast<uint32_t>(static_cast<int32_t>(token)));
        }
        const auto token_digest = domain_hash(
            token_digest_domain, sizeof(token_digest_domain), token_payload);
        if (!nonzero(token_digest)) {
            result.status = context_store_v1_transformer_codec_status::internal_error;
            return result;
        }

        result.encoded.frames[0] = make_frame(token_stream, token_payload);
        result.encoded.frames[1] = make_frame(state_stream, snapshot.state);
        for (const auto & frame : result.encoded.frames) {
            if (frame.empty() || frame.size() > limits.max_frame_bytes) {
                result.status = context_store_v1_transformer_codec_status::limit_exceeded;
                clear(result.encoded);
                return result;
            }
        }
        std::array<context_store_format_digest, context_store_v1_transformer_frame_count> object_ids {};
        for (size_t i = 0; i < object_ids.size(); ++i) {
            if (!context_store_sha256(result.encoded.frames[i].data(),
                    result.encoded.frames[i].size(), object_ids[i])) {
                result.status = context_store_v1_transformer_codec_status::internal_error;
                clear(result.encoded);
                return result;
            }
        }

        bytes body;
        cbor_map(body, 15);
        cbor_uint(body, 0); cbor_uint(body, 1);
        cbor_uint(body, 1); cbor_uint(body, 0);
        cbor_uint(body, 2); cbor_bytes(body, parameters.store_uuid);
        cbor_uint(body, 3); cbor_bytes(body, snapshot.compatibility_identity.checkpoint_lineage_id);
        cbor_uint(body, 4); cbor_uint(body, parameters.generation);
        cbor_uint(body, 5);
        if (parameters.has_predecessor) cbor_bytes(body, parameters.predecessor_manifest_digest);
        else body.push_back(0xf6);
        cbor_uint(body, 6); append(body, compatibility.data(), compatibility.size());
        cbor_uint(body, 7); cbor_bytes(body, snapshot.compatibility_identity.compatibility_root);
        cbor_uint(body, 8); cbor_bytes(body, snapshot.compatibility_identity.scope_namespace);
        cbor_uint(body, 9); cbor_uint(body, snapshot.compatibility_identity.policy_epoch);
        cbor_uint(body, 10); cbor_map(body, 6);
        cbor_uint(body, 0); cbor_text(body, topology_schema);
        cbor_uint(body, 1); cbor_text(body, execution_mode);
        cbor_uint(body, 2); cbor_uint(body, 1);
        cbor_uint(body, 3); cbor_bytes(body, parameters.global_plan_digest);
        cbor_uint(body, 4); cbor_uint(body, parameters.topology_epoch);
        cbor_uint(body, 5); cbor_array(body, 1); cbor_map(body, 3);
        cbor_uint(body, 0); cbor_uint(body, 0);
        cbor_uint(body, 1); cbor_bytes(body, parameters.rank_ownership_digest);
        cbor_uint(body, 2); cbor_bytes(body, parameters.rank_placement_digest);
        cbor_uint(body, 11); cbor_text(body, state_profile);
        cbor_uint(body, 12); cbor_array(body, context_store_v1_transformer_frame_count);
        emit_descriptor(body, object_ids[0], token_stream, token_codec,
            result.encoded.frames[0].size(), token_digest, parameters,
            snapshot.compatibility_identity);
        emit_descriptor(body, object_ids[1], state_stream, state_codec,
            result.encoded.frames[1].size(), token_digest, parameters,
            snapshot.compatibility_identity);
        cbor_uint(body, 13); cbor_bytes(body, parameters.producer_identity);
        cbor_uint(body, 14); cbor_uint(body, parameters.durability_mode);

        bytes auth_input;
        cbor_map(auth_input, 4);
        cbor_uint(auth_input, 0); append(auth_input, body.data(), body.size());
        cbor_uint(auth_input, 1); cbor_text(auth_input, parameters.signing_key.key_id);
        cbor_uint(auth_input, 2); cbor_uint(auth_input, 1);
        cbor_uint(auth_input, 3); cbor_uint(auth_input, parameters.signing_key.generation);

        bytes auth_message;
        auth_message.reserve(sizeof(manifest_auth_domain) + auth_input.size());
        append(auth_message, manifest_auth_domain, sizeof(manifest_auth_domain));
        append(auth_message, auth_input.data(), auth_input.size());
        context_store_format_digest derived_key {};
        if (!derive_manifest_key(parameters, snapshot.compatibility_identity, derived_key)) {
            std::fill(derived_key.begin(), derived_key.end(), 0);
            result.status = context_store_v1_transformer_codec_status::authentication_failed;
            clear(result.encoded);
            return result;
        }
        context_store_format_digest tag {};
        const bool signed_ok = context_store_hmac_sha256(
            derived_key.data(), derived_key.size(), auth_message.data(), auth_message.size(), tag);
        std::fill(derived_key.begin(), derived_key.end(), 0);
        if (!signed_ok) {
            result.status = context_store_v1_transformer_codec_status::authentication_failed;
            clear(result.encoded);
            return result;
        }
        cbor_map(result.encoded.manifest, 2);
        cbor_uint(result.encoded.manifest, 0);
        append(result.encoded.manifest, auth_input.data(), auth_input.size());
        cbor_uint(result.encoded.manifest, 1); cbor_bytes(result.encoded.manifest, tag);
        if (result.encoded.manifest.size() > limits.max_manifest_bytes ||
                !context_store_manifest_digest_v1(
                    result.encoded.manifest.data(), result.encoded.manifest.size(),
                    result.encoded.manifest_digest)) {
            result.status = context_store_v1_transformer_codec_status::limit_exceeded;
            clear(result.encoded);
            return result;
        }

        context_store_manifest_verification_policy policy;
        policy.key = parameters.signing_key;
        policy.anchor.store_uuid = parameters.store_uuid;
        policy.anchor.checkpoint_lineage_id = snapshot.compatibility_identity.checkpoint_lineage_id;
        policy.anchor.namespace_id = snapshot.compatibility_identity.scope_namespace;
        policy.anchor.policy_epoch = snapshot.compatibility_identity.policy_epoch;
        policy.anchor.key_generation = parameters.signing_key.generation;
        policy.anchor.generation = parameters.generation;
        policy.anchor.has_predecessor = parameters.has_predecessor;
        policy.anchor.predecessor_manifest_digest = parameters.predecessor_manifest_digest;
        policy.anchor.selected_manifest_digest = result.encoded.manifest_digest;
        policy.compatibility.components = parameters.compatibility_components;
        policy.compatibility.root = snapshot.compatibility_identity.compatibility_root;
        const auto verified = context_store_verify_manifest_v1(
            result.encoded.manifest.data(), result.encoded.manifest.size(), policy);
        if (verified.status != context_store_manifest_verify_status::authenticated_unadmitted ||
                verified.authenticated_object_count() != context_store_v1_transformer_frame_count ||
                verified.authenticated_manifest_metadata() == nullptr) {
            result.status = context_store_v1_transformer_codec_status::authentication_failed;
            clear(result.encoded);
            return result;
        }
        result.encoded.admission_metadata = *verified.authenticated_manifest_metadata();
        for (size_t i = 0; i < result.encoded.admission_objects.size(); ++i) {
            const auto * descriptor = verified.authenticated_object_reference(i);
            if (descriptor == nullptr) {
                result.status = context_store_v1_transformer_codec_status::authentication_failed;
                clear(result.encoded);
                return result;
            }
            result.encoded.admission_objects[i] = *descriptor;
        }
        result.status = context_store_v1_transformer_codec_status::encoded;
        return result;
    } catch (const std::bad_alloc &) {
        result.status = context_store_v1_transformer_codec_status::allocation_failed;
    } catch (...) {
        result.status = context_store_v1_transformer_codec_status::internal_error;
    }
    clear(result.encoded);
    return result;
}

context_store_v1_transformer_decode_result context_store_decode_transformer_snapshot_v1(
        const context_store_v1_transformer_decode_request & request) noexcept {
    context_store_v1_transformer_decode_result result;
    try {
        if (request.candidate == nullptr || request.expected_tokens == nullptr ||
                request.expected_token_count == 0 || request.limits.max_tokens == 0 ||
                request.limits.max_state_bytes == 0 || !nonzero(request.producer_identity) ||
                !nonzero(request.rank_ownership_digest)) {
            result.status = context_store_v1_transformer_codec_status::invalid_argument;
            return result;
        }
        if (!complete(request.compatibility_identity)) {
            result.status = context_store_v1_transformer_codec_status::incomplete_identity;
            return result;
        }
        if (!context_store_transformer_profile_v1_is_admitted(request.profile)) {
            result.status = context_store_v1_transformer_codec_status::unsupported_profile;
            return result;
        }
        if (request.expected_token_count > request.limits.max_tokens) {
            result.status = context_store_v1_transformer_codec_status::token_limit_exceeded;
            return result;
        }
        if (request.logical_position != request.expected_token_count ||
                request.output_boundary > request.logical_position) {
            result.status = context_store_v1_transformer_codec_status::wrong_boundary;
            return result;
        }
        const auto & candidate = *request.candidate;
        if (!same_identity(candidate.identity(), request.compatibility_identity)) {
            result.status = context_store_v1_transformer_codec_status::incompatible_identity;
            return result;
        }
        if (candidate.producer_identity() != request.producer_identity) {
            result.status = context_store_v1_transformer_codec_status::incompatible_runtime;
            return result;
        }
        if (!same_id(candidate.state_profile_id(), state_profile)) {
            result.status = context_store_v1_transformer_codec_status::wrong_profile;
            return result;
        }
        if (candidate.world_size() != 1 || candidate.topology_epoch() != request.topology_epoch ||
                candidate.object_count() != context_store_v1_transformer_frame_count) {
            result.status = context_store_v1_transformer_codec_status::wrong_object_roster;
            return result;
        }

        const auto * tokens_descriptor = candidate.descriptor(0);
        const auto * state_descriptor = candidate.descriptor(1);
        const auto tokens_payload = candidate.payload(0);
        const auto state_payload = candidate.payload(1);
        if (tokens_descriptor == nullptr || state_descriptor == nullptr ||
                tokens_payload.data == nullptr || state_payload.data == nullptr ||
                tokens_payload.size == 0 || state_payload.size == 0) {
            result.status = context_store_v1_transformer_codec_status::corrupt_payload;
            return result;
        }
        if (!same_id(tokens_descriptor->stream_type, token_stream) ||
                !same_id(tokens_descriptor->codec_id, token_codec) ||
                !same_id(state_descriptor->stream_type, state_stream) ||
                !same_id(state_descriptor->codec_id, state_codec) ||
                !tokens_descriptor->required || !state_descriptor->required ||
                !tokens_descriptor->has_logical_rank || !state_descriptor->has_logical_rank ||
                tokens_descriptor->logical_rank != 0 || state_descriptor->logical_rank != 0 ||
                tokens_descriptor->rank_ownership_digest != request.rank_ownership_digest ||
                state_descriptor->rank_ownership_digest != request.rank_ownership_digest ||
                tokens_descriptor->compatibility_root != request.compatibility_identity.compatibility_root ||
                state_descriptor->compatibility_root != request.compatibility_identity.compatibility_root) {
            result.status = context_store_v1_transformer_codec_status::wrong_descriptor;
            return result;
        }
        if (tokens_descriptor->codec_schema_major != 1 ||
                tokens_descriptor->codec_schema_minor != 0 ||
                state_descriptor->codec_schema_major != 1 ||
                state_descriptor->codec_schema_minor != 0) {
            result.status = context_store_v1_transformer_codec_status::wrong_schema;
            return result;
        }
        if (tokens_descriptor->logical_position != request.logical_position ||
                state_descriptor->logical_position != request.logical_position ||
                tokens_descriptor->output_boundary != request.output_boundary ||
                state_descriptor->output_boundary != request.output_boundary ||
                tokens_descriptor->frame_bytes != expected_frame_size(token_stream, tokens_payload.size) ||
                state_descriptor->frame_bytes != expected_frame_size(state_stream, state_payload.size)) {
            result.status = context_store_v1_transformer_codec_status::wrong_boundary;
            return result;
        }
        if (tokens_payload.size % sizeof(int32_t) != 0 ||
                tokens_payload.size / sizeof(int32_t) != request.expected_token_count) {
            result.status = context_store_v1_transformer_codec_status::token_mismatch;
            return result;
        }
        if (state_payload.size > request.limits.max_state_bytes) {
            result.status = context_store_v1_transformer_codec_status::state_limit_exceeded;
            return result;
        }
        bytes token_bytes(tokens_payload.data, tokens_payload.data + tokens_payload.size);
        const auto token_digest = domain_hash(
            token_digest_domain, sizeof(token_digest_domain), token_bytes);
        if (!nonzero(token_digest) || tokens_descriptor->token_sequence_digest != token_digest ||
                state_descriptor->token_sequence_digest != token_digest) {
            result.status = context_store_v1_transformer_codec_status::token_mismatch;
            return result;
        }

        std::vector<llama_token> decoded_tokens;
        decoded_tokens.reserve(request.expected_token_count);
        for (size_t offset = 0; offset < tokens_payload.size; offset += 4) {
            const uint32_t bits =
                (static_cast<uint32_t>(tokens_payload.data[offset]) << 24) |
                (static_cast<uint32_t>(tokens_payload.data[offset + 1]) << 16) |
                (static_cast<uint32_t>(tokens_payload.data[offset + 2]) << 8) |
                static_cast<uint32_t>(tokens_payload.data[offset + 3]);
            const auto token = static_cast<llama_token>(static_cast<int32_t>(bits));
            if (token != request.expected_tokens[offset / 4]) {
                result.status = context_store_v1_transformer_codec_status::token_mismatch;
                return result;
            }
            decoded_tokens.push_back(token);
        }
        std::vector<uint8_t> decoded_state(state_payload.data, state_payload.data + state_payload.size);
        result.snapshot.compatibility_identity = request.compatibility_identity;
        result.snapshot.profile = request.profile;
        result.snapshot.tokens = std::move(decoded_tokens);
        result.snapshot.state = std::move(decoded_state);
        result.status = context_store_v1_transformer_codec_status::decoded;
        return result;
    } catch (const std::bad_alloc &) {
        result.status = context_store_v1_transformer_codec_status::allocation_failed;
    } catch (...) {
        result.status = context_store_v1_transformer_codec_status::internal_error;
    }
    result.snapshot = {};
    return result;
}

const char * context_store_v1_transformer_codec_status_name(
        context_store_v1_transformer_codec_status status) noexcept {
    switch (status) {
        case context_store_v1_transformer_codec_status::encoded: return "encoded";
        case context_store_v1_transformer_codec_status::decoded: return "decoded";
        case context_store_v1_transformer_codec_status::invalid_argument: return "invalid_argument";
        case context_store_v1_transformer_codec_status::unsupported_profile: return "unsupported_profile";
        case context_store_v1_transformer_codec_status::incomplete_identity: return "incomplete_identity";
        case context_store_v1_transformer_codec_status::incompatible_identity: return "incompatible_identity";
        case context_store_v1_transformer_codec_status::incompatible_runtime: return "incompatible_runtime";
        case context_store_v1_transformer_codec_status::wrong_profile: return "wrong_profile";
        case context_store_v1_transformer_codec_status::wrong_object_roster: return "wrong_object_roster";
        case context_store_v1_transformer_codec_status::wrong_descriptor: return "wrong_descriptor";
        case context_store_v1_transformer_codec_status::wrong_schema: return "wrong_schema";
        case context_store_v1_transformer_codec_status::wrong_boundary: return "wrong_boundary";
        case context_store_v1_transformer_codec_status::token_mismatch: return "token_mismatch";
        case context_store_v1_transformer_codec_status::token_limit_exceeded: return "token_limit_exceeded";
        case context_store_v1_transformer_codec_status::state_limit_exceeded: return "state_limit_exceeded";
        case context_store_v1_transformer_codec_status::corrupt_payload: return "corrupt_payload";
        case context_store_v1_transformer_codec_status::authentication_failed: return "authentication_failed";
        case context_store_v1_transformer_codec_status::limit_exceeded: return "limit_exceeded";
        case context_store_v1_transformer_codec_status::allocation_failed: return "allocation_failed";
        case context_store_v1_transformer_codec_status::internal_error: return "internal_error";
    }
    return "unknown";
}

} // namespace halofpx
