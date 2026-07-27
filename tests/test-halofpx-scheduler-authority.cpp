#include "ggml.h"
#include "ggml-backend.h"
#include "ggml-rpc.h"
extern "C" {
#include "../examples/gguf-hash/deps/sha256/sha256.h"
}

#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>

static constexpr char AUTH_DOMAIN[] = "halofpx.scheduler-execution-authority.v2";

static uint16_t read_u16(const uint8_t * p) {
    return static_cast<uint16_t>(p[0]) | static_cast<uint16_t>(p[1]) << 8;
}

static uint32_t read_u32(const uint8_t * p) {
    return static_cast<uint32_t>(p[0]) | static_cast<uint32_t>(p[1]) << 8 |
        static_cast<uint32_t>(p[2]) << 16 | static_cast<uint32_t>(p[3]) << 24;
}

static uint64_t read_u64(const uint8_t * p) {
    uint64_t value = 0;
    for (uint32_t i = 0; i < 8; ++i) value |= static_cast<uint64_t>(p[i]) << (8 * i);
    return value;
}

static void write_u16(uint8_t * p, uint16_t value) {
    p[0] = static_cast<uint8_t>(value);
    p[1] = static_cast<uint8_t>(value >> 8);
}

static void write_u32(uint8_t * p, uint32_t value) {
    for (uint32_t i = 0; i < 4; ++i) p[i] = static_cast<uint8_t>(value >> (8 * i));
}

static void write_u64(uint8_t * p, uint64_t value) {
    for (uint32_t i = 0; i < 8; ++i) p[i] = static_cast<uint8_t>(value >> (8 * i));
}

template<typename T>
static void append_le(std::vector<uint8_t> & out, T value) {
    using U = typename std::make_unsigned<T>::type;
    const U v = static_cast<U>(value);
    for (size_t i = 0; i < sizeof(T); ++i) out.push_back(static_cast<uint8_t>(v >> (8 * i)));
}

static std::array<uint8_t, 32> auth_hmac(
        const std::array<uint8_t, 32> & key,
        const uint8_t * data,
        size_t size) {
    std::array<uint8_t, 64> inner {};
    std::array<uint8_t, 64> outer {};
    for (size_t i = 0; i < inner.size(); ++i) {
        const uint8_t byte = i < key.size() ? key[i] : 0;
        inner[i] = byte ^ 0x36;
        outer[i] = byte ^ 0x5c;
    }
    std::array<uint8_t, 32> middle {};
    std::array<uint8_t, 32> result {};
    sha256_t ctx;
    sha256_init(&ctx);
    sha256_update(&ctx, inner.data(), inner.size());
    sha256_update(&ctx, reinterpret_cast<const uint8_t *>(AUTH_DOMAIN), sizeof(AUTH_DOMAIN) - 1);
    sha256_update(&ctx, data, size);
    sha256_final(&ctx, middle.data());
    sha256_init(&ctx);
    sha256_update(&ctx, outer.data(), outer.size());
    sha256_update(&ctx, middle.data(), middle.size());
    sha256_final(&ctx, result.data());
    return result;
}

struct parsed_event {
    uint16_t type;
    const uint8_t * body;
    uint32_t body_size;
    uint32_t offset;
};

struct run_result {
    std::array<float, 32> values {};
    struct ggml_backend_sched_authority_result authority {};
    std::array<uint8_t, 65536> transcript {};
    bool authority_available = false;
};

static uint32_t transcript_event_offset(const run_result & result, uint32_t wanted) {
    uint32_t offset = 0;
    for (uint32_t index = 0; offset < result.authority.exported_size; ++index) {
        if (index == wanted) return offset;
        if (result.authority.exported_size - offset < 88) return UINT32_MAX;
        const uint64_t total = UINT64_C(88) + read_u32(result.transcript.data() + offset + 8);
        if (total > result.authority.exported_size - offset) return UINT32_MAX;
        offset += static_cast<uint32_t>(total);
    }
    return UINT32_MAX;
}

static bool resign_transcript(run_result & result, const std::array<uint8_t, 32> & key) {
    std::vector<uint8_t> seed;
    seed.insert(seed.end(), AUTH_DOMAIN, AUTH_DOMAIN + sizeof(AUTH_DOMAIN) - 1);
    seed.insert(seed.end(), result.authority.attempt_nonce, result.authority.attempt_nonce + 32);
    append_le<uint64_t>(seed, result.authority.execution_sequence);
    auto chain = auth_hmac(key, seed.data(), seed.size());
    uint32_t offset = 0;
    while (offset < result.authority.exported_size) {
        uint8_t * record = result.transcript.data() + offset;
        const uint32_t body_size = read_u32(record + 8);
        const uint64_t total = UINT64_C(88) + body_size;
        if (total > result.authority.exported_size - offset) return false;
        std::memcpy(record + 24, chain.data(), chain.size());
        if (read_u16(record + 6) == 0xffff && body_size == 52) {
            std::memcpy(record + 56 + 20, chain.data(), chain.size());
        }
        chain = auth_hmac(key, record, 56 + body_size);
        std::memcpy(record + 56 + body_size, chain.data(), chain.size());
        offset += static_cast<uint32_t>(total);
    }
    std::memcpy(result.authority.chain_root, chain.data(), chain.size());
    std::vector<uint8_t> canonical;
    append_le<uint16_t>(canonical, result.authority.major);
    append_le<uint16_t>(canonical, result.authority.minor);
    append_le<uint32_t>(canonical, result.authority.encoded_size);
    append_le<uint32_t>(canonical, result.authority.status);
    append_le<uint32_t>(canonical, result.authority.event_count);
    append_le<uint32_t>(canonical, result.authority.split_count);
    append_le<uint32_t>(canonical, result.authority.copy_map_count);
    append_le<uint32_t>(canonical, result.authority.verified_copy_count);
    append_le<uint32_t>(canonical, result.authority.verified_partial_count);
    append_le<uint32_t>(canonical, result.authority.exported_size);
    append_le<uint32_t>(canonical, result.authority.trailer_offset);
    append_le<uint64_t>(canonical, result.authority.execution_sequence);
    canonical.insert(canonical.end(), result.authority.attempt_nonce, result.authority.attempt_nonce + 32);
    canonical.insert(canonical.end(), result.authority.chain_root, result.authority.chain_root + 32);
    canonical.resize(canonical.size() + 32, 0);
    const auto tag = auth_hmac(key, canonical.data(), canonical.size());
    std::memcpy(result.authority.tag, tag.data(), tag.size());
    return true;
}

