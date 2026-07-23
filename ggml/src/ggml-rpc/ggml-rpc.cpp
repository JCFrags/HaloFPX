#include "ggml-rpc.h"
#include "ggml-impl.h"
#include "ggml-backend-impl.h"
#include "ggml-cpp.h"
#include "transport.h"

#include <array>
#include <cerrno>
#include <cinttypes>
#include <optional>
#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <cstring>
#include <fstream>
#include <filesystem>
#include <algorithm>

#ifdef GGML_RPC_HALOFPX_LOCAL_STATE
#if !defined(__BYTE_ORDER__) || __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
#error "GGML_RPC_HALOFPX_LOCAL_STATE requires a little-endian Linux target"
#endif
extern "C" {
#include "sha256/sha256.h"
}
#include <chrono>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/random.h>
#include <sys/types.h>
#include <unistd.h>
#endif

static const char * RPC_DEBUG = std::getenv("GGML_RPC_DEBUG");

#define LOG_DBG(...) \
    do { if (RPC_DEBUG) GGML_LOG_DEBUG(__VA_ARGS__); } while (0)


namespace fs = std::filesystem;

// macro for nicer error messages on server crash
#define RPC_STATUS_ASSERT(x) if (!(x)) GGML_ABORT("Remote RPC server crashed or returned malformed response")

// all RPC structures must be packed
#pragma pack(push, 1)
// ggml_tensor is serialized into rpc_tensor
struct rpc_tensor {
    uint64_t id;
    uint32_t type;
    uint64_t buffer;
    uint32_t ne[GGML_MAX_DIMS];
    uint32_t nb[GGML_MAX_DIMS];
    uint32_t op;
    int32_t  op_params[GGML_MAX_OP_PARAMS / sizeof(int32_t)];
    int32_t  flags;
    uint64_t src[GGML_MAX_SRC];
    uint64_t view_src;
    uint64_t view_offs;
    uint64_t data;
    char name[GGML_MAX_NAME];

    char padding[4];
};

static_assert(sizeof(rpc_tensor) % 8 == 0, "rpc_tensor size must be multiple of 8");

// RPC commands
enum rpc_cmd {
    RPC_CMD_ALLOC_BUFFER = 0,
    RPC_CMD_GET_ALIGNMENT,
    RPC_CMD_GET_MAX_SIZE,
    RPC_CMD_BUFFER_GET_BASE,
    RPC_CMD_FREE_BUFFER,
    RPC_CMD_BUFFER_CLEAR,
    RPC_CMD_SET_TENSOR,
    RPC_CMD_SET_TENSOR_HASH,
    RPC_CMD_GET_TENSOR,
    RPC_CMD_COPY_TENSOR,
    RPC_CMD_GRAPH_COMPUTE,
    RPC_CMD_GET_DEVICE_MEMORY,
    RPC_CMD_INIT_TENSOR,
    RPC_CMD_GET_ALLOC_SIZE,
    RPC_CMD_HELLO,
    RPC_CMD_DEVICE_COUNT,
    RPC_CMD_GRAPH_RECOMPUTE,
#ifdef GGML_RPC_HALOFPX_LOCAL_STATE
    RPC_CMD_HALOFPX_STATE_CAPS,
    RPC_CMD_HALOFPX_STATE_CAPTURE,
    RPC_CMD_HALOFPX_STATE_STAGE,
    RPC_CMD_HALOFPX_STATE_COMMIT_APPLY,
    RPC_CMD_HALOFPX_STATE_ABORT,
#endif
    RPC_CMD_COUNT,
};

static_assert(RPC_CMD_HELLO == 14, "RPC_CMD_HELLO must be always 14");
#ifdef GGML_RPC_HALOFPX_LOCAL_STATE
static_assert(RPC_CMD_HALOFPX_STATE_CAPS == 17, "HaloFPX CAPS command ordinal must be always 17");
#endif

// Try RPC_CMD_SET_TENSOR_HASH first when data size is larger than this threshold
const size_t HASH_THRESHOLD = 10 * 1024 * 1024;

struct rpc_msg_hello_req {
    uint8_t conn_caps[RPC_CONN_CAPS_SIZE];
};

struct rpc_msg_hello_rsp {
    uint8_t major;
    uint8_t minor;
    uint8_t patch;
    uint8_t padding;
    uint8_t conn_caps[RPC_CONN_CAPS_SIZE];
};

struct rpc_msg_device_count_rsp {
    uint32_t device_count;
};

struct rpc_msg_get_alloc_size_req {
    uint32_t   device;
    rpc_tensor tensor;
    rpc_tensor srcs[GGML_MAX_SRC];
};

struct rpc_msg_get_alloc_size_rsp {
    uint64_t alloc_size;
};

struct rpc_msg_init_tensor_req {
    rpc_tensor tensor;
};

struct rpc_msg_alloc_buffer_req {
    uint32_t device;
    uint64_t size;
};

struct rpc_msg_alloc_buffer_rsp {
    uint64_t remote_ptr;
    uint64_t remote_size;
};

struct rpc_msg_get_alignment_req {
    uint32_t device;
};

struct rpc_msg_get_alignment_rsp {
    uint64_t alignment;
};

struct rpc_msg_get_max_size_req {
    uint32_t device;
};

struct rpc_msg_get_max_size_rsp {
    uint64_t max_size;
};

struct rpc_msg_buffer_get_base_req {
    uint64_t remote_ptr;
};

struct rpc_msg_buffer_get_base_rsp {
    uint64_t base_ptr;
};

struct rpc_msg_free_buffer_req {
    uint64_t remote_ptr;
};

struct rpc_msg_buffer_clear_req {
    uint64_t remote_ptr;
    uint8_t value;
};

struct rpc_msg_set_tensor_hash_req {
    rpc_tensor tensor;
    uint64_t offset;
    uint64_t hash;
};

struct rpc_msg_set_tensor_hash_rsp {
    uint8_t result;
};

struct rpc_msg_get_tensor_req {
    rpc_tensor tensor;
    uint64_t offset;
    uint64_t size;
};

struct rpc_msg_copy_tensor_req {
    rpc_tensor src;
    rpc_tensor dst;
};

struct rpc_msg_copy_tensor_rsp {
    uint8_t result;
};

struct rpc_msg_get_device_memory_req {
    uint32_t device;
};

struct rpc_msg_get_device_memory_rsp {
    uint64_t free_mem;
    uint64_t total_mem;
};

struct rpc_msg_graph_recompute_req {
    uint32_t device;
};

#ifdef GGML_RPC_HALOFPX_LOCAL_STATE
static constexpr uint16_t HFX_STATE_MAJOR = 1;
static constexpr uint16_t HFX_STATE_MINOR = 0;
static constexpr uint32_t HFX_STATE_MAX_COMPONENTS = GGML_RPC_HALOFPX_STATE_MAX_COMPONENTS;
static constexpr uint64_t HFX_STATE_MAX_COMPONENT_BYTES = UINT64_C(1) << 30;
static constexpr uint64_t HFX_STATE_MAX_OBJECT_BYTES = UINT64_C(64) << 30;
static constexpr uint64_t HFX_STATE_TIMEOUT_MS = 5000;
static constexpr size_t HFX_STATE_MAX_SEEN_ATTEMPTS = 4096;
static constexpr uint32_t HFX_STATE_COMMAND_MASK =
    (UINT32_C(1) << (RPC_CMD_HALOFPX_STATE_CAPS - RPC_CMD_HALOFPX_STATE_CAPS)) |
    (UINT32_C(1) << (RPC_CMD_HALOFPX_STATE_CAPTURE - RPC_CMD_HALOFPX_STATE_CAPS)) |
    (UINT32_C(1) << (RPC_CMD_HALOFPX_STATE_STAGE - RPC_CMD_HALOFPX_STATE_CAPS)) |
    (UINT32_C(1) << (RPC_CMD_HALOFPX_STATE_COMMIT_APPLY - RPC_CMD_HALOFPX_STATE_CAPS)) |
    (UINT32_C(1) << (RPC_CMD_HALOFPX_STATE_ABORT - RPC_CMD_HALOFPX_STATE_CAPS));
static constexpr char HFX_STATE_DOMAIN[] = "halofpx.rpc-local-state.v1";

struct hfx_state_identity_wire {
    uint64_t key_generation;
    uint64_t generation;
    uint64_t token_count;
    uint64_t token_boundary;
    uint32_t world_size;
    uint32_t logical_rank;
    uint8_t model_digest[32];
    uint8_t compatibility_root[32];
    uint8_t plan_digest[32];
    uint8_t topology_digest[32];
    uint8_t placement_digest[32];
    uint8_t checkpoint_digest[32];
    uint8_t token_prefix_digest[32];
    uint8_t component_manifest_digest[32];
    uint8_t attempt_nonce[32];
    uint8_t channel_binding[32];
};

struct hfx_state_component_wire {
    uint64_t buffer;
    uint64_t data;
    uint64_t offset;
    uint64_t size;
    uint32_t ordinal;
    uint32_t kind;
    uint32_t type;
    uint32_t reserved;
    uint32_t ne[GGML_MAX_DIMS];
    uint32_t nb[GGML_MAX_DIMS];
    uint8_t label_digest[32];
};

struct hfx_state_request_header {
    uint8_t magic[8];
    uint16_t major;
    uint16_t minor;
    uint16_t message_type;
    uint16_t reserved;
    uint32_t encoded_size;
    uint32_t component_count;
    hfx_state_identity_wire identity;
    uint8_t expected_object_digest[32];
    uint8_t worker_nonce[32];
    uint8_t tag[32];
};

struct hfx_state_response_wire {
    uint8_t magic[8];
    uint16_t major;
    uint16_t minor;
    uint16_t message_type;
    uint16_t status;
    uint32_t logical_rank;
    uint32_t verified_components;
    uint64_t generation;
    uint64_t verified_bytes;
    uint8_t attempt_nonce[32];
    uint8_t object_digest[32];
    uint8_t worker_nonce[32];
    uint8_t channel_binding[32];
    uint8_t request_digest[32];
    uint8_t reserved[24];
    uint8_t tag[32];
};

struct hfx_state_caps_wire {
    uint8_t magic[8];
    uint16_t major;
    uint16_t minor;
    uint32_t encoded_size;
    uint32_t command_mask;
    uint32_t max_request;
    uint32_t max_response;
    uint32_t max_components;
    uint32_t logical_rank;
    uint32_t world_size;
    uint32_t reserved_zero;
    uint64_t max_component_bytes;
    uint64_t max_object_bytes;
    uint64_t timeout_ms;
    uint64_t key_generation;
    uint8_t channel_binding[32];
    uint8_t reserved[20];
};

static_assert(sizeof(hfx_state_component_wire) == 112, "unexpected HaloFPX component wire size");
static_assert(sizeof(hfx_state_request_header) == 480, "unexpected HaloFPX request header size");
static_assert(sizeof(hfx_state_response_wire) == 256, "HaloFPX response must be exactly 256 bytes");
static_assert(sizeof(hfx_state_caps_wire) == 128, "HaloFPX caps must be exactly 128 bytes");
#endif

#pragma pack(pop)

// RPC data structures

static ggml_guid_t ggml_backend_rpc_guid() {
    static ggml_guid guid = {0x99, 0x68, 0x5b, 0x6c, 0xd2, 0x83, 0x3d, 0x24, 0x25, 0x36, 0x72, 0xe1, 0x5b, 0x0e, 0x14, 0x03};
    return &guid;
}

struct ggml_backend_rpc_buffer_type_context {
    std::string endpoint;
    uint32_t    device;
    std::string name;
    size_t      alignment;
    size_t      max_size;
};

struct ggml_backend_rpc_context {
    std::string endpoint;
    uint32_t    device;
    std::string name;
    uint64_t    last_graph_uid;
};

struct ggml_backend_rpc_buffer_context {
    std::shared_ptr<socket_t> sock;
    void * base_ptr;
    uint64_t remote_ptr;
};

// RPC helper functions

// Computes FNV-1a hash of the data
static uint64_t fnv_hash(const uint8_t * data, size_t len) {
    const uint64_t fnv_prime = 0x100000001b3ULL;
    uint64_t hash = 0xcbf29ce484222325ULL;

    for (size_t i = 0; i < len; ++i) {
        hash ^= data[i];
        hash *= fnv_prime;
    }
    return hash;
}

#ifdef GGML_RPC_HALOFPX_LOCAL_STATE
namespace {

using hfx_digest = std::array<uint8_t, 32>;

void hfx_wipe(void * memory, size_t size) {
    volatile uint8_t * p = static_cast<volatile uint8_t *>(memory);
    while (size-- != 0) *p++ = 0;
}

hfx_digest hfx_sha256(const void * data, size_t size) {
    hfx_digest result {};
    sha256_t ctx;
    sha256_init(&ctx);
    if (size != 0) sha256_update(&ctx, static_cast<const uint8_t *>(data), size);
    sha256_final(&ctx, result.data());
    hfx_wipe(&ctx, sizeof(ctx));
    return result;
}

hfx_digest hfx_hmac(const uint8_t key[32], const void * data, size_t size) {
    std::array<uint8_t, 64> inner {};
    std::array<uint8_t, 64> outer {};
    for (size_t i = 0; i < 64; ++i) {
        const uint8_t b = i < 32 ? key[i] : 0;
        inner[i] = b ^ 0x36;
        outer[i] = b ^ 0x5c;
    }
    sha256_t ctx;
    hfx_digest mid {};
    hfx_digest result {};
    sha256_init(&ctx);
    sha256_update(&ctx, inner.data(), inner.size());
    sha256_update(&ctx, reinterpret_cast<const uint8_t *>(HFX_STATE_DOMAIN), sizeof(HFX_STATE_DOMAIN));
    if (size != 0) sha256_update(&ctx, static_cast<const uint8_t *>(data), size);
    sha256_final(&ctx, mid.data());
    sha256_init(&ctx);
    sha256_update(&ctx, outer.data(), outer.size());
    sha256_update(&ctx, mid.data(), mid.size());
    sha256_final(&ctx, result.data());
    hfx_wipe(&ctx, sizeof(ctx));
    hfx_wipe(inner.data(), inner.size());
    hfx_wipe(outer.data(), outer.size());
    hfx_wipe(mid.data(), mid.size());
    return result;
}

bool hfx_equal(const uint8_t * a, const uint8_t * b, size_t n) {
    uint8_t diff = 0;
    for (size_t i = 0; i < n; ++i) diff |= a[i] ^ b[i];
    return diff == 0;
}

bool hfx_zero(const uint8_t * p, size_t n) {
    uint8_t value = 0;
    for (size_t i = 0; i < n; ++i) value |= p[i];
    return value == 0;
}

void hfx_set_magic(uint8_t magic[8], const char value[8]) {
    memcpy(magic, value, 8);
}

bool hfx_magic(const uint8_t magic[8], const char value[8]) {
    return hfx_equal(magic, reinterpret_cast<const uint8_t *>(value), 8);
}

bool hfx_add(uint64_t a, uint64_t b, uint64_t & result) {
    if (a > UINT64_MAX - b) return false;
    result = a + b;
    return true;
}

bool hfx_mul(uint64_t a, uint64_t b, uint64_t & result) {
    if (a != 0 && b > UINT64_MAX / a) return false;
    result = a * b;
    return true;
}

std::string hfx_hex(const uint8_t * bytes, size_t size) {
    static constexpr char digits[] = "0123456789abcdef";
    std::string result(size * 2, '0');
    for (size_t i = 0; i < size; ++i) {
        result[2*i] = digits[bytes[i] >> 4];
        result[2*i + 1] = digits[bytes[i] & 15];
    }
    return result;
}

void hfx_identity_from_public(
        const ggml_backend_rpc_halofpx_state_identity & src,
        hfx_state_identity_wire & dst) {
    memset(&dst, 0, sizeof(dst));
    dst.key_generation = src.key_generation;
    dst.generation = src.generation;
    dst.token_count = src.token_count;
    dst.token_boundary = src.token_boundary;
    dst.world_size = src.world_size;
    dst.logical_rank = src.logical_rank;
#define HFX_COPY_ID(field) memcpy(dst.field, src.field, sizeof(dst.field))
    HFX_COPY_ID(model_digest);
    HFX_COPY_ID(compatibility_root);
    HFX_COPY_ID(plan_digest);
    HFX_COPY_ID(topology_digest);
    HFX_COPY_ID(placement_digest);
    HFX_COPY_ID(checkpoint_digest);
    HFX_COPY_ID(token_prefix_digest);
    HFX_COPY_ID(component_manifest_digest);
    HFX_COPY_ID(attempt_nonce);
    HFX_COPY_ID(channel_binding);
#undef HFX_COPY_ID
}

bool hfx_identity_stable_equal(
        const hfx_state_identity_wire & a,
        const hfx_state_identity_wire & b) {
    hfx_state_identity_wire ca = a;
    hfx_state_identity_wire cb = b;
    memset(ca.attempt_nonce, 0, sizeof(ca.attempt_nonce));
    memset(cb.attempt_nonce, 0, sizeof(cb.attempt_nonce));
    return hfx_equal(reinterpret_cast<const uint8_t *>(&ca),
                     reinterpret_cast<const uint8_t *>(&cb), sizeof(ca));
}

hfx_digest hfx_object_key(
        const uint8_t key[32],
        const hfx_state_identity_wire & identity) {
    hfx_state_identity_wire stable = identity;
    memset(stable.attempt_nonce, 0, sizeof(stable.attempt_nonce));
    return hfx_hmac(key, &stable, sizeof(stable));
}

bool hfx_request_shape(const std::vector<uint8_t> & input,
                       uint16_t message_type,
                       const hfx_state_request_header *& header,
                       const hfx_state_component_wire *& components) {
    if (input.size() < sizeof(hfx_state_request_header) ||
        input.size() > GGML_RPC_HALOFPX_STATE_MAX_REQUEST) return false;
    header = reinterpret_cast<const hfx_state_request_header *>(input.data());
    if (!hfx_magic(header->magic, "HFXREQ1\0") ||
        header->major != HFX_STATE_MAJOR || header->minor != HFX_STATE_MINOR ||
        header->message_type != message_type || header->reserved != 0 ||
        header->encoded_size != input.size() ||
        header->component_count > HFX_STATE_MAX_COMPONENTS) return false;
    const uint64_t expected = sizeof(hfx_state_request_header) +
        uint64_t(header->component_count) * sizeof(hfx_state_component_wire);
    if (expected != input.size()) return false;
    components = reinterpret_cast<const hfx_state_component_wire *>(
        input.data() + sizeof(hfx_state_request_header));
    uint64_t total = 0;
    for (uint32_t i = 0; i < header->component_count; ++i) {
        const auto & c = components[i];
        if (c.reserved != 0 || c.ordinal != i ||
            (c.kind != GGML_RPC_HALOFPX_COMPONENT_ATTENTION_K &&
             c.kind != GGML_RPC_HALOFPX_COMPONENT_ATTENTION_V) ||
            c.type >= GGML_TYPE_COUNT || c.size == 0 ||
            c.size > HFX_STATE_MAX_COMPONENT_BYTES ||
            !hfx_add(total, c.size, total) || total > HFX_STATE_MAX_OBJECT_BYTES) return false;
        const auto type = static_cast<ggml_type>(c.type);
        const uint64_t block = ggml_blck_size(type);
        const uint64_t type_size = ggml_type_size(type);
        if (block == 0 || type_size == 0 || c.ne[0] == 0 || c.ne[0] % block != 0 ||
            c.ne[1] == 0 || c.ne[2] == 0 || c.ne[3] == 0 || c.nb[0] != type_size) return false;
        uint64_t expected_stride = 0;
        if (!hfx_mul(type_size, c.ne[0] / block, expected_stride) || c.nb[1] != expected_stride ||
            !hfx_mul(expected_stride, c.ne[1], expected_stride) || c.nb[2] != expected_stride ||
            !hfx_mul(expected_stride, c.ne[2], expected_stride) || c.nb[3] != expected_stride ||
            !hfx_mul(expected_stride, c.ne[3], expected_stride) || expected_stride > HFX_STATE_MAX_COMPONENT_BYTES) return false;
        uint64_t logical_end = 0;
        if (!hfx_add(c.offset, c.size, logical_end) || logical_end > expected_stride) return false;
        uint64_t current_begin = 0;
        uint64_t current_end = 0;
        if (!hfx_add(c.data, c.offset, current_begin) ||
            !hfx_add(current_begin, c.size, current_end)) return false;
        for (uint32_t j = 0; j < i; ++j) {
            if (components[j].buffer != c.buffer) continue;
            uint64_t other_begin = 0;
            uint64_t other_end = 0;
            if (!hfx_add(components[j].data, components[j].offset, other_begin) ||
                !hfx_add(other_begin, components[j].size, other_end)) return false;
            if (current_begin < other_end && other_begin < current_end) return false;
        }
    }
    return true;
}

bool hfx_verify_request_auth(
        const std::vector<uint8_t> & input,
        const uint8_t key[32]) {
    std::vector<uint8_t> copy = input;
    auto * header = reinterpret_cast<hfx_state_request_header *>(copy.data());
    const hfx_digest supplied = [&]() {
        hfx_digest d {};
        memcpy(d.data(), header->tag, d.size());
        return d;
    }();
    memset(header->tag, 0, sizeof(header->tag));
    const auto expected = hfx_hmac(key, copy.data(), copy.size());
    return hfx_equal(supplied.data(), expected.data(), supplied.size());
}

void hfx_sign_response(hfx_state_response_wire & response, const uint8_t key[32]) {
    memset(response.tag, 0, sizeof(response.tag));
    const auto tag = hfx_hmac(key, &response, sizeof(response));
    memcpy(response.tag, tag.data(), tag.size());
}

void hfx_bind_response(
        hfx_state_response_wire & response,
        const std::vector<uint8_t> & request,
        const uint8_t key[32]) {
    const auto digest = hfx_sha256(request.data(), request.size());
    memcpy(response.request_digest, digest.data(), digest.size());
    hfx_sign_response(response, key);
}

hfx_state_response_wire hfx_response(
        uint16_t message_type,
        ggml_backend_rpc_halofpx_state_status status,
        const hfx_state_identity_wire & identity,
        const uint8_t key[32]) {
    hfx_state_response_wire response {};
    hfx_set_magic(response.magic, "HFXRSP1\0");
    response.major = HFX_STATE_MAJOR;
    response.minor = HFX_STATE_MINOR;
    response.message_type = message_type;
    response.status = static_cast<uint16_t>(status);
    response.logical_rank = identity.logical_rank;
    response.generation = identity.generation;
    memcpy(response.attempt_nonce, identity.attempt_nonce, sizeof(response.attempt_nonce));
    memcpy(response.channel_binding, identity.channel_binding, sizeof(response.channel_binding));
    hfx_sign_response(response, key);
    return response;
}

bool hfx_write_all(int fd, const void * data, size_t size) {
    const uint8_t * p = static_cast<const uint8_t *>(data);
    while (size != 0) {
        const ssize_t n = write(fd, p, size);
        if (n <= 0) return false;
        p += n;
        size -= static_cast<size_t>(n);
    }
    return true;
}

bool hfx_read_all(int fd, void * data, size_t size) {
    uint8_t * p = static_cast<uint8_t *>(data);
    while (size != 0) {
        const ssize_t n = read(fd, p, size);
        if (n <= 0) return false;
        p += n;
        size -= static_cast<size_t>(n);
    }
    return true;
}

bool hfx_random_all(uint8_t * data, size_t size) {
    while (size != 0) {
        const ssize_t n = getrandom(data, size, 0);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) return false;
        data += static_cast<size_t>(n);
        size -= static_cast<size_t>(n);
    }
    return true;
}

} // namespace
#endif

