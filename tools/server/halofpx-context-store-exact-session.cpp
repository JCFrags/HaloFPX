#include "halofpx-context-store-exact-session.h"

extern "C" {
#include "sha256/sha256.h"
}

#include <array>
#include <climits>
#include <cstddef>
#include <cstdint>

namespace halofpx {
namespace {

constexpr uint8_t domain[] = {
    'h','a','l','o','f','p','x','.',
    'e','x','a','c','t','-','s','e','s','s','i','o','n','.',
    'v','1',0
};

class hmac_sha256_stream {
public:
    bool begin(const uint8_t * key, size_t size) noexcept {
        if (key == nullptr || size != context_store_exact_session_key_bytes) return false;
        std::array<uint8_t, 64> pad {};
        for (size_t index = 0; index < pad.size(); ++index) {
            const uint8_t byte = index < size ? key[index] : 0;
            pad[index] = static_cast<uint8_t>(byte ^ 0x36U);
        }
        sha256_init(&inner_);
        sha256_update(&inner_, pad.data(), pad.size());
        for (size_t index = 0; index < pad.size(); ++index) {
            const uint8_t byte = index < size ? key[index] : 0;
            pad[index] = static_cast<uint8_t>(byte ^ 0x5cU);
        }
        sha256_init(&outer_);
        sha256_update(&outer_, pad.data(), pad.size());
        wipe(pad.data(), pad.size());
        active_ = true;
        return true;
    }

    void update(const uint8_t * data, size_t size) noexcept {
        if (active_ && size != 0) sha256_update(&inner_, data, size);
    }

    bool finish(context_store_format_digest & output) noexcept {
        if (!active_) return false;
        context_store_format_digest inner_digest {};
        sha256_final(&inner_, inner_digest.data());
        sha256_update(&outer_, inner_digest.data(), inner_digest.size());
        sha256_final(&outer_, output.data());
        wipe(inner_digest.data(), inner_digest.size());
        wipe(&inner_, sizeof(inner_));
        wipe(&outer_, sizeof(outer_));
        active_ = false;
        return true;
    }

    ~hmac_sha256_stream() {
        wipe(&inner_, sizeof(inner_));
        wipe(&outer_, sizeof(outer_));
    }

private:
    static void wipe(void * memory, size_t size) noexcept {
        volatile uint8_t * bytes = static_cast<volatile uint8_t *>(memory);
        while (size-- != 0) *bytes++ = 0;
    }