static bool verify_transcript(
        const run_result & result,
        const std::array<uint8_t, 32> & key,
        uint64_t execution_sequence,
        bool expert) {
    std::vector<parsed_event> events;
    std::array<uint8_t, 32> chain {};
    std::array<uint8_t, 32> nonce {};
    for (uint32_t i = 0; i < 32; ++i) nonce[i] = static_cast<uint8_t>((expert ? 0x20 : 0x40) + i);
    std::vector<uint8_t> seed;
    seed.insert(seed.end(), AUTH_DOMAIN, AUTH_DOMAIN + sizeof(AUTH_DOMAIN) - 1);
    seed.insert(seed.end(), nonce.begin(), nonce.end());
    for (uint32_t i = 0; i < 8; ++i) seed.push_back(static_cast<uint8_t>(execution_sequence >> (8 * i)));
    chain = auth_hmac(key, seed.data(), seed.size());

    uint32_t offset = 0;
    uint32_t index = 0;
    uint16_t phase = 0;
    while (offset < result.authority.exported_size) {
        constexpr uint32_t header = 56;
        if (result.authority.exported_size - offset < header + 32) return false;
        const uint8_t * record = result.transcript.data() + offset;
        const uint32_t body_size = read_u32(record + 8);
        const uint64_t sequence = read_u64(record + 12);
        const uint32_t event_index = read_u32(record + 20);
        const uint64_t total = static_cast<uint64_t>(header) + body_size + 32;
        if (read_u32(record) != 0x32534148 || read_u16(record + 4) != 1 ||
            sequence != execution_sequence ||
            event_index != index || total > result.authority.exported_size - offset ||
            std::memcmp(record + 24, chain.data(), chain.size()) != 0) return false;
        const auto tag = auth_hmac(key, record, header + body_size);
        if (std::memcmp(record + header + body_size, tag.data(), tag.size()) != 0) return false;
        const uint16_t type = read_u16(record + 6);
        if (!((type >= 1 && type <= 8) || type == 0xffff)) return false;
        const uint16_t event_phase =
            type == 1 ? 0 : type == 2 ? 1 : type == 3 ? 2 : type == 4 ? 3 :
            type == 0xffff ? 5 : 4;
        if (event_phase < phase || (type == 0xffff && offset + total != result.authority.exported_size)) return false;
        phase = event_phase;
        chain = tag;
        events.push_back({ type, record + header, body_size, offset });
        offset += static_cast<uint32_t>(total);
        ++index;
    }
    if (offset != result.authority.exported_size || index != result.authority.event_count ||
        std::memcmp(chain.data(), result.authority.chain_root, chain.size()) != 0 ||
        events.empty() || events.back().type != 0xffff ||
        events.back().offset != result.authority.trailer_offset) return false;
    const auto & trailer = events.back();
    if (trailer.body_size != 52 ||
        read_u32(trailer.body + 0) != result.authority.split_count ||
        read_u32(trailer.body + 4) != result.authority.copy_map_count ||
        read_u32(trailer.body + 8) != result.authority.verified_copy_count ||
        read_u32(trailer.body + 12) != result.authority.verified_partial_count ||
        read_u32(trailer.body + 16) + 1 != result.authority.event_count ||
        std::memcmp(trailer.body + 20, result.transcript.data() + trailer.offset + 24, 32) != 0) {
        std::fprintf(stderr, "trailer verification failed expert=%d\n", expert ? 1 : 0);
        return false;
    }
    std::vector<uint8_t> result_record;
    append_le<uint16_t>(result_record, result.authority.major);
    append_le<uint16_t>(result_record, result.authority.minor);
    append_le<uint32_t>(result_record, result.authority.encoded_size);
    append_le<uint32_t>(result_record, result.authority.status);
    append_le<uint32_t>(result_record, result.authority.event_count);
    append_le<uint32_t>(result_record, result.authority.split_count);
    append_le<uint32_t>(result_record, result.authority.copy_map_count);
    append_le<uint32_t>(result_record, result.authority.verified_copy_count);
    append_le<uint32_t>(result_record, result.authority.verified_partial_count);
    append_le<uint32_t>(result_record, result.authority.exported_size);
    append_le<uint32_t>(result_record, result.authority.trailer_offset);
    append_le<uint64_t>(result_record, result.authority.execution_sequence);
    result_record.insert(result_record.end(), result.authority.attempt_nonce, result.authority.attempt_nonce + 32);
    result_record.insert(result_record.end(), result.authority.chain_root, result.authority.chain_root + 32);
    result_record.resize(result_record.size() + 32, 0);
    const auto result_tag = auth_hmac(key, result_record.data(), result_record.size());
    if (std::memcmp(result_tag.data(), result.authority.tag, result_tag.size()) != 0) {
        std::fprintf(stderr, "result tag verification failed expert=%d canonical=%zu\n", expert ? 1 : 0, result_record.size());
        return false;
    }

    uint32_t splits = 0, maps = 0, before = 0, after = 0;
    std::vector<std::array<uint64_t, 4>> ranges;
    std::vector<std::array<uint8_t, 64>> hashes;
    std::vector<std::array<uint64_t, 6>> source_ranges;
    for (const auto & event : events) {
        if (event.type == 4) {
            if (event.body_size < 20) return false;
            if (expert) {
                if (splits != 0 || event.body_size != 24 ||
                    read_u32(event.body + 0) != 0 || read_u32(event.body + 4) != 0 ||
                    read_u32(event.body + 8) != 0 || read_u32(event.body + 12) != 1 ||
                    read_u32(event.body + 16) != 1 || read_u32(event.body + 20) != 1) return false;
            } else if (splits == 0) {
                if (event.body_size != 20 ||
                    read_u32(event.body + 0) != 0 || read_u32(event.body + 4) != 0 ||
                    read_u32(event.body + 8) != 0 || read_u32(event.body + 12) != 3 ||
                    read_u32(event.body + 16) != 0) return false;
            } else {
                if (splits != 1 || event.body_size != 24 ||
                    read_u32(event.body + 0) != 1 || read_u32(event.body + 4) != 1 ||
                    read_u32(event.body + 8) != 3 || read_u32(event.body + 12) != 4 ||
                    read_u32(event.body + 16) != 1 || read_u32(event.body + 20) != 2) return false;
            }
            ++splits;
        } else if (event.type == 3) {
            const uint32_t expected_source = expert ? 1 : 2;
            const uint32_t expected_destination = expert ? 0 : 1;
            const uint32_t expected_consumer = expert ? 0 : 3;
            const uint32_t expected_views = expert ? 0 : 1;
            const uint32_t destination_layout = 108 + expected_views * 12;
            if (event.body_size != 184 + expected_views * 12 || read_u32(event.body) != expected_source ||
                read_u32(event.body + 4) != expected_destination ||
                read_u64(event.body + 12) != 1 ||
                read_u32(event.body + 20) != expected_consumer ||
                read_u32(event.body + 28) != 1 ||
                read_u32(event.body + 32) != expected_source ||
                read_u32(event.body + 36) != GGML_TYPE_F32 ||
                read_u64(event.body + 40) != 8 ||
                read_u64(event.body + 48) != (expert ? 8u : 1u) ||
                read_u64(event.body + 56) != (expert ? 4u : 1u) ||
                read_u32(event.body + 104) != expected_views ||
                read_u32(event.body + destination_layout) != expected_source ||
                read_u32(event.body + destination_layout + 4) != GGML_TYPE_F32 ||
                read_u32(event.body + destination_layout + 72) != 0) return false;
            const std::array<uint64_t, 4> expected_ne = expert ?
                std::array<uint64_t, 4>({ 8, 8, 4, 1 }) :
                std::array<uint64_t, 4>({ 8, 1, 1, 1 });
            const std::array<uint64_t, 4> expected_nb = expert ?
                std::array<uint64_t, 4>({ 4, 32, 256, 1024 }) :
                std::array<uint64_t, 4>({ 4, 32, 32, 32 });
            for (uint32_t d = 0; d < 4; ++d) {
                if (read_u64(event.body + 40 + d * 8) != expected_ne[d] ||
                    read_u64(event.body + 72 + d * 8) != expected_nb[d] ||
                    read_u64(event.body + destination_layout + 8 + d * 8) != expected_ne[d] ||
                    read_u64(event.body + destination_layout + 40 + d * 8) != expected_nb[d]) return false;
            }
            if (!expert && (read_u32(event.body + 108) != 0 || read_u64(event.body + 112) != 64)) return false;
            ++maps;
        } else if (event.type >= 5 && event.type <= 8) {
            if (event.body_size < 232) return false;
            const uint32_t role = read_u32(event.body);
            const uint32_t source_id = read_u32(event.body + 4);
            const uint32_t destination_backend = read_u32(event.body + 76);
            const uint32_t copy_slot = read_u32(event.body + 80);
            const uint64_t generation = read_u64(event.body + 84);
            const uint64_t range_offset = read_u64(event.body + 92);
            const uint64_t range_size = read_u64(event.body + 100);
            const uint64_t logical = read_u64(event.body + 108);
            const uint64_t padding = read_u64(event.body + 116);
            const uint32_t view_count = read_u32(event.body + 164);
            const uint32_t hash_offset = 168 + view_count * 12;
            const uint32_t allocation_backend = read_u32(event.body + 124);
            const uint32_t allocation_ordinal = read_u32(event.body + 128);
            const uint64_t relative_offset = read_u64(event.body + 132);
            const uint64_t allocation_range = read_u64(event.body + 140);
            const uint64_t buffer_size = read_u64(event.body + 148);
            const std::array<uint64_t, 4> expected_ne = expert ?
                std::array<uint64_t, 4>({ 8, 8, 4, 1 }) :
                std::array<uint64_t, 4>({ 8, 1, 1, 1 });
            const std::array<uint64_t, 4> expected_nb = expert ?
                std::array<uint64_t, 4>({ 4, 32, 256, 1024 }) :
                std::array<uint64_t, 4>({ 4, 32, 32, 32 });
            for (uint32_t d = 0; d < 4; ++d) {
                if (read_u64(event.body + 12 + d * 8) != expected_ne[d] ||
                    read_u64(event.body + 44 + d * 8) != expected_nb[d]) return false;
            }
            const uint64_t expected_relative = expert ? (role == 0 ? 0u : 96u) : (role == 0 ? 64u : 0u);
            const uint64_t expected_allocation_range = expert ? 1024u : 32u;
            const uint64_t expected_buffer_size = expert ? (role == 0 ? 1024u : 1184u) : (role == 0 ? 512u : 32u);
            if (hash_offset + 64 != event.body_size || logical + padding != range_size ||
                source_id != (expert ? 1u : 2u) || destination_backend != (expert ? 0u : 1u) ||
                copy_slot != 0 || generation != 1 ||
                allocation_backend != (role == 0 ? (expert ? 1u : 0u) : destination_backend) ||
                allocation_ordinal != (role == 0 ? 0u : 1u) ||
                relative_offset != expected_relative || allocation_range != expected_allocation_range ||
                buffer_size != expected_buffer_size ||
                relative_offset > buffer_size || allocation_range > buffer_size - relative_offset ||
                range_offset > allocation_range || range_size > allocation_range - range_offset ||
                view_count != (role == 0 && !expert ? 1u : 0u)) return false;
            if (role == 0 && !expert &&
                (read_u32(event.body + 168) != 0 || read_u64(event.body + 172) != 64)) return false;
            if ((!expert && !((event.type == 5 && role == 0) || (event.type == 6 && role == 1))) ||
                (expert && !((event.type == 7 && role == 0) || (event.type == 8 && role == 1)))) return false;
            if (role == 0) {
                if (range_offset > UINT64_MAX - range_size) return false;
                for (const auto & prior : source_ranges) {
                    if (prior[0] == source_id && prior[1] == destination_backend &&
                        prior[2] == copy_slot && prior[3] == generation &&
                        range_offset < prior[4] + prior[5] && prior[4] < range_offset + range_size) return false;
                }
                source_ranges.push_back({ source_id, destination_backend, copy_slot, generation, range_offset, range_size });
            }
            ranges.push_back({ range_offset, range_size, logical, padding });
            std::array<uint8_t, 64> digest {};
            std::memcpy(digest.data(), event.body + hash_offset, digest.size());
            hashes.push_back(digest);
            role == 0 ? ++before : ++after;
        }
    }
    if (splits != (expert ? 1u : 2u) || maps != 1 || before != after) return false;
    if (result.authority.split_count != splits || result.authority.copy_map_count != maps ||
        result.authority.verified_copy_count != (expert ? 0u : before) ||
        result.authority.verified_partial_count != (expert ? before : 0u)) return false;
    if (!expert) {
        return before == 1 && ranges.size() == 2 &&
            ranges[0] == std::array<uint64_t, 4>({ 0, 32, 32, 0 }) &&
            ranges[1] == ranges[0] && hashes[0] == hashes[1];
    }
    return before == 2 && ranges.size() == 4 &&
        ranges[0] == std::array<uint64_t, 4>({ 0, 512, 256, 256 }) &&
        ranges[1] == ranges[0] && hashes[0] == hashes[1] &&
        ranges[2] == std::array<uint64_t, 4>({ 512, 512, 256, 256 }) &&
        ranges[3] == ranges[2] && hashes[2] == hashes[3];
}