static bool send_msg(socket_ptr sock, const void * msg, size_t msg_size) {
    if (!sock->send_data(&msg_size, sizeof(msg_size))) {
        return false;
    }
    return sock->send_data(msg, msg_size);
}

static bool recv_msg(socket_ptr sock, void * msg, size_t msg_size) {
    uint64_t size;
    if (!sock->recv_data(&size, sizeof(size))) {
        return false;
    }
    if (size != msg_size) {
        return false;
    }
    return sock->recv_data(msg, msg_size);
}

static bool recv_msg(socket_ptr sock, std::vector<uint8_t> & input) {
    uint64_t size;
    if (!sock->recv_data(&size, sizeof(size))) {
        return false;
    }
    try {
        input.resize(size);
    } catch (const std::bad_alloc & e) {
        GGML_LOG_ERROR("Failed to allocate input buffer of size %" PRIu64 "\n", size);
        return false;
    }
    return sock->recv_data(input.data(), size);
}

static bool parse_endpoint(const std::string & endpoint, std::string & host, int & port) {
    size_t pos = endpoint.find(':');
    if (pos == std::string::npos) {
        return false;
    }
    host = endpoint.substr(0, pos);
    try {
        port = std::stoi(endpoint.substr(pos + 1));
    } catch (...) {
        return false;
    }
    return true;
}

// RPC request : | rpc_cmd (1 byte) | request_size (8 bytes) | request_data (request_size bytes) |
// No response
static bool send_rpc_cmd(socket_ptr sock, enum rpc_cmd cmd, const void * input, size_t input_size) {
    uint8_t cmd_byte = cmd;
    if (!sock->send_data(&cmd_byte, sizeof(cmd_byte))) {
        return false;
    }
    if (!sock->send_data(&input_size, sizeof(input_size))) {
        return false;
    }
    if (!sock->send_data(input, input_size)) {
        return false;
    }
    return true;
}

// RPC request : | rpc_cmd (1 byte) | request_size (8 bytes) | request_data (request_size bytes) |
// RPC response: | response_size (8 bytes) | response_data (response_size bytes) |
static bool send_rpc_cmd(socket_ptr sock, enum rpc_cmd cmd, const void * input, size_t input_size, void * output, size_t output_size) {
    if (!send_rpc_cmd(sock, cmd, input, input_size)) {
        return false;
    }
    uint64_t out_size;
    if (!sock->recv_data(&out_size, sizeof(out_size))) {
        return false;
    }
    if (out_size != output_size) {
        return false;
    }
    if (!sock->recv_data(output, output_size)) {
        return false;
    }
    return true;
}

// RPC client-side implementation

// Performs HELLO handshake with transport auto-negotiation.
// Advertises local capabilities via conn_caps; if the server responds with
// matching capabilities, the socket is upgraded transparently.
static bool negotiate_hello(const std::shared_ptr<socket_t> & sock) {
    rpc_msg_hello_req request = {};
    rpc_msg_hello_rsp response = {};

    sock->get_caps(request.conn_caps);

    bool status = send_rpc_cmd(sock, RPC_CMD_HELLO, &request, sizeof(request), &response, sizeof(response));
    RPC_STATUS_ASSERT(status);

    if (response.major != RPC_PROTO_MAJOR_VERSION || response.minor > RPC_PROTO_MINOR_VERSION) {
        GGML_LOG_ERROR("RPC server version mismatch: %d.%d.%d\n",
                       response.major, response.minor, response.patch);
        return false;
    }

    sock->update_caps(response.conn_caps);
    return true;
}

static std::shared_ptr<socket_t> get_socket(const std::string & endpoint) {
    static std::mutex mutex;
    std::lock_guard<std::mutex> lock(mutex);
    static std::unordered_map<std::string, std::weak_ptr<socket_t>> sockets;

    auto it = sockets.find(endpoint);
    if (it != sockets.end()) {
        if (auto sock = it->second.lock()) {
            return sock;
        }
    }
    std::string host;
    int port;
    if (!parse_endpoint(endpoint, host, port)) {
        GGML_LOG_ERROR("Failed to parse endpoint: %s\n", endpoint.c_str());
        return nullptr;
    }

    if (!rpc_transport_init()) {
        return nullptr;
    }
    auto sock = socket_t::connect(host.c_str(), port);
    if (sock == nullptr) {
        return nullptr;
    }
    if (!negotiate_hello(sock)) {
        return nullptr;
    }
    LOG_DBG("[%s] connected to %s\n", __func__, endpoint.c_str());
    sockets[endpoint] = sock;
    return sock;
}

static void ggml_backend_rpc_buffer_free_buffer(ggml_backend_buffer_t buffer) {
    ggml_backend_rpc_buffer_context * ctx = (ggml_backend_rpc_buffer_context *)buffer->context;
    rpc_msg_free_buffer_req request = {ctx->remote_ptr};
    bool status = send_rpc_cmd(ctx->sock, RPC_CMD_FREE_BUFFER, &request, sizeof(request), nullptr, 0);
    RPC_STATUS_ASSERT(status);
    delete ctx;
}

static void * ggml_backend_rpc_buffer_get_base(ggml_backend_buffer_t buffer) {
    ggml_backend_rpc_buffer_context * ctx = (ggml_backend_rpc_buffer_context *)buffer->context;
    if (ctx->base_ptr != nullptr) {
        return ctx->base_ptr;
    }
    rpc_msg_buffer_get_base_req request = {ctx->remote_ptr};
    rpc_msg_buffer_get_base_rsp response;
    bool status = send_rpc_cmd(ctx->sock, RPC_CMD_BUFFER_GET_BASE, &request, sizeof(request), &response, sizeof(response));
    RPC_STATUS_ASSERT(status);
    ctx->base_ptr = reinterpret_cast<void *>(response.base_ptr);
    return ctx->base_ptr;
}

static bool ggml_backend_buffer_is_rpc(ggml_backend_buffer_t buffer) {
    return buffer->iface.free_buffer == ggml_backend_rpc_buffer_free_buffer;
}

static rpc_tensor serialize_tensor(const ggml_tensor * tensor) {
    rpc_tensor result;
    if (!tensor) {
        memset(&result, 0, sizeof(result));
        return result;
    }

    result.id = reinterpret_cast<uint64_t>(tensor);
    result.type = tensor->type;
    if (tensor->buffer && ggml_backend_buffer_is_rpc(tensor->buffer)) {
        ggml_backend_buffer_t buffer = tensor->buffer;
        ggml_backend_rpc_buffer_context * ctx = (ggml_backend_rpc_buffer_context *)buffer->context;
        result.buffer = ctx != nullptr ? ctx->remote_ptr : 0;
        result.data = reinterpret_cast<uint64_t>(tensor->data);
    } else {
        result.buffer = 0;
        result.data   = 0;
    }
    for (uint32_t i = 0; i < GGML_MAX_DIMS; i++) {
        result.ne[i] = tensor->ne[i];
        result.nb[i] = tensor->nb[i];
    }
    result.op = tensor->op;
    for (uint32_t i = 0; i < GGML_MAX_OP_PARAMS / sizeof(int32_t); i++) {
        result.op_params[i] = tensor->op_params[i];
    }
    result.flags = tensor->flags;
    for (uint32_t i = 0; i < GGML_MAX_SRC; i++) {
        result.src[i] = reinterpret_cast<uint64_t>(tensor->src[i]);
    }
    result.view_src = reinterpret_cast<uint64_t>(tensor->view_src);
    result.view_offs = tensor->view_offs;

    // Avoid sending uninitialized data over the wire
    memset(result.name, 0, sizeof(result.name));
    memset(result.padding, 0, sizeof(result.padding));

    snprintf(result.name, GGML_MAX_NAME, "%s", tensor->name);
    return result;
}

static bool recv_msg_bounded(socket_ptr sock, std::vector<uint8_t> & input, uint64_t max_size) {
    uint64_t size;
    if (!sock->recv_data(&size, sizeof(size)) || size > max_size) {
        return false;
    }
    try {
        input.resize(static_cast<size_t>(size));
    } catch (const std::bad_alloc & e) {
        GGML_LOG_ERROR("Failed to allocate bounded input buffer of size %" PRIu64 "\n", size);
        return false;
    }
    return sock->recv_data(input.data(), input.size());
}

#ifdef GGML_RPC_HALOFPX_LOCAL_STATE
namespace {

std::mutex hfx_client_attempt_mutex;
std::unordered_map<std::string, std::weak_ptr<socket_t>> hfx_client_attempt_sockets;
std::mutex hfx_server_seen_attempt_mutex;
std::unordered_set<std::string> hfx_server_seen_attempts;

bool hfx_accept_attempt_nonce(const uint8_t nonce[32]) {
    std::lock_guard<std::mutex> lock(hfx_server_seen_attempt_mutex);
    if (hfx_server_seen_attempts.size() >= HFX_STATE_MAX_SEEN_ATTEMPTS) return false;
    return hfx_server_seen_attempts.emplace(reinterpret_cast<const char *>(nonce), 32).second;
}

std::shared_ptr<socket_t> hfx_component_socket(const ggml_backend_rpc_halofpx_state_component & component) {
    if (!component.tensor || !component.tensor->buffer ||
        !ggml_backend_buffer_is_rpc(component.tensor->buffer)) return nullptr;
    auto * ctx = static_cast<ggml_backend_rpc_buffer_context *>(component.tensor->buffer->context);
    return ctx ? ctx->sock : nullptr;
}

bool hfx_component_from_public(
        const ggml_backend_rpc_halofpx_state_component & src,
        hfx_state_component_wire & dst) {
    if (!src.tensor || src.ordinal > UINT32_MAX || src.size == 0) return false;
    const rpc_tensor rpc = serialize_tensor(src.tensor);
    if (rpc.buffer == 0 || rpc.data == 0) return false;
    memset(&dst, 0, sizeof(dst));
    dst.buffer = rpc.buffer;
    dst.data = rpc.data;
    dst.offset = src.offset;
    dst.size = src.size;
    dst.ordinal = src.ordinal;
    dst.kind = src.kind;
    dst.type = rpc.type;
    memcpy(dst.ne, rpc.ne, sizeof(dst.ne));
    memcpy(dst.nb, rpc.nb, sizeof(dst.nb));
    memcpy(dst.label_digest, src.label_digest, sizeof(dst.label_digest));
    return true;
}

ggml_backend_rpc_halofpx_state_result hfx_public_result_disabled() {
    ggml_backend_rpc_halofpx_state_result result {};
    result.status = GGML_RPC_HALOFPX_STATE_DISABLED;
    return result;
}

bool hfx_verify_response(
        const hfx_state_response_wire & response,
        uint16_t message_type,
        const hfx_state_identity_wire & identity,
        const uint8_t request_digest[32],
        const uint8_t key[32]) {
    if (!hfx_magic(response.magic, "HFXRSP1\0") || response.major != HFX_STATE_MAJOR ||
        response.minor != HFX_STATE_MINOR || response.message_type != message_type ||
        response.status > GGML_RPC_HALOFPX_STATE_APPLY_ERROR ||
        response.logical_rank != identity.logical_rank || response.generation != identity.generation ||
        !hfx_equal(response.attempt_nonce, identity.attempt_nonce, 32) ||
        !hfx_equal(response.channel_binding, identity.channel_binding, 32) ||
        !hfx_equal(response.request_digest, request_digest, 32) ||
        !hfx_zero(response.reserved, sizeof(response.reserved))) return false;
    hfx_state_response_wire copy = response;
    memset(copy.tag, 0, sizeof(copy.tag));
    const auto tag = hfx_hmac(key, &copy, sizeof(copy));
    return hfx_equal(tag.data(), response.tag, tag.size());
}

bool hfx_verify_response_semantics(
        const hfx_state_response_wire & response,
        uint16_t message_type,
        uint32_t component_count,
        uint64_t component_bytes,
        const uint8_t expected_object_digest[32],
        const uint8_t worker_nonce[32]) {
    const auto status = static_cast<ggml_backend_rpc_halofpx_state_status>(response.status);
    if (status == GGML_RPC_HALOFPX_STATE_STORED) {
        return message_type == RPC_CMD_HALOFPX_STATE_CAPTURE &&
            response.verified_components == component_count && response.verified_bytes == component_bytes &&
            !hfx_zero(response.object_digest, 32) && hfx_zero(response.worker_nonce, 32);
    }
    if (status == GGML_RPC_HALOFPX_STATE_READY) {
        return message_type == RPC_CMD_HALOFPX_STATE_STAGE && expected_object_digest &&
            response.verified_components == component_count && response.verified_bytes == component_bytes &&
            hfx_equal(response.object_digest, expected_object_digest, 32) && !hfx_zero(response.worker_nonce, 32);
    }
    if (status == GGML_RPC_HALOFPX_STATE_APPLIED) {
        return message_type == RPC_CMD_HALOFPX_STATE_COMMIT_APPLY && expected_object_digest && worker_nonce &&
            response.verified_components == component_count && response.verified_bytes == component_bytes &&
            hfx_equal(response.object_digest, expected_object_digest, 32) &&
            hfx_equal(response.worker_nonce, worker_nonce, 32);
    }
    if (status == GGML_RPC_HALOFPX_STATE_ABORTED) {
        return message_type == RPC_CMD_HALOFPX_STATE_ABORT && response.verified_components == 0 &&
            response.verified_bytes == 0 && hfx_zero(response.object_digest, 32) && hfx_zero(response.worker_nonce, 32);
    }
    if (status != GGML_RPC_HALOFPX_STATE_MISS && status != GGML_RPC_HALOFPX_STATE_REJECTED &&
        status != GGML_RPC_HALOFPX_STATE_STORAGE_ERROR && status != GGML_RPC_HALOFPX_STATE_APPLY_ERROR) return false;
    return response.verified_components == 0 && response.verified_bytes == 0 &&
        hfx_zero(response.object_digest, 32) && hfx_zero(response.worker_nonce, 32);
}

ggml_backend_rpc_halofpx_state_result hfx_public_result(const hfx_state_response_wire & response) {
    ggml_backend_rpc_halofpx_state_result result {};
    result.status = static_cast<ggml_backend_rpc_halofpx_state_status>(response.status);
    result.logical_rank = response.logical_rank;
    result.generation = response.generation;
    result.verified_bytes = response.verified_bytes;
    result.verified_components = response.verified_components;
    memcpy(result.object_digest, response.object_digest, 32);
    memcpy(result.worker_nonce, response.worker_nonce, 32);
    return result;
}

ggml_backend_rpc_halofpx_state_result hfx_client_request(
        uint16_t message_type,
        const ggml_backend_rpc_halofpx_state_identity * public_identity,
        const ggml_backend_rpc_halofpx_state_component * public_components,
        size_t component_count,
        const uint8_t expected_object_digest[32],
        const uint8_t worker_nonce[32],
        const uint8_t control_key[32]) {
    auto failed = hfx_public_result_disabled();
    failed.status = GGML_RPC_HALOFPX_STATE_REJECTED;
    if (!public_identity || !control_key || component_count > HFX_STATE_MAX_COMPONENTS ||
        (component_count != 0 && !public_components)) return failed;
    if (message_type != RPC_CMD_HALOFPX_STATE_ABORT && component_count == 0) return failed;
    const uint64_t size = sizeof(hfx_state_request_header) + component_count * sizeof(hfx_state_component_wire);
    if (size > GGML_RPC_HALOFPX_STATE_MAX_REQUEST) return failed;
    std::shared_ptr<socket_t> sock;
    if (component_count != 0) {
        sock = hfx_component_socket(public_components[0]);
        if (!sock) return failed;
    } else {
        std::lock_guard<std::mutex> lock(hfx_client_attempt_mutex);
        const std::string key(reinterpret_cast<const char *>(public_identity->attempt_nonce), 32);
        auto it = hfx_client_attempt_sockets.find(key);
        if (it != hfx_client_attempt_sockets.end()) sock = it->second.lock();
        if (!sock) return failed;
    }
    hfx_state_caps_wire caps {};
    if (!send_rpc_cmd(sock, RPC_CMD_HALOFPX_STATE_CAPS, nullptr, 0, &caps, sizeof(caps)) ||
        !hfx_magic(caps.magic, "HFXCAP2\0") || caps.major != HFX_STATE_MAJOR ||
        caps.minor != HFX_STATE_MINOR || caps.encoded_size != sizeof(caps) ||
        caps.command_mask != HFX_STATE_COMMAND_MASK ||
        caps.max_request != GGML_RPC_HALOFPX_STATE_MAX_REQUEST ||
        caps.max_response != sizeof(hfx_state_response_wire) ||
        caps.max_components != HFX_STATE_MAX_COMPONENTS || caps.max_component_bytes != HFX_STATE_MAX_COMPONENT_BYTES ||
        caps.max_object_bytes != HFX_STATE_MAX_OBJECT_BYTES || caps.timeout_ms != HFX_STATE_TIMEOUT_MS ||
        caps.logical_rank != public_identity->logical_rank || caps.world_size != public_identity->world_size ||
        caps.reserved_zero != 0 || caps.key_generation != public_identity->key_generation ||
        !hfx_equal(caps.channel_binding, public_identity->channel_binding, 32) ||
        !hfx_zero(caps.reserved, sizeof(caps.reserved))) return failed;
    std::vector<uint8_t> input(size, 0);
    auto * header = reinterpret_cast<hfx_state_request_header *>(input.data());
    hfx_set_magic(header->magic, "HFXREQ1\0");
    header->major = HFX_STATE_MAJOR;
    header->minor = HFX_STATE_MINOR;
    header->message_type = message_type;
    header->encoded_size = static_cast<uint32_t>(size);
    header->component_count = static_cast<uint32_t>(component_count);
    hfx_identity_from_public(*public_identity, header->identity);
    if (expected_object_digest) memcpy(header->expected_object_digest, expected_object_digest, 32);
    if (worker_nonce) memcpy(header->worker_nonce, worker_nonce, 32);
    auto * components = reinterpret_cast<hfx_state_component_wire *>(input.data() + sizeof(*header));
    uint64_t component_bytes = 0;
    for (size_t i = 0; i < component_count; ++i) {
        if (public_components[i].ordinal != i ||
            !hfx_component_from_public(public_components[i], components[i]) ||
            hfx_component_socket(public_components[i]) != sock ||
            !hfx_add(component_bytes, public_components[i].size, component_bytes)) return failed;
    }
    const auto tag = hfx_hmac(control_key, input.data(), input.size());
    memcpy(header->tag, tag.data(), tag.size());
    const auto request_digest = hfx_sha256(input.data(), input.size());
    hfx_state_response_wire response {};
    if (!send_rpc_cmd(sock, static_cast<rpc_cmd>(message_type), input.data(), input.size(), &response, sizeof(response)) ||
        !hfx_verify_response(response, message_type, header->identity, request_digest.data(), control_key) ||
        !hfx_verify_response_semantics(response, message_type, static_cast<uint32_t>(component_count),
                                       component_bytes, expected_object_digest, worker_nonce)) return failed;
    if (message_type == RPC_CMD_HALOFPX_STATE_STAGE && response.status == GGML_RPC_HALOFPX_STATE_READY) {
        std::lock_guard<std::mutex> lock(hfx_client_attempt_mutex);
        hfx_client_attempt_sockets[std::string(reinterpret_cast<const char *>(public_identity->attempt_nonce), 32)] = sock;
    }
    if ((message_type == RPC_CMD_HALOFPX_STATE_COMMIT_APPLY &&
         response.status == GGML_RPC_HALOFPX_STATE_APPLIED) ||
        message_type == RPC_CMD_HALOFPX_STATE_ABORT) {
        std::lock_guard<std::mutex> lock(hfx_client_attempt_mutex);
        hfx_client_attempt_sockets.erase(std::string(reinterpret_cast<const char *>(public_identity->attempt_nonce), 32));
    }
    return hfx_public_result(response);
}

} // namespace
#endif

