#include "ggml-rpc.h"
#include "ggml-backend.h"
#include "ggml.h"

#include <array>
#include <cassert>
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <thread>
#include <chrono>
#include <sys/random.h>

namespace {

struct context_deleter { void operator()(ggml_context * p) const { ggml_free(p); } };
struct buffer_deleter { void operator()(ggml_backend_buffer * p) const { ggml_backend_buffer_free(p); } };
struct backend_deleter { void operator()(ggml_backend * p) const { ggml_backend_free(p); } };

bool parse_hex(const char * value, uint8_t output[32]) {
    if (!value || std::strlen(value) != 64) return false;
    auto nibble = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        return -1;
    };
    for (size_t i = 0; i < 32; ++i) {
        const int a = nibble(value[2*i]);
        const int b = nibble(value[2*i + 1]);
        if (a < 0 || b < 0) return false;
        output[i] = static_cast<uint8_t>((a << 4) | b);
    }
    return true;
}

std::string hex(const uint8_t value[32]) {
    static const char digits[] = "0123456789abcdef";
    std::string result(64, '0');
    for (size_t i = 0; i < 32; ++i) {
        result[2*i] = digits[value[i] >> 4];
        result[2*i + 1] = digits[value[i] & 15];
    }
    return result;
}

ggml_backend_rpc_halofpx_state_identity identity(uint8_t attempt, bool mismatch) {
    ggml_backend_rpc_halofpx_state_identity value {};
    value.key_generation = 7;
    value.generation = 1;
    value.token_count = 18;
    value.token_boundary = 18;
    value.world_size = 2;
    value.logical_rank = 1;
    std::memset(value.model_digest, 0x31, 32);
    std::memset(value.compatibility_root, 0x32, 32);
    std::memset(value.plan_digest, mismatch ? 0x99 : 0x33, 32);
    std::memset(value.topology_digest, 0x34, 32);
    std::memset(value.placement_digest, 0x35, 32);
    std::memset(value.checkpoint_digest, 0x36, 32);
    std::memset(value.token_prefix_digest, 0x37, 32);
    std::memset(value.component_manifest_digest, 0x38, 32);
    std::memset(value.attempt_nonce, attempt, 32);
    std::memset(value.channel_binding, 0x22, 32);
    return value;
}

struct fixture {
    std::unique_ptr<ggml_backend, backend_deleter> backend;
    std::unique_ptr<ggml_context, context_deleter> ctx;
    std::unique_ptr<ggml_backend_buffer, buffer_deleter> buffer;
    ggml_tensor * staged = nullptr;
    ggml_tensor * live = nullptr;

    explicit fixture(const char * endpoint) {
        backend.reset(ggml_backend_rpc_init(endpoint, 0));
        assert(backend);
        ggml_init_params params { 2 * ggml_tensor_overhead(), nullptr, true };
        ctx.reset(ggml_init(params));
        assert(ctx);
        staged = ggml_new_tensor_1d(ctx.get(), GGML_TYPE_F32, 16);
        live = ggml_new_tensor_1d(ctx.get(), GGML_TYPE_F32, 16);
        auto buft = ggml_backend_get_default_buffer_type(backend.get());
        buffer.reset(ggml_backend_alloc_ctx_tensors_from_buft(ctx.get(), buft));
        assert(buffer);
    }
};

ggml_backend_rpc_halofpx_state_component component(ggml_tensor * tensor) {
    ggml_backend_rpc_halofpx_state_component value {};
    value.tensor = tensor;
    value.ordinal = 0;
    value.kind = GGML_RPC_HALOFPX_COMPONENT_ATTENTION_K;
    value.size = ggml_nbytes(tensor);
    std::memset(value.label_digest, 0x41, 32);
    return value;
}

bool fresh_nonce(uint8_t nonce[32]) {
    size_t done = 0;
    while (done < 32) {
        const ssize_t n = getrandom(nonce + done, 32 - done, 0);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) return false;
        done += static_cast<size_t>(n);
    }
    return true;
}

} // namespace