static bool run_graph(ggml_backend_t rpc, ggml_backend_t cpu, bool enable, run_result & result, bool composed = false) {
    std::vector<uint8_t> metadata(ggml_tensor_overhead() * 12 + ggml_graph_overhead());
    ggml_init_params params {
        /* .mem_size   = */ metadata.size(),
        /* .mem_buffer = */ metadata.data(),
        /* .no_alloc   = */ true,
    };
    ggml_context * ctx = ggml_init(params);
    if (ctx == nullptr) return false;
    ggml_tensor * a = ggml_new_tensor_1d(ctx, GGML_TYPE_F32, 64);
    ggml_tensor * b = ggml_new_tensor_1d(ctx, GGML_TYPE_F32, 64);
    ggml_set_input(a);
    ggml_set_input(b);
    ggml_tensor * sum = ggml_add(ctx, a, b);
    ggml_tensor * strided = ggml_view_2d(ctx, sum, 8, 4, 16 * sizeof(float), 0);
    ggml_tensor * nested = ggml_view_1d(ctx, strided, 8, 16 * sizeof(float));
    ggml_tensor * out = ggml_sqr(ctx, nested);
    ggml_set_output(out);

    ggml_backend_t backends[] = { rpc, cpu };
    ggml_backend_buffer_type_t bufts[] = {
        ggml_backend_get_default_buffer_type(rpc),
        ggml_backend_get_default_buffer_type(cpu),
    };
    ggml_backend_sched_t sched = ggml_backend_sched_new(backends, bufts, 2, 64, false, false);
    if (sched == nullptr) {
        ggml_free(ctx);
        return false;
    }
    ggml_backend_sched_set_tensor_backend(sched, a, rpc);
    ggml_backend_sched_set_tensor_backend(sched, b, rpc);
    ggml_backend_sched_set_tensor_backend(sched, sum, rpc);
    ggml_backend_sched_set_tensor_backend(sched, strided, rpc);
    ggml_backend_sched_set_tensor_backend(sched, nested, rpc);
    ggml_backend_sched_set_tensor_backend(sched, out, cpu);

    ggml_backend_sched_authority_handle composed_handle {};
    ggml_backend_sched_authority_prepared composed_prepared {};
    if (enable) {
        ggml_backend_sched_authority_config config {};
        config.major = 1;
        config.minor = 0;
        config.encoded_size = sizeof(config);
        config.max_events = 256;
        config.event_buffer_size = result.transcript.size();
        config.execution_sequence = 41;
        for (uint32_t i = 0; i < 32; ++i) {
            config.attempt_nonce[i] = static_cast<uint8_t>(0x40 + i);
            config.key[i] = static_cast<uint8_t>(0x80 + i);
        }
        config.event_buffer = result.transcript.data();
        const bool enabled = composed ?
            ggml_backend_sched_authority_arm(sched, &config, &composed_handle) :
            ggml_backend_sched_authority_enable(sched, &config);
        if (!enabled) {
            ggml_backend_sched_free(sched);
            ggml_free(ctx);
            return false;
        }
        if (composed &&
            (!ggml_backend_sched_authority_mark_rpc_backend(sched, &composed_handle, 0) ||
             !ggml_backend_sched_authority_register_root(
                sched, &composed_handle, a, GGML_BACKEND_SCHED_AUTH_MUTABLE, 1, 0) ||
             !ggml_backend_sched_authority_register_root(
                sched, &composed_handle, b, GGML_BACKEND_SCHED_AUTH_MUTABLE, 1, 1))) {
            ggml_backend_sched_authority_abort_execution(sched, &composed_handle);
            ggml_backend_sched_free(sched);
            ggml_free(ctx);
            return false;
        }
        std::memset(config.key, 0, sizeof(config.key));
    }

    ggml_cgraph * graph = ggml_new_graph(ctx);
    ggml_build_forward_expand(graph, out);
    bool ok = ggml_backend_sched_alloc_graph(sched, graph);
    if (ok && composed) {
        ok = ggml_backend_sched_authority_prepare(
            sched, &composed_handle, graph, &composed_prepared) &&
            composed_prepared.status == 1 &&
            composed_prepared.execution_sequence == 41 &&
            composed_prepared.rpc_count == 2 &&
            composed_prepared.local_count == 0;
    }
    std::array<float, 64> av {};
    std::array<float, 64> bv {};
    for (size_t i = 0; i < av.size(); ++i) {
        av[i] = static_cast<float>(i) / 16.0f;
        bv[i] = static_cast<float>(31 - i) / 32.0f;
    }
    if (ok) {
        ggml_backend_tensor_set(a, av.data(), 0, sizeof(av));
        ggml_backend_tensor_set(b, bv.data(), 0, sizeof(bv));
        ok = ggml_backend_sched_graph_compute(sched, graph) == GGML_STATUS_SUCCESS;
    }
    if (ok) {
        ggml_backend_tensor_get(out, result.values.data(), 0, 8 * sizeof(float));
        for (size_t i = 0; i < 8; ++i) {
            const float want = (av[16 + i] + bv[16 + i]) * (av[16 + i] + bv[16 + i]);
            ok = ok && std::isfinite(result.values[i]) && std::abs(result.values[i] - want) <= 1e-6f;
        }
    }
    result.authority_available = composed ?
        ggml_backend_sched_authority_finalize_execution(sched, &composed_handle, &result.authority) :
        ggml_backend_sched_authority_result(sched, &result.authority);
    ggml_backend_sched_free(sched);
    ggml_free(ctx);
    return ok;
}