static enum ggml_status ggml_backend_rpc_buffer_init_tensor(ggml_backend_buffer_t buffer, ggml_tensor * tensor) {
    ggml_backend_rpc_buffer_context * ctx = (ggml_backend_rpc_buffer_context *)buffer->context;

    // CUDA backend on the server pads everything to 512 due to CUDA limitations.
    // Due to bandwidth constraints, we only call the server init tensor functions if necessary.
    // In particular, only quantized tensors need padding
    if (ggml_is_quantized(tensor->type) && (tensor->ne[0] % 512 != 0) && (tensor->view_src == nullptr)) {
        rpc_msg_init_tensor_req request;

        request.tensor = serialize_tensor(tensor);

        bool status = send_rpc_cmd(ctx->sock, RPC_CMD_INIT_TENSOR, &request, sizeof(request), nullptr, 0);
        RPC_STATUS_ASSERT(status);
    }
    return GGML_STATUS_SUCCESS;
}

static void ggml_backend_rpc_buffer_set_tensor(ggml_backend_buffer_t buffer, ggml_tensor * tensor, const void * data, size_t offset, size_t size) {
    ggml_backend_rpc_buffer_context * ctx = (ggml_backend_rpc_buffer_context *)buffer->context;
    rpc_tensor rpc_tensor = serialize_tensor(tensor);
    if (size > HASH_THRESHOLD) {
        rpc_msg_set_tensor_hash_req request;
        request.tensor = rpc_tensor;
        request.offset = offset;
        request.hash = fnv_hash((const uint8_t*)data, size);
        rpc_msg_set_tensor_hash_rsp response;
        bool status = send_rpc_cmd(ctx->sock, RPC_CMD_SET_TENSOR_HASH, &request, sizeof(request), &response, sizeof(response));
        RPC_STATUS_ASSERT(status);
        if (response.result) {
            // the server has the same data, no need to send it
            return;
        }
    }
    // input serialization format: | rpc_tensor | offset (8 bytes) | data (size bytes)
    size_t input_size = sizeof(rpc_tensor) + sizeof(uint64_t) + size;
    std::vector<uint8_t> input(input_size, 0);
    memcpy(input.data(), &rpc_tensor, sizeof(rpc_tensor));
    memcpy(input.data() + sizeof(rpc_tensor), &offset, sizeof(offset));
    memcpy(input.data() + sizeof(rpc_tensor) + sizeof(offset), data, size);
    bool status = send_rpc_cmd(ctx->sock, RPC_CMD_SET_TENSOR, input.data(), input.size());
    RPC_STATUS_ASSERT(status);
}

static void ggml_backend_rpc_buffer_get_tensor(ggml_backend_buffer_t buffer, const ggml_tensor * tensor, void * data, size_t offset, size_t size) {
    ggml_backend_rpc_buffer_context * ctx = (ggml_backend_rpc_buffer_context *)buffer->context;
    rpc_msg_get_tensor_req request;
    request.tensor = serialize_tensor(tensor);
    request.offset = offset;
    request.size = size;
    bool status = send_rpc_cmd(ctx->sock, RPC_CMD_GET_TENSOR, &request, sizeof(request), data, size);
    RPC_STATUS_ASSERT(status);
}

static bool ggml_backend_rpc_buffer_cpy_tensor(ggml_backend_buffer_t buffer, const ggml_tensor * src, ggml_tensor * dst) {
    if (ggml_backend_buffer_is_rpc(src->buffer)) {
        // check if src and dst are on the same server
        ggml_backend_buffer_t src_buffer = src->buffer;
        ggml_backend_rpc_buffer_context * src_ctx = (ggml_backend_rpc_buffer_context *)src_buffer->context;
        ggml_backend_buffer_t dst_buffer = dst->buffer;
        ggml_backend_rpc_buffer_context * dst_ctx = (ggml_backend_rpc_buffer_context *)dst_buffer->context;
        if (src_ctx->sock != dst_ctx->sock) {
            return false;
        }
        ggml_backend_rpc_buffer_context * ctx = (ggml_backend_rpc_buffer_context *)buffer->context;
        rpc_msg_copy_tensor_req request;
        request.src = serialize_tensor(src);
        request.dst = serialize_tensor(dst);
        rpc_msg_copy_tensor_rsp response;
        bool status = send_rpc_cmd(ctx->sock, RPC_CMD_COPY_TENSOR, &request, sizeof(request), &response, sizeof(response));
        RPC_STATUS_ASSERT(status);
        return response.result;
    }
    return false;
}

static void ggml_backend_rpc_buffer_clear(ggml_backend_buffer_t buffer, uint8_t value) {
    ggml_backend_rpc_buffer_context * ctx = (ggml_backend_rpc_buffer_context *)buffer->context;
    rpc_msg_buffer_clear_req request = {ctx->remote_ptr, value};
    bool status = send_rpc_cmd(ctx->sock, RPC_CMD_BUFFER_CLEAR, &request, sizeof(request), nullptr, 0);
    RPC_STATUS_ASSERT(status);
}

static ggml_backend_buffer_i ggml_backend_rpc_buffer_interface = {
    /* .free_buffer     = */ ggml_backend_rpc_buffer_free_buffer,
    /* .get_base        = */ ggml_backend_rpc_buffer_get_base,
    /* .init_tensor     = */ ggml_backend_rpc_buffer_init_tensor,
    /* .memset_tensor   = */ NULL,
    /* .set_tensor      = */ ggml_backend_rpc_buffer_set_tensor,
    /* .get_tensor      = */ ggml_backend_rpc_buffer_get_tensor,
    /* .set_tensor_2d   = */ NULL,
    /* .get_tensor_2d   = */ NULL,
    /* .cpy_tensor      = */ ggml_backend_rpc_buffer_cpy_tensor,
    /* .clear           = */ ggml_backend_rpc_buffer_clear,
    /* .reset           = */ NULL,
};

static const char * ggml_backend_rpc_buffer_type_name(ggml_backend_buffer_type_t buft) {
    ggml_backend_rpc_buffer_type_context * buft_ctx = (ggml_backend_rpc_buffer_type_context *)buft->context;
    return buft_ctx->name.c_str();
}

static ggml_backend_buffer_t ggml_backend_rpc_buffer_type_alloc_buffer(ggml_backend_buffer_type_t buft, size_t size) {
    ggml_backend_rpc_buffer_type_context * buft_ctx = (ggml_backend_rpc_buffer_type_context *)buft->context;
    rpc_msg_alloc_buffer_req request = {buft_ctx->device, size};
    rpc_msg_alloc_buffer_rsp response;
    auto sock = get_socket(buft_ctx->endpoint);
    bool status = send_rpc_cmd(sock, RPC_CMD_ALLOC_BUFFER, &request, sizeof(request), &response, sizeof(response));
    RPC_STATUS_ASSERT(status);
    if (response.remote_ptr != 0) {
        ggml_backend_buffer_t buffer = ggml_backend_buffer_init(buft,
            ggml_backend_rpc_buffer_interface,
            new ggml_backend_rpc_buffer_context{sock, nullptr, response.remote_ptr},
            response.remote_size);
        return buffer;
    } else {
        return nullptr;
    }
}

static size_t get_alignment(const std::shared_ptr<socket_t> & sock, uint32_t device) {
    rpc_msg_get_alignment_req request = {device};
    rpc_msg_get_alignment_rsp response;
    bool status = send_rpc_cmd(sock, RPC_CMD_GET_ALIGNMENT, &request, sizeof(request), &response, sizeof(response));
    RPC_STATUS_ASSERT(status);
    return response.alignment;
}

static size_t ggml_backend_rpc_buffer_type_get_alignment(ggml_backend_buffer_type_t buft) {
    ggml_backend_rpc_buffer_type_context * buft_ctx = (ggml_backend_rpc_buffer_type_context *)buft->context;
    return buft_ctx->alignment;
}

static size_t get_max_size(const std::shared_ptr<socket_t> & sock, uint32_t device) {
    rpc_msg_get_max_size_req request = {device};
    rpc_msg_get_max_size_rsp response;
    bool status = send_rpc_cmd(sock, RPC_CMD_GET_MAX_SIZE, &request, sizeof(request), &response, sizeof(response));
    RPC_STATUS_ASSERT(status);
    return response.max_size;
}

static size_t ggml_backend_rpc_get_max_size(ggml_backend_buffer_type_t buft) {
    ggml_backend_rpc_buffer_type_context * buft_ctx = (ggml_backend_rpc_buffer_type_context *)buft->context;
    return buft_ctx->max_size;
}

static size_t ggml_backend_rpc_buffer_type_get_alloc_size(ggml_backend_buffer_type_t buft, const ggml_tensor * tensor) {
    // should we query the remote server for the actual size
    bool rpc_get = false;

    // See comments in init_tensor.
    rpc_get |= ggml_is_quantized(tensor->type) && (tensor->ne[0] % 512 != 0) && (tensor->view_src == nullptr);

    // ops that require additional memory for fleeting data on certain backends
    // ref: https://github.com/ggml-org/llama.cpp/pull/15966
    rpc_get |= tensor->op == GGML_OP_FLASH_ATTN_EXT;
    rpc_get |= tensor->op == GGML_OP_MUL_MAT_ID;

    if (rpc_get) {
        ggml_backend_rpc_buffer_type_context * buft_ctx = (ggml_backend_rpc_buffer_type_context *)buft->context;
        auto sock = get_socket(buft_ctx->endpoint);

        rpc_msg_get_alloc_size_req request = {
            /*.device =*/ buft_ctx->device,
            /*.tensor =*/ serialize_tensor(tensor),
            /*.srcs   =*/ {},
        };

        // .get_alloc_size could be a function of the tensor's srcs, so we must serialize them as well
        for (int i = 0; i < GGML_MAX_SRC; i++) {
            request.srcs[i] = serialize_tensor(tensor->src[i]);
        }

        // TODO: cache the alloc responses to avoid extra RPC calls?
        rpc_msg_get_alloc_size_rsp response;
        bool status = send_rpc_cmd(sock, RPC_CMD_GET_ALLOC_SIZE, &request, sizeof(request), &response, sizeof(response));
        RPC_STATUS_ASSERT(status);

        return response.alloc_size;
    }

    return ggml_nbytes(tensor);
}

static ggml_backend_buffer_type_i ggml_backend_rpc_buffer_type_interface = {
    /* .get_name         = */ ggml_backend_rpc_buffer_type_name,
    /* .alloc_buffer     = */ ggml_backend_rpc_buffer_type_alloc_buffer,
    /* .get_alignment    = */ ggml_backend_rpc_buffer_type_get_alignment,
    /* .get_max_size     = */ ggml_backend_rpc_get_max_size,
    /* .get_alloc_size   = */ ggml_backend_rpc_buffer_type_get_alloc_size,
    /* .is_host          = */ NULL,
};

static const char * ggml_backend_rpc_name(ggml_backend_t backend) {
    ggml_backend_rpc_context * rpc_ctx = (ggml_backend_rpc_context *)backend->context;

    return rpc_ctx->name.c_str();
}

static void ggml_backend_rpc_free(ggml_backend_t backend) {
    ggml_backend_rpc_context * rpc_ctx = (ggml_backend_rpc_context *)backend->context;
    delete rpc_ctx;
    delete backend;
}

static void ggml_backend_rpc_synchronize(ggml_backend_t backend) {
    GGML_UNUSED(backend);
    // this is no-op because we don't have any async operations
}

static void add_tensor(ggml_tensor * tensor, std::vector<rpc_tensor> & tensors, std::unordered_set<ggml_tensor*> & visited) {
    if (tensor == nullptr) {
        return;
    }
    if (visited.find(tensor) != visited.end()) {
        return;
    }
    visited.insert(tensor);
    for (int i = 0; i < GGML_MAX_SRC; i++) {
        add_tensor(tensor->src[i], tensors, visited);
    }
    add_tensor(tensor->view_src, tensors, visited);
    tensors.push_back(serialize_tensor(tensor));
}

static void serialize_graph(uint32_t device, const ggml_cgraph * cgraph, std::vector<uint8_t> & output) {
    uint32_t n_nodes = cgraph->n_nodes;
    std::vector<rpc_tensor> tensors;
    std::unordered_set<ggml_tensor*> visited;
    for (uint32_t i = 0; i < n_nodes; i++) {
        add_tensor(cgraph->nodes[i], tensors, visited);
    }
    // serialization format:
    // | device (4 bytes) | n_nodes (4 bytes) | nodes (n_nodes * sizeof(uint64_t) | n_tensors (4 bytes) | tensors (n_tensors * sizeof(rpc_tensor)) |
    uint32_t n_tensors = tensors.size();
    int output_size = 2*sizeof(uint32_t) + n_nodes * sizeof(uint64_t) + sizeof(uint32_t) + n_tensors * sizeof(rpc_tensor);
    output.resize(output_size, 0);
    uint8_t * dest = output.data();
    memcpy(dest, &device, sizeof(device));
    dest += sizeof(device);
    memcpy(dest, &n_nodes, sizeof(n_nodes));
    dest += sizeof(n_nodes);
    for (uint32_t i = 0; i < n_nodes; i++) {
        memcpy(dest + i * sizeof(uint64_t), &cgraph->nodes[i], sizeof(uint64_t));
    }
    dest += n_nodes * sizeof(uint64_t);
    memcpy(dest, &n_tensors, sizeof(n_tensors));
    dest += sizeof(n_tensors);
    rpc_tensor * out_tensors = (rpc_tensor *)dest;
    memcpy(out_tensors, tensors.data(), n_tensors * sizeof(rpc_tensor));
}

