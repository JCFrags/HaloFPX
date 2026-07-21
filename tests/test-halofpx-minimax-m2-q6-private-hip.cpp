#include "ggml.h"
#include "ggml-backend.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>

namespace {

constexpr int64_t k = 3072;
constexpr int64_t m = 1536;
constexpr int64_t experts = 96;

using owned_fn = bool (*)(ggml_backend_t,
    const ggml_tensor *, const ggml_tensor *, const ggml_tensor *,
    ggml_tensor *, ggml_tensor *, ggml_tensor *, ggml_tensor *, ggml_tensor *, int);

double nmse(const std::vector<float> & expected, const std::vector<float> & actual) {
    double error = 0.0;
    double reference = 0.0;
    for (size_t i = 0; i < expected.size(); ++i) {
        const double delta = static_cast<double>(expected[i]) - actual[i];
        error += delta*delta;
        reference += static_cast<double>(expected[i])*expected[i];
    }
    return error/std::max(reference, 1.0e-20);
}

int run() {
    ggml_backend_load_all();
    ggml_backend_t backend = ggml_backend_init_by_name("ROCm0", nullptr);
    if (backend == nullptr) {
        std::fprintf(stderr, "ROCm0 unavailable\n");
        return 2;
    }
    ggml_backend_reg_t reg = ggml_backend_dev_backend_reg(ggml_backend_get_device(backend));
    auto owned = reinterpret_cast<owned_fn>(
        ggml_backend_reg_get_proc_address(reg, "halofpx_minimax_m2_q6_owned_private_v1"));
    if (owned == nullptr) {
        std::fprintf(stderr, "private proc unavailable\n");
        ggml_backend_free(backend);
        return 2;
    }

    ggml_init_params params = {
        /* .mem_size   = */ ggml_tensor_overhead()*32 + ggml_graph_overhead()*4,
        /* .mem_buffer = */ nullptr,
        /* .no_alloc   = */ true,
    };
    ggml_context * ctx = ggml_init(params);
    ggml_tensor * weights = ggml_new_tensor_3d(ctx, GGML_TYPE_Q6_0_ROCMFPX, k, m, experts);
    ggml_tensor * global_ids = ggml_new_tensor_2d(ctx, GGML_TYPE_I32, 8, 1);
    ggml_tensor * activations = ggml_new_tensor_3d(ctx, GGML_TYPE_F32, k, 8, 1);
    ggml_tensor * compact_ids = ggml_new_tensor_2d(ctx, GGML_TYPE_I32, 4, 1);
    ggml_tensor * compact_activations = ggml_new_tensor_3d(ctx, GGML_TYPE_F32, k, 4, 1);
    ggml_tensor * compact_output = ggml_new_tensor_3d(ctx, GGML_TYPE_F32, m, 4, 1);
    ggml_tensor * scattered_output = ggml_new_tensor_3d(ctx, GGML_TYPE_F32, m, 8, 1);
    ggml_tensor * trace = ggml_new_tensor_1d(ctx, GGML_TYPE_I32, 5);
    ggml_tensor * oracle_ids = ggml_new_tensor_2d(ctx, GGML_TYPE_I32, 4, 1);
    ggml_tensor * oracle_activations = ggml_new_tensor_3d(ctx, GGML_TYPE_F32, k, 4, 1);
    ggml_tensor * oracle = ggml_mul_mat_id(ctx, weights, oracle_activations, oracle_ids);
    ggml_tensor * padded_ids = ggml_new_tensor_2d(ctx, GGML_TYPE_I32, 8, 1);
    ggml_tensor * padded_activations = ggml_new_tensor_3d(ctx, GGML_TYPE_F32, k, 8, 1);
    ggml_tensor * padded_output = ggml_mul_mat_id(ctx, weights, padded_activations, padded_ids);

    ggml_backend_buffer_t buffer = ggml_backend_alloc_ctx_tensors(ctx, backend);
    if (buffer == nullptr) {
        std::fprintf(stderr, "tensor allocation failed\n");
        ggml_free(ctx);
        ggml_backend_free(backend);
        return 2;
    }

    constexpr std::array<int32_t, 5> distinguished_ids = {0, 20, 24, 47, 95};
    std::vector<float> source(static_cast<size_t>(k*m));
    std::array<std::vector<uint8_t>, 6> quantized_patterns;
    for (size_t pattern = 0; pattern < quantized_patterns.size(); ++pattern) {
        quantized_patterns[pattern].resize(weights->nb[2]);
        for (size_t i = 0; i < source.size(); ++i) {
            const float base = static_cast<float>(static_cast<int>((i*17 + 11) % 251) - 125)/125.0f;
            source[i] = base*(0.35f + 0.11f*pattern) + 0.013f*pattern;
        }
        const size_t written = ggml_quantize_chunk(
            GGML_TYPE_Q6_0_ROCMFPX, source.data(), quantized_patterns[pattern].data(), 0, m, k, nullptr);
        if (written != quantized_patterns[pattern].size()) {
            std::fprintf(stderr, "unexpected quantized size %zu != %zu\n", written, quantized_patterns[pattern].size());
            return 2;
        }
    }
    for (int expert = 0; expert < experts; ++expert) {
        const auto it = std::find(distinguished_ids.begin(), distinguished_ids.end(), expert);
        const size_t pattern = it == distinguished_ids.end() ? distinguished_ids.size() :
            static_cast<size_t>(it - distinguished_ids.begin());
        ggml_backend_tensor_set(weights, quantized_patterns[pattern].data(), expert*weights->nb[2], weights->nb[2]);
    }

    std::vector<float> activation_values(static_cast<size_t>(k*8));
    for (size_t i = 0; i < activation_values.size(); ++i) {
        activation_values[i] = static_cast<float>(static_cast<int>((i*13 + 7) % 127) - 63)/63.0f;
    }
    ggml_backend_tensor_set(activations, activation_values.data(), 0, activation_values.size()*sizeof(float));

    ggml_cgraph * oracle_graph = ggml_new_graph_custom(ctx, 64, false);
    ggml_build_forward_expand(oracle_graph, oracle);
    ggml_cgraph * padded_graph = ggml_new_graph_custom(ctx, 64, false);
    ggml_build_forward_expand(padded_graph, padded_output);

    const std::array<int32_t, 8> ids = {0, 95, 96, 191, 47, 143, 20, 120};
    for (int base : {0, 96}) {
        std::array<int32_t, 4> expected_local_ids{};
        std::array<int32_t, 4> expected_slots{};
        std::vector<float> expected_compact_activations(static_cast<size_t>(k*4));
        int expected_owned = 0;
        for (int slot = 0; slot < 8; ++slot) {
            if (ids[slot] >= base && ids[slot] < base + 96) {
                expected_local_ids[expected_owned] = ids[slot] - base;
                expected_slots[expected_owned] = slot;
                std::copy_n(activation_values.data() + slot*k, k,
                    expected_compact_activations.data() + expected_owned*k);
                ++expected_owned;
            }
        }
        if (expected_owned != 4) {
            return 1;
        }
        ggml_backend_tensor_set(oracle_ids, expected_local_ids.data(), 0, sizeof(expected_local_ids));
        ggml_backend_tensor_set(oracle_activations, expected_compact_activations.data(), 0,
            expected_compact_activations.size()*sizeof(float));
        ggml_backend_tensor_set(global_ids, ids.data(), 0, sizeof(ids));
        if (!owned(backend, weights, global_ids, activations, compact_ids, compact_activations,
                compact_output, scattered_output, trace, base)) {
            std::fprintf(stderr, "host validation rejected base %d\n", base);
            return 1;
        }
        ggml_backend_synchronize(backend);
        std::array<int32_t, 5> trace_values{};
        ggml_backend_tensor_get(trace, trace_values.data(), 0, sizeof(trace_values));
        if (trace_values[0] != 0) {
            std::fprintf(stderr, "device validation rejected base %d status %d\n", base, trace_values[0]);
            return 1;
        }
        std::array<int32_t, 4> actual_local_ids{};
        std::vector<float> actual_compact_activations(expected_compact_activations.size());
        ggml_backend_tensor_get(compact_ids, actual_local_ids.data(), 0, sizeof(actual_local_ids));
        ggml_backend_tensor_get(compact_activations, actual_compact_activations.data(), 0,
            actual_compact_activations.size()*sizeof(float));
        for (int i = 0; i < 4; ++i) {
            if (actual_local_ids[i] != expected_local_ids[i] || trace_values[i + 1] != expected_slots[i]) {
                std::fprintf(stderr, "compaction mismatch base %d index %d\n", base, i);
                return 1;
            }
        }
        if (actual_compact_activations != expected_compact_activations) {
            std::fprintf(stderr, "activation gather mismatch base %d\n", base);
            return 1;
        }
        if (ggml_backend_graph_compute(backend, oracle_graph) != GGML_STATUS_SUCCESS) {
            std::fprintf(stderr, "oracle graph failed\n");
            return 1;
        }
        std::vector<float> oracle_values(static_cast<size_t>(m*4));
        std::vector<float> scattered_values(static_cast<size_t>(m*8));
        ggml_backend_tensor_get(oracle, oracle_values.data(), 0, oracle_values.size()*sizeof(float));
        ggml_backend_tensor_get(scattered_output, scattered_values.data(), 0, scattered_values.size()*sizeof(float));
        std::vector<float> expected(scattered_values.size(), 0.0f);
        for (int compact = 0; compact < 4; ++compact) {
            std::copy_n(oracle_values.data() + compact*m, m,
                expected.data() + expected_slots[compact]*m);
        }
        const double error = nmse(expected, scattered_values);
        std::printf("base=%d status=0 nmse=%.9g\n", base, error);
        if (!std::isfinite(error) || error > 1.0e-6) {
            return 1;
        }
    }

    const std::array<std::array<int32_t, 8>, 3> invalid = {{
        {{0, 95, 96, 192, 47, 143, 20, 120}},
        {{0, 95, 96, 191, 47, 143, 20, 20}},
        {{0, 95, 96, 191, 47, 143, 120, 121}},
    }};
    if (owned(backend, weights, global_ids, activations, compact_ids, compact_activations,
            compact_output, scattered_output, trace, 1)) {
        std::fprintf(stderr, "invalid expert base passed host validation\n");
        return 1;
    }
    std::printf("invalid_base=1 host_rejected=1\n");
    for (size_t i = 0; i < invalid.size(); ++i) {
        ggml_backend_tensor_set(global_ids, invalid[i].data(), 0, sizeof(invalid[i]));
        if (!owned(backend, weights, global_ids, activations, compact_ids, compact_activations,
                compact_output, scattered_output, trace, 0)) {
            return 1;
        }
        ggml_backend_synchronize(backend);
        std::array<int32_t, 5> trace_values{};
        std::vector<float> scattered_values(static_cast<size_t>(m*8));
        ggml_backend_tensor_get(trace, trace_values.data(), 0, sizeof(trace_values));
        ggml_backend_tensor_get(scattered_output, scattered_values.data(), 0, scattered_values.size()*sizeof(float));
        const bool all_zero = std::all_of(scattered_values.begin(), scattered_values.end(), [](float value) { return value == 0.0f; });
        std::printf("invalid=%zu status=%d zero=%d\n", i, trace_values[0], all_zero ? 1 : 0);
        if (trace_values[0] == 0 || !all_zero) {
            return 1;
        }
    }

    ggml_backend_tensor_set(global_ids, ids.data(), 0, sizeof(ids));
    std::array<int32_t, 8> padded_id_values{};
    std::vector<float> padded_activation_values(static_cast<size_t>(k*8), 0.0f);
    int compact = 0;
    for (int slot = 0; slot < 8; ++slot) {
        if (ids[slot] < 96) {
            padded_id_values[slot] = ids[slot];
            std::copy_n(activation_values.data() + slot*k, k, padded_activation_values.data() + slot*k);
            ++compact;
        }
    }
    if (compact != 4) {
        return 1;
    }
    ggml_backend_tensor_set(padded_ids, padded_id_values.data(), 0, sizeof(padded_id_values));
    ggml_backend_tensor_set(padded_activations, padded_activation_values.data(), 0, padded_activation_values.size()*sizeof(float));

    constexpr int repeats = 50;
    if (!owned(backend, weights, global_ids, activations, compact_ids, compact_activations,
            compact_output, scattered_output, trace, 0) ||
            ggml_backend_graph_compute(backend, padded_graph) != GGML_STATUS_SUCCESS) {
        return 1;
    }
    ggml_backend_synchronize(backend);
    auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < repeats; ++i) {
        owned(backend, weights, global_ids, activations, compact_ids, compact_activations,
            compact_output, scattered_output, trace, 0);
        ggml_backend_synchronize(backend);
    }
    auto middle = std::chrono::steady_clock::now();
    for (int i = 0; i < repeats; ++i) {
        if (ggml_backend_graph_compute(backend, padded_graph) != GGML_STATUS_SUCCESS) {
            return 1;
        }
        ggml_backend_synchronize(backend);
    }
    auto end = std::chrono::steady_clock::now();
    const double private_us = std::chrono::duration<double, std::micro>(middle - start).count()/repeats;
    const double padded_us = std::chrono::duration<double, std::micro>(end - middle).count()/repeats;
    std::printf("private_us=%.3f padded_us=%.3f ratio=%.6f\n", private_us, padded_us, private_us/padded_us);

    ggml_backend_buffer_free(buffer);
    ggml_free(ctx);
    ggml_backend_free(backend);
    return private_us < padded_us ? 0 : 1;
}

} // namespace

int main() {
    return run();
}