static bool run_composed_refusals(ggml_backend_t rpc, ggml_backend_t cpu) {
    std::vector<uint8_t> metadata(ggml_tensor_overhead() * 8 + ggml_graph_overhead());
    ggml_init_params params { metadata.size(), metadata.data(), true };
    ggml_context * ctx = ggml_init(params);
    if (ctx == nullptr) return false;
    ggml_tensor * a = ggml_new_tensor_1d(ctx, GGML_TYPE_F32, 8);
    ggml_tensor * b = ggml_new_tensor_1d(ctx, GGML_TYPE_F32, 8);
    ggml_tensor * out = ggml_add(ctx, a, b);
    ggml_cgraph * graph = ggml_new_graph(ctx);
    ggml_build_forward_expand(graph, out);
    ggml_backend_t backends[] = { rpc, cpu };
    ggml_backend_buffer_type_t bufts[] = {
        ggml_backend_get_default_buffer_type(rpc),
        ggml_backend_get_default_buffer_type(cpu),
    };
    ggml_backend_sched_t sched = ggml_backend_sched_new(backends, bufts, 2, 32, false, false);
    if (sched == nullptr) {
        ggml_free(ctx);
        return false;
    }
    ggml_backend_sched_set_tensor_backend(sched, a, rpc);
    ggml_backend_sched_set_tensor_backend(sched, b, rpc);
    ggml_backend_sched_set_tensor_backend(sched, out, rpc);
    std::array<uint8_t, 4096> transcript {};
    ggml_backend_sched_authority_config config {};
    config.major = 1;
    config.minor = 0;
    config.encoded_size = sizeof(config);
    config.max_events = 64;
    config.event_buffer = transcript.data();
    config.event_buffer_size = transcript.size();
    config.execution_sequence = 77;
    for (uint32_t i = 0; i < 32; ++i) {
        config.attempt_nonce[i] = static_cast<uint8_t>(0x20 + i);
        config.key[i] = static_cast<uint8_t>(0x60 + i);
    }
    ggml_backend_sched_authority_handle handle {};
    bool ok = ggml_backend_sched_authority_arm(sched, &config, &handle);
    ggml_backend_sched_authority_handle stale = handle;
    ++stale.execution_sequence;
    ok = ok &&
        !ggml_backend_sched_authority_mark_rpc_backend(sched, &stale, 0) &&
        ggml_backend_sched_authority_mark_rpc_backend(sched, &handle, 0) &&
        ggml_backend_sched_authority_register_root(
            sched, &handle, a, GGML_BACKEND_SCHED_AUTH_MUTABLE, 1, 0) &&
        ggml_backend_sched_alloc_graph(sched, graph);
    ggml_backend_sched_authority_prepared prepared {};
    ok = ok && !ggml_backend_sched_authority_prepare(sched, &handle, graph, &prepared);
    ok = ok && ggml_backend_sched_authority_abort_execution(sched, &handle);
    ok = ok &&
        !ggml_backend_sched_authority_abort_execution(sched, &handle) &&
        !ggml_backend_sched_authority_finalize_execution(sched, &handle, nullptr);
    std::memset(config.key, 0, sizeof(config.key));
    ggml_backend_sched_free(sched);
    ggml_free(ctx);
    return ok;
}