    sha256_t inner_ {};
    sha256_t outer_ {};
    bool active_ = false;
};

bool nonzero(const context_store_format_digest & value) noexcept {
    uint8_t combined = 0;
    for (const uint8_t byte : value) combined |= byte;
    return combined != 0;
}

bool nonzero_key(const context_store_exact_session_key_view & value) noexcept {
    if (value.data == nullptr || value.size != context_store_exact_session_key_bytes) return false;
    uint8_t combined = 0;
    for (size_t index = 0; index < value.size; ++index) combined |= value.data[index];
    return combined != 0;
}

void append_head(hmac_sha256_stream & stream, uint8_t major, uint64_t value) noexcept {
    std::array<uint8_t, 9> bytes {};
    size_t size = 0;
    if (value < 24) {
        bytes[size++] = static_cast<uint8_t>((major << 5) | value);
    } else if (value <= UINT8_MAX) {
        bytes[size++] = static_cast<uint8_t>((major << 5) | 24);
        bytes[size++] = static_cast<uint8_t>(value);
    } else if (value <= UINT16_MAX) {
        bytes[size++] = static_cast<uint8_t>((major << 5) | 25);
        bytes[size++] = static_cast<uint8_t>(value >> 8);
        bytes[size++] = static_cast<uint8_t>(value);
    } else if (value <= UINT32_MAX) {
        bytes[size++] = static_cast<uint8_t>((major << 5) | 26);
        for (size_t index = 0; index < 4; ++index) {
            bytes[size++] = static_cast<uint8_t>(value >> ((3 - index) * 8));
        }
    } else {
        bytes[size++] = static_cast<uint8_t>((major << 5) | 27);
        for (size_t index = 0; index < 8; ++index) {
            bytes[size++] = static_cast<uint8_t>(value >> ((7 - index) * 8));
        }
    }
    stream.update(bytes.data(), size);
}

void append_unsigned(hmac_sha256_stream & stream, uint64_t value) noexcept {
    append_head(stream, 0, value);
}

void append_digest(hmac_sha256_stream & stream, const context_store_format_digest & value) noexcept {
    append_head(stream, 2, value.size());
    stream.update(value.data(), value.size());
}

} // namespace

context_store_exact_session_result_v1 context_store_resolve_exact_session_v1(
        const context_store_exact_session_inputs_v1 & inputs) noexcept {
    context_store_exact_session_result_v1 result;
    if (!nonzero_key(inputs.derivation_key)) {
        return result;
    }
    if (!nonzero(inputs.scope_namespace)) {
        result.status = context_store_exact_session_status_v1::invalid_scope_namespace;
        return result;
    }
    if (!nonzero(inputs.compatibility_root)) {
        result.status = context_store_exact_session_status_v1::invalid_compatibility_root;
        return result;
    }
    if (inputs.tokens == nullptr || inputs.token_count == 0 ||
        inputs.token_count > context_store_exact_session_max_tokens) {
        result.status = context_store_exact_session_status_v1::invalid_tokens;
        return result;
    }
    for (size_t index = 0; index < inputs.token_count; ++index) {
        if (inputs.tokens[index] < 0) {
            result.status = context_store_exact_session_status_v1::invalid_tokens;
            return result;
        }
    }
    if (inputs.logical_boundary == 0 || inputs.output_boundary == 0 ||
        inputs.logical_boundary != inputs.token_count ||
        inputs.output_boundary > inputs.logical_boundary) {
        result.status = context_store_exact_session_status_v1::invalid_boundaries;
        return result;
    }
    if (inputs.profile != context_store_exact_session_profile_v1::target_only_greedy_memoryless) {
        result.status = context_store_exact_session_status_v1::invalid_profile;
        return result;
    }
    if (!nonzero(inputs.global_plan_digest) ||
        !nonzero(inputs.rank_ownership_digest) ||
        !nonzero(inputs.rank_placement_digest)) {
        result.status = context_store_exact_session_status_v1::invalid_topology_digest;
        return result;
    }
    if (inputs.topology_epoch == 0 || inputs.world_size == 0 ||
        inputs.world_size > context_store_manifest_max_ranks || inputs.rank >= inputs.world_size) {
        result.status = context_store_exact_session_status_v1::invalid_topology;
        return result;
    }

    hmac_sha256_stream stream;
    if (!stream.begin(inputs.derivation_key.data, inputs.derivation_key.size)) {
        result.status = context_store_exact_session_status_v1::derivation_failed;
        return result;
    }
    stream.update(domain, sizeof(domain));
    append_head(stream, 5, 13);
    append_unsigned(stream, 0); append_digest(stream, inputs.scope_namespace);
    append_unsigned(stream, 1); append_digest(stream, inputs.compatibility_root);
    append_unsigned(stream, 2);
    append_head(stream, 2, inputs.token_count * 4ULL);
    for (size_t index = 0; index < inputs.token_count; ++index) {
        const uint32_t token = static_cast<uint32_t>(inputs.tokens[index]);
        const std::array<uint8_t, 4> encoded = {
            static_cast<uint8_t>(token >> 24),
            static_cast<uint8_t>(token >> 16),
            static_cast<uint8_t>(token >> 8),
            static_cast<uint8_t>(token),
        };
        stream.update(encoded.data(), encoded.size());
    }
    append_unsigned(stream, 3); append_unsigned(stream, inputs.logical_boundary);
    append_unsigned(stream, 4); append_unsigned(stream, inputs.output_boundary);
    append_unsigned(stream, 5); append_unsigned(stream, static_cast<uint8_t>(inputs.profile));
    append_unsigned(stream, 6); append_digest(stream, inputs.global_plan_digest);
    append_unsigned(stream, 7); append_digest(stream, inputs.rank_ownership_digest);
    append_unsigned(stream, 8); append_digest(stream, inputs.rank_placement_digest);
    append_unsigned(stream, 9); append_unsigned(stream, inputs.topology_epoch);
    append_unsigned(stream, 10); append_unsigned(stream, inputs.world_size);
    append_unsigned(stream, 11); append_unsigned(stream, inputs.rank);
    append_unsigned(stream, 12); append_unsigned(stream, 1); // exact-token schema v1

    if (!stream.finish(result.session_id)) {
        result.session_id.fill(0);
        result.status = context_store_exact_session_status_v1::derivation_failed;
        return result;
    }
    result.status = context_store_exact_session_status_v1::resolved;
    return result;
}

const char * context_store_exact_session_status_v1_name(
        context_store_exact_session_status_v1 status) noexcept {
    switch (status) {
        case context_store_exact_session_status_v1::resolved:                   return "resolved";
        case context_store_exact_session_status_v1::invalid_key:                return "invalid-key";
        case context_store_exact_session_status_v1::invalid_scope_namespace:    return "invalid-scope-namespace";
        case context_store_exact_session_status_v1::invalid_compatibility_root: return "invalid-compatibility-root";
        case context_store_exact_session_status_v1::invalid_tokens:             return "invalid-tokens";
        case context_store_exact_session_status_v1::invalid_boundaries:         return "invalid-boundaries";
        case context_store_exact_session_status_v1::invalid_profile:            return "invalid-profile";
        case context_store_exact_session_status_v1::invalid_topology_digest:    return "invalid-topology-digest";
        case context_store_exact_session_status_v1::invalid_topology:           return "invalid-topology";
        case context_store_exact_session_status_v1::derivation_failed:          return "derivation-failed";
    }
    return "unknown";
}

} // namespace halofpx