static enum ggml_status ggml_backend_rpc_graph_compute(ggml_backend_t backend, ggml_cgraph * cgraph) {
    ggml_backend_rpc_context * rpc_ctx = (ggml_backend_rpc_context *)backend->context;

    GGML_ASSERT(cgraph->n_nodes > 0);
    bool reuse = cgraph->uid != 0 && rpc_ctx->last_graph_uid == cgraph->uid;
    if (reuse) {
        rpc_msg_graph_recompute_req request;
        request.device = rpc_ctx->device;
        auto sock = get_socket(rpc_ctx->endpoint);
        bool status = send_rpc_cmd(sock, RPC_CMD_GRAPH_RECOMPUTE, &request, sizeof(request));
        RPC_STATUS_ASSERT(status);
    } else {
        rpc_ctx->last_graph_uid = cgraph->uid;
        std::vector<uint8_t> input;
        serialize_graph(rpc_ctx->device, cgraph, input);
        auto sock = get_socket(rpc_ctx->endpoint);
        bool status = send_rpc_cmd(sock, RPC_CMD_GRAPH_COMPUTE, input.data(), input.size());
        RPC_STATUS_ASSERT(status);
    }
    return GGML_STATUS_SUCCESS;
}

static ggml_backend_i ggml_backend_rpc_interface = {
    /* .get_name                = */ ggml_backend_rpc_name,
    /* .free                    = */ ggml_backend_rpc_free,
    /* .set_tensor_async        = */ NULL,
    /* .get_tensor_async        = */ NULL,
    /* .set_tensor_2d_async     = */ NULL,
    /* .get_tensor_2d_async     = */ NULL,
    /* .cpy_tensor_async        = */ NULL,
    /* .synchronize             = */ ggml_backend_rpc_synchronize,
    /* .graph_plan_create       = */ NULL,
    /* .graph_plan_free         = */ NULL,
    /* .graph_plan_update       = */ NULL,
    /* .graph_plan_compute      = */ NULL,
    /* .graph_compute           = */ ggml_backend_rpc_graph_compute,
    /* .event_record            = */ NULL,
    /* .event_wait              = */ NULL,
    /* .graph_optimize          = */ NULL,
};

ggml_backend_buffer_type_t ggml_backend_rpc_buffer_type(const char * endpoint, uint32_t device) {
    static std::mutex mutex;
    std::lock_guard<std::mutex> lock(mutex);
    std::string buft_name = "RPC" + std::to_string(device) + "[" + std::string(endpoint) + "]";
    // NOTE: buffer types are allocated and never freed; this is by design
    static std::unordered_map<std::string, ggml_backend_buffer_type_t> buft_map;
    auto it = buft_map.find(buft_name);
    if (it != buft_map.end()) {
        return it->second;
    }
    auto sock = get_socket(endpoint);
    if (sock == nullptr) {
        GGML_LOG_ERROR("Failed to connect to %s\n", endpoint);
        return nullptr;
    }
    size_t alignment = get_alignment(sock, device);
    size_t max_size = get_max_size(sock, device);
    ggml_backend_rpc_buffer_type_context * buft_ctx = new ggml_backend_rpc_buffer_type_context {
        /* .endpoint  = */ endpoint,
        /* .device    = */ device,
        /* .name      = */ buft_name,
        /* .alignment = */ alignment,
        /* .max_size  = */ max_size
    };
    auto reg = ggml_backend_rpc_add_server(endpoint);
    ggml_backend_buffer_type_t buft = new ggml_backend_buffer_type {
        /* .iface   = */ ggml_backend_rpc_buffer_type_interface,
        /* .device  = */ ggml_backend_reg_dev_get(reg, device),
        /* .context = */ buft_ctx
    };
    buft_map[buft_name] = buft;
    return buft;
}

ggml_backend_t ggml_backend_rpc_init(const char * endpoint, uint32_t device) {
    std::string dev_name = "RPC" + std::to_string(device) + "[" + std::string(endpoint) + "]";
    ggml_backend_rpc_context * ctx = new ggml_backend_rpc_context {
        /* .endpoint       = */ endpoint,
        /* .device         = */ device,
        /* .name           = */ dev_name,
        /* .last_graph_uid = */ 0,
    };
    auto reg = ggml_backend_rpc_add_server(endpoint);
    ggml_backend_t backend = new ggml_backend {
        /* .guid    = */ ggml_backend_rpc_guid(),
        /* .iface   = */ ggml_backend_rpc_interface,
        /* .device  = */ ggml_backend_reg_dev_get(reg, device),
        /* .context = */ ctx
    };
    return backend;
}

bool ggml_backend_is_rpc(ggml_backend_t backend) {
    return backend != NULL && ggml_guid_matches(backend->guid, ggml_backend_rpc_guid());
}

static void get_device_memory(const std::shared_ptr<socket_t> & sock, uint32_t device, size_t * free, size_t * total) {
    rpc_msg_get_device_memory_req request;
    request.device = device;
    rpc_msg_get_device_memory_rsp response;
    bool status = send_rpc_cmd(sock, RPC_CMD_GET_DEVICE_MEMORY, &request, sizeof(request), &response, sizeof(response));
    RPC_STATUS_ASSERT(status);
    *free = response.free_mem;
    *total = response.total_mem;
}

void ggml_backend_rpc_get_device_memory(const char * endpoint, uint32_t device, size_t * free, size_t * total) {
    auto sock = get_socket(endpoint);
    if (sock == nullptr) {
        *free = 0;
        *total = 0;
        return;
    }
    get_device_memory(sock, device, free, total);
}

ggml_backend_rpc_halofpx_state_result ggml_backend_rpc_halofpx_state_capture(
        const ggml_backend_rpc_halofpx_state_identity * identity,
        const ggml_backend_rpc_halofpx_state_component * components,
        size_t component_count,
        const uint8_t control_key[GGML_RPC_HALOFPX_STATE_KEY_BYTES]) {
#ifdef GGML_RPC_HALOFPX_LOCAL_STATE
    return hfx_client_request(RPC_CMD_HALOFPX_STATE_CAPTURE, identity, components, component_count,
                              nullptr, nullptr, control_key);
#else
    GGML_UNUSED(identity); GGML_UNUSED(components); GGML_UNUSED(component_count); GGML_UNUSED(control_key);
    ggml_backend_rpc_halofpx_state_result result {};
    result.status = GGML_RPC_HALOFPX_STATE_DISABLED;
    return result;
#endif
}

ggml_backend_rpc_halofpx_state_result ggml_backend_rpc_halofpx_state_stage(
        const ggml_backend_rpc_halofpx_state_identity * identity,
        const ggml_backend_rpc_halofpx_state_component * components,
        size_t component_count,
        const uint8_t expected_object_digest[GGML_RPC_HALOFPX_STATE_DIGEST_BYTES],
        const uint8_t control_key[GGML_RPC_HALOFPX_STATE_KEY_BYTES]) {
#ifdef GGML_RPC_HALOFPX_LOCAL_STATE
    return hfx_client_request(RPC_CMD_HALOFPX_STATE_STAGE, identity, components, component_count,
                              expected_object_digest, nullptr, control_key);
#else
    GGML_UNUSED(identity); GGML_UNUSED(components); GGML_UNUSED(component_count);
    GGML_UNUSED(expected_object_digest); GGML_UNUSED(control_key);
    ggml_backend_rpc_halofpx_state_result result {};
    result.status = GGML_RPC_HALOFPX_STATE_DISABLED;
    return result;
#endif
}

ggml_backend_rpc_halofpx_state_result ggml_backend_rpc_halofpx_state_commit_apply(
        const ggml_backend_rpc_halofpx_state_identity * identity,
        const ggml_backend_rpc_halofpx_state_component * live_components,
        size_t component_count,
        const uint8_t expected_object_digest[GGML_RPC_HALOFPX_STATE_DIGEST_BYTES],
        const uint8_t worker_nonce[GGML_RPC_HALOFPX_STATE_DIGEST_BYTES],
        const uint8_t control_key[GGML_RPC_HALOFPX_STATE_KEY_BYTES]) {
#ifdef GGML_RPC_HALOFPX_LOCAL_STATE
    return hfx_client_request(RPC_CMD_HALOFPX_STATE_COMMIT_APPLY, identity, live_components, component_count,
                              expected_object_digest, worker_nonce, control_key);
#else
    GGML_UNUSED(identity); GGML_UNUSED(live_components); GGML_UNUSED(component_count);
    GGML_UNUSED(expected_object_digest); GGML_UNUSED(worker_nonce); GGML_UNUSED(control_key);
    ggml_backend_rpc_halofpx_state_result result {};
    result.status = GGML_RPC_HALOFPX_STATE_DISABLED;
    return result;
#endif
}

ggml_backend_rpc_halofpx_state_result ggml_backend_rpc_halofpx_state_abort(
        const ggml_backend_rpc_halofpx_state_identity * identity,
        const uint8_t worker_nonce[GGML_RPC_HALOFPX_STATE_DIGEST_BYTES],
        const uint8_t control_key[GGML_RPC_HALOFPX_STATE_KEY_BYTES]) {
#ifdef GGML_RPC_HALOFPX_LOCAL_STATE
    return hfx_client_request(RPC_CMD_HALOFPX_STATE_ABORT, identity, nullptr, 0,
                              nullptr, worker_nonce, control_key);
#else
    GGML_UNUSED(identity); GGML_UNUSED(worker_nonce); GGML_UNUSED(control_key);
    ggml_backend_rpc_halofpx_state_result result {};
    result.status = GGML_RPC_HALOFPX_STATE_DISABLED;
    return result;
#endif
}

bool ggml_backend_rpc_halofpx_state_sha256(const void * data, size_t size, uint8_t digest[32]) {
#ifdef GGML_RPC_HALOFPX_LOCAL_STATE
    if ((size != 0 && data == nullptr) || digest == nullptr) return false;
    const auto value = hfx_sha256(data, size);
    memcpy(digest, value.data(), value.size());
    return true;
#else
    GGML_UNUSED(data); GGML_UNUSED(size); GGML_UNUSED(digest);
    return false;
#endif
}

// RPC server-side implementation

#ifdef GGML_RPC_HALOFPX_LOCAL_STATE
#pragma pack(push, 1)
struct hfx_state_object_header {
    uint8_t magic[8];
    uint16_t major;
    uint16_t minor;
    uint32_t header_size;
    uint32_t component_count;
    uint32_t reserved;
    uint64_t payload_bytes;
    hfx_state_identity_wire identity;
};

struct hfx_state_object_component {
    hfx_state_component_wire descriptor;
    uint8_t content_digest[32];
};
#pragma pack(pop)

static bool hfx_state_diagnostics_enabled() {
    const char * value = std::getenv("HALOFPX_STATE_DIAGNOSTICS");
    return value && std::strcmp(value, "1") == 0;
}

static void hfx_state_log_component_digest(
        const char * phase,
        const std::vector<hfx_state_object_component> & components) {
    if (!hfx_state_diagnostics_enabled()) return;
    const auto digest = hfx_sha256(
        components.data(), components.size() * sizeof(components[0]));
    GGML_LOG_INFO(
        "[halofpx-state-diag] phase=%s components=%zu descriptor_content_sha256=%s\n",
        phase, components.size(), hfx_hex(digest.data(), digest.size()).c_str());
}

struct hfx_state_server_config_owned {
    std::string root;
    uint32_t logical_rank = 0;
    uint32_t world_size = 0;
    uint64_t key_generation = 0;
    std::array<uint8_t, 32> control_key {};
    std::array<uint8_t, 32> channel_binding {};
};

struct hfx_state_pending_attempt {
    hfx_state_identity_wire identity {};
    std::vector<hfx_state_component_wire> staged;
    std::array<uint8_t, 32> object_digest {};
    std::array<uint8_t, 32> worker_nonce {};
    std::chrono::steady_clock::time_point expires {};
};
#endif

class rpc_server {
public:
    rpc_server(std::vector<ggml_backend_t> all_backends, const char * cache_dir
#ifdef GGML_RPC_HALOFPX_LOCAL_STATE
            , const hfx_state_server_config_owned * hfx_state_config
#endif
            )
        : backends(std::move(all_backends)), cache_dir(cache_dir)
#ifdef GGML_RPC_HALOFPX_LOCAL_STATE
        , hfx_state_config(hfx_state_config ? std::optional<hfx_state_server_config_owned>(*hfx_state_config) : std::nullopt)
#endif
        {
        stored_graphs.resize(backends.size());
    }
    ~rpc_server();

    void hello(rpc_msg_hello_rsp & response);
    bool alloc_buffer(const rpc_msg_alloc_buffer_req & request, rpc_msg_alloc_buffer_rsp & response);
    bool get_alignment(const rpc_msg_get_alignment_req & request, rpc_msg_get_alignment_rsp & response);
    bool get_max_size(const rpc_msg_get_max_size_req & request, rpc_msg_get_max_size_rsp & response);
    bool buffer_get_base(const rpc_msg_buffer_get_base_req & request, rpc_msg_buffer_get_base_rsp & response);
    bool free_buffer(const rpc_msg_free_buffer_req & request);
    bool buffer_clear(const rpc_msg_buffer_clear_req & request);
    bool set_tensor(const std::vector<uint8_t> & input);
    bool set_tensor_hash(const rpc_msg_set_tensor_hash_req & request, rpc_msg_set_tensor_hash_rsp & response);
    bool get_tensor(const rpc_msg_get_tensor_req & request, std::vector<uint8_t> & response);
    bool copy_tensor(const rpc_msg_copy_tensor_req & request, rpc_msg_copy_tensor_rsp & response);
    bool graph_compute(const std::vector<uint8_t> & input);
    bool graph_recompute(const rpc_msg_graph_recompute_req & request);
    bool init_tensor(const rpc_msg_init_tensor_req & request);
    bool get_alloc_size(const rpc_msg_get_alloc_size_req & request, rpc_msg_get_alloc_size_rsp & response);
    bool get_device_memory(const rpc_msg_get_device_memory_req & request, rpc_msg_get_device_memory_rsp & response);
#ifdef GGML_RPC_HALOFPX_LOCAL_STATE
    bool hfx_state_capture(const std::vector<uint8_t> & input, hfx_state_response_wire & response);
    bool hfx_state_stage(const std::vector<uint8_t> & input, hfx_state_response_wire & response);
    bool hfx_state_commit_apply(const std::vector<uint8_t> & input, hfx_state_response_wire & response);
    bool hfx_state_abort(const std::vector<uint8_t> & input, hfx_state_response_wire & response);
    void hfx_state_discard_for_legacy_mutation();
#endif

    struct stored_graph {
        std::vector<uint8_t>   buffer;
        ggml_cgraph          * graph;
    };

private:
    bool get_cached_file(uint64_t hash, std::vector<uint8_t> & data);
    ggml_tensor * deserialize_tensor(struct ggml_context * ctx, const rpc_tensor * tensor);
    ggml_tensor * create_node(uint64_t id,
                              struct ggml_context * ctx,
                              const std::unordered_map<uint64_t, const rpc_tensor*> & tensor_ptrs,
                              std::unordered_map<uint64_t, struct ggml_tensor*> & tensor_map);


    std::vector<ggml_backend_t> backends;
    const char * cache_dir;
    std::unordered_set<ggml_backend_buffer_t> buffers;
    // store the last computed graph for each backend
    std::vector<stored_graph> stored_graphs;
#ifdef GGML_RPC_HALOFPX_LOCAL_STATE
    std::optional<hfx_state_server_config_owned> hfx_state_config;
    std::optional<hfx_state_pending_attempt> hfx_state_pending;
#endif
};

void rpc_server::hello(rpc_msg_hello_rsp & response) {
    response.major = RPC_PROTO_MAJOR_VERSION;
    response.minor = RPC_PROTO_MINOR_VERSION;
    response.patch = RPC_PROTO_PATCH_VERSION;
    LOG_DBG("[%s] version: %d.%d.%d\n", __func__, response.major, response.minor, response.patch);
}

bool rpc_server::get_alloc_size(const rpc_msg_get_alloc_size_req & request, rpc_msg_get_alloc_size_rsp & response) {
    uint32_t dev_id = request.device;
    if (dev_id >= backends.size()) {
        return false;
    }
    ggml_backend_buffer_type_t buft;
    struct ggml_init_params params {
        /*.mem_size   =*/ ggml_tensor_overhead()*(1 + GGML_MAX_SRC),
        /*.mem_buffer =*/ NULL,
        /*.no_alloc   =*/ true,
    };

    ggml_context_ptr ctx_ptr { ggml_init(params) };
    GGML_ASSERT(ctx_ptr != nullptr);
    ggml_context * ctx = ctx_ptr.get();

    ggml_tensor * tensor = deserialize_tensor(ctx, &request.tensor);
    if (tensor == nullptr) {
        GGML_LOG_ERROR("Null tensor pointer passed to server get_alloc_size function.\n");
        return false;
    }
    for (int i = 0; i < GGML_MAX_SRC; i++) {
        if (request.srcs[i].id != 0) {
            tensor->src[i] = deserialize_tensor(ctx, &request.srcs[i]);
        }
    }

    LOG_DBG("[%s] device: %d, buffer: %p, data: %p\n", __func__, dev_id, (void*)tensor->buffer, tensor->data);
    if (tensor->buffer == nullptr) {
        //No buffer allocated.
        buft = ggml_backend_get_default_buffer_type(backends[dev_id]);
    } else {
        buft = tensor->buffer->buft;
    }

    response.alloc_size = ggml_backend_buft_get_alloc_size(buft, tensor);

    return true;
}

bool rpc_server::alloc_buffer(const rpc_msg_alloc_buffer_req & request, rpc_msg_alloc_buffer_rsp & response) {
    uint32_t dev_id = request.device;
    if (dev_id >= backends.size()) {
        return false;
    }
    ggml_backend_buffer_type_t buft = ggml_backend_get_default_buffer_type(backends[dev_id]);
    ggml_backend_buffer_t buffer = ggml_backend_buft_alloc_buffer(buft, request.size);
    response.remote_ptr = 0;
    response.remote_size = 0;
    if (buffer != nullptr) {
        response.remote_ptr = reinterpret_cast<uint64_t>(buffer);
        response.remote_size = buffer->size;
        LOG_DBG("[%s] device: %d, size: %" PRIu64 " -> remote_ptr: %" PRIx64 ", remote_size: %" PRIu64 "\n",
            __func__, dev_id, request.size, response.remote_ptr, response.remote_size);
        buffers.insert(buffer);
    } else {
        LOG_DBG("[%s] device: %d, size: %" PRIu64 " -> failed\n", __func__, dev_id, request.size);
    }
    return true;
}

