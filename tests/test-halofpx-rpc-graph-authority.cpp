#include "ggml.h"
#include "ggml-backend.h"
#include "ggml-rpc.h"

#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>

extern "C" uint32_t ggml_backend_rpc_halofpx_graph_auth_self_test(void);

int main(int argc, char ** argv) {
    if (argc != 2) {
        std::fprintf(stderr, "usage: %s HOST:PORT\n", argv[0]);
        return 2;
    }
    if (ggml_backend_rpc_halofpx_graph_auth_self_test() != 0x3ffffU) {
        std::fprintf(stderr, "focused protocol refusal self-test failed\n");
        return 1;
    }

    ggml_backend_load_all();
    ggml_backend_t backend = ggml_backend_rpc_init(argv[1], 0);
    ggml_backend_t cpu = ggml_backend_init_by_type(GGML_BACKEND_DEVICE_TYPE_CPU, nullptr);
    if (backend == nullptr || cpu == nullptr) return 2;
    std::vector<uint8_t> metadata(ggml_tensor_overhead() * 8 + ggml_graph_overhead());
    ggml_init_params params {
        /* .mem_size   = */ metadata.size(),
        /* .mem_buffer = */ metadata.data(),
        /* .no_alloc   = */ true,
    };
    ggml_context * ctx = ggml_init(params);
    if (ctx == nullptr) return 2;
    ggml_tensor * a = ggml_new_tensor_1d(ctx, GGML_TYPE_F32, 32);
    ggml_tensor * b = ggml_new_tensor_1d(ctx, GGML_TYPE_F32, 32);
    ggml_set_input(a);
    ggml_set_input(b);
    ggml_tensor * sum = ggml_add(ctx, a, b);
    ggml_tensor * out = ggml_sqr(ctx, sum);
    ggml_set_output(out);
    ggml_backend_t backends[] = { backend, cpu };
    ggml_backend_buffer_type_t bufts[] = {
        ggml_backend_get_default_buffer_type(backend),
        ggml_backend_get_default_buffer_type(cpu),
    };
    ggml_backend_sched_t sched = ggml_backend_sched_new(backends, bufts, 2, 64, false, false);
    if (sched == nullptr) return 2;
    ggml_backend_sched_set_tensor_backend(sched, a, backend);
    ggml_backend_sched_set_tensor_backend(sched, b, backend);
    ggml_backend_sched_set_tensor_backend(sched, sum, backend);
    ggml_backend_sched_set_tensor_backend(sched, out, backend);
    ggml_cgraph * graph = ggml_new_graph(ctx);
    ggml_build_forward_expand(graph, out);
    if (!ggml_backend_sched_alloc_graph(sched, graph)) return 2;

    std::array<float, 32> av {};
    std::array<float, 32> bv {};
    for (size_t i = 0; i < av.size(); ++i) {
        av[i] = static_cast<float>(i) / 16.0f;
        bv[i] = static_cast<float>(31 - i) / 32.0f;
    }
    ggml_backend_tensor_set(a, av.data(), 0, sizeof(av));
    ggml_backend_tensor_set(b, bv.data(), 0, sizeof(bv));
    if (ggml_backend_sched_graph_compute(sched, graph) != GGML_STATUS_SUCCESS) return 1;
    std::array<float, 32> first {};
    ggml_backend_tensor_get(out, first.data(), 0, sizeof(first));

    ggml_backend_tensor_set(a, av.data(), 0, sizeof(av));
    ggml_backend_tensor_set(b, bv.data(), 0, sizeof(bv));
    if (ggml_backend_sched_graph_compute(sched, graph) != GGML_STATUS_SUCCESS) return 1;
    std::array<float, 32> second {};
    ggml_backend_tensor_get(out, second.data(), 0, sizeof(second));
    bool exact = std::memcmp(first.data(), second.data(), sizeof(first)) == 0;
    bool expected = true;
    for (size_t i = 0; i < first.size(); ++i) {
        const float want = (av[i] + bv[i]) * (av[i] + bv[i]);
        expected = expected && std::isfinite(first[i]) && std::abs(first[i] - want) <= 1e-6f;
    }
    std::printf("protocol_self_tests=18 compute_recompute_exact=%d expected=%d first0=%.9g second0=%.9g\n",
                exact ? 1 : 0, expected ? 1 : 0, first[0], second[0]);

    ggml_backend_sched_free(sched);
    ggml_free(ctx);
    ggml_backend_free(backend);
    ggml_backend_free(cpu);
    return exact && expected ? 0 : 1;
}