static bool run_expert_partial(ggml_backend_t rpc, ggml_backend_t cpu, run_result & result) {
    std::vector<uint8_t> metadata(ggml_tensor_overhead() * 10 + ggml_graph_overhead());
    ggml_init_params params { metadata.size(), metadata.data(), true };
    ggml_context * ctx = ggml_init(params);
    if (ctx == nullptr) return false;
    constexpr int64_t k = 8;
    constexpr int64_t m = 8;
    constexpr int64_t experts = 4;
    constexpr int64_t used = 2;
    ggml_tensor * weights = ggml_new_tensor_3d(ctx, GGML_TYPE_F32, k, m, experts);
    ggml_tensor * ids = ggml_new_tensor_2d(ctx, GGML_TYPE_I32, used, 1);
    ggml_tensor * activations = ggml_new_tensor_3d(ctx, GGML_TYPE_F32, k, used, 1);
    ggml_set_input(ids);
    ggml_set_input(activations);
    ggml_tensor * out = ggml_mul_mat_id(ctx, weights, activations, ids);
    ggml_set_output(out);
    ggml_backend_t backends[] = { rpc, cpu };
    ggml_backend_buffer_type_t bufts[] = {
        ggml_backend_get_default_buffer_type(rpc),
        ggml_backend_get_default_buffer_type(cpu),
    };
    ggml_backend_sched_t sched = ggml_backend_sched_new(backends, bufts, 2, 64, false, false);
    if (sched == nullptr) {
        ggml_free(ctx);
        return false;
    }
    ggml_backend_sched_set_tensor_backend(sched, weights, cpu);
    ggml_backend_sched_set_tensor_backend(sched, ids, rpc);
    ggml_backend_sched_set_tensor_backend(sched, activations, rpc);
    ggml_backend_sched_set_tensor_backend(sched, out, rpc);
    struct ggml_backend_sched_authority_config config {};
    config.major = 1;
    config.minor = 0;
    config.encoded_size = sizeof(config);
    config.max_events = 256;
    config.event_buffer_size = result.transcript.size();
    config.execution_sequence = 42;
    for (uint32_t i = 0; i < 32; ++i) {
        config.attempt_nonce[i] = static_cast<uint8_t>(0x20 + i);
        config.key[i] = static_cast<uint8_t>(0xc0 + i);
    }
    config.event_buffer = result.transcript.data();
    bool ok = ggml_backend_sched_authority_enable(sched, &config);
    std::memset(config.key, 0, sizeof(config.key));
    ggml_cgraph * graph = ggml_new_graph(ctx);
    ggml_build_forward_expand(graph, out);
    ok = ok && ggml_backend_sched_alloc_graph(sched, graph);
    if (ok) ggml_backend_buffer_set_usage(weights->buffer, GGML_BACKEND_BUFFER_USAGE_WEIGHTS);
    std::array<float, k * m * experts> weight_data {};
    std::array<float, k * used> activation_data {};
    std::array<int32_t, used> id_data {{ 0, 2 }};
    for (size_t i = 0; i < weight_data.size(); ++i) weight_data[i] = static_cast<float>((i % 17) + 1) / 32.0f;
    for (size_t i = 0; i < activation_data.size(); ++i) activation_data[i] = static_cast<float>((i % 7) + 1) / 16.0f;
    if (ok) {
        ggml_backend_tensor_set(weights, weight_data.data(), 0, sizeof(weight_data));
        ggml_backend_tensor_set(ids, id_data.data(), 0, sizeof(id_data));
        ggml_backend_tensor_set(activations, activation_data.data(), 0, sizeof(activation_data));
        ok = ggml_backend_sched_graph_compute(sched, graph) == GGML_STATUS_SUCCESS;
    }
    std::array<float, m * used> output {};
    if (ok) {
        ggml_backend_tensor_get(out, output.data(), 0, sizeof(output));
        for (int64_t j = 0; j < used; ++j) {
            for (int64_t row = 0; row < m; ++row) {
                float expected = 0.0f;
                for (int64_t x = 0; x < k; ++x) {
                    expected += weight_data[id_data[j] * k * m + row * k + x] *
                                activation_data[j * k + x];
                }
                ok = ok && std::isfinite(output[j * m + row]) &&
                    std::abs(output[j * m + row] - expected) <= 1e-5f;
            }
        }
    }
    result.authority_available = ggml_backend_sched_authority_result(sched, &result.authority);
    ok = ok && result.authority_available && result.authority.verified_partial_count == 2;
    ggml_backend_sched_free(sched);
    ggml_free(ctx);
    return ok;
}