int main(int argc, char ** argv) {
    if (argc < 3) {
        std::fprintf(stderr, "usage: %s capture|restore|replay|stage-replay|destroy|timeout|range|shape|auth|mismatch|missing|corrupt ENDPOINT [OBJECT_DIGEST]\n", argv[0]);
        return 2;
    }
    const std::string mode = argv[1];
    fixture f(argv[2]);
    std::array<float, 16> expected {};
    for (size_t i = 0; i < expected.size(); ++i) expected[i] = float(i) + 0.25f;
    std::array<uint8_t, 32> key {};
    key.fill(0x11);
    auto staged = component(f.staged);
    if (mode == "capture" || mode == "range" || mode == "shape") {
        ggml_backend_tensor_set(f.staged, expected.data(), 0, sizeof(expected));
        const auto id = identity(0x51, false);
        if (mode == "range") staged.size += 1;
        if (mode == "shape") f.staged->nb[1] += 1;
        const auto result = ggml_backend_rpc_halofpx_state_capture(&id, &staged, 1, key.data());
        if (mode == "range") {
            std::printf("status=%u negative=range\n", unsigned(result.status));
            std::fflush(nullptr);
            std::_Exit(result.status == GGML_RPC_HALOFPX_STATE_REJECTED ? 0 : 8);
        }
        if (mode == "shape") {
            std::printf("status=%u negative=shape\n", unsigned(result.status));
            std::fflush(nullptr);
            std::_Exit(result.status == GGML_RPC_HALOFPX_STATE_REJECTED ? 0 : 12);
        }
        if (result.status != GGML_RPC_HALOFPX_STATE_STORED || result.verified_bytes != sizeof(expected)) return 3;
        std::printf("status=stored object=%s bytes=%llu components=%u\n",
            hex(result.object_digest).c_str(), (unsigned long long) result.verified_bytes,
            result.verified_components);
        return 0;
    }
    if (argc != 4) return 2;
    std::array<uint8_t, 32> object {};
    if (!parse_hex(argv[3], object.data())) return 2;
    const bool mismatch = mode == "mismatch";
    const bool expected_miss = mismatch || mode == "missing" || mode == "corrupt";
    const bool auth = mode == "auth";
    if (mode != "restore" && mode != "replay" && mode != "stage-replay" && mode != "destroy" &&
        mode != "timeout" && !auth && !expected_miss) return 2;
    auto id = identity(0x52, mismatch);
    if (!fresh_nonce(id.attempt_nonce)) return 2;
    if (auth) key.fill(0x12);
    const auto ready = ggml_backend_rpc_halofpx_state_stage(&id, &staged, 1, object.data(), key.data());
    if (auth) {
        std::printf("status=%u negative=auth\n", unsigned(ready.status));
        return ready.status == GGML_RPC_HALOFPX_STATE_REJECTED ? 0 : 9;
    }
    if (expected_miss) {
        std::printf("status=%u\n", unsigned(ready.status));
        return ready.status == GGML_RPC_HALOFPX_STATE_MISS ? 0 : 4;
    }
    if (ready.status != GGML_RPC_HALOFPX_STATE_READY) return 5;
    if (mode == "stage-replay") {
        const auto replayed = ggml_backend_rpc_halofpx_state_stage(&id, &staged, 1, object.data(), key.data());
        std::printf("status=%u negative=stage-replay\n", unsigned(replayed.status));
        return replayed.status == GGML_RPC_HALOFPX_STATE_REJECTED ? 0 : 13;
    }
    if (mode == "destroy") {
        f.buffer.reset();
        std::unique_ptr<ggml_context, context_deleter> replacement_ctx;
        ggml_init_params params { ggml_tensor_overhead(), nullptr, true };
        replacement_ctx.reset(ggml_init(params));
        if (!replacement_ctx) return 14;
        ggml_tensor * replacement = ggml_new_tensor_1d(replacement_ctx.get(), GGML_TYPE_F32, 16);
        auto buft = ggml_backend_get_default_buffer_type(f.backend.get());
        std::unique_ptr<ggml_backend_buffer, buffer_deleter> replacement_buffer(
            ggml_backend_alloc_ctx_tensors_from_buft(replacement_ctx.get(), buft));
        if (!replacement_buffer) return 14;
        auto replacement_live = component(replacement);
        const auto rejected = ggml_backend_rpc_halofpx_state_commit_apply(
            &id, &replacement_live, 1, object.data(), ready.worker_nonce, key.data());
        std::printf("status=%u negative=destroy-before-commit\n", unsigned(rejected.status));
        return rejected.status == GGML_RPC_HALOFPX_STATE_REJECTED ? 0 : 14;
    }
    if (mode == "timeout") std::this_thread::sleep_for(std::chrono::milliseconds(5200));
    auto live = component(f.live);
    const auto applied = ggml_backend_rpc_halofpx_state_commit_apply(
        &id, &live, 1, object.data(), ready.worker_nonce, key.data());
    if (mode == "timeout") {
        std::printf("status=%u negative=timeout\n", unsigned(applied.status));
        return applied.status == GGML_RPC_HALOFPX_STATE_REJECTED ? 0 : 10;
    }
    if (applied.status != GGML_RPC_HALOFPX_STATE_APPLIED) return 6;
    std::array<float, 16> actual {};
    ggml_backend_tensor_get(f.live, actual.data(), 0, sizeof(actual));
    if (actual != expected) return 7;
    if (mode == "replay") {
        const auto replayed = ggml_backend_rpc_halofpx_state_commit_apply(
            &id, &live, 1, object.data(), ready.worker_nonce, key.data());
        std::printf("status=%u negative=replay\n", unsigned(replayed.status));
        if (replayed.status != GGML_RPC_HALOFPX_STATE_REJECTED) return 11;
    }
    std::printf("status=applied object=%s bytes=%llu components=%u\n",
        hex(object.data()).c_str(), (unsigned long long) applied.verified_bytes,
        applied.verified_components);
    return 0;
}