bool rpc_server::get_alignment(const rpc_msg_get_alignment_req & request, rpc_msg_get_alignment_rsp & response) {
    uint32_t dev_id = request.device;
    if (dev_id >= backends.size()) {
        return false;
    }
    ggml_backend_buffer_type_t buft = ggml_backend_get_default_buffer_type(backends[dev_id]);
    size_t alignment = ggml_backend_buft_get_alignment(buft);
    LOG_DBG("[%s] device: %d, alignment: %lu\n", __func__, dev_id, alignment);
    response.alignment = alignment;
    return true;
}

bool rpc_server::get_max_size(const rpc_msg_get_max_size_req & request, rpc_msg_get_max_size_rsp & response) {
    uint32_t dev_id = request.device;
    if (dev_id >= backends.size()) {
        return false;
    }
    ggml_backend_buffer_type_t buft = ggml_backend_get_default_buffer_type(backends[dev_id]);
    size_t max_size = ggml_backend_buft_get_max_size(buft);
    LOG_DBG("[%s] device: %d, max_size: %lu\n", __func__, dev_id, max_size);
    response.max_size = max_size;
    return true;
}

bool rpc_server::buffer_get_base(const rpc_msg_buffer_get_base_req & request, rpc_msg_buffer_get_base_rsp & response) {
    LOG_DBG("[%s] remote_ptr: %" PRIx64 "\n", __func__, request.remote_ptr);
    ggml_backend_buffer_t buffer = reinterpret_cast<ggml_backend_buffer_t>(request.remote_ptr);
    if (buffers.find(buffer) == buffers.end()) {
        GGML_LOG_ERROR("[%s] buffer not found\n", __func__);
        return false;
    }
    void * base = ggml_backend_buffer_get_base(buffer);
    response.base_ptr = reinterpret_cast<uint64_t>(base);
    return true;
}

bool rpc_server::free_buffer(const rpc_msg_free_buffer_req & request) {
    LOG_DBG("[%s] remote_ptr: %" PRIx64 "\n", __func__, request.remote_ptr);
    ggml_backend_buffer_t buffer = reinterpret_cast<ggml_backend_buffer_t>(request.remote_ptr);
    if (buffers.find(buffer) == buffers.end()) {
        GGML_LOG_ERROR("[%s] buffer not found\n", __func__);
        return false;
    }
    ggml_backend_buffer_free(buffer);
    buffers.erase(buffer);
    return true;
}

bool rpc_server::buffer_clear(const rpc_msg_buffer_clear_req & request) {
    LOG_DBG("[%s] remote_ptr: %" PRIx64 ", value: %u\n", __func__, request.remote_ptr, request.value);
    ggml_backend_buffer_t buffer = reinterpret_cast<ggml_backend_buffer_t>(request.remote_ptr);
    if (buffers.find(buffer) == buffers.end()) {
        GGML_LOG_ERROR("[%s] buffer not found\n", __func__);
        return false;
    }
    ggml_backend_buffer_clear(buffer, request.value);
    return true;
}

ggml_tensor * rpc_server::deserialize_tensor(struct ggml_context * ctx, const rpc_tensor * tensor) {
    // Validate tensor type before using it
    if (tensor->type >= GGML_TYPE_COUNT) {
        GGML_LOG_ERROR("[%s] invalid tensor type received: %u\n", __func__, tensor->type);
        return nullptr;
    }

    // Fix: Prevent division by zero if blck_size is 0 (e.g., deprecated types)
    if (ggml_blck_size((enum ggml_type)tensor->type) == 0) {
        GGML_LOG_ERROR("[%s] invalid tensor type received (blck_size is 0): %u\n", __func__, tensor->type);
        return nullptr;
    }

    ggml_tensor * result = ggml_new_tensor_4d(ctx, (ggml_type) tensor->type,
        tensor->ne[0], tensor->ne[1], tensor->ne[2], tensor->ne[3]);

    // ggml_new_tensor_4d might fail if dimensions are invalid, although less likely to crash than invalid type
    if (result == nullptr) {
        GGML_LOG_ERROR("[%s] ggml_new_tensor_4d failed for type %u\n", __func__, tensor->type);
        return nullptr;
    }

    for (uint32_t i = 0; i < GGML_MAX_DIMS; i++) {
        result->nb[i] = tensor->nb[i];
    }
    result->buffer = reinterpret_cast<ggml_backend_buffer_t>(tensor->buffer);
    if (result->buffer && buffers.find(result->buffer) == buffers.end()) {
        result->buffer = nullptr;
    }

    if (result->buffer) {
        // require that the tensor data does not go beyond the buffer end
        uint64_t tensor_size = (uint64_t) ggml_nbytes(result);
        uint64_t buffer_start = (uint64_t) ggml_backend_buffer_get_base(result->buffer);
        uint64_t buffer_size = (uint64_t) ggml_backend_buffer_get_size(result->buffer);
        GGML_ASSERT(tensor->data + tensor_size >= tensor->data); // check for overflow
        GGML_ASSERT(tensor->data >= buffer_start && tensor->data + tensor_size <= buffer_start + buffer_size);
    }

    result->op = (ggml_op) tensor->op;
    for (uint32_t i = 0; i < GGML_MAX_OP_PARAMS / sizeof(int32_t); i++) {
        result->op_params[i] = tensor->op_params[i];
    }
    result->flags = tensor->flags;
    result->data = reinterpret_cast<void *>(tensor->data);
    ggml_set_name(result, tensor->name);
    return result;
}


bool rpc_server::set_tensor(const std::vector<uint8_t> & input) {
    // serialization format: | rpc_tensor | offset (8 bytes) | data (size bytes) |
    if (input.size() < sizeof(rpc_tensor) + sizeof(uint64_t)) {
        return false;
    }
    const rpc_tensor * in_tensor = (const rpc_tensor *)input.data();
    uint64_t offset;
    memcpy(&offset, input.data() + sizeof(rpc_tensor), sizeof(offset));
    const size_t size = input.size() - sizeof(rpc_tensor) - sizeof(offset);

    struct ggml_init_params params {
        /*.mem_size   =*/ ggml_tensor_overhead(),
        /*.mem_buffer =*/ NULL,
        /*.no_alloc   =*/ true,
    };
    ggml_context_ptr ctx_ptr { ggml_init(params) };
    GGML_ASSERT(ctx_ptr != nullptr);
    ggml_context * ctx = ctx_ptr.get();
    ggml_tensor * tensor = deserialize_tensor(ctx, in_tensor);
    if (tensor == nullptr || tensor->buffer == nullptr) {
        GGML_LOG_ERROR("[%s] error deserializing tensor\n", __func__);
        return false;
    }
    LOG_DBG("[%s] buffer: %p, data: %p, offset: %" PRIu64 ", size: %zu\n", __func__, (void*)tensor->buffer, tensor->data, offset, size);

    // sanitize tensor->data
    {
        const size_t p0 = (size_t) ggml_backend_buffer_get_base(tensor->buffer);
        const size_t p1 = p0 + ggml_backend_buffer_get_size(tensor->buffer);

        if (in_tensor->data + offset < p0 || in_tensor->data + offset >= p1 || size > (p1 - in_tensor->data - offset)) {
            GGML_LOG_ERROR("[%s] tensor data region (data=0x%" PRIx64 ", offset=%" PRIu64 ", size=%zu) out of buffer bounds [0x%zx, 0x%zx)\n",
                           __func__, in_tensor->data, offset, size, p0, p1);
            return false;
        }
    }

    const void * data = input.data() + sizeof(rpc_tensor) + sizeof(offset);
    if (cache_dir && size > HASH_THRESHOLD) {
        uint64_t hash = fnv_hash((const uint8_t*)data, size);
        char hash_str[17];
        snprintf(hash_str, sizeof(hash_str), "%016" PRIx64, hash);
        // save to cache_dir/hash_str
        fs::path cache_file = fs::path(cache_dir) / hash_str;
        std::ofstream ofs(cache_file, std::ios::binary);
        ofs.write((const char *)data, size);
        GGML_LOG_INFO("[%s] saved to '%s'\n", __func__, cache_file.string().c_str());
    }
    ggml_backend_tensor_set(tensor, data, offset, size);
    return true;
}

bool rpc_server::get_cached_file(uint64_t hash, std::vector<uint8_t> & data) {
    if (!cache_dir) {
        return false;
    }
    char hash_str[17];
    snprintf(hash_str, sizeof(hash_str), "%016" PRIx64, hash);
    fs::path cache_file = fs::path(cache_dir) / hash_str;
    std::error_code ec;
    if (!fs::exists(cache_file, ec)) {
        return false;
    }
    std::ifstream ifs(cache_file, std::ios::binary);
    ifs.seekg(0, std::ios::end);
    size_t size = ifs.tellg();
    ifs.seekg(0, std::ios::beg);
    data.resize(size);
    ifs.read((char *)data.data(), size);
    return true;
}

bool rpc_server::set_tensor_hash(const rpc_msg_set_tensor_hash_req & request, rpc_msg_set_tensor_hash_rsp & response)
{
    std::vector<uint8_t> cached_file;
    if (!get_cached_file(request.hash, cached_file)) {
        response.result = 0;
        return true;
    }
    size_t size = cached_file.size();
    struct ggml_init_params params {
        /*.mem_size   =*/ ggml_tensor_overhead(),
        /*.mem_buffer =*/ NULL,
        /*.no_alloc   =*/ true,
    };
    ggml_context_ptr ctx_ptr { ggml_init(params) };
    GGML_ASSERT(ctx_ptr != nullptr);
    ggml_context * ctx = ctx_ptr.get();
    ggml_tensor * tensor = deserialize_tensor(ctx, &request.tensor);
    if (tensor == nullptr || tensor->buffer == nullptr) {
        GGML_LOG_ERROR("[%s] error deserializing tensor\n", __func__);
        return false;
    }
    LOG_DBG("[%s] buffer: %p, data: %p, offset: %" PRIu64 ", size: %zu, hash: %" PRIx64 "\n",
            __func__, (void*)tensor->buffer, tensor->data, request.offset, size, request.hash);

    // sanitize tensor->data
    {
        const size_t p0 = (size_t) ggml_backend_buffer_get_base(tensor->buffer);
        const size_t p1 = p0 + ggml_backend_buffer_get_size(tensor->buffer);

        if (request.tensor.data + request.offset < p0
         || request.tensor.data + request.offset >= p1
         || size > (p1 - request.tensor.data - request.offset)) {
            GGML_LOG_ERROR("[%s] tensor data region (data=0x%" PRIx64 ", offset=%" PRIu64 ", size=%zu, hash=0x%" PRIx64 ") out of buffer bounds [0x%zx, 0x%zx)\n",
                           __func__, request.tensor.data, request.offset, size, request.hash, p0, p1);
            return false;
        }
    }
    ggml_backend_tensor_set(tensor, cached_file.data(), request.offset, size);
    response.result = 1;
    return true;
}

bool rpc_server::init_tensor(const rpc_msg_init_tensor_req & request) {
    struct ggml_init_params params {
        /*.mem_size   =*/ ggml_tensor_overhead(),
        /*.mem_buffer =*/ NULL,
        /*.no_alloc   =*/ true,
    };
    ggml_context_ptr ctx_ptr { ggml_init(params) };
    GGML_ASSERT(ctx_ptr != nullptr);
    ggml_context * ctx = ctx_ptr.get();
    ggml_tensor * tensor = deserialize_tensor(ctx, &request.tensor);
    if (tensor == nullptr) {
        GGML_LOG_ERROR("Null tensor pointer passed to server init_tensor function.\n");
        return false;
    }
    LOG_DBG("[%s] buffer: %p, data: %p\n", __func__, (void*)tensor->buffer, tensor->data);
    // Call the backend's buffer_init_tensor function
    ggml_backend_buffer_t buffer = tensor->buffer;
    if (buffer && buffer->iface.init_tensor) {
        buffer->iface.init_tensor(buffer, tensor);
    } else {
        if (!buffer) {
            GGML_LOG_ERROR("Tensor with null buffer passed to init_tensor function\n");
        }
    }

    if (tensor->extra != nullptr) {
        // This pointer can either be passed around client/server, or probably better stored server-side and kept track of.
        // Currently unimplemented.
        GGML_LOG_ERROR("tensor->extra populated by the backend, this is currently unsupported.\n");
        return false;
    }

    return true;
}

bool rpc_server::get_tensor(const rpc_msg_get_tensor_req & request, std::vector<uint8_t> & response) {
    struct ggml_init_params params {
        /*.mem_size   =*/ ggml_tensor_overhead(),
        /*.mem_buffer =*/ NULL,
        /*.no_alloc   =*/ true,
    };
    ggml_context_ptr ctx_ptr { ggml_init(params) };
    GGML_ASSERT(ctx_ptr != nullptr);
    ggml_context * ctx = ctx_ptr.get();
    ggml_tensor * tensor = deserialize_tensor(ctx, &request.tensor);
    if (tensor == nullptr || tensor->buffer == nullptr) {
        GGML_LOG_ERROR("[%s] error deserializing tensor\n", __func__);
        return false;
    }
    LOG_DBG("[%s] buffer: %p, data: %p, offset: %" PRIu64 ", size: %" PRIu64 "\n", __func__, (void*)tensor->buffer, tensor->data, request.offset, request.size);

    // sanitize tensor->data
    {
        const size_t p0 = (size_t) ggml_backend_buffer_get_base(tensor->buffer);
        const size_t p1 = p0 + ggml_backend_buffer_get_size(tensor->buffer);

        if (request.tensor.data + request.offset < p0 ||
            request.tensor.data + request.offset >= p1 ||
            request.size > (p1 - request.tensor.data - request.offset)) {
                GGML_LOG_ERROR("[%s] requested tensor region (data=0x%" PRIx64 ", offset=%" PRIu64 ", size=%" PRIu64 ") out of buffer bounds [0x%zx, 0x%zx)\n",
                               __func__, request.tensor.data, request.offset, request.size, p0, p1);
                return false;
        }
    }

    response.resize(request.size, 0);
    ggml_backend_tensor_get(tensor, response.data(), request.offset, request.size);
    return true;
}

bool rpc_server::copy_tensor(const rpc_msg_copy_tensor_req & request, rpc_msg_copy_tensor_rsp & response) {
    struct ggml_init_params params {
        /*.mem_size   =*/ 2*ggml_tensor_overhead(),
        /*.mem_buffer =*/ NULL,
        /*.no_alloc   =*/ true,
    };
    ggml_context_ptr ctx_ptr { ggml_init(params) };
    GGML_ASSERT(ctx_ptr != nullptr);
    ggml_context * ctx = ctx_ptr.get();

    ggml_tensor * src = deserialize_tensor(ctx, &request.src);
    ggml_tensor * dst = deserialize_tensor(ctx, &request.dst);
    if (src == nullptr || dst == nullptr || src->buffer == nullptr || dst->buffer == nullptr) {
        GGML_LOG_ERROR("[%s] error deserializing tensors\n", __func__);
        return false;
    }

    uint64_t src_size   = (uint64_t) ggml_nbytes(src);
    uint64_t dst_data   = (uint64_t) dst->data;
    uint64_t dst_base   = (uint64_t) ggml_backend_buffer_get_base(dst->buffer);
    uint64_t dst_buf_sz = (uint64_t) ggml_backend_buffer_get_size(dst->buffer);

    if (dst_data + src_size > dst_base + dst_buf_sz) {
        GGML_LOG_ERROR("[%s] out-of-bounds write in rpc_server::copy_tensor:\n"
                         "    write range : [0x%" PRIx64 ", 0x%" PRIx64 "]\n"
                         "    buffer base: [0x%" PRIx64 ", 0x%" PRIx64 "]\n",
                         __func__,
                         dst_data,
                         dst_data + src_size,
                         dst_base,
                         dst_base + dst_buf_sz);
        return false;
    }

    LOG_DBG("[%s] src->buffer: %p, dst->buffer: %p\n",
            __func__, (void*) src->buffer, (void*) dst->buffer);

    response.result = ggml_backend_buffer_copy_tensor(src, dst);
    return true;
}

ggml_tensor * rpc_server::create_node(uint64_t id,
                                      struct ggml_context * ctx,
                                      const std::unordered_map<uint64_t, const rpc_tensor*> & tensor_ptrs,
                                      std::unordered_map<uint64_t, struct ggml_tensor*> & tensor_map) {
    if (tensor_map.find(id) != tensor_map.end()) {
        return tensor_map[id];
    }
    // Safely find the tensor pointer
    auto it_ptr = tensor_ptrs.find(id);
    if (it_ptr == tensor_ptrs.end()) {
        return nullptr;
    }
    const rpc_tensor * tensor = it_ptr->second;

    struct ggml_tensor * result = deserialize_tensor(ctx, tensor);
    if (result == nullptr) {
        return nullptr;
    }
    if (result->buffer == nullptr && result->data != nullptr) {
        GGML_LOG_ERROR("[%s] invalid data ptr", __func__);
        return nullptr;
    }
    tensor_map[id] = result;
    for (int i = 0; i < GGML_MAX_SRC; i++) {
        // Check if the source ID is 0 before calling create_node recursively
        if (tensor->src[i] == 0) {
            result->src[i] = nullptr;
        } else {
            result->src[i] = create_node(tensor->src[i], ctx, tensor_ptrs, tensor_map);
            // If the recursive call failed for a non-zero ID, propagate the error
            if (result->src[i] == nullptr) {
                GGML_LOG_ERROR("[%s] failed to create source node %d (src_id=%" PRIu64 ") for node id %" PRIu64 "\n",
                               __func__, i, tensor->src[i], id);
                // Must return nullptr to signal failure up the call stack
                return nullptr;
            }
        }
    }

    // Handle view_src similarly
    if (tensor->view_src == 0) {
        result->view_src = nullptr;
    } else {
        result->view_src = create_node(tensor->view_src, ctx, tensor_ptrs, tensor_map);
        // If the recursive call failed for a non-zero ID, propagate the error
        if (result->view_src == nullptr) {
            GGML_LOG_ERROR("[%s] failed to create view_src node (view_src_id=%" PRIu64 ") for node id %" PRIu64 "\n",
                           __func__, tensor->view_src, id);
            // Must return nullptr to signal failure up the call stack
            return nullptr;
        }
        if (result->buffer != result->view_src->buffer) {
            GGML_LOG_ERROR("[%s] view buffer does not match its source buffer for node id %" PRIu64 "\n",
                           __func__, id);
            return nullptr;
        }

        // A graph-compute view has no buffer until the backend allocator
        // assigns its source. Preserve that unresolved state so allocation can
        // initialize the view later. Weight views already have an owning
        // server buffer and must be rebuilt from that allocation because the
        // client pointer is only an opaque remote address and may not account
        // for a backend-specific device layout (for example expanded ROCmFPX
        // Q6).
        result->buffer = nullptr;
        result->data = nullptr;
        result->view_offs = tensor->view_offs;
        if (result->view_src->buffer != nullptr) {
            const uintptr_t base = (uintptr_t) ggml_backend_buffer_get_base(result->view_src->buffer);
            const uintptr_t data = (uintptr_t) result->view_src->data;
            const size_t buffer_size = ggml_backend_buffer_get_size(result->view_src->buffer);
            const size_t view_size = ggml_nbytes(result);
            if (data < base || data - base > buffer_size ||
                    result->view_offs > buffer_size - (data - base) ||
                    view_size > buffer_size - (data - base) - result->view_offs) {
                GGML_LOG_ERROR("[%s] server-side view is out of bounds for node id %" PRIu64 "\n",
                               __func__, id);
                return nullptr;
            }
            if (ggml_backend_view_init(result) != GGML_STATUS_SUCCESS) {
                GGML_LOG_ERROR("[%s] failed to initialize server-side view for node id %" PRIu64 "\n",
                               __func__, id);
                return nullptr;
            }
        }
    }
    result->view_offs = tensor->view_offs;
    return result;
}