static bool run_hash_fixtures(ggml_backend_t cpu) {
    std::vector<uint8_t> metadata(ggml_tensor_overhead() * 8);
    ggml_init_params params { metadata.size(), metadata.data(), true };
    ggml_context * ctx = ggml_init(params);
    if (ctx == nullptr) return false;
    ggml_tensor * base = ggml_new_tensor_1d(ctx, GGML_TYPE_F32, 64);
    ggml_tensor * strided = ggml_view_2d(ctx, base, 8, 4, 16 * sizeof(float), 0);
    ggml_tensor * nested = ggml_view_1d(ctx, strided, 8, 16 * sizeof(float));
    ggml_tensor * quantized = ggml_new_tensor_2d(ctx, GGML_TYPE_Q8_0, 32, 2);
    ggml_backend_buffer_t buffer = ggml_backend_alloc_ctx_tensors(ctx, cpu);
    if (buffer == nullptr) {
        ggml_free(ctx);
        return false;
    }
    std::array<float, 64> values {};
    for (size_t i = 0; i < values.size(); ++i) values[i] = static_cast<float>(i) / 32.0f;
    ggml_backend_tensor_set(base, values.data(), 0, sizeof(values));
    std::vector<uint8_t> qbytes(ggml_nbytes(quantized), 0x5a);
    ggml_backend_tensor_set(quantized, qbytes.data(), 0, qbytes.size());
    struct ggml_backend_sched_authority_hash_probe nested_result {};
    struct ggml_backend_sched_authority_hash_probe strided_result {};
    struct ggml_backend_sched_authority_hash_probe quantized_result {};
    struct ggml_backend_sched_authority_hash_probe padding_result {};
    struct ggml_backend_sched_authority_hash_probe refused_result {};
    const bool nested_ok = ggml_backend_sched_authority_hash_probe(
        cpu, nested, 0, ggml_nbytes(nested), 0, &nested_result);
    const bool strided_ok = ggml_backend_sched_authority_hash_probe(
        cpu, strided, 0, ggml_nbytes(strided), 0, &strided_result);
    const bool quantized_ok = ggml_backend_sched_authority_hash_probe(
        cpu, quantized, 0, ggml_nbytes(quantized), 0, &quantized_result);
    const bool quantized_offset_refused = !ggml_backend_sched_authority_hash_probe(
        cpu, quantized, 1, ggml_nbytes(quantized) - 1, 0, &refused_result);
    const bool quantized_size_refused = !ggml_backend_sched_authority_hash_probe(
        cpu, quantized, 0, ggml_nbytes(quantized) - 1, 0, &refused_result);
    const bool padding_ok = ggml_backend_sched_authority_hash_probe(
        cpu, base, 0, 64, 32, &padding_result);
    const bool oob_refused = !ggml_backend_sched_authority_hash_probe(
        cpu, base, ggml_nbytes(base) - 16, 32, 0, &refused_result);
    const bool exact =
        nested_ok && nested_result.logical_bytes == 32 && nested_result.padding_bytes == 0 &&
        strided_ok && strided_result.logical_bytes == 128 && strided_result.padding_bytes == 96 &&
        quantized_ok && quantized_result.logical_bytes == ggml_nbytes(quantized) &&
        quantized_result.padding_bytes == 0 && quantized_offset_refused && quantized_size_refused &&
        padding_ok && padding_result.logical_bytes == 32 && padding_result.padding_bytes == 32 &&
        oob_refused;
    ggml_backend_buffer_free(buffer);
    ggml_free(ctx);
    return exact;
}

