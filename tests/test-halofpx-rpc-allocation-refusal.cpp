#include "ggml-backend.h"
#include "ggml-rpc.h"

#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

int main(int argc, char ** argv) {
    if (argc != 3 || std::strcmp(argv[1], "--endpoint") != 0) {
        std::fprintf(stderr, "usage: %s --endpoint HOST:PORT\n", argv[0]);
        return 2;
    }
    const std::string endpoint = argv[2];
    ggml_backend_reg_t rpc_reg = ggml_backend_rpc_add_server(endpoint.c_str());
    if (!rpc_reg) {
        std::fprintf(stderr, "allocation refusal fixture: RPC registration failed\n");
        return 3;
    }
    ggml_backend_register(rpc_reg);
    ggml_backend_dev_t device = ggml_backend_dev_by_name("RPC0");
    if (!device || endpoint != ggml_backend_dev_description(device)) {
        std::fprintf(stderr, "allocation refusal fixture: RPC0 identity mismatch\n");
        return 3;
    }
    size_t free_bytes = 0;
    size_t total_bytes = 0;
    ggml_backend_dev_memory(device, &free_bytes, &total_bytes);
    if (total_bytes == 0 || total_bytes > SIZE_MAX - 4096) {
        std::fprintf(stderr, "allocation refusal fixture: invalid device capacity\n");
        return 3;
    }
    const size_t requested = total_bytes + 4096;
    ggml_backend_buffer_type_t buft = ggml_backend_dev_buffer_type(device);
    ggml_backend_buffer_t buffer = ggml_backend_buft_alloc_buffer(buft, requested);
    if (buffer) {
        ggml_backend_buffer_free(buffer);
        std::fprintf(stderr, "allocation refusal fixture: oversized allocation unexpectedly succeeded\n");
        return 4;
    }
    std::fprintf(stderr,
        "allocation refusal fixture: refused endpoint=%s requested=%zu total=%zu free=%zu\n",
        endpoint.c_str(), requested, total_bytes, free_bytes);
    return 23;
}