bool rpc_server::graph_compute(const std::vector<uint8_t> & input) {
    // serialization format:
    // | device (4 bytes) | n_nodes (4 bytes) | nodes (n_nodes * sizeof(uint64_t) | n_tensors (4 bytes) | tensors (n_tensors * sizeof(rpc_tensor)) |
    if (input.size() < 2*sizeof(uint32_t)) {
        return false;
    }
    const uint8_t * src = input.data();
    uint32_t device;
    memcpy(&device, src, sizeof(device));
    src += sizeof(device);
    if (device >= backends.size()) {
        return false;
    }
    uint32_t n_nodes;
    memcpy(&n_nodes, src, sizeof(n_nodes));
    src += sizeof(n_nodes);
    if (input.size() < 2*sizeof(uint32_t) + n_nodes*sizeof(uint64_t) + sizeof(uint32_t)) {
        return false;
    }
    const uint64_t * nodes = (const uint64_t *)src;
    src += n_nodes*sizeof(uint64_t);
    uint32_t n_tensors;
    memcpy(&n_tensors, src, sizeof(n_tensors));
    src += sizeof(n_tensors);
    if (input.size() < 2*sizeof(uint32_t) + n_nodes*sizeof(uint64_t) + sizeof(uint32_t) + n_tensors*sizeof(rpc_tensor)) {
        return false;
    }
    const rpc_tensor * tensors = (const rpc_tensor *)src;
    LOG_DBG("[%s] device: %u, n_nodes: %u, n_tensors: %u\n", __func__, device, n_nodes, n_tensors);

    size_t buf_size = ggml_tensor_overhead()*(n_nodes + n_tensors) + ggml_graph_overhead_custom(n_nodes, false);
    if (stored_graphs[device].buffer.size() < buf_size) {
        stored_graphs[device].buffer.resize(buf_size);
    }
    struct ggml_init_params params = {
        /*.mem_size   =*/ buf_size,
        /*.mem_buffer =*/ stored_graphs[device].buffer.data(),
        /*.no_alloc   =*/ true,
    };
    ggml_context_ptr ctx_ptr { ggml_init(params) };
    GGML_ASSERT(ctx_ptr != nullptr);
    ggml_context * ctx = ctx_ptr.get();
    struct ggml_cgraph * graph = ggml_new_graph_custom(ctx, n_nodes, false);
    graph->n_nodes = n_nodes;
    std::unordered_map<uint64_t, const rpc_tensor*> tensor_ptrs;
    tensor_ptrs.reserve(n_tensors);
    for (uint32_t i = 0; i < n_tensors; i++) {
        tensor_ptrs.emplace(tensors[i].id, &tensors[i]);
    }
    std::unordered_map<uint64_t, ggml_tensor*> tensor_map;
    tensor_map.reserve(n_nodes);
    for (uint32_t i = 0; i < n_nodes; i++) {
        int64_t id;
        memcpy(&id, &nodes[i], sizeof(id));
        graph->nodes[i] = create_node(id, ctx, tensor_ptrs, tensor_map);

        // Check if create_node failed for a *non-zero* ID.
        // If id was 0, create_node returning nullptr is expected.
        // If id was non-zero and create_node returned nullptr, it indicates a deserialization error.
        if (graph->nodes[i] == nullptr && id != 0) {
            GGML_LOG_ERROR("[%s] failed to create graph node %d (id=%" PRId64 ")\n", __func__, i, id);
            return false;
        }
    }
    ggml_status status = ggml_backend_graph_compute(backends[device], graph);
    GGML_ASSERT(status == GGML_STATUS_SUCCESS && "Unsuccessful graph computations are not supported with RPC");
    stored_graphs[device].graph = graph;
    return true;
}

bool rpc_server::graph_recompute(const rpc_msg_graph_recompute_req & request) {
    uint32_t device = request.device;
    if (device >= backends.size()) {
        return false;
    }
    if (stored_graphs[device].graph == nullptr) {
        return false;
    }
    ggml_cgraph * graph = stored_graphs[device].graph;
    LOG_DBG("[%s] device: %u\n", __func__, device);
    ggml_status status = ggml_backend_graph_compute(backends[device], graph);
    GGML_ASSERT(status == GGML_STATUS_SUCCESS && "Unsuccessful graph computations are not supported with RPC");
    return true;
}

bool rpc_server::get_device_memory(const rpc_msg_get_device_memory_req & request, rpc_msg_get_device_memory_rsp & response) {
    uint32_t dev_id = request.device;
    if (dev_id >= backends.size()) {
        return false;
    }
    size_t free, total;
    ggml_backend_dev_t dev = ggml_backend_get_device(backends[dev_id]);
    ggml_backend_dev_memory(dev, &free, &total);
    response.free_mem = free;
    response.total_mem = total;
    LOG_DBG("[%s] device: %u, free_mem: %" PRIu64 ", total_mem: %" PRIu64 "\n", __func__, dev_id, response.free_mem, response.total_mem);
    return true;
}