static bool write_evidence(const char * directory, const char * stem, const run_result & result) {
    if (directory == nullptr || directory[0] == '\0') return true;
    const std::string prefix = std::string(directory) + "/" + stem;
    std::ofstream transcript(prefix + ".transcript.bin", std::ios::binary | std::ios::trunc);
    transcript.write(reinterpret_cast<const char *>(result.transcript.data()), result.authority.exported_size);
    transcript.close();
    std::ofstream receipt(prefix + ".result.bin", std::ios::binary | std::ios::trunc);
    receipt.write(reinterpret_cast<const char *>(&result.authority), sizeof(result.authority));
    receipt.close();
    return transcript.good() && receipt.good();
}

int main(int argc, char ** argv) {
    if (argc != 2) {
        std::fprintf(stderr, "usage: %s HOST:PORT\n", argv[0]);
        return 2;
    }
    const uint32_t self_tests = ggml_backend_sched_authority_self_test();
    if (self_tests != 0x1ffffU) {
        std::fprintf(stderr, "focused scheduler authority refusal self-test failed: 0x%x\n", self_tests);
        return 1;
    }
    ggml_backend_load_all();
    ggml_backend_t rpc = ggml_backend_rpc_init(argv[1], 0);
    ggml_backend_t cpu = ggml_backend_init_by_type(GGML_BACKEND_DEVICE_TYPE_CPU, nullptr);
    if (rpc == nullptr || cpu == nullptr) return 2;

    run_result off {};
    run_result on {};
    run_result composed {};
    run_result expert {};
    const bool off_ok = run_graph(rpc, cpu, false, off);
    const bool on_ok = run_graph(rpc, cpu, true, on);
    const bool composed_ok = run_graph(rpc, cpu, true, composed, true);
    const bool composed_refusals = run_composed_refusals(rpc, cpu);
    const bool expert_ok = run_expert_partial(rpc, cpu, expert);
    const bool hash_fixtures = run_hash_fixtures(cpu);
    const bool feature_off = off_ok && !off.authority_available;
    const bool exact = on_ok && std::memcmp(off.values.data(), on.values.data(), sizeof(off.values)) == 0;
    const bool contract =
        on.authority_available &&
        on.authority.status == 1 &&
        on.authority.execution_sequence == 41 &&
        on.authority.split_count == 2 &&
        on.authority.copy_map_count >= 1 &&
        on.authority.verified_copy_count >= 1 &&
        on.authority.verified_partial_count == 0;
    std::array<uint8_t, 32> ordinary_key {};
    std::array<uint8_t, 32> expert_key {};
    for (uint32_t i = 0; i < 32; ++i) {
        ordinary_key[i] = static_cast<uint8_t>(0x80 + i);
        expert_key[i] = static_cast<uint8_t>(0xc0 + i);
    }
    const bool ordinary_transcript = verify_transcript(on, ordinary_key, 41, false);
    const bool expert_transcript = verify_transcript(expert, expert_key, 42, true);
    run_result tampered = on;
    tampered.transcript[64] ^= 1;
    const bool tamper_refused = !verify_transcript(tampered, ordinary_key, 41, false);
    run_result out_of_order = on;
    write_u32(out_of_order.transcript.data() + 20, 1);
    const bool order_refused = resign_transcript(out_of_order, ordinary_key) &&
        !verify_transcript(out_of_order, ordinary_key, 41, false);
    run_result duplicate = on;
    const uint32_t second_event = transcript_event_offset(duplicate, 1);
    if (second_event != UINT32_MAX) write_u32(duplicate.transcript.data() + second_event + 20, 0);
    const bool duplicate_refused = second_event != UINT32_MAX &&
        resign_transcript(duplicate, ordinary_key) &&
        !verify_transcript(duplicate, ordinary_key, 41, false);
    run_result unknown = on;
    write_u16(unknown.transcript.data() + 6, 9);
    const bool unknown_refused = resign_transcript(unknown, ordinary_key) &&
        !verify_transcript(unknown, ordinary_key, 41, false);
    run_result overlap = expert;
    const uint32_t second_partial_before = transcript_event_offset(overlap, 9);
    if (second_partial_before != UINT32_MAX) write_u64(overlap.transcript.data() + second_partial_before + 56 + 92, 128);
    const bool overlap_refused = second_partial_before != UINT32_MAX &&
        resign_transcript(overlap, expert_key) &&
        !verify_transcript(overlap, expert_key, 42, true);
    run_result out_of_bounds = on;
    const uint32_t ordinary_before = transcript_event_offset(out_of_bounds, 13);
    if (ordinary_before != UINT32_MAX) {
        write_u64(out_of_bounds.transcript.data() + ordinary_before + 56 + 100, 64);
        write_u64(out_of_bounds.transcript.data() + ordinary_before + 56 + 108, 64);
    }
    const bool bounds_refused = ordinary_before != UINT32_MAX &&
        resign_transcript(out_of_bounds, ordinary_key) &&
        !verify_transcript(out_of_bounds, ordinary_key, 41, false);
    run_result malformed = on;
    malformed.authority.exported_size--;
    const bool malformed_refused = !verify_transcript(malformed, ordinary_key, 41, false);
    const char * evidence_directory = std::getenv("HALOFPX_SCHED_AUTH_EVIDENCE_DIR");
    const bool evidence_written =
        write_evidence(evidence_directory, "ordinary", on) &&
        write_evidence(evidence_directory, "expert", expert);
    std::printf(
        "self_tests=17 feature_off=%d split_exact=%d composed=%d composed_refusals=%d hash_fixtures=%d evidence_written=%d authority=%d ordinary_transcript=%d expert_fixture=%d expert_transcript=%d tamper_refused=%d order_refused=%d duplicate_refused=%d unknown_refused=%d overlap_refused=%d bounds_refused=%d malformed_refused=%d events=%u splits=%u maps=%u copies=%u expert_status=%u expert_events=%u expert_maps=%u expert_copies=%u partial=%u\n",
        feature_off ? 1 : 0,
        exact ? 1 : 0,
        composed_ok ? 1 : 0,
        composed_refusals ? 1 : 0,
        hash_fixtures ? 1 : 0,
        evidence_written ? 1 : 0,
        contract ? 1 : 0,
        ordinary_transcript ? 1 : 0,
        expert_ok ? 1 : 0,
        expert_transcript ? 1 : 0,
        tamper_refused ? 1 : 0,
        order_refused ? 1 : 0,
        duplicate_refused ? 1 : 0,
        unknown_refused ? 1 : 0,
        overlap_refused ? 1 : 0,
        bounds_refused ? 1 : 0,
        malformed_refused ? 1 : 0,
        on.authority.event_count,
        on.authority.split_count,
        on.authority.copy_map_count,
        on.authority.verified_copy_count,
        expert.authority.status,
        expert.authority.event_count,
        expert.authority.copy_map_count,
        expert.authority.verified_copy_count,
        expert.authority.verified_partial_count);

    ggml_backend_free(rpc);
    ggml_backend_free(cpu);
    return feature_off && exact && composed_ok && composed_refusals && hash_fixtures && evidence_written && contract && expert_ok && ordinary_transcript &&
        expert_transcript && tamper_refused && order_refused && duplicate_refused &&
        unknown_refused && overlap_refused && bounds_refused && malformed_refused ? 0 : 1;
}
