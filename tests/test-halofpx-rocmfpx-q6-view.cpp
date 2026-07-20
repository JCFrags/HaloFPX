#include "ggml.h"
#include "ggml-backend.h"
#include "ggml-rpc.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>

namespace {

constexpr int64_t k = 256;
constexpr int64_t m = 128;
constexpr int64_t expert_count = 4;
constexpr int64_t view_expert_count = 2;
constexpr int64_t view_expert_begin = 2;
constexpr int64_t selected_count = 2;

int run(ggml_backend_t backend) {
    const size_t context_size = ggml_tensor_overhead() * 16 + ggml_graph_overhead();
    ggml_init_params params = {
        /* .mem_size   = */ context_size,
        /* .mem_buffer = */ nullptr,
        /* .no_alloc   = */ true,
    };
    ggml_context * ctx = ggml_init(params);
    if (ctx == nullptr) {
        std::fprintf(stderr, "failed to create ggml context\n");
        return 2;
    }

    ggml_tensor * weights = ggml_new_tensor_3d(
        ctx, GGML_TYPE_Q6_0_ROCMFPX, k, m, expert_count);
    ggml_set_name(weights, "q6_weights_full");
    ggml_tensor * weights_view = ggml_view_3d(
        ctx, weights, k, m, view_expert_count,
        weights->nb[1], weights->nb[2], view_expert_begin * weights->nb[2]);
    ggml_set_name(weights_view, "q6_weights_nonzero_view");

    ggml_tensor * activations = ggml_new_tensor_3d(
        ctx, GGML_TYPE_F32, k, selected_count, 1);
    ggml_set_name(activations, "activations");
    ggml_tensor * global_ids = ggml_new_tensor_2d(ctx, GGML_TYPE_I32, selected_count, 1);
    ggml_set_name(global_ids, "global_ids");
    ggml_tensor * local_ids = ggml_new_tensor_2d(ctx, GGML_TYPE_I32, selected_count, 1);
    ggml_set_name(local_ids, "local_ids");

    ggml_tensor * full = ggml_mul_mat_id(ctx, weights, activations, global_ids);
    ggml_set_name(full, "full_global_id_result");
    ggml_tensor * view = ggml_mul_mat_id(ctx, weights_view, activations, local_ids);
    ggml_set_name(view, "view_local_id_result");

    ggml_backend_buffer_t buffer = ggml_backend_alloc_ctx_tensors(ctx, backend);
    if (buffer == nullptr) {
        std::fprintf(stderr, "failed to allocate tensors on %s\n", ggml_backend_name(backend));
        ggml_free(ctx);
        return 2;
    }

    const size_t values_per_expert = static_cast<size_t>(k * m);
    const size_t bytes_per_expert = weights->nb[2];
    std::vector<float> values(values_per_expert);
    std::vector<uint8_t> quantized(bytes_per_expert);
    for (int64_t expert = 0; expert < expert_count; ++expert) {
        for (size_t i = 0; i < values.size(); ++i) {
            const int value = static_cast<int>((i * 17 + expert * 29) % 255) - 127;
            values[i] = static_cast<float>(value) / 127.0f;
        }
        const size_t written = ggml_quantize_chunk(
            GGML_TYPE_Q6_0_ROCMFPX, values.data(), quantized.data(), 0, m, k, nullptr);
        if (written != bytes_per_expert) {
            std::fprintf(stderr, "unexpected Q6 bytes: %zu != %zu\n", written, bytes_per_expert);
            ggml_backend_buffer_free(buffer);
            ggml_free(ctx);
            return 2;
        }
        ggml_backend_tensor_set(
            weights, quantized.data(), static_cast<size_t>(expert) * bytes_per_expert, bytes_per_expert);
    }

    std::vector<float> activation_values(static_cast<size_t>(k * selected_count));
    for (size_t i = 0; i < activation_values.size(); ++i) {
        const int value = static_cast<int>((i * 13 + 7) % 127) - 63;
        activation_values[i] = static_cast<float>(value) / 63.0f;
    }
    ggml_backend_tensor_set(
        activations, activation_values.data(), 0, activation_values.size() * sizeof(float));

    constexpr std::array<int32_t, selected_count> global_values = {2, 3};
    constexpr std::array<int32_t, selected_count> local_values = {0, 1};
    ggml_backend_tensor_set(global_ids, global_values.data(), 0, sizeof(global_values));
    ggml_backend_tensor_set(local_ids, local_values.data(), 0, sizeof(local_values));

    ggml_cgraph * graph = ggml_new_graph(ctx);
    ggml_build_forward_expand(graph, full);
    ggml_build_forward_expand(graph, view);
    const ggml_status status = ggml_backend_graph_compute(backend, graph);
    if (status != GGML_STATUS_SUCCESS) {
        std::fprintf(stderr, "graph compute failed on %s: %s\n",
            ggml_backend_name(backend), ggml_status_to_string(status));
        ggml_backend_buffer_free(buffer);
        ggml_free(ctx);
        return 1;
    }

    const size_t output_count = static_cast<size_t>(ggml_nelements(full));
    std::vector<float> full_values(output_count);
    std::vector<float> view_values(output_count);
    ggml_backend_tensor_get(full, full_values.data(), 0, full_values.size() * sizeof(float));
    ggml_backend_tensor_get(view, view_values.data(), 0, view_values.size() * sizeof(float));

    double squared_error = 0.0;
    double squared_reference = 0.0;
    double max_abs_error = 0.0;
    for (size_t i = 0; i < output_count; ++i) {
        const double delta = static_cast<double>(full_values[i]) - view_values[i];
        squared_error += delta * delta;
        squared_reference += static_cast<double>(full_values[i]) * full_values[i];
        max_abs_error = std::max(max_abs_error, std::abs(delta));
    }
    const double reference_l2 = std::sqrt(squared_reference);
    const double nmse = squared_error / std::max(squared_reference, 1.0e-20);
    std::printf("backend=%s packed_view_offset=%zu reference_l2=%.9g nmse=%.9g max_abs_error=%.9g\n",
        ggml_backend_name(backend), view_expert_begin * weights->nb[2], reference_l2, nmse, max_abs_error);

    ggml_backend_buffer_free(buffer);
    ggml_free(ctx);
    return std::isfinite(reference_l2) && reference_l2 > 1.0e-6 &&
           std::isfinite(nmse) && nmse <= 1.0e-6 ? 0 : 1;
}

} // namespace

int main(int argc, char ** argv) {
    ggml_backend_load_all();

    ggml_backend_t backend = nullptr;
    if (argc == 1) {
        backend = ggml_backend_init_by_name("ROCm0", nullptr);
    } else if (argc == 3 && std::strcmp(argv[1], "--rpc") == 0) {
        backend = ggml_backend_rpc_init(argv[2], 0);
    } else {
        std::fprintf(stderr, "usage: %s [--rpc HOST:PORT]\n", argv[0]);
        return 2;
    }
    if (backend == nullptr) {
        std::fprintf(stderr, "failed to initialize requested backend\n");
        return 2;
    }

    const int result = run(backend);
    ggml_backend_free(backend);
    return result;
}