#ifdef GGML_RPC_HALOFPX_LOCAL_STATE
namespace {

bool hfx_server_identity_valid(
        const hfx_state_server_config_owned & config,
        const hfx_state_identity_wire & identity) {
    return identity.key_generation == config.key_generation &&
        identity.logical_rank == config.logical_rank &&
        identity.world_size == config.world_size &&
        identity.world_size == 2 && identity.logical_rank == 1 &&
        hfx_equal(identity.channel_binding, config.channel_binding.data(), 32) &&
        !hfx_zero(identity.attempt_nonce, 32) &&
        !hfx_zero(identity.model_digest, 32) &&
        !hfx_zero(identity.compatibility_root, 32) &&
        !hfx_zero(identity.plan_digest, 32) &&
        !hfx_zero(identity.topology_digest, 32) &&
        !hfx_zero(identity.placement_digest, 32) &&
        !hfx_zero(identity.checkpoint_digest, 32) &&
        !hfx_zero(identity.token_prefix_digest, 32) &&
        !hfx_zero(identity.component_manifest_digest, 32) &&
        identity.generation != 0 && identity.token_count == identity.token_boundary;
}

int hfx_open_objects(const hfx_state_server_config_owned & config) {
    if (config.root.empty() || config.root.front() != '/') return -1;
    struct stat st {};
    if (lstat(config.root.c_str(), &st) != 0 || !S_ISDIR(st.st_mode) ||
        S_ISLNK(st.st_mode) || st.st_uid != geteuid() || (st.st_mode & 0022) != 0) return -1;
    const int root = open(config.root.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    if (root < 0) return -1;
    if (mkdirat(root, "objects", 0700) != 0 && errno != EEXIST) {
        close(root);
        return -1;
    }
    const int objects = openat(root, "objects", O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    close(root);
    if (objects < 0) return -1;
    if (fstat(objects, &st) != 0 || !S_ISDIR(st.st_mode) || st.st_uid != geteuid() ||
        (st.st_mode & 0022) != 0) {
        close(objects);
        return -1;
    }
    return objects;
}

ggml_tensor * hfx_component_tensor(
        const std::unordered_set<ggml_backend_buffer_t> & buffers,
        ggml_context * ctx,
        const hfx_state_component_wire & component) {
    auto buffer = reinterpret_cast<ggml_backend_buffer_t>(component.buffer);
    if (buffer == nullptr || buffers.find(buffer) == buffers.end() ||
        component.type >= GGML_TYPE_COUNT || ggml_blck_size((ggml_type) component.type) == 0) return nullptr;
    ggml_tensor * tensor = ggml_new_tensor_4d(ctx, (ggml_type) component.type,
        component.ne[0], component.ne[1], component.ne[2], component.ne[3]);
    if (!tensor) return nullptr;
    for (size_t i = 0; i < GGML_MAX_DIMS; ++i) tensor->nb[i] = component.nb[i];
    tensor->buffer = buffer;
    tensor->data = reinterpret_cast<void *>(component.data);
    uint64_t logical_end = 0;
    if (!hfx_add(component.offset, component.size, logical_end) ||
        logical_end > ggml_nbytes(tensor)) return nullptr;
    const uint64_t base = reinterpret_cast<uint64_t>(ggml_backend_buffer_get_base(buffer));
    const uint64_t buffer_size = ggml_backend_buffer_get_size(buffer);
    uint64_t begin = 0;
    uint64_t end = 0;
    uint64_t buffer_end = 0;
    if (!hfx_add(component.data, component.offset, begin) ||
        !hfx_add(begin, component.size, end) ||
        !hfx_add(base, buffer_size, buffer_end) || begin < base || end > buffer_end) return nullptr;
    return tensor;
}

static bool hfx_live_component_digests(
        const std::unordered_set<ggml_backend_buffer_t> & buffers,
        const hfx_state_component_wire * components,
        uint32_t component_count,
        std::vector<hfx_state_object_component> & output) {
    output.assign(component_count, {});
    std::array<uint8_t, 1 << 20> chunk {};
    for (uint32_t i = 0; i < component_count; ++i) {
        ggml_init_params params { ggml_tensor_overhead(), nullptr, true };
        ggml_context_ptr ctx { ggml_init(params) };
        ggml_tensor * tensor = ctx ? hfx_component_tensor(buffers, ctx.get(), components[i]) : nullptr;
        if (!tensor) return false;
        output[i].descriptor = components[i];
        output[i].descriptor.buffer = output[i].descriptor.data = 0;
        sha256_t sha;
        sha256_init(&sha);
        uint64_t done = 0;
        while (done < components[i].size) {
            const size_t n = static_cast<size_t>(
                std::min<uint64_t>(chunk.size(), components[i].size - done));
            ggml_backend_tensor_get(tensor, chunk.data(), components[i].offset + done, n);
            sha256_update(&sha, chunk.data(), n);
            done += n;
        }
        sha256_final(&sha, output[i].content_digest);
        hfx_wipe(&sha, sizeof(sha));
    }
    return true;
}

bool hfx_component_semantic_equal(
        const hfx_state_component_wire & stored,
        const hfx_state_component_wire & requested) {
    hfx_state_component_wire a = stored;
    hfx_state_component_wire b = requested;
    a.buffer = a.data = 0;
    b.buffer = b.data = 0;
    return hfx_equal(reinterpret_cast<const uint8_t *>(&a),
                     reinterpret_cast<const uint8_t *>(&b), sizeof(a));
}

bool hfx_hash_fd(int fd, hfx_digest & digest) {
    if (lseek(fd, 0, SEEK_SET) < 0) return false;
    sha256_t ctx;
    sha256_init(&ctx);
    std::array<uint8_t, 1 << 20> chunk {};
    for (;;) {
        const ssize_t n = read(fd, chunk.data(), chunk.size());
        if (n < 0) return false;
        if (n == 0) break;
        sha256_update(&ctx, chunk.data(), static_cast<size_t>(n));
    }
    sha256_final(&ctx, digest.data());
    hfx_wipe(&ctx, sizeof(ctx));
    return true;
}

bool hfx_load_object_metadata(
        int fd,
        const hfx_state_request_header & request,
        const hfx_state_component_wire * requested,
        hfx_state_object_header & object,
        std::vector<hfx_state_object_component> & components,
        uint64_t & payload_offset) {
    struct stat st {};
    if (fstat(fd, &st) != 0 || !S_ISREG(st.st_mode) || st.st_uid != geteuid() ||
        (st.st_mode & 0022) != 0 || st.st_size < 0 ||
        uint64_t(st.st_size) > HFX_STATE_MAX_OBJECT_BYTES + GGML_RPC_HALOFPX_STATE_MAX_REQUEST) return false;
    if (lseek(fd, 0, SEEK_SET) < 0 || !hfx_read_all(fd, &object, sizeof(object))) return false;
    if (!hfx_magic(object.magic, "HFXOBJ1\0") || object.major != HFX_STATE_MAJOR ||
        object.minor != HFX_STATE_MINOR || object.reserved != 0 ||
        object.component_count != request.component_count ||
        object.header_size != sizeof(object) + object.component_count * sizeof(hfx_state_object_component) ||
        !hfx_identity_stable_equal(object.identity, request.identity)) return false;
    components.resize(object.component_count);
    if (!components.empty() && !hfx_read_all(fd, components.data(), components.size() * sizeof(components[0]))) return false;
    uint64_t total = 0;
    for (uint32_t i = 0; i < object.component_count; ++i) {
        if (!hfx_component_semantic_equal(components[i].descriptor, requested[i]) ||
            !hfx_add(total, components[i].descriptor.size, total)) return false;
    }
    payload_offset = object.header_size;
    uint64_t expected_size = 0;
    if (total != object.payload_bytes || !hfx_add(payload_offset, total, expected_size) ||
        expected_size != uint64_t(st.st_size)) return false;
    return true;
}

} // namespace

bool rpc_server::hfx_state_capture(const std::vector<uint8_t> & input, hfx_state_response_wire & response) {
    const hfx_state_request_header * request = nullptr;
    const hfx_state_component_wire * components = nullptr;
    if (!hfx_state_config) return false;
    if (!hfx_request_shape(input, RPC_CMD_HALOFPX_STATE_CAPTURE, request, components)) {
        GGML_LOG_WARN("[halofpx-state] capture rejected: shape\n"); return false;
    }
    response = hfx_response(RPC_CMD_HALOFPX_STATE_CAPTURE, GGML_RPC_HALOFPX_STATE_REJECTED,
                            request->identity, hfx_state_config->control_key.data());
    hfx_bind_response(response, input, hfx_state_config->control_key.data());
    if (!hfx_verify_request_auth(input, hfx_state_config->control_key.data())) {
        GGML_LOG_WARN("[halofpx-state] capture rejected: auth\n"); return true;
    }
    if (!hfx_server_identity_valid(*hfx_state_config, request->identity)) {
        GGML_LOG_WARN("[halofpx-state] capture rejected: identity\n"); return true;
    }
    if (request->component_count == 0 ||
        !hfx_zero(request->expected_object_digest, 32) || !hfx_zero(request->worker_nonce, 32)) {
        GGML_LOG_WARN("[halofpx-state] capture rejected: unexpected fields\n"); return true;
    }
    response = hfx_response(RPC_CMD_HALOFPX_STATE_CAPTURE, GGML_RPC_HALOFPX_STATE_STORAGE_ERROR,
                            request->identity, hfx_state_config->control_key.data());
    hfx_bind_response(response, input, hfx_state_config->control_key.data());
    const int objects = hfx_open_objects(*hfx_state_config);
    if (objects < 0) return true;
    const auto key = hfx_object_key(hfx_state_config->control_key.data(), request->identity);
    const std::string final_name = hfx_hex(key.data(), key.size()) + ".hfx";
    const std::string temp_name = ".capture-" + hfx_hex(request->identity.attempt_nonce, 32);
    const int fd = openat(objects, temp_name.c_str(), O_RDWR | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW, 0600);
    if (fd < 0) { close(objects); return true; }
    bool ok = true;
    hfx_state_object_header object {};
    hfx_set_magic(object.magic, "HFXOBJ1\0");
    object.major = HFX_STATE_MAJOR;
    object.minor = HFX_STATE_MINOR;
    object.header_size = sizeof(object) + request->component_count * sizeof(hfx_state_object_component);
    object.component_count = request->component_count;
    object.identity = request->identity;
    memset(object.identity.attempt_nonce, 0, sizeof(object.identity.attempt_nonce));
    std::vector<hfx_state_object_component> stored(request->component_count);
    for (uint32_t i = 0; i < request->component_count; ++i) {
        stored[i].descriptor = components[i];
        stored[i].descriptor.buffer = stored[i].descriptor.data = 0;
        ok = ok && hfx_add(object.payload_bytes, components[i].size, object.payload_bytes);
    }
    ok = ok && hfx_write_all(fd, &object, sizeof(object));
    ok = ok && (stored.empty() || hfx_write_all(fd, stored.data(), stored.size() * sizeof(stored[0])));
    ggml_init_params params { ggml_tensor_overhead(), nullptr, true };
    std::array<uint8_t, 1 << 20> chunk {};
    uint64_t verified = 0;
    for (uint32_t i = 0; ok && i < request->component_count; ++i) {
        ggml_context_ptr ctx { ggml_init(params) };
        ggml_tensor * tensor = ctx ? hfx_component_tensor(buffers, ctx.get(), components[i]) : nullptr;
        if (!tensor) { ok = false; break; }
        sha256_t sha;
        sha256_init(&sha);
        uint64_t done = 0;
        while (done < components[i].size) {
            const size_t n = static_cast<size_t>(std::min<uint64_t>(chunk.size(), components[i].size - done));
            ggml_backend_tensor_get(tensor, chunk.data(), components[i].offset + done, n);
            sha256_update(&sha, chunk.data(), n);
            if (!hfx_write_all(fd, chunk.data(), n)) { ok = false; break; }
            done += n;
        }
        sha256_final(&sha, stored[i].content_digest);
        hfx_wipe(&sha, sizeof(sha));
        verified += done;
    }
    if (ok) {
        ok = pwrite(fd, stored.data(), stored.size() * sizeof(stored[0]), sizeof(object)) ==
             static_cast<ssize_t>(stored.size() * sizeof(stored[0]));
    }
    if (ok) hfx_state_log_component_digest("capture", stored);
    hfx_digest object_digest {};
    ok = ok && fsync(fd) == 0 && hfx_hash_fd(fd, object_digest);
    if (ok) {
        if (linkat(objects, temp_name.c_str(), objects, final_name.c_str(), 0) != 0) ok = false;
        if (ok) ok = fsync(objects) == 0;
    }
    close(fd);
    unlinkat(objects, temp_name.c_str(), 0);
    close(objects);
    if (!ok) return true;
    response = hfx_response(RPC_CMD_HALOFPX_STATE_CAPTURE, GGML_RPC_HALOFPX_STATE_STORED,
                            request->identity, hfx_state_config->control_key.data());
    response.verified_components = request->component_count;
    response.verified_bytes = verified;
    memcpy(response.object_digest, object_digest.data(), object_digest.size());
    hfx_bind_response(response, input, hfx_state_config->control_key.data());
    GGML_LOG_INFO("[halofpx-state] stored rank=%u generation=%" PRIu64 " components=%u bytes=%" PRIu64 "\n",
                  response.logical_rank, response.generation, response.verified_components, response.verified_bytes);
    return true;
}

bool rpc_server::hfx_state_stage(const std::vector<uint8_t> & input, hfx_state_response_wire & response) {
    const hfx_state_request_header * request = nullptr;
    const hfx_state_component_wire * components = nullptr;
    if (!hfx_state_config) return false;
    if (!hfx_request_shape(input, RPC_CMD_HALOFPX_STATE_STAGE, request, components)) {
        GGML_LOG_WARN("[halofpx-state] stage rejected: shape\n"); return false;
    }
    response = hfx_response(RPC_CMD_HALOFPX_STATE_STAGE, GGML_RPC_HALOFPX_STATE_REJECTED,
                            request->identity, hfx_state_config->control_key.data());
    hfx_bind_response(response, input, hfx_state_config->control_key.data());
    if (!hfx_verify_request_auth(input, hfx_state_config->control_key.data())) {
        GGML_LOG_WARN("[halofpx-state] stage rejected: auth\n"); return true;
    }
    if (!hfx_server_identity_valid(*hfx_state_config, request->identity)) {
        GGML_LOG_WARN("[halofpx-state] stage rejected: identity\n"); return true;
    }
    if (request->component_count == 0 ||
        hfx_zero(request->expected_object_digest, 32) || !hfx_zero(request->worker_nonce, 32)) {
        GGML_LOG_WARN("[halofpx-state] stage rejected: unexpected fields\n"); return true;
    }
    if (!hfx_accept_attempt_nonce(request->identity.attempt_nonce)) {
        hfx_state_pending.reset();
        GGML_LOG_WARN("[halofpx-state] stage rejected: replay or nonce ledger full\n");
        return true;
    }
    response = hfx_response(RPC_CMD_HALOFPX_STATE_STAGE, GGML_RPC_HALOFPX_STATE_MISS,
                            request->identity, hfx_state_config->control_key.data());
    hfx_bind_response(response, input, hfx_state_config->control_key.data());
    hfx_state_pending.reset();
    const int objects = hfx_open_objects(*hfx_state_config);
    if (objects < 0) return true;
    const auto key = hfx_object_key(hfx_state_config->control_key.data(), request->identity);
    const std::string name = hfx_hex(key.data(), key.size()) + ".hfx";
    const int fd = openat(objects, name.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    close(objects);
    if (fd < 0) return true;
    hfx_digest actual {};
    bool ok = hfx_hash_fd(fd, actual) && hfx_equal(actual.data(), request->expected_object_digest, 32);
    hfx_state_object_header object {};
    std::vector<hfx_state_object_component> stored;
    uint64_t payload_offset = 0;
    ok = ok && hfx_load_object_metadata(fd, *request, components, object, stored, payload_offset);
    std::array<uint8_t, 1 << 20> chunk {};
    uint64_t file_offset = payload_offset;
    for (uint32_t i = 0; ok && i < request->component_count; ++i) {
        if (lseek(fd, file_offset, SEEK_SET) < 0) { ok = false; break; }
        sha256_t sha;
        sha256_init(&sha);
        uint64_t done = 0;
        while (done < components[i].size) {
            const size_t n = static_cast<size_t>(std::min<uint64_t>(chunk.size(), components[i].size - done));
            if (!hfx_read_all(fd, chunk.data(), n)) { ok = false; break; }
            sha256_update(&sha, chunk.data(), n);
            done += n;
        }
        hfx_digest digest {};
        sha256_final(&sha, digest.data());
        hfx_wipe(&sha, sizeof(sha));
        if (!hfx_equal(digest.data(), stored[i].content_digest, 32)) ok = false;
        file_offset += components[i].size;
    }
    if (ok) hfx_state_log_component_digest("stage", stored);
    // A second pass loads only after the complete object and every component
    // digest have validated. The addressed tensors are disposable staging.
    uint64_t load_offset = payload_offset;
    for (uint32_t i = 0; ok && i < request->component_count; ++i) {
        ggml_init_params params { ggml_tensor_overhead(), nullptr, true };
        ggml_context_ptr ctx { ggml_init(params) };
        ggml_tensor * tensor = ctx ? hfx_component_tensor(buffers, ctx.get(), components[i]) : nullptr;
        if (!tensor || lseek(fd, load_offset, SEEK_SET) < 0) { ok = false; break; }
        uint64_t done = 0;
        while (done < components[i].size) {
            const size_t n = static_cast<size_t>(std::min<uint64_t>(chunk.size(), components[i].size - done));
            if (!hfx_read_all(fd, chunk.data(), n)) { ok = false; break; }
            ggml_backend_tensor_set(tensor, chunk.data(), components[i].offset + done, n);
            done += n;
        }
        load_offset += components[i].size;
    }
    close(fd);
    if (!ok) return true;
    hfx_state_pending_attempt pending;
    pending.identity = request->identity;
    pending.staged.assign(components, components + request->component_count);
    memcpy(pending.object_digest.data(), actual.data(), 32);
    if (!hfx_random_all(pending.worker_nonce.data(), pending.worker_nonce.size())) return true;
    pending.expires = std::chrono::steady_clock::now() + std::chrono::milliseconds(HFX_STATE_TIMEOUT_MS);
    hfx_state_pending = pending;
    response = hfx_response(RPC_CMD_HALOFPX_STATE_STAGE, GGML_RPC_HALOFPX_STATE_READY,
                            request->identity, hfx_state_config->control_key.data());
    response.verified_components = request->component_count;
    response.verified_bytes = object.payload_bytes;
    memcpy(response.object_digest, actual.data(), 32);
    memcpy(response.worker_nonce, pending.worker_nonce.data(), 32);
    hfx_bind_response(response, input, hfx_state_config->control_key.data());
    GGML_LOG_INFO("[halofpx-state] ready rank=%u generation=%" PRIu64 " components=%u bytes=%" PRIu64 "\n",
                  response.logical_rank, response.generation, response.verified_components, response.verified_bytes);
    return true;
}

bool rpc_server::hfx_state_commit_apply(const std::vector<uint8_t> & input, hfx_state_response_wire & response) {
    const hfx_state_request_header * request = nullptr;
    const hfx_state_component_wire * live = nullptr;
    if (!hfx_state_config) return false;
    if (!hfx_request_shape(input, RPC_CMD_HALOFPX_STATE_COMMIT_APPLY, request, live)) {
        GGML_LOG_WARN("[halofpx-state] commit rejected: shape\n"); return false;
    }
    response = hfx_response(RPC_CMD_HALOFPX_STATE_COMMIT_APPLY, GGML_RPC_HALOFPX_STATE_REJECTED,
                            request->identity, hfx_state_config->control_key.data());
    hfx_bind_response(response, input, hfx_state_config->control_key.data());
    if (!hfx_verify_request_auth(input, hfx_state_config->control_key.data())) {
        GGML_LOG_WARN("[halofpx-state] commit rejected: auth\n"); return true;
    }
    if (!hfx_server_identity_valid(*hfx_state_config, request->identity)) {
        GGML_LOG_WARN("[halofpx-state] commit rejected: identity\n"); return true;
    }
    if (request->component_count == 0) return true;
    if (!hfx_state_pending || std::chrono::steady_clock::now() > hfx_state_pending->expires ||
        !hfx_equal(request->worker_nonce, hfx_state_pending->worker_nonce.data(), 32) ||
        !hfx_equal(request->expected_object_digest, hfx_state_pending->object_digest.data(), 32) ||
        !hfx_equal(reinterpret_cast<const uint8_t *>(&request->identity),
                   reinterpret_cast<const uint8_t *>(&hfx_state_pending->identity), sizeof(request->identity)) ||
        request->component_count != hfx_state_pending->staged.size()) {
        hfx_state_pending.reset();
        return true;
    }
    bool ok = true;
    uint64_t applied = 0;
    for (uint32_t i = 0; ok && i < request->component_count; ++i) {
        if (!hfx_component_semantic_equal(hfx_state_pending->staged[i], live[i])) { ok = false; break; }
        ggml_init_params params { 4 * ggml_tensor_overhead(), nullptr, true };
        ggml_context_ptr ctx { ggml_init(params) };
        ggml_tensor * src = ctx ? hfx_component_tensor(buffers, ctx.get(), hfx_state_pending->staged[i]) : nullptr;
        ggml_tensor * dst = ctx ? hfx_component_tensor(buffers, ctx.get(), live[i]) : nullptr;
        if (!src || !dst) { ok = false; break; }
        const size_t element_size = ggml_element_size(src);
        if (element_size == 0 || hfx_state_pending->staged[i].size % element_size != 0) { ok = false; break; }
        const int64_t elements = hfx_state_pending->staged[i].size / element_size;
        if (elements <= 0) { ok = false; break; }
        ggml_tensor * src_view = ggml_view_1d(ctx.get(), src, elements, hfx_state_pending->staged[i].offset);
        ggml_tensor * dst_view = ggml_view_1d(ctx.get(), dst, elements, live[i].offset);
        ggml_backend_view_init(src_view);
        ggml_backend_view_init(dst_view);
        ok = ggml_backend_buffer_copy_tensor(src_view, dst_view);
        applied += ok ? live[i].size : 0;
    }
    if (ok && hfx_state_diagnostics_enabled()) {
        std::vector<hfx_state_object_component> applied_components;
        ok = hfx_live_component_digests(
            buffers, live, request->component_count, applied_components);
        if (ok) hfx_state_log_component_digest("apply", applied_components);
    }
    hfx_state_pending.reset();
    response = hfx_response(RPC_CMD_HALOFPX_STATE_COMMIT_APPLY,
                            ok ? GGML_RPC_HALOFPX_STATE_APPLIED : GGML_RPC_HALOFPX_STATE_APPLY_ERROR,
                            request->identity, hfx_state_config->control_key.data());
    response.verified_components = ok ? request->component_count : 0;
    response.verified_bytes = ok ? applied : 0;
    if (ok) {
        memcpy(response.object_digest, request->expected_object_digest, 32);
        memcpy(response.worker_nonce, request->worker_nonce, 32);
    }
    hfx_bind_response(response, input, hfx_state_config->control_key.data());
    GGML_LOG_INFO("[halofpx-state] apply rank=%u generation=%" PRIu64 " status=%u components=%u bytes=%" PRIu64 "\n",
                  response.logical_rank, response.generation, response.status,
                  response.verified_components, response.verified_bytes);
    return true;
}

bool rpc_server::hfx_state_abort(const std::vector<uint8_t> & input, hfx_state_response_wire & response) {
    const hfx_state_request_header * request = nullptr;
    const hfx_state_component_wire * components = nullptr;
    if (!hfx_state_config || !hfx_request_shape(input, RPC_CMD_HALOFPX_STATE_ABORT, request, components)) return false;
    response = hfx_response(RPC_CMD_HALOFPX_STATE_ABORT, GGML_RPC_HALOFPX_STATE_REJECTED,
                            request->identity, hfx_state_config->control_key.data());
    hfx_bind_response(response, input, hfx_state_config->control_key.data());
    if (request->component_count != 0 || !hfx_zero(request->expected_object_digest, 32) ||
        !hfx_verify_request_auth(input, hfx_state_config->control_key.data()) ||
        !hfx_server_identity_valid(*hfx_state_config, request->identity)) return true;
    if (hfx_state_pending &&
        (!hfx_equal(reinterpret_cast<const uint8_t *>(&request->identity),
                    reinterpret_cast<const uint8_t *>(&hfx_state_pending->identity), sizeof(request->identity)) ||
         !hfx_equal(request->worker_nonce, hfx_state_pending->worker_nonce.data(), 32))) {
        hfx_state_pending.reset();
        return true;
    }
    hfx_state_pending.reset();
    response = hfx_response(RPC_CMD_HALOFPX_STATE_ABORT, GGML_RPC_HALOFPX_STATE_ABORTED,
                            request->identity, hfx_state_config->control_key.data());
    hfx_bind_response(response, input, hfx_state_config->control_key.data());
    return true;
}

void rpc_server::hfx_state_discard_for_legacy_mutation() {
    hfx_state_pending.reset();
}
#endif

rpc_server::~rpc_server() {
    for (auto buffer : buffers) {
        ggml_backend_buffer_free(buffer);
    }
}

static void rpc_serve_client(const std::vector<ggml_backend_t> & backends, const char * cache_dir,
#ifdef GGML_RPC_HALOFPX_LOCAL_STATE
                             const hfx_state_server_config_owned * hfx_state_config,
#endif
                             socket_ptr sock) {
    rpc_server server(backends, cache_dir
#ifdef GGML_RPC_HALOFPX_LOCAL_STATE
        , hfx_state_config
#endif
    );
    uint8_t cmd;
    if (!sock->recv_data(&cmd, 1)) {
        return;
    }
    if (cmd != RPC_CMD_HELLO) {
        GGML_LOG_ERROR("Expected HELLO command, update client\n");
        return;
    }

    // Read input_size and validate protocol version
    uint64_t hello_input_size;
    if (!sock->recv_data(&hello_input_size, sizeof(hello_input_size))) {
        return;
    }

    if (hello_input_size != sizeof(rpc_msg_hello_req)) {
        GGML_LOG_ERROR("HELLO request size mismatch (%zu vs %zu) — client needs upgrade to protocol v%d.x\n",
                       (size_t)hello_input_size, sizeof(rpc_msg_hello_req), RPC_PROTO_MAJOR_VERSION);
        return;
    }

    rpc_msg_hello_req req = {};
    if (!sock->recv_data(&req, sizeof(req))) {
        return;
    }

    rpc_msg_hello_rsp rsp = {};
    server.hello(rsp);
    // Advertise server transport capabilities based on client's caps
    sock->get_caps(rsp.conn_caps);
    if (!send_msg(sock, &rsp, sizeof(rsp))) {
        return;
    }

    // Activate transport upgrade using client's caps
    sock->update_caps(req.conn_caps);
    while (true) {
        if (!sock->recv_data(&cmd, 1)) {
            break;
        }
        if (cmd >= RPC_CMD_COUNT) {
            // fail fast if the command is invalid
            GGML_LOG_ERROR("Unknown command: %d\n", cmd);
            break;
        }
#ifdef GGML_RPC_HALOFPX_LOCAL_STATE
        if (cmd == RPC_CMD_ALLOC_BUFFER || cmd == RPC_CMD_FREE_BUFFER || cmd == RPC_CMD_BUFFER_CLEAR ||
            cmd == RPC_CMD_SET_TENSOR || cmd == RPC_CMD_SET_TENSOR_HASH || cmd == RPC_CMD_COPY_TENSOR ||
            cmd == RPC_CMD_GRAPH_COMPUTE || cmd == RPC_CMD_INIT_TENSOR || cmd == RPC_CMD_GRAPH_RECOMPUTE) {
            server.hfx_state_discard_for_legacy_mutation();
        }
#endif
        switch (cmd) {
            case RPC_CMD_HELLO: {
                // HELLO command is handled above
                return;
            }
            case RPC_CMD_DEVICE_COUNT: {
                if (!recv_msg(sock, nullptr, 0)) {
                    return;
                }
                rpc_msg_device_count_rsp response;
                response.device_count = backends.size();
                if (!send_msg(sock, &response, sizeof(response))) {
                    return;
                }
                break;
            }
            case RPC_CMD_ALLOC_BUFFER: {
                rpc_msg_alloc_buffer_req request;
                if (!recv_msg(sock, &request, sizeof(request))) {
                    return;
                }
                rpc_msg_alloc_buffer_rsp response;
                if (!server.alloc_buffer(request, response)) {
                    return;
                }
                if (!send_msg(sock, &response, sizeof(response))) {
                    return;
                }
                break;
            }
            case RPC_CMD_GET_ALLOC_SIZE: {
                rpc_msg_get_alloc_size_req request;
                if (!recv_msg(sock, &request, sizeof(request))) {
                    return;
                }
                rpc_msg_get_alloc_size_rsp response;
                if (!server.get_alloc_size(request, response)) {
                    return;
                }
                if (!send_msg(sock, &response, sizeof(response))) {
                    return;
                }
                break;
            }
            case RPC_CMD_GET_ALIGNMENT: {
                rpc_msg_get_alignment_req request;
                if (!recv_msg(sock, &request, sizeof(request))) {
                    return;
                }
                rpc_msg_get_alignment_rsp response;
                if (!server.get_alignment(request, response)) {
                    return;
                }
                if (!send_msg(sock, &response, sizeof(response))) {
                    return;
                }
                break;
            }
            case RPC_CMD_GET_MAX_SIZE: {
                rpc_msg_get_max_size_req request;
                if (!recv_msg(sock, &request, sizeof(request))) {
                    return;
                }
                rpc_msg_get_max_size_rsp response;
                if (!server.get_max_size(request, response)) {
                    return;
                }
                if (!send_msg(sock, &response, sizeof(response))) {
                    return;
                }
                break;
            }
            case RPC_CMD_BUFFER_GET_BASE: {
                rpc_msg_buffer_get_base_req request;
                if (!recv_msg(sock, &request, sizeof(request))) {
                    return;
                }
                rpc_msg_buffer_get_base_rsp response;
                if (!server.buffer_get_base(request, response)) {
                    return;
                }
                if (!send_msg(sock, &response, sizeof(response))) {
                    return;
                }
                break;
            }
            case RPC_CMD_FREE_BUFFER: {
                rpc_msg_free_buffer_req request;
                if (!recv_msg(sock, &request, sizeof(request))) {
                    return;
                }
                if (!server.free_buffer(request)) {
                    return;
                }
                if (!send_msg(sock, nullptr, 0)) {
                    return;
                }
                break;
            }
            case RPC_CMD_BUFFER_CLEAR: {
                rpc_msg_buffer_clear_req request;
                if (!recv_msg(sock, &request, sizeof(request))) {
                    return;
                }
                if (!server.buffer_clear(request)) {
                    return;
                }
                if (!send_msg(sock, nullptr, 0)) {
                    return;
                }
                break;
            }
            case RPC_CMD_SET_TENSOR: {
                std::vector<uint8_t> input;
                if (!recv_msg(sock, input)) {
                    return;
                }
                if (!server.set_tensor(input)) {
                    return;
                }
                break;
            }
            case RPC_CMD_SET_TENSOR_HASH: {
                rpc_msg_set_tensor_hash_req request;
                if (!recv_msg(sock, &request, sizeof(request))) {
                    return;
                }
                rpc_msg_set_tensor_hash_rsp response;
                if (!server.set_tensor_hash(request, response)) {
                    return;
                }
                if (!send_msg(sock, &response, sizeof(response))) {
                    return;
                }
                break;
            }
            case RPC_CMD_INIT_TENSOR: {
                rpc_msg_init_tensor_req request;
                if (!recv_msg(sock, &request,sizeof(request))) {
                    return;
                }
                if (!server.init_tensor(request)) {
                    return;
                }
                if (!send_msg(sock, nullptr, 0)) {
                    return;
                }
                break;
            }
            case RPC_CMD_GET_TENSOR: {
                rpc_msg_get_tensor_req request;
                if (!recv_msg(sock, &request, sizeof(request))) {
                    return;
                }
                std::vector<uint8_t> response;
                if (!server.get_tensor(request, response)) {
                    return;
                }
                if (!send_msg(sock, response.data(), response.size())) {
                    return;
                }
                break;
            }
            case RPC_CMD_COPY_TENSOR: {
                rpc_msg_copy_tensor_req request;
                if (!recv_msg(sock, &request, sizeof(request))) {
                    return;
                }
                rpc_msg_copy_tensor_rsp response;
                if (!server.copy_tensor(request, response)) {
                    return;
                }
                if (!send_msg(sock, &response, sizeof(response))) {
                    return;
                }
                break;
            }
            case RPC_CMD_GRAPH_COMPUTE: {
                std::vector<uint8_t> input;
                if (!recv_msg(sock, input)) {
                    return;
                }
                if (!server.graph_compute(input)) {
                    return;
                }
                break;
            }
            case RPC_CMD_GRAPH_RECOMPUTE: {
                rpc_msg_graph_recompute_req request;
                if (!recv_msg(sock, &request, sizeof(request))) {
                    return;
                }
                if (!server.graph_recompute(request)) {
                    return;
                }
                break;
            }
            case RPC_CMD_GET_DEVICE_MEMORY: {
                rpc_msg_get_device_memory_req request;
                if (!recv_msg(sock, &request, sizeof(request))) {
                    return;
                }
                rpc_msg_get_device_memory_rsp response;
                if (!server.get_device_memory(request, response)) {
                    return;
                }
                if (!send_msg(sock, &response, sizeof(response))) {
                    return;
                }
                break;
            }
#ifdef GGML_RPC_HALOFPX_LOCAL_STATE
            case RPC_CMD_HALOFPX_STATE_CAPS: {
                if (!recv_msg(sock, nullptr, 0)) return;
                if (!hfx_state_config) return;
                hfx_state_caps_wire response {};
                hfx_set_magic(response.magic, "HFXCAP2\0");
                response.major = HFX_STATE_MAJOR;
                response.minor = HFX_STATE_MINOR;
                response.encoded_size = sizeof(response);
                response.command_mask = HFX_STATE_COMMAND_MASK;
                response.max_request = GGML_RPC_HALOFPX_STATE_MAX_REQUEST;
                response.max_response = sizeof(hfx_state_response_wire);
                response.max_components = HFX_STATE_MAX_COMPONENTS;
                response.logical_rank = hfx_state_config->logical_rank;
                response.world_size = hfx_state_config->world_size;
                response.max_component_bytes = HFX_STATE_MAX_COMPONENT_BYTES;
                response.max_object_bytes = HFX_STATE_MAX_OBJECT_BYTES;
                response.timeout_ms = HFX_STATE_TIMEOUT_MS;
                response.key_generation = hfx_state_config->key_generation;
                memcpy(response.channel_binding, hfx_state_config->channel_binding.data(), 32);
                if (!send_msg(sock, &response, sizeof(response))) return;
                break;
            }
            case RPC_CMD_HALOFPX_STATE_CAPTURE:
            case RPC_CMD_HALOFPX_STATE_STAGE:
            case RPC_CMD_HALOFPX_STATE_COMMIT_APPLY:
            case RPC_CMD_HALOFPX_STATE_ABORT: {
                std::vector<uint8_t> input;
                if (!recv_msg_bounded(sock, input, GGML_RPC_HALOFPX_STATE_MAX_REQUEST)) return;
                hfx_state_response_wire response {};
                bool handled = false;
                if (cmd == RPC_CMD_HALOFPX_STATE_CAPTURE) handled = server.hfx_state_capture(input, response);
                if (cmd == RPC_CMD_HALOFPX_STATE_STAGE) handled = server.hfx_state_stage(input, response);
                if (cmd == RPC_CMD_HALOFPX_STATE_COMMIT_APPLY) handled = server.hfx_state_commit_apply(input, response);
                if (cmd == RPC_CMD_HALOFPX_STATE_ABORT) handled = server.hfx_state_abort(input, response);
                if (!handled || !send_msg(sock, &response, sizeof(response))) return;
                break;
            }
#endif
            default: {
                GGML_LOG_ERROR("Unknown command: %d\n", cmd);
                return;
            }
        }
    }
}

static void ggml_backend_rpc_start_server_internal(const char * endpoint, const char * cache_dir,
                                   size_t n_threads, size_t n_devices, ggml_backend_dev_t * devices
#ifdef GGML_RPC_HALOFPX_LOCAL_STATE
                                   , const hfx_state_server_config_owned * hfx_state_config
#endif
                                   ) {
    if (n_devices == 0 || devices == nullptr) {
        fprintf(stderr, "Invalid arguments to ggml_backend_rpc_start_server\n");
        return;
    }
    std::vector<ggml_backend_t> backends;
    printf("Starting RPC server v%d.%d.%d\n",
        RPC_PROTO_MAJOR_VERSION,
        RPC_PROTO_MINOR_VERSION,
        RPC_PROTO_PATCH_VERSION);
    printf("  endpoint       : %s\n", endpoint);
    printf("  local cache    : %s\n", cache_dir ? cache_dir : "n/a");
#ifdef GGML_RPC_HALOFPX_LOCAL_STATE
    printf("  HaloFPX state  : %s\n", hfx_state_config ? "enabled (canary)" : "off");
#endif
    printf("Devices:\n");
    for (size_t i = 0; i < n_devices; i++) {
        auto dev = devices[i];
        size_t free, total;
        ggml_backend_dev_memory(dev, &free, &total);
        printf("  %s: %s (%zu MiB, %zu MiB free)\n", ggml_backend_dev_name(dev), ggml_backend_dev_description(dev),
               total / 1024 / 1024, free / 1024 / 1024);
        auto backend = ggml_backend_dev_init(dev, nullptr);
        if (!backend) {
            fprintf(stderr, "Failed to create backend for device %s\n", dev->iface.get_name(dev));
            return;
        }
        backends.push_back(backend);
        ggml_backend_reg_t reg = dev ? ggml_backend_dev_backend_reg(dev) : nullptr;
        if (reg) {
            auto ggml_backend_set_n_threads_fn = (ggml_backend_set_n_threads_t) ggml_backend_reg_get_proc_address(reg, "ggml_backend_set_n_threads");
            if (ggml_backend_set_n_threads_fn) {
                ggml_backend_set_n_threads_fn(backend, n_threads);
            }
        }
    }

    std::string host;
    int port;
    if (!parse_endpoint(endpoint, host, port)) {
        return;
    }

#ifdef GGML_RPC_RDMA
    printf("  transport      : TCP (RDMA auto-negotiate enabled)\n");
#else
    printf("  transport      : TCP\n");
#endif // GGML_RPC_RDMA
    if (!rpc_transport_init()) {
        fprintf(stderr, "Failed to initialize RPC transport\n");
        return;
    }
    auto server_socket = socket_t::create_server(host.c_str(), port);
    if (server_socket == nullptr) {
        fprintf(stderr, "Failed to create server socket\n");
        return;
    }
    while (true) {
        auto client_socket = server_socket->accept();
        if (client_socket == nullptr) {
            fprintf(stderr, "Failed to accept client connection\n");
            return;
        }
        printf("Accepted client connection\n");
        fflush(stdout);
        rpc_serve_client(backends, cache_dir,
#ifdef GGML_RPC_HALOFPX_LOCAL_STATE
                         hfx_state_config,
#endif
                         client_socket);
        printf("Client connection closed\n");
        fflush(stdout);
    }
    rpc_transport_shutdown();
    for (auto backend : backends) {
        ggml_backend_free(backend);
    }
}

void ggml_backend_rpc_start_server(const char * endpoint, const char * cache_dir,
                                   size_t n_threads, size_t n_devices, ggml_backend_dev_t * devices) {
    ggml_backend_rpc_start_server_internal(endpoint, cache_dir, n_threads, n_devices, devices
#ifdef GGML_RPC_HALOFPX_LOCAL_STATE
                                           , nullptr
#endif
    );
}

void ggml_backend_rpc_start_server_with_halofpx_state(
        const char * endpoint, const char * cache_dir,
        size_t n_threads, size_t n_devices, ggml_backend_dev_t * devices,
        const ggml_backend_rpc_halofpx_state_server_config * state_config) {
#ifdef GGML_RPC_HALOFPX_LOCAL_STATE
    if (!state_config || !state_config->root || state_config->root[0] != '/' ||
        state_config->logical_rank != 1 || state_config->world_size != 2 ||
        state_config->key_generation == 0 ||
        hfx_zero(state_config->control_key, 32) || hfx_zero(state_config->channel_binding, 32)) {
        fprintf(stderr, "Invalid HaloFPX worker-local state configuration\n");
        return;
    }
    hfx_state_server_config_owned owned;
    owned.root = state_config->root;
    owned.logical_rank = state_config->logical_rank;
    owned.world_size = state_config->world_size;
    owned.key_generation = state_config->key_generation;
    memcpy(owned.control_key.data(), state_config->control_key, 32);
    memcpy(owned.channel_binding.data(), state_config->channel_binding, 32);
    ggml_backend_rpc_start_server_internal(endpoint, cache_dir, n_threads, n_devices, devices, &owned);
#else
    GGML_UNUSED(endpoint); GGML_UNUSED(cache_dir); GGML_UNUSED(n_threads);
    GGML_UNUSED(n_devices); GGML_UNUSED(devices); GGML_UNUSED(state_config);
    fprintf(stderr, "HaloFPX worker-local state support is not compiled in\n");
#endif
}

// device interface

struct ggml_backend_rpc_device_context {
    std::string endpoint;
    uint32_t    device;
    std::string name;
    std::string description;
};

static const char * ggml_backend_rpc_device_get_name(ggml_backend_dev_t dev) {
    ggml_backend_rpc_device_context * ctx = (ggml_backend_rpc_device_context *)dev->context;

    return ctx->name.c_str();
}

static const char * ggml_backend_rpc_device_get_description(ggml_backend_dev_t dev) {
    ggml_backend_rpc_device_context * ctx = (ggml_backend_rpc_device_context *)dev->context;

    return ctx->description.c_str();
}

static void ggml_backend_rpc_device_get_memory(ggml_backend_dev_t dev, size_t * free, size_t * total) {
    ggml_backend_rpc_device_context * ctx = (ggml_backend_rpc_device_context *)dev->context;

    ggml_backend_rpc_get_device_memory(ctx->endpoint.c_str(), ctx->device, free, total);
}

static enum ggml_backend_dev_type ggml_backend_rpc_device_get_type(ggml_backend_dev_t dev) {
    // TODO: obtain value from the server
    return GGML_BACKEND_DEVICE_TYPE_GPU;

    GGML_UNUSED(dev);
}

static void ggml_backend_rpc_device_get_props(ggml_backend_dev_t dev, struct ggml_backend_dev_props * props) {
    props->name        = ggml_backend_rpc_device_get_name(dev);
    props->description = ggml_backend_rpc_device_get_description(dev);
    props->type        = ggml_backend_rpc_device_get_type(dev);
    ggml_backend_rpc_device_get_memory(dev, &props->memory_free, &props->memory_total);
    props->caps = {
        /* .async                 = */ false,
        /* .host_buffer           = */ false,
        /* .buffer_from_host_ptr  = */ false,
        /* .events                = */ false,
    };
}

static ggml_backend_t ggml_backend_rpc_device_init(ggml_backend_dev_t dev, const char * params) {
    ggml_backend_rpc_device_context * ctx = (ggml_backend_rpc_device_context *)dev->context;

    return ggml_backend_rpc_init(ctx->endpoint.c_str(), ctx->device);

    GGML_UNUSED(params);
}

static ggml_backend_buffer_type_t ggml_backend_rpc_device_get_buffer_type(ggml_backend_dev_t dev) {
    ggml_backend_rpc_device_context * ctx = (ggml_backend_rpc_device_context *)dev->context;

    return ggml_backend_rpc_buffer_type(ctx->endpoint.c_str(), ctx->device);

    GGML_UNUSED(dev);
}

static bool ggml_backend_rpc_device_supports_op(ggml_backend_dev_t dev, const struct ggml_tensor * op) {
    GGML_UNUSED(dev);
    GGML_UNUSED(op);
    //TODO: call the remote backend and cache the results
    return true;
}

static bool ggml_backend_rpc_device_supports_buft(ggml_backend_dev_t dev, ggml_backend_buffer_type_t buft) {
    if (!buft || buft->iface.get_name != ggml_backend_rpc_buffer_type_name) {
        return false;
    }
    ggml_backend_rpc_buffer_type_context * buft_ctx = (ggml_backend_rpc_buffer_type_context *)buft->context;
    ggml_backend_rpc_device_context * dev_ctx = (ggml_backend_rpc_device_context *)dev->context;
    return buft_ctx->endpoint == dev_ctx->endpoint && buft_ctx->device == dev_ctx->device;
}

static const struct ggml_backend_device_i ggml_backend_rpc_device_i = {
    /* .get_name             = */ ggml_backend_rpc_device_get_name,
    /* .get_description      = */ ggml_backend_rpc_device_get_description,
    /* .get_memory           = */ ggml_backend_rpc_device_get_memory,
    /* .get_type             = */ ggml_backend_rpc_device_get_type,
    /* .get_props            = */ ggml_backend_rpc_device_get_props,
    /* .init_backend         = */ ggml_backend_rpc_device_init,
    /* .get_buffer_type      = */ ggml_backend_rpc_device_get_buffer_type,
    /* .get_host_buffer_type = */ NULL,
    /* .buffer_from_host_ptr = */ NULL,
    /* .supports_op          = */ ggml_backend_rpc_device_supports_op,
    /* .supports_buft        = */ ggml_backend_rpc_device_supports_buft,
    /* .offload_op           = */ NULL,
    /* .event_new            = */ NULL,
    /* .event_free           = */ NULL,
    /* .event_synchronize    = */ NULL,
};

// backend reg interface

struct ggml_backend_rpc_reg_context {
    std::string                     name;
    std::vector<ggml_backend_dev_t> devices;
};

static const char * ggml_backend_rpc_reg_get_name(ggml_backend_reg_t reg) {
    ggml_backend_rpc_reg_context * ctx = (ggml_backend_rpc_reg_context *)reg->context;
    return ctx ? ctx->name.c_str() : "RPC";
}

static size_t ggml_backend_rpc_reg_get_device_count(ggml_backend_reg_t reg) {
    ggml_backend_rpc_reg_context * ctx = (ggml_backend_rpc_reg_context *)reg->context;
    return ctx ? ctx->devices.size() : 0;
}

static ggml_backend_dev_t ggml_backend_rpc_reg_get_device(ggml_backend_reg_t reg, size_t index) {
    ggml_backend_rpc_reg_context * ctx = (ggml_backend_rpc_reg_context *)reg->context;
    if (ctx == nullptr) {
        GGML_ABORT("The RPC backend does not have enumerated devices - use ggml_backend_rpc_add_server instead");
    } else {
        GGML_ASSERT(index < ctx->devices.size());
        return ctx->devices[index];
    }
}

static void * ggml_backend_rpc_get_proc_address(ggml_backend_reg_t reg, const char * name) {
    if (std::strcmp(name, "ggml_backend_rpc_add_server") == 0) {
        return (void *)ggml_backend_rpc_add_server;
    }
    if (std::strcmp(name, "ggml_backend_rpc_start_server") == 0) {
        return (void *)ggml_backend_rpc_start_server;
    }
    if (std::strcmp(name, "ggml_backend_rpc_start_server_with_halofpx_state") == 0) {
        return (void *)ggml_backend_rpc_start_server_with_halofpx_state;
    }
    return NULL;

    GGML_UNUSED(reg);
}

static const struct ggml_backend_reg_i ggml_backend_rpc_reg_i = {
    /* .get_name         = */ ggml_backend_rpc_reg_get_name,
    /* .get_device_count = */ ggml_backend_rpc_reg_get_device_count,
    /* .get_device       = */ ggml_backend_rpc_reg_get_device,
    /* .get_proc_address = */ ggml_backend_rpc_get_proc_address,
};

ggml_backend_reg_t ggml_backend_rpc_reg(void) {
    static struct ggml_backend_reg ggml_backend_rpc_reg = {
        /* .api_version = */ GGML_BACKEND_API_VERSION,
        /* .iface       = */ ggml_backend_rpc_reg_i,
        /* .context     = */ NULL,
    };

    return &ggml_backend_rpc_reg;
}

static uint32_t ggml_backend_rpc_get_device_count(const char * endpoint) {
    auto sock = get_socket(endpoint);
    if (sock == nullptr) {
        GGML_LOG_ERROR("Failed to connect to %s\n", endpoint);
        return 0;
    }
    rpc_msg_device_count_rsp response;
    bool status = send_rpc_cmd(sock, RPC_CMD_DEVICE_COUNT, nullptr, 0, &response, sizeof(response));
    RPC_STATUS_ASSERT(status);
    return response.device_count;
}

static const ggml_backend_reg_i ggml_backend_rpc_reg_interface = {
    /* .get_name          = */ ggml_backend_rpc_reg_get_name,
    /* .get_device_count  = */ ggml_backend_rpc_reg_get_device_count,
    /* .get_device        = */ ggml_backend_rpc_reg_get_device,
    /* .get_proc_address  = */ ggml_backend_rpc_get_proc_address,
};

ggml_backend_reg_t ggml_backend_rpc_add_server(const char * endpoint) {
    static std::unordered_map<std::string, ggml_backend_reg_t> reg_map;
    static std::mutex mutex;
    static uint32_t dev_id = 0;
    std::lock_guard<std::mutex> lock(mutex);
    if (reg_map.find(endpoint) != reg_map.end()) {
        return reg_map[endpoint];
    }
    uint32_t dev_count = ggml_backend_rpc_get_device_count(endpoint);
    if (dev_count == 0) {
        return nullptr;
    }
    ggml_backend_rpc_reg_context * ctx = new ggml_backend_rpc_reg_context;
    ctx->name = "RPC[" + std::string(endpoint) + "]";
    for (uint32_t ind = 0; ind < dev_count; ind++) {
        std::string dev_name = "RPC" + std::to_string(dev_id);
        std::string dev_desc = std::string(endpoint);
        ggml_backend_rpc_device_context * dev_ctx = new ggml_backend_rpc_device_context {
            /* .endpoint    = */ endpoint,
            /* .device      = */ ind,
            /* .name        = */ dev_name,
            /* .description = */ dev_desc
        };

        ggml_backend_dev_t dev = new ggml_backend_device {
            /* .iface   = */ ggml_backend_rpc_device_i,
            /* .reg     = */ ggml_backend_rpc_reg(),
            /* .context = */ dev_ctx,
        };
        ctx->devices.push_back(dev);
        dev_id++;
    }
    ggml_backend_reg_t reg = new ggml_backend_reg {
        /* .api_version = */ GGML_BACKEND_API_VERSION,
        /* .iface       = */ ggml_backend_rpc_reg_interface,
        /* .context     = */ ctx
    };
    reg_map[endpoint] = reg;
    return reg;
}


GGML_BACKEND_DL_IMPL(ggml_backend_rpc_reg)
