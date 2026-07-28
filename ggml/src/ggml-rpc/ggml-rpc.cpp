#include "ggml-rpc.h"
#include "ggml-impl.h"
#include "ggml-backend-impl.h"
#include "ggml-cpp.h"
#include "transport.h"

#include <array>
#include <atomic>
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
#include <functional>
#include <tuple>
#include <type_traits>

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
    RPC_CMD_HALOFPX_GRAPH_AUTH_CAPS,
    RPC_CMD_HALOFPX_GRAPH_AUTH_COMPUTE,
    RPC_CMD_HALOFPX_GRAPH_AUTH_RECOMPUTE,
    RPC_CMD_HALOFPX_GRAPH_AUTH_EXECUTE,
    RPC_CMD_HALOFPX_MUTABLE_CAPS,
    RPC_CMD_HALOFPX_MUTABLE_SET,
    RPC_CMD_HALOFPX_MUTABLE_SET_HASH,
    RPC_CMD_HALOFPX_MUTABLE_COMMIT,
    RPC_CMD_HALOFPX_MUTABLE_BIND,
    RPC_CMD_HALOFPX_MUTABLE_PREFLIGHT,
#endif
    RPC_CMD_COUNT,
};

static_assert(RPC_CMD_HELLO == 14, "RPC_CMD_HELLO must be always 14");
#ifdef GGML_RPC_HALOFPX_LOCAL_STATE
static_assert(RPC_CMD_HALOFPX_STATE_CAPS == 17, "HaloFPX CAPS command ordinal must be always 17");
static_assert(RPC_CMD_HALOFPX_GRAPH_AUTH_CAPS == 22, "HaloFPX graph auth CAPS ordinal must be always 22");
static_assert(RPC_CMD_HALOFPX_GRAPH_AUTH_COMPUTE == 23, "HaloFPX graph auth COMPUTE ordinal must be always 23");
static_assert(RPC_CMD_HALOFPX_GRAPH_AUTH_RECOMPUTE == 24, "HaloFPX graph auth RECOMPUTE ordinal must be always 24");
static_assert(RPC_CMD_HALOFPX_GRAPH_AUTH_EXECUTE == 25, "HaloFPX graph auth EXECUTE ordinal must be always 25");
static_assert(RPC_CMD_HALOFPX_MUTABLE_CAPS == 26, "HaloFPX mutable CAPS ordinal must be always 26");
static_assert(RPC_CMD_HALOFPX_MUTABLE_SET == 27, "HaloFPX mutable SET ordinal must be always 27");
static_assert(RPC_CMD_HALOFPX_MUTABLE_SET_HASH == 28, "HaloFPX mutable SET_HASH ordinal must be always 28");
static_assert(RPC_CMD_HALOFPX_MUTABLE_COMMIT == 29, "HaloFPX mutable COMMIT ordinal must be always 29");
static_assert(RPC_CMD_HALOFPX_MUTABLE_BIND == 30, "HaloFPX mutable BIND ordinal must be always 30");
static_assert(RPC_CMD_HALOFPX_MUTABLE_PREFLIGHT == 31, "HaloFPX mutable PREFLIGHT ordinal must be always 31");
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
extern "C" uint32_t ggml_backend_rpc_halofpx_graph_auth_self_test(void);

static constexpr uint16_t HFX_GRAPH_AUTH_MAJOR = 1;
static constexpr uint16_t HFX_GRAPH_AUTH_MINOR = 0;
static constexpr uint32_t HFX_GRAPH_AUTH_MAX_TENSORS = 65536;
static constexpr uint32_t HFX_GRAPH_AUTH_MAX_NODES = 65536;
static constexpr uint32_t HFX_GRAPH_AUTH_MAX_GRAPH_BYTES = 64U << 20;
static constexpr char HFX_GRAPH_AUTH_DOMAIN[] = "halofpx.rpc-graph-authority.v1";
static constexpr char HFX_RPC_RESPONSE_DOMAIN[] = "halofpx.rpc-response-boundary.v1";
static constexpr char HFX_PREEXECUTE_DOMAIN[] = "halofpx.preexecute-authority.v1";
static constexpr uint32_t HFX_PREEXECUTE_MAX_EVENTS = 256;
static constexpr uint32_t HFX_RPC_RESPONSE_MAX_EVENTS = 64;

struct hfx_graph_auth_caps_req {
    uint8_t magic[8];
    uint16_t major;
    uint16_t minor;
    uint32_t encoded_size;
    uint8_t attempt_nonce[32];
    uint8_t tag[32];
};

struct hfx_graph_auth_caps_rsp {
    uint8_t magic[8];
    uint16_t major;
    uint16_t minor;
    uint32_t encoded_size;
    uint32_t status;
    uint32_t max_graph_bytes;
    uint32_t max_tensors;
    uint32_t max_nodes;
    uint8_t attempt_nonce[32];
    uint8_t server_nonce[32];
    uint8_t tag[32];
};

struct hfx_graph_auth_header {
    uint8_t magic[8];
    uint16_t major;
    uint16_t minor;
    uint32_t encoded_size;
    uint32_t graph_size;
    uint32_t device;
    uint64_t graph_uid;
    uint64_t exec_sequence;
    uint8_t attempt_nonce[32];
    uint8_t server_nonce[32];
    uint8_t graph_digest[32];
    uint8_t transcript_root[32];
    uint8_t admission_object_id[32];
    uint8_t expected_admission_digest[32];
    uint8_t tag[32];
};

struct hfx_graph_auth_receipt {
    uint8_t magic[8];
    uint16_t major;
    uint16_t minor;
    uint32_t encoded_size;
    uint32_t status;
    uint32_t device;
    uint64_t graph_uid;
    uint64_t exec_sequence;
    uint8_t attempt_nonce[32];
    uint8_t server_nonce[32];
    uint8_t graph_digest[32];
    uint8_t transcript_root[32];
    uint8_t admission_object_id[32];
    uint8_t expected_admission_digest[32];
    uint8_t tag[32];
};

static_assert(sizeof(hfx_graph_auth_caps_req) == 80, "graph auth caps request size");
static_assert(sizeof(hfx_graph_auth_caps_rsp) == 128, "graph auth caps response size");
static_assert(sizeof(hfx_graph_auth_header) == 264, "graph auth header size");
static_assert(sizeof(hfx_graph_auth_receipt) == 264, "graph auth receipt size");

static constexpr uint16_t HFX_MUTABLE_MAJOR = 3;
static constexpr uint16_t HFX_MUTABLE_MINOR = 1;
// L85 resolved-storage census projection on every L44 census wire record.
static constexpr uint64_t HFX_MUTABLE_CAPABILITIES = UINT64_C(0x7ff);
static constexpr uint32_t HFX_MUTABLE_MAX_MUTATIONS = 4096;
static constexpr uint32_t HFX_MUTABLE_MAX_CENSUS = 4096;
static constexpr uint64_t HFX_MUTABLE_MAX_BYTES = UINT64_C(64) << 20;
static constexpr char HFX_MUTABLE_DOMAIN[] = "halofpx.rpc-mutable-authority.v1";
static constexpr uint32_t HFX_MUTABLE_EXCLUSION_BASE = UINT32_C(0x80000000);

struct hfx_mutable_caps {
    uint8_t magic[8];
    uint16_t major;
    uint16_t minor;
    uint32_t encoded_size;
    uint32_t status;
    uint32_t max_mutations;
    uint32_t max_census;
    uint32_t reserved;
    uint64_t capabilities;
    uint16_t graph_auth_major;
    uint16_t scheduler_admission_major;
    uint16_t mutable_major;
    uint16_t version_reserved;
    uint64_t graph_uid;
    uint64_t exec_sequence;
    uint64_t scheduler_sequence;
    uint64_t client_connection_epoch;
    uint64_t server_connection_epoch;
    uint64_t allocation_epoch;
    uint8_t attempt_nonce[32];
    uint8_t scheduler_nonce[32];
    uint8_t scheduler_root[32];
    uint8_t expected_admission_digest[32];
    struct ggml_backend_sched_authority_prepared_admission scheduler_expected_admission;
    struct ggml_backend_sched_authority_prepared_admission scheduler_admission;
    uint8_t server_nonce[32];
    uint8_t tag[32];
};
static rpc_tensor serialize_tensor(const ggml_tensor * tensor);

struct hfx_mutable_update_header {
    uint8_t magic[8];
    uint16_t major;
    uint16_t minor;
    uint32_t encoded_size;
    uint32_t role;
    uint32_t role_ordinal;
    uint32_t allocation_ordinal;
    uint32_t type;
    uint64_t graph_uid;
    uint64_t exec_sequence;
    uint64_t mutation_sequence;
    uint64_t tensor_relative;
    uint64_t logical_offset;
    uint64_t logical_size;
    uint64_t cache_hash;
    uint32_t ne[GGML_MAX_DIMS];
    uint32_t nb[GGML_MAX_DIMS];
    uint8_t attempt_nonce[32];
    uint8_t server_nonce[32];
    uint8_t content_digest[32];
    uint8_t view_digest[32];
    uint8_t prior_root[32];
    uint8_t tag[32];
};

struct hfx_mutable_receipt {
    uint8_t magic[8];
    uint16_t major;
    uint16_t minor;
    uint32_t encoded_size;
    uint32_t status;
    uint32_t role;
    uint32_t role_ordinal;
    uint32_t allocation_ordinal;
    uint64_t graph_uid;
    uint64_t exec_sequence;
    uint64_t mutation_sequence;
    uint64_t tensor_relative;
    uint64_t logical_offset;
    uint64_t logical_size;
    uint8_t attempt_nonce[32];
    uint8_t server_nonce[32];
    uint8_t content_digest[32];
    uint8_t view_digest[32];
    uint8_t mutation_root[32];
    uint8_t tag[32];
};

struct hfx_mutable_census_entry {
    uint32_t role;
    uint32_t role_ordinal;
    uint32_t allocation_ordinal;
    uint32_t type;
    uint64_t tensor_relative;
    uint64_t logical_size;
    uint32_t ne[GGML_MAX_DIMS];
    uint32_t nb[GGML_MAX_DIMS];
    uint8_t view_digest[32];
    uint8_t logical_tensor_identity[32];
    uint8_t storage_tensor_identity[32];
    uint8_t runtime_semantic_identity[32];
    uint8_t rpc_endpoint_identity[32];
    uint64_t rpc_connection_epoch;
    uint32_t destination_backend_ordinal;
    uint32_t rpc_device;
};

struct hfx_mutable_commit_header {
    uint8_t magic[8];
    uint16_t major;
    uint16_t minor;
    uint32_t encoded_size;
    uint32_t census_count;
    uint32_t mutation_count;
    uint64_t graph_uid;
    uint64_t exec_sequence;
    uint64_t parent_graph_uid;
    uint32_t split_ordinal;
    uint32_t backend_ordinal;
    uint8_t attempt_nonce[32];
    uint8_t server_nonce[32];
    uint8_t scheduler_nonce[32];
    uint8_t scheduler_root[32];
    uint8_t mutation_root[32];
    uint8_t census_root[32];
    uint8_t split_mapping_root[32];
    uint8_t tag[32];
};

struct hfx_mutable_preflight_wire {
    uint8_t magic[8];
    uint16_t major;
    uint16_t minor;
    uint32_t encoded_size;
    uint32_t status;
    uint32_t reserved;
    uint64_t capabilities;
    uint64_t key_generation;
    uint64_t client_connection_epoch;
    uint64_t server_connection_epoch;
    uint64_t allocation_topology_epoch;
    uint8_t client_nonce[32];
    uint8_t server_nonce[32];
    uint8_t tag[32];
};

static_assert(sizeof(hfx_mutable_preflight_wire) == 160, "mutable preflight size");
static_assert(sizeof(hfx_mutable_update_header) == 312, "mutable update size");
static_assert(sizeof(hfx_mutable_receipt) == 272, "mutable receipt size");
static_assert(sizeof(hfx_mutable_census_entry) == 240, "mutable census entry size");
static_assert(sizeof(hfx_mutable_commit_header) == 312, "mutable commit size");

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

#ifdef GGML_RPC_HALOFPX_LOCAL_STATE
static constexpr size_t HFX_PREEXEC_REGISTER_EVENT_INDEX = 4;
static constexpr size_t HFX_PREEXEC_EXCLUDE_EVENT_INDEX = 5;

struct hfx_preexecute_recorder {
    enum role_type : uint32_t { CLIENT = 1, SERVER = 2 };
    std::mutex mutex;
    role_type role = CLIENT;
    uint64_t generation = 0;
    uint64_t event_sequence = 0;
    uint64_t execution_sequence = 0;
    uint64_t parent_uid = 0;
    uint64_t split_uid = 0;
    uint64_t client_connection_epoch = 0;
    uint64_t server_connection_epoch = 0;
    uint64_t allocation_epoch = 0;
    uint32_t backend_ordinal = UINT32_MAX;
    uint32_t split_ordinal = UINT32_MAX;
    uint32_t execute_receipt_state = 0;
    uint32_t terminal_branch = 0;
    uint32_t expected_register = 0;
    uint32_t expected_exclude = 0;
    bool expected_census_installed = false;
    std::array<uint32_t, 12> event_counts {};
    std::array<uint32_t, 256> transport_phase {};
    std::array<uint8_t, 32> attempt_nonce {};
    std::array<uint8_t, 32> chain {};
    std::array<uint8_t, 32> admission_object_id {};
    std::array<uint8_t, 32> expected_admission_digest {};
    std::array<uint8_t, 32> graph_digest {};
    std::array<uint8_t, 32> execute_receipt {};
    std::vector<std::string> pending_records;
    std::vector<uint32_t> observed_events;
    std::vector<uint32_t> observed_transport_states;
    std::vector<uint32_t> server_events;
    std::vector<uint32_t> server_reasons;
    bool terminal = false;
};

static bool hfx_preexecute_install_expected_census(
        const std::shared_ptr<hfx_preexecute_recorder> & recorder,
        const struct ggml_backend_sched_authority_prepared_admission & admission) {
    if (!recorder ||
        admission.logical_expected_mutable_count > HFX_MUTABLE_MAX_CENSUS ||
        admission.logical_expected_exclusion_count > HFX_MUTABLE_MAX_CENSUS) {
        return false;
    }
    std::lock_guard<std::mutex> lock(recorder->mutex);
    if (recorder->terminal || recorder->expected_census_installed ||
        recorder->event_counts[HFX_PREEXEC_REGISTER_EVENT_INDEX] != 0 ||
        recorder->event_counts[HFX_PREEXEC_EXCLUDE_EVENT_INDEX] != 0) {
        return false;
    }
    recorder->expected_register = admission.logical_expected_mutable_count;
    recorder->expected_exclude = admission.logical_expected_exclusion_count;
    recorder->expected_census_installed = true;
    return true;
}
#endif

struct ggml_backend_rpc_context {
    std::string endpoint;
    uint32_t    device;
    std::string name;
    uint64_t    last_graph_uid;
#ifdef GGML_RPC_HALOFPX_LOCAL_STATE
    bool graph_auth_negotiated;
    uint64_t graph_auth_sequence;
    std::array<uint8_t, 32> graph_auth_key;
    std::array<uint8_t, 32> graph_auth_attempt_nonce;
    std::array<uint8_t, 32> graph_auth_server_nonce;
    std::array<uint8_t, 32> graph_auth_last_digest;
    std::array<uint8_t, 32> graph_auth_transcript_root;
    ggml_backend_rpc_halofpx_graph_result graph_auth_result;
    std::vector<ggml_backend_rpc_halofpx_graph_result> graph_auth_results;
    bool execution_armed;
    bool execution_splits_bound;
    bool execution_mutable_begin_consumed;
    uint64_t execution_sequence;
    uint64_t last_execution_sequence;
    uint64_t execution_parent_graph_uid;
    uint32_t execution_backend_ordinal;
    std::array<uint8_t, 32> execution_split_mapping_root;
    std::vector<ggml_backend_rpc_halofpx_split_identity> execution_splits;
    std::unordered_set<uint64_t> execution_consumed_split_uids;
    std::array<uint8_t, 32> execution_attempt_nonce;
    std::shared_ptr<hfx_preexecute_recorder> preexecute_recorder;
#endif
};

struct ggml_backend_rpc_buffer_context {
    std::shared_ptr<socket_t> sock;
    void * base_ptr;
    uint64_t remote_ptr;
    uint32_t device;
#ifdef GGML_RPC_HALOFPX_LOCAL_STATE
    uint32_t allocation_ordinal;
#endif
};

#ifdef GGML_RPC_HALOFPX_LOCAL_STATE
struct hfx_mutable_registration {
    uint32_t role = 0;
    uint32_t role_ordinal = 0;
    std::array<uint8_t, 32> logical_tensor_identity {};
    std::array<uint8_t, 32> storage_tensor_identity {};
    std::array<uint8_t, 32> runtime_semantic_identity {};
    std::array<uint8_t, 32> rpc_endpoint_identity {};
    uint64_t rpc_connection_epoch = 0;
    uint32_t destination_backend_ordinal = UINT32_MAX;
    uint32_t rpc_device = UINT32_MAX;
};

struct hfx_mutable_client_session {
    std::mutex mutex;
    uint64_t session_id = 0;
    uint64_t generation = 0;
    uint64_t connection_epoch = 0;
    uint64_t client_connection_epoch = 0;
    uint64_t allocation_epoch = 0;
    uint64_t bound_graph_uid = 0;
    uint64_t bound_parent_graph_uid = 0;
    uint32_t bound_split_ordinal = UINT32_MAX;
    uint32_t bound_backend_ordinal = UINT32_MAX;
    std::array<uint8_t, 32> split_mapping_root {};
    std::string endpoint;
    socket_ptr sock;
    ggml_backend_rpc_halofpx_mutable_attempt attempt {};
    struct ggml_backend_sched_authority_prepared_admission admission {};
    struct ggml_backend_sched_authority_prepared_admission expected_admission {};
    std::array<uint8_t, 32> key {};
    std::array<uint8_t, 32> server_nonce {};
    std::array<uint8_t, 32> mutation_root {};
    std::array<uint8_t, 32> semantic_root {};
    uint64_t mutation_sequence = 0;
    uint32_t census_count = 0;
    uint32_t set_count = 0;
    uint32_t set_hash_hit_count = 0;
    uint32_t set_hash_miss_count = 0;
    bool negotiated = false;
    bool prepared = false;
    bool committed = false;
    bool failed = false;
    std::unordered_map<const ggml_tensor *, hfx_mutable_registration> roles;
    std::unordered_map<const ggml_tensor *, hfx_mutable_registration> exclusions;
    std::vector<std::pair<const ggml_tensor *, hfx_mutable_registration>>
        projected_census;
    std::vector<hfx_mutable_census_entry> mutations;
    std::shared_ptr<hfx_preexecute_recorder> recorder;
};

static std::mutex hfx_mutable_mutex;
static std::unordered_map<uint64_t, std::shared_ptr<hfx_mutable_client_session>> hfx_mutable_sessions;
static std::unordered_map<const socket_t *, uint64_t> hfx_mutable_active_sockets;
static std::atomic<uint64_t> hfx_mutable_next_session { 1 };
static std::atomic<uint64_t> hfx_preexecute_next_generation { 1 };

static void hfx_mutable_close_session_locked(uint64_t session_id) {
    auto it = hfx_mutable_sessions.find(session_id);
    if (it == hfx_mutable_sessions.end()) return;
    auto session = it->second;
    auto active = hfx_mutable_active_sockets.find(session->sock.get());
    if (active != hfx_mutable_active_sockets.end() && active->second == session_id) {
        hfx_mutable_active_sockets.erase(active);
    }
    hfx_mutable_sessions.erase(it);
    std::lock_guard<std::mutex> session_lock(session->mutex);
    std::fill(session->key.begin(), session->key.end(), 0);
    std::fill(session->server_nonce.begin(), session->server_nonce.end(), 0);
    session->roles.clear();
    session->exclusions.clear();
    session->failed = true;
}

static bool hfx_mutable_requested() {
    const char * value = std::getenv("HALOFPX_RPC_MUTABLE_AUTH");
    return value != nullptr && strcmp(value, "1") == 0;
}

static bool hfx_mutable_session_active(const socket_t * sock) {
    std::lock_guard<std::mutex> lock(hfx_mutable_mutex);
    return hfx_mutable_active_sockets.find(sock) != hfx_mutable_active_sockets.end();
}

static bool ggml_backend_buffer_is_rpc(ggml_backend_buffer_t buffer);

static bool hfx_mutable_same_tensor_authority(
        const ggml_tensor * left,
        const ggml_tensor * right) {
    if (left == nullptr || right == nullptr ||
        left->buffer == nullptr || right->buffer == nullptr ||
        !ggml_backend_buffer_is_rpc(left->buffer) ||
        !ggml_backend_buffer_is_rpc(right->buffer) ||
        left->type != right->type) return false;
    auto * lctx = static_cast<ggml_backend_rpc_buffer_context *>(left->buffer->context);
    auto * rctx = static_cast<ggml_backend_rpc_buffer_context *>(right->buffer->context);
    if (lctx->sock.get() != rctx->sock.get() ||
        lctx->device != rctx->device ||
        lctx->allocation_ordinal != rctx->allocation_ordinal) return false;
    const uint64_t lbase = reinterpret_cast<uint64_t>(ggml_backend_buffer_get_base(left->buffer));
    const uint64_t rbase = reinterpret_cast<uint64_t>(ggml_backend_buffer_get_base(right->buffer));
    const uint64_t ldata = reinterpret_cast<uint64_t>(left->data);
    const uint64_t rdata = reinterpret_cast<uint64_t>(right->data);
    if (ldata < lbase || rdata < rbase || ldata - lbase != rdata - rbase) return false;
    for (size_t i = 0; i < GGML_MAX_DIMS; ++i) {
        if (left->ne[i] != right->ne[i] || left->nb[i] != right->nb[i]) return false;
    }
    return ggml_nbytes(left) == ggml_nbytes(right);
}

static bool hfx_mutable_registration_equal(
        const hfx_mutable_registration & left,
        const hfx_mutable_registration & right) {
    return left.role == right.role &&
        left.role_ordinal == right.role_ordinal &&
        left.logical_tensor_identity == right.logical_tensor_identity &&
        left.storage_tensor_identity == right.storage_tensor_identity &&
        left.runtime_semantic_identity == right.runtime_semantic_identity &&
        left.rpc_endpoint_identity == right.rpc_endpoint_identity &&
        left.rpc_connection_epoch == right.rpc_connection_epoch &&
        left.destination_backend_ordinal == right.destination_backend_ordinal &&
        left.rpc_device == right.rpc_device;
}

static const hfx_mutable_registration * hfx_mutable_find_registration(
        const std::unordered_map<const ggml_tensor *, hfx_mutable_registration> & registrations,
        const ggml_tensor * tensor) {
    const auto exact = registrations.find(tensor);
    if (exact != registrations.end()) return &exact->second;
    const hfx_mutable_registration * result = nullptr;
    for (const auto & entry : registrations) {
        if (!hfx_mutable_same_tensor_authority(entry.first, tensor)) continue;
        if (result != nullptr &&
            (result->role != entry.second.role ||
             result->role_ordinal != entry.second.role_ordinal)) return nullptr;
        result = &entry.second;
    }
    return result;
}

static std::shared_ptr<hfx_mutable_client_session> hfx_mutable_lookup_locked(
        const ggml_backend_rpc_halofpx_mutable_session * handle,
        bool allow_failed = false) {
    if (handle == nullptr || handle->version != HFX_MUTABLE_MAJOR || handle->reserved != 0 ||
        handle->session_id == 0 || handle->generation == 0 || handle->connection_epoch == 0 ||
        handle->allocation_epoch == 0 ||
        handle->graph_uid == 0 || handle->execution_sequence == 0 ||
        std::all_of(std::begin(handle->attempt_nonce), std::end(handle->attempt_nonce),
                    [](uint8_t value) { return value == 0; })) return nullptr;
    auto it = hfx_mutable_sessions.find(handle->session_id);
    if (it == hfx_mutable_sessions.end()) return nullptr;
    auto session = it->second;
    if (session == nullptr || (!allow_failed && session->failed) || !session->negotiated ||
        session->session_id != handle->session_id ||
        session->generation != handle->generation ||
        session->connection_epoch != handle->connection_epoch ||
        session->allocation_epoch != handle->allocation_epoch ||
        session->client_connection_epoch != session->sock->authority_epoch() ||
        session->attempt.graph_uid != handle->graph_uid ||
        session->attempt.execution_sequence != handle->execution_sequence ||
        memcmp(session->attempt.attempt_nonce, handle->attempt_nonce, 32) != 0) return nullptr;
    auto active = hfx_mutable_active_sockets.find(session->sock.get());
    return active != hfx_mutable_active_sockets.end() && active->second == session->session_id
        ? session : nullptr;
}

static bool hfx_mutable_role_valid(uint32_t role) {
    return role >= GGML_RPC_HALOFPX_MUTABLE_TOKEN &&
           role <= GGML_RPC_HALOFPX_MUTABLE_SCHEDULER_COPY;
}

static bool hfx_mutable_census_role_valid(uint32_t role) {
    return hfx_mutable_role_valid(role) ||
        role == HFX_MUTABLE_EXCLUSION_BASE + GGML_RPC_HALOFPX_EXCLUDE_IMMUTABLE_MODEL_WEIGHT ||
        role == HFX_MUTABLE_EXCLUSION_BASE + GGML_RPC_HALOFPX_EXCLUDE_LOCAL_STATE_PAYLOAD;
}
#endif

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

hfx_digest hfx_graph_hmac(const uint8_t key[32], const void * data, size_t size) {
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
    sha256_update(&ctx, reinterpret_cast<const uint8_t *>(HFX_GRAPH_AUTH_DOMAIN), sizeof(HFX_GRAPH_AUTH_DOMAIN) - 1);
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

hfx_digest hfx_rpc_response_hmac(const uint8_t key[32], const void * data, size_t size) {
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
    sha256_update(&ctx, reinterpret_cast<const uint8_t *>(HFX_RPC_RESPONSE_DOMAIN),
                  sizeof(HFX_RPC_RESPONSE_DOMAIN) - 1);
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

bool hfx_graph_key(std::array<uint8_t, 32> & key) {
    auto hex = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    };
    std::array<char, 65> file_hex {};
    const char * value = std::getenv("HALOFPX_RPC_GRAPH_AUTH_KEY_HEX");
    const char * key_file = std::getenv("HALOFPX_RPC_GRAPH_AUTH_KEY_FILE");
    if (key_file != nullptr) {
        if (value != nullptr || key_file[0] != '/') return false;
        const int fd = open(key_file, O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
        struct stat metadata {};
        std::array<char, 131> raw {};
        size_t used = 0;
        const char * expected_digest =
            std::getenv("HALOFPX_RPC_GRAPH_AUTH_KEY_SHA256");
        std::array<uint8_t, 32> expected_digest_bytes {};
        bool expected_digest_valid =
            expected_digest != nullptr && strlen(expected_digest) == 64;
        for (size_t i = 0; expected_digest_valid && i < expected_digest_bytes.size(); ++i) {
            const int hi = hex(expected_digest[2*i]);
            const int lo = hex(expected_digest[2*i + 1]);
            if (hi < 0 || lo < 0) expected_digest_valid = false;
            else expected_digest_bytes[i] = static_cast<uint8_t>((hi << 4) | lo);
        }
        bool valid = fd >= 0 && fstat(fd, &metadata) == 0 &&
            S_ISREG(metadata.st_mode) && (metadata.st_mode & 0777) == 0600 &&
            metadata.st_size == 130 && metadata.st_uid == geteuid() &&
            expected_digest_valid;
        while (valid && used < raw.size()) {
            const ssize_t count = read(fd, raw.data() + used, raw.size() - used);
            if (count < 0 && errno == EINTR) continue;
            if (count < 0) valid = false;
            if (count <= 0) break;
            used += static_cast<size_t>(count);
        }
        if (fd >= 0 && close(fd) != 0) valid = false;
        const hfx_digest actual_digest =
            valid && used == 130 ? hfx_sha256(raw.data(), used) : hfx_digest {};
        uint8_t expected_digest_diff = 0;
        for (size_t i = 0; i < expected_digest_bytes.size(); ++i) {
            expected_digest_diff |= actual_digest[i] ^ expected_digest_bytes[i];
        }
        valid = valid && used == 130 && raw[64] == '\n' && raw[129] == '\n' &&
            expected_digest_valid && expected_digest_diff == 0;
        hfx_wipe(expected_digest_bytes.data(), expected_digest_bytes.size());
        if (!valid) {
            hfx_wipe(raw.data(), raw.size());
            return false;
        }
        memcpy(file_hex.data(), raw.data(), 64);
        hfx_wipe(raw.data(), raw.size());
        value = file_hex.data();
    }
    if (value == nullptr || strlen(value) != 64) return false;
    for (size_t i = 0; i < key.size(); ++i) {
        const int hi = hex(value[2*i]);
        const int lo = hex(value[2*i + 1]);
        if (hi < 0 || lo < 0) {
            hfx_wipe(key.data(), key.size());
            hfx_wipe(file_hex.data(), file_hex.size());
            return false;
        }
        key[i] = static_cast<uint8_t>((hi << 4) | lo);
    }
    uint8_t nonzero = 0;
    for (uint8_t byte : key) nonzero |= byte;
    hfx_wipe(file_hex.data(), file_hex.size());
    return nonzero != 0;
}

bool hfx_graph_requested() {
    const char * value = std::getenv("HALOFPX_RPC_GRAPH_AUTH");
    return value != nullptr && strcmp(value, "1") == 0;
}

template<typename T>
void hfx_le(std::vector<uint8_t> & out, T value) {
    static_assert(std::is_integral<T>::value, "LE values must be integral");
    using U = typename std::make_unsigned<T>::type;
    const U v = static_cast<U>(value);
    for (size_t i = 0; i < sizeof(T); ++i) out.push_back(static_cast<uint8_t>(v >> (8*i)));
}

void hfx_bytes(std::vector<uint8_t> & out, const void * data, size_t size) {
    const auto * begin = static_cast<const uint8_t *>(data);
    out.insert(out.end(), begin, begin + size);
}

struct hfx_le_reader {
    const uint8_t * data;
    size_t size;
    size_t offset = 0;

    bool bytes(void * output, size_t count) {
        if (count > size - offset) return false;
        memcpy(output, data + offset, count);
        offset += count;
        return true;
    }

    template<typename T>
    bool integer(T & output) {
        static_assert(std::is_integral<T>::value, "LE values must be integral");
        using U = typename std::make_unsigned<T>::type;
        if (sizeof(T) > size - offset) return false;
        U value = 0;
        for (size_t i = 0; i < sizeof(T); ++i) value |= static_cast<U>(data[offset + i]) << (8*i);
        offset += sizeof(T);
        output = static_cast<T>(value);
        return true;
    }
};

std::vector<uint8_t> hfx_graph_encode(const hfx_graph_auth_caps_req & value, bool include_tag = true) {
    std::vector<uint8_t> out;
    out.reserve(sizeof(value));
    hfx_bytes(out, value.magic, 8);
    hfx_le<uint16_t>(out, value.major);
    hfx_le<uint16_t>(out, value.minor);
    hfx_le<uint32_t>(out, value.encoded_size);
    hfx_bytes(out, value.attempt_nonce, 32);
    if (include_tag) hfx_bytes(out, value.tag, 32); else out.resize(out.size() + 32, 0);
    return out;
}

std::vector<uint8_t> hfx_graph_encode(const hfx_graph_auth_caps_rsp & value, bool include_tag = true) {
    std::vector<uint8_t> out;
    out.reserve(sizeof(value));
    hfx_bytes(out, value.magic, 8);
    hfx_le<uint16_t>(out, value.major);
    hfx_le<uint16_t>(out, value.minor);
    hfx_le<uint32_t>(out, value.encoded_size);
    hfx_le<uint32_t>(out, value.status);
    hfx_le<uint32_t>(out, value.max_graph_bytes);
    hfx_le<uint32_t>(out, value.max_tensors);
    hfx_le<uint32_t>(out, value.max_nodes);
    hfx_bytes(out, value.attempt_nonce, 32);
    hfx_bytes(out, value.server_nonce, 32);
    if (include_tag) hfx_bytes(out, value.tag, 32); else out.resize(out.size() + 32, 0);
    return out;
}

template<typename T>
std::vector<uint8_t> hfx_graph_encode_exec(const T & value, bool include_tag = true) {
    std::vector<uint8_t> out;
    out.reserve(sizeof(value));
    hfx_bytes(out, value.magic, 8);
    hfx_le<uint16_t>(out, value.major);
    hfx_le<uint16_t>(out, value.minor);
    hfx_le<uint32_t>(out, value.encoded_size);
    hfx_le<uint32_t>(out, value.graph_size);
    hfx_le<uint32_t>(out, value.device);
    hfx_le<uint64_t>(out, value.graph_uid);
    hfx_le<uint64_t>(out, value.exec_sequence);
    hfx_bytes(out, value.attempt_nonce, 32);
    hfx_bytes(out, value.server_nonce, 32);
    hfx_bytes(out, value.graph_digest, 32);
    hfx_bytes(out, value.transcript_root, 32);
    hfx_bytes(out, value.admission_object_id, 32);
    hfx_bytes(out, value.expected_admission_digest, 32);
    if (include_tag) hfx_bytes(out, value.tag, 32); else out.resize(out.size() + 32, 0);
    return out;
}

std::vector<uint8_t> hfx_graph_encode(const hfx_graph_auth_header & value, bool include_tag = true) {
    return hfx_graph_encode_exec(value, include_tag);
}

std::vector<uint8_t> hfx_graph_encode(const hfx_graph_auth_receipt & value, bool include_tag = true) {
    std::vector<uint8_t> out;
    out.reserve(sizeof(value));
    hfx_bytes(out, value.magic, 8);
    hfx_le<uint16_t>(out, value.major);
    hfx_le<uint16_t>(out, value.minor);
    hfx_le<uint32_t>(out, value.encoded_size);
    hfx_le<uint32_t>(out, value.status);
    hfx_le<uint32_t>(out, value.device);
    hfx_le<uint64_t>(out, value.graph_uid);
    hfx_le<uint64_t>(out, value.exec_sequence);
    hfx_bytes(out, value.attempt_nonce, 32);
    hfx_bytes(out, value.server_nonce, 32);
    hfx_bytes(out, value.graph_digest, 32);
    hfx_bytes(out, value.transcript_root, 32);
    hfx_bytes(out, value.admission_object_id, 32);
    hfx_bytes(out, value.expected_admission_digest, 32);
    if (include_tag) hfx_bytes(out, value.tag, 32); else out.resize(out.size() + 32, 0);
    return out;
}

bool hfx_graph_decode(const uint8_t * data, size_t size, hfx_graph_auth_caps_req & value) {
    if (size != sizeof(value)) return false;
    hfx_le_reader in { data, size };
    return in.bytes(value.magic, 8) && in.integer(value.major) && in.integer(value.minor) &&
        in.integer(value.encoded_size) && in.bytes(value.attempt_nonce, 32) &&
        in.bytes(value.tag, 32) && in.offset == size;
}

bool hfx_graph_decode(const uint8_t * data, size_t size, hfx_graph_auth_caps_rsp & value) {
    if (size != sizeof(value)) return false;
    hfx_le_reader in { data, size };
    return in.bytes(value.magic, 8) && in.integer(value.major) && in.integer(value.minor) &&
        in.integer(value.encoded_size) && in.integer(value.status) &&
        in.integer(value.max_graph_bytes) && in.integer(value.max_tensors) &&
        in.integer(value.max_nodes) && in.bytes(value.attempt_nonce, 32) &&
        in.bytes(value.server_nonce, 32) && in.bytes(value.tag, 32) && in.offset == size;
}

bool hfx_graph_decode(const uint8_t * data, size_t size, hfx_graph_auth_header & value) {
    if (size != sizeof(value)) return false;
    hfx_le_reader in { data, size };
    return in.bytes(value.magic, 8) && in.integer(value.major) && in.integer(value.minor) &&
        in.integer(value.encoded_size) && in.integer(value.graph_size) &&
        in.integer(value.device) && in.integer(value.graph_uid) &&
        in.integer(value.exec_sequence) && in.bytes(value.attempt_nonce, 32) &&
        in.bytes(value.server_nonce, 32) && in.bytes(value.graph_digest, 32) &&
        in.bytes(value.transcript_root, 32) &&
        in.bytes(value.admission_object_id, 32) &&
        in.bytes(value.expected_admission_digest, 32) &&
        in.bytes(value.tag, 32) && in.offset == size;
}

bool hfx_graph_decode(const uint8_t * data, size_t size, hfx_graph_auth_receipt & value) {
    if (size != sizeof(value)) return false;
    hfx_le_reader in { data, size };
    return in.bytes(value.magic, 8) && in.integer(value.major) && in.integer(value.minor) &&
        in.integer(value.encoded_size) && in.integer(value.status) &&
        in.integer(value.device) && in.integer(value.graph_uid) &&
        in.integer(value.exec_sequence) && in.bytes(value.attempt_nonce, 32) &&
        in.bytes(value.server_nonce, 32) && in.bytes(value.graph_digest, 32) &&
        in.bytes(value.transcript_root, 32) &&
        in.bytes(value.admission_object_id, 32) &&
        in.bytes(value.expected_admission_digest, 32) &&
        in.bytes(value.tag, 32) && in.offset == size;
}

hfx_digest hfx_mutable_hmac(const uint8_t key[32], const void * data, size_t size) {
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
    sha256_update(&ctx, reinterpret_cast<const uint8_t *>(HFX_MUTABLE_DOMAIN), sizeof(HFX_MUTABLE_DOMAIN) - 1);
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

template<typename T>
void hfx_mutable_common(std::vector<uint8_t> & out, const T & v) {
    hfx_bytes(out, v.magic, 8);
    hfx_le<uint16_t>(out, v.major);
    hfx_le<uint16_t>(out, v.minor);
    hfx_le<uint32_t>(out, v.encoded_size);
}

static void hfx_admission_encode(
        std::vector<uint8_t> & out,
        const struct ggml_backend_sched_authority_prepared_admission & a) {
    hfx_le<uint16_t>(out, a.major); hfx_le<uint16_t>(out, a.minor);
    hfx_le<uint32_t>(out, a.encoded_size); hfx_le<uint64_t>(out, a.capabilities);
    hfx_le<uint32_t>(out, a.state); hfx_le<uint32_t>(out, a.allowed_operation);
    hfx_le<uint64_t>(out, a.key_generation);
    hfx_le<uint64_t>(out, a.scheduler_session_id);
    hfx_le<uint64_t>(out, a.scheduler_generation);
    hfx_le<uint64_t>(out, a.execution_sequence);
    hfx_le<uint64_t>(out, a.parent_graph_uid);
    hfx_le<uint64_t>(out, a.client_connection_epoch);
    hfx_le<uint64_t>(out, a.server_connection_epoch);
    hfx_le<uint64_t>(out, a.allocation_topology_epoch);
    hfx_le<uint32_t>(out, a.split_count); hfx_le<uint32_t>(out, a.backend_ordinal);
    hfx_le<uint32_t>(out, a.logical_expected_mutable_count);
    hfx_le<uint32_t>(out, a.logical_expected_exclusion_count);
    hfx_le<uint64_t>(out, a.issued_unix_ns); hfx_le<uint64_t>(out, a.expires_unix_ns);
    hfx_bytes(out, a.attempt_nonce, 32); hfx_bytes(out, a.object_id, 32);
    hfx_bytes(out, a.prepared_graph_digest, 32);
    hfx_bytes(out, a.prepared_root, 32);
    hfx_bytes(out, a.split_mapping_root, 32);
    hfx_bytes(out, a.scheduler_chain_root, 32);
    hfx_bytes(out, a.logical_expected_census_root, 32);
    for (const auto & split : a.ordered_splits) {
        hfx_le<uint64_t>(out, split.split_graph_uid);
        hfx_le<uint32_t>(out, split.split_ordinal);
        hfx_le<uint32_t>(out, split.backend_ordinal);
    }
    hfx_bytes(out, a.tag, 32);
}

std::vector<uint8_t> hfx_mutable_encode(const hfx_mutable_caps & v, bool tag = true) {
    std::vector<uint8_t> out; out.reserve(sizeof(v)); hfx_mutable_common(out, v);
    hfx_le<uint32_t>(out, v.status); hfx_le<uint32_t>(out, v.max_mutations);
    hfx_le<uint32_t>(out, v.max_census); hfx_le<uint32_t>(out, v.reserved);
    hfx_le<uint64_t>(out, v.capabilities);
    hfx_le<uint16_t>(out, v.graph_auth_major);
    hfx_le<uint16_t>(out, v.scheduler_admission_major);
    hfx_le<uint16_t>(out, v.mutable_major);
    hfx_le<uint16_t>(out, v.version_reserved);
    hfx_le<uint64_t>(out, v.graph_uid); hfx_le<uint64_t>(out, v.exec_sequence);
    hfx_le<uint64_t>(out, v.scheduler_sequence);
    hfx_le<uint64_t>(out, v.client_connection_epoch);
    hfx_le<uint64_t>(out, v.server_connection_epoch);
    hfx_le<uint64_t>(out, v.allocation_epoch);
    hfx_bytes(out, v.attempt_nonce, 32); hfx_bytes(out, v.scheduler_nonce, 32);
    hfx_bytes(out, v.scheduler_root, 32);
    hfx_bytes(out, v.expected_admission_digest, 32);
    hfx_admission_encode(out, v.scheduler_expected_admission);
    hfx_admission_encode(out, v.scheduler_admission);
    hfx_bytes(out, v.server_nonce, 32);
    if (tag) hfx_bytes(out, v.tag, 32); else out.resize(out.size() + 32, 0);
    return out;
}

std::vector<uint8_t> hfx_mutable_encode(const hfx_mutable_update_header & v, bool tag = true) {
    std::vector<uint8_t> out; out.reserve(sizeof(v)); hfx_mutable_common(out, v);
    hfx_le<uint32_t>(out, v.role); hfx_le<uint32_t>(out, v.role_ordinal);
    hfx_le<uint32_t>(out, v.allocation_ordinal); hfx_le<uint32_t>(out, v.type);
    hfx_le<uint64_t>(out, v.graph_uid); hfx_le<uint64_t>(out, v.exec_sequence);
    hfx_le<uint64_t>(out, v.mutation_sequence); hfx_le<uint64_t>(out, v.tensor_relative);
    hfx_le<uint64_t>(out, v.logical_offset); hfx_le<uint64_t>(out, v.logical_size);
    hfx_le<uint64_t>(out, v.cache_hash);
    for (uint32_t x : v.ne) hfx_le<uint32_t>(out, x);
    for (uint32_t x : v.nb) hfx_le<uint32_t>(out, x);
    hfx_bytes(out, v.attempt_nonce, 32); hfx_bytes(out, v.server_nonce, 32);
    hfx_bytes(out, v.content_digest, 32); hfx_bytes(out, v.view_digest, 32);
    hfx_bytes(out, v.prior_root, 32);
    if (tag) hfx_bytes(out, v.tag, 32); else out.resize(out.size() + 32, 0);
    return out;
}

std::vector<uint8_t> hfx_mutable_encode(const hfx_mutable_receipt & v, bool tag = true) {
    std::vector<uint8_t> out; out.reserve(sizeof(v)); hfx_mutable_common(out, v);
    hfx_le<uint32_t>(out, v.status); hfx_le<uint32_t>(out, v.role);
    hfx_le<uint32_t>(out, v.role_ordinal); hfx_le<uint32_t>(out, v.allocation_ordinal);
    hfx_le<uint64_t>(out, v.graph_uid); hfx_le<uint64_t>(out, v.exec_sequence);
    hfx_le<uint64_t>(out, v.mutation_sequence); hfx_le<uint64_t>(out, v.tensor_relative);
    hfx_le<uint64_t>(out, v.logical_offset); hfx_le<uint64_t>(out, v.logical_size);
    hfx_bytes(out, v.attempt_nonce, 32); hfx_bytes(out, v.server_nonce, 32);
    hfx_bytes(out, v.content_digest, 32); hfx_bytes(out, v.view_digest, 32);
    hfx_bytes(out, v.mutation_root, 32);
    if (tag) hfx_bytes(out, v.tag, 32); else out.resize(out.size() + 32, 0);
    return out;
}

std::vector<uint8_t> hfx_mutable_encode(const hfx_mutable_census_entry & v) {
    std::vector<uint8_t> out; out.reserve(sizeof(v));
    hfx_le<uint32_t>(out, v.role); hfx_le<uint32_t>(out, v.role_ordinal);
    hfx_le<uint32_t>(out, v.allocation_ordinal); hfx_le<uint32_t>(out, v.type);
    hfx_le<uint64_t>(out, v.tensor_relative); hfx_le<uint64_t>(out, v.logical_size);
    for (uint32_t x : v.ne) hfx_le<uint32_t>(out, x);
    for (uint32_t x : v.nb) hfx_le<uint32_t>(out, x);
    hfx_bytes(out, v.view_digest, 32);
    hfx_bytes(out, v.logical_tensor_identity, 32);
    hfx_bytes(out, v.storage_tensor_identity, 32);
    hfx_bytes(out, v.runtime_semantic_identity, 32);
    hfx_bytes(out, v.rpc_endpoint_identity, 32);
    hfx_le<uint64_t>(out, v.rpc_connection_epoch);
    hfx_le<uint32_t>(out, v.destination_backend_ordinal);
    hfx_le<uint32_t>(out, v.rpc_device);
    return out;
}

std::vector<uint8_t> hfx_mutable_encode(const hfx_mutable_commit_header & v, bool tag = true) {
    std::vector<uint8_t> out; out.reserve(sizeof(v)); hfx_mutable_common(out, v);
    hfx_le<uint32_t>(out, v.census_count); hfx_le<uint32_t>(out, v.mutation_count);
    hfx_le<uint64_t>(out, v.graph_uid); hfx_le<uint64_t>(out, v.exec_sequence);
    hfx_le<uint64_t>(out, v.parent_graph_uid);
    hfx_le<uint32_t>(out, v.split_ordinal); hfx_le<uint32_t>(out, v.backend_ordinal);
    hfx_bytes(out, v.attempt_nonce, 32); hfx_bytes(out, v.server_nonce, 32);
    hfx_bytes(out, v.scheduler_nonce, 32); hfx_bytes(out, v.scheduler_root, 32);
    hfx_bytes(out, v.mutation_root, 32); hfx_bytes(out, v.census_root, 32);
    hfx_bytes(out, v.split_mapping_root, 32);
    if (tag) hfx_bytes(out, v.tag, 32); else out.resize(out.size() + 32, 0);
    return out;
}

std::vector<uint8_t> hfx_mutable_encode(
        const hfx_mutable_preflight_wire & v, bool tag);

template<typename T>
bool hfx_mutable_sign(T & v, const uint8_t key[32]) {
    memset(v.tag, 0, 32);
    const auto wire = hfx_mutable_encode(v, false);
    if (wire.size() != sizeof(v)) return false;
    const auto mac = hfx_mutable_hmac(key, wire.data(), wire.size());
    memcpy(v.tag, mac.data(), 32);
    return true;
}

bool hfx_equal(const uint8_t * a, const uint8_t * b, size_t n);

template<typename T>
bool hfx_mutable_verify(const T & v, const uint8_t key[32]) {
    const auto wire = hfx_mutable_encode(v, false);
    if (wire.size() != sizeof(v)) return false;
    const auto mac = hfx_mutable_hmac(key, wire.data(), wire.size());
    return hfx_equal(mac.data(), v.tag, 32);
}

bool hfx_mutable_decode(const uint8_t * data, size_t size, hfx_mutable_caps & v) {
    if (size != sizeof(v)) return false;
    hfx_le_reader in { data, size };
    const auto read_admission = [&in](
            struct ggml_backend_sched_authority_prepared_admission & admission) {
        if (!(in.integer(admission.major) && in.integer(admission.minor) &&
              in.integer(admission.encoded_size) &&
              in.integer(admission.capabilities) &&
              in.integer(admission.state) &&
              in.integer(admission.allowed_operation) &&
              in.integer(admission.key_generation) &&
              in.integer(admission.scheduler_session_id) &&
              in.integer(admission.scheduler_generation) &&
              in.integer(admission.execution_sequence) &&
              in.integer(admission.parent_graph_uid) &&
              in.integer(admission.client_connection_epoch) &&
              in.integer(admission.server_connection_epoch) &&
              in.integer(admission.allocation_topology_epoch) &&
              in.integer(admission.split_count) &&
              in.integer(admission.backend_ordinal) &&
              in.integer(admission.logical_expected_mutable_count) &&
              in.integer(admission.logical_expected_exclusion_count) &&
              in.integer(admission.issued_unix_ns) &&
              in.integer(admission.expires_unix_ns) &&
              in.bytes(admission.attempt_nonce, 32) &&
              in.bytes(admission.object_id, 32) &&
              in.bytes(admission.prepared_graph_digest, 32) &&
              in.bytes(admission.prepared_root, 32) &&
              in.bytes(admission.split_mapping_root, 32) &&
              in.bytes(admission.scheduler_chain_root, 32) &&
              in.bytes(admission.logical_expected_census_root, 32))) {
            return false;
        }
        for (auto & split : admission.ordered_splits) {
            if (!in.integer(split.split_graph_uid) ||
                !in.integer(split.split_ordinal) ||
                !in.integer(split.backend_ordinal)) return false;
        }
        return in.bytes(admission.tag, 32);
    };
    if (!(in.bytes(v.magic, 8) && in.integer(v.major) && in.integer(v.minor) &&
        in.integer(v.encoded_size) && in.integer(v.status) && in.integer(v.max_mutations) &&
        in.integer(v.max_census) && in.integer(v.reserved) &&
        in.integer(v.capabilities) && in.integer(v.graph_auth_major) &&
        in.integer(v.scheduler_admission_major) && in.integer(v.mutable_major) &&
        in.integer(v.version_reserved) && in.integer(v.graph_uid) &&
        in.integer(v.exec_sequence) && in.integer(v.scheduler_sequence) &&
        in.integer(v.client_connection_epoch) && in.integer(v.server_connection_epoch) &&
        in.integer(v.allocation_epoch) &&
        in.bytes(v.attempt_nonce, 32) && in.bytes(v.scheduler_nonce, 32) &&
        in.bytes(v.scheduler_root, 32) &&
        in.bytes(v.expected_admission_digest, 32) &&
        read_admission(v.scheduler_expected_admission) &&
        read_admission(v.scheduler_admission))) {
        return false;
    }
    return in.bytes(v.server_nonce, 32) &&
        in.bytes(v.tag, 32) && in.offset == size;
}

bool hfx_mutable_decode(
        const uint8_t * data, size_t size, hfx_mutable_preflight_wire & v) {
    if (size != sizeof(v)) return false;
    hfx_le_reader in { data, size };
    return in.bytes(v.magic, 8) && in.integer(v.major) && in.integer(v.minor) &&
        in.integer(v.encoded_size) && in.integer(v.status) &&
        in.integer(v.reserved) && in.integer(v.capabilities) &&
        in.integer(v.key_generation) && in.integer(v.client_connection_epoch) &&
        in.integer(v.server_connection_epoch) &&
        in.integer(v.allocation_topology_epoch) &&
        in.bytes(v.client_nonce, 32) && in.bytes(v.server_nonce, 32) &&
        in.bytes(v.tag, 32) && in.offset == size;
}

bool hfx_mutable_decode(const uint8_t * data, size_t size, hfx_mutable_update_header & v) {
    if (size != sizeof(v)) return false;
    hfx_le_reader in { data, size };
    if (!(in.bytes(v.magic, 8) && in.integer(v.major) && in.integer(v.minor) &&
          in.integer(v.encoded_size) && in.integer(v.role) && in.integer(v.role_ordinal) &&
          in.integer(v.allocation_ordinal) && in.integer(v.type) && in.integer(v.graph_uid) &&
          in.integer(v.exec_sequence) && in.integer(v.mutation_sequence) &&
          in.integer(v.tensor_relative) && in.integer(v.logical_offset) &&
          in.integer(v.logical_size) && in.integer(v.cache_hash))) return false;
    for (uint32_t & x : v.ne) if (!in.integer(x)) return false;
    for (uint32_t & x : v.nb) if (!in.integer(x)) return false;
    return in.bytes(v.attempt_nonce, 32) && in.bytes(v.server_nonce, 32) &&
        in.bytes(v.content_digest, 32) && in.bytes(v.view_digest, 32) &&
        in.bytes(v.prior_root, 32) &&
        in.bytes(v.tag, 32) && in.offset == size;
}

bool hfx_mutable_decode(const uint8_t * data, size_t size, hfx_mutable_receipt & v) {
    if (size != sizeof(v)) return false;
    hfx_le_reader in { data, size };
    return in.bytes(v.magic, 8) && in.integer(v.major) && in.integer(v.minor) &&
        in.integer(v.encoded_size) && in.integer(v.status) && in.integer(v.role) &&
        in.integer(v.role_ordinal) && in.integer(v.allocation_ordinal) &&
        in.integer(v.graph_uid) && in.integer(v.exec_sequence) &&
        in.integer(v.mutation_sequence) && in.integer(v.tensor_relative) &&
        in.integer(v.logical_offset) && in.integer(v.logical_size) &&
        in.bytes(v.attempt_nonce, 32) && in.bytes(v.server_nonce, 32) &&
        in.bytes(v.content_digest, 32) && in.bytes(v.view_digest, 32) &&
        in.bytes(v.mutation_root, 32) &&
        in.bytes(v.tag, 32) && in.offset == size;
}

bool hfx_mutable_decode(const uint8_t * data, size_t size, hfx_mutable_census_entry & v) {
    if (size != sizeof(v)) return false;
    hfx_le_reader in { data, size };
    if (!(in.integer(v.role) && in.integer(v.role_ordinal) &&
          in.integer(v.allocation_ordinal) && in.integer(v.type) &&
          in.integer(v.tensor_relative) && in.integer(v.logical_size))) return false;
    for (uint32_t & x : v.ne) if (!in.integer(x)) return false;
    for (uint32_t & x : v.nb) if (!in.integer(x)) return false;
    return in.bytes(v.view_digest, 32) &&
        in.bytes(v.logical_tensor_identity, 32) &&
        in.bytes(v.storage_tensor_identity, 32) &&
        in.bytes(v.runtime_semantic_identity, 32) &&
        in.bytes(v.rpc_endpoint_identity, 32) &&
        in.integer(v.rpc_connection_epoch) &&
        in.integer(v.destination_backend_ordinal) &&
        in.integer(v.rpc_device) && in.offset == size;
}

bool hfx_mutable_decode(const uint8_t * data, size_t size, hfx_mutable_commit_header & v) {
    if (size != sizeof(v)) return false;
    hfx_le_reader in { data, size };
    return in.bytes(v.magic, 8) && in.integer(v.major) && in.integer(v.minor) &&
        in.integer(v.encoded_size) && in.integer(v.census_count) &&
        in.integer(v.mutation_count) && in.integer(v.graph_uid) &&
        in.integer(v.exec_sequence) && in.integer(v.parent_graph_uid) &&
        in.integer(v.split_ordinal) && in.integer(v.backend_ordinal) &&
        in.bytes(v.attempt_nonce, 32) &&
        in.bytes(v.server_nonce, 32) && in.bytes(v.scheduler_nonce, 32) &&
        in.bytes(v.scheduler_root, 32) && in.bytes(v.mutation_root, 32) &&
        in.bytes(v.census_root, 32) && in.bytes(v.split_mapping_root, 32) &&
        in.bytes(v.tag, 32) && in.offset == size;
}

template<typename T>
bool hfx_graph_sign_record(T & value, const uint8_t key[32]) {
    memset(value.tag, 0, sizeof(value.tag));
    const auto encoded = hfx_graph_encode(value, false);
    if (encoded.size() != sizeof(value)) return false;
    const auto tag = hfx_graph_hmac(key, encoded.data(), encoded.size());
    memcpy(value.tag, tag.data(), tag.size());
    return true;
}

template<typename T>
bool hfx_graph_verify_record(const T & value, const uint8_t key[32]) {
    const auto encoded = hfx_graph_encode(value, false);
    if (encoded.size() != sizeof(value)) return false;
    const auto expected = hfx_graph_hmac(key, encoded.data(), encoded.size());
    uint8_t difference = 0;
    for (size_t i = 0; i < expected.size(); ++i) difference |= value.tag[i] ^ expected[i];
    return difference == 0;
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

bool hfx_write_all(int fd, const void * data, size_t size);
hfx_digest hfx_preexecute_hmac(const uint8_t key[32], const void * data, size_t size) {
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
    sha256_update(&ctx, reinterpret_cast<const uint8_t *>(HFX_PREEXECUTE_DOMAIN),
                  sizeof(HFX_PREEXECUTE_DOMAIN) - 1);
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

static hfx_digest hfx_mutable_logical_expected_census(
        const std::vector<hfx_mutable_census_entry> & census,
        uint32_t & mutable_count,
        uint32_t & exclusion_count) {
    std::vector<std::pair<uint32_t, uint32_t>> mutable_roles;
    std::vector<std::pair<uint32_t, uint32_t>> excluded_roles;
    for (const auto & entry : census) {
        auto & target = entry.role >= HFX_MUTABLE_EXCLUSION_BASE ?
            excluded_roles : mutable_roles;
        target.emplace_back(entry.role, entry.role_ordinal);
    }
    std::sort(mutable_roles.begin(), mutable_roles.end());
    std::sort(excluded_roles.begin(), excluded_roles.end());
    mutable_count = static_cast<uint32_t>(mutable_roles.size());
    exclusion_count = static_cast<uint32_t>(excluded_roles.size());
    std::vector<uint8_t> canonical;
    hfx_le<uint32_t>(canonical, mutable_count);
    hfx_le<uint32_t>(canonical, exclusion_count);
    for (const auto & value : mutable_roles) {
        hfx_le<uint32_t>(canonical, value.first);
        hfx_le<uint32_t>(canonical, value.second);
    }
    for (const auto & value : excluded_roles) {
        hfx_le<uint32_t>(canonical, value.first);
        hfx_le<uint32_t>(canonical, value.second);
    }
    for (const auto & entry : census) {
        const bool excluded = entry.role >= HFX_MUTABLE_EXCLUSION_BASE;
        hfx_bytes(canonical, entry.logical_tensor_identity, 32);
        hfx_bytes(canonical, entry.storage_tensor_identity, 32);
        hfx_bytes(canonical, entry.runtime_semantic_identity, 32);
        hfx_le<uint32_t>(
            canonical, excluded ? GGML_BACKEND_SCHED_CENSUS_EXCLUDE :
                                  GGML_BACKEND_SCHED_CENSUS_REGISTER);
        hfx_le<uint32_t>(
            canonical, excluded ? entry.role - HFX_MUTABLE_EXCLUSION_BASE :
                                  entry.role);
        hfx_le<uint32_t>(canonical, entry.role_ordinal);
        hfx_le<uint32_t>(canonical, entry.destination_backend_ordinal);
        hfx_bytes(canonical, entry.rpc_endpoint_identity, 32);
        hfx_le<uint32_t>(canonical, entry.rpc_device);
        hfx_le<uint64_t>(canonical, entry.rpc_connection_epoch);
    }
    return hfx_sha256(canonical.data(), canonical.size());
}

std::vector<uint8_t> hfx_mutable_encode(
        const hfx_mutable_preflight_wire & v, bool tag = true) {
    std::vector<uint8_t> out; out.reserve(sizeof(v)); hfx_mutable_common(out, v);
    hfx_le<uint32_t>(out, v.status); hfx_le<uint32_t>(out, v.reserved);
    hfx_le<uint64_t>(out, v.capabilities); hfx_le<uint64_t>(out, v.key_generation);
    hfx_le<uint64_t>(out, v.client_connection_epoch);
    hfx_le<uint64_t>(out, v.server_connection_epoch);
    hfx_le<uint64_t>(out, v.allocation_topology_epoch);
    hfx_bytes(out, v.client_nonce, 32); hfx_bytes(out, v.server_nonce, 32);
    if (tag) hfx_bytes(out, v.tag, 32); else out.resize(out.size() + 32, 0);
    return out;
}

enum hfx_preexecute_event_type : uint32_t {
    HFX_PREEXEC_BEGIN = 1,
    HFX_PREEXEC_L42_PREPARED,
    HFX_PREEXEC_L44_BEGIN,
    HFX_PREEXEC_L44_REGISTER,
    HFX_PREEXEC_L44_EXCLUDE,
    HFX_PREEXEC_L44_PREPARE,
    HFX_PREEXEC_L44_COMMIT,
    HFX_PREEXEC_GRAPH_DECISION,
    HFX_PREEXEC_TRANSPORT,
    HFX_PREEXEC_L44_ABORT,
    HFX_PREEXEC_END,
};

enum hfx_preexecute_reason : uint32_t {
    HFX_PREEXEC_OK = 0,
    HFX_PREEXEC_INVALID_ARGUMENT,
    HFX_PREEXEC_NOT_ARMED,
    HFX_PREEXEC_PREPARED_MISSING,
    HFX_PREEXEC_PREPARED_MISMATCH,
    HFX_PREEXEC_SOCKET_MISSING,
    HFX_PREEXEC_CAPS_REFUSED,
    HFX_PREEXEC_SESSION_CONFLICT,
    HFX_PREEXEC_STALE_HANDLE,
    HFX_PREEXEC_WRONG_CONNECTION,
    HFX_PREEXEC_WRONG_ALLOCATION_EPOCH,
    HFX_PREEXEC_UNKNOWN_ROLE,
    HFX_PREEXEC_DUPLICATE_ROLE,
    HFX_PREEXEC_GRAPH_UNBOUND,
    HFX_PREEXEC_SPLIT_MISSING,
    HFX_PREEXEC_SESSION_MISSING,
    HFX_PREEXEC_SESSION_UNPREPARED,
    HFX_PREEXEC_BIND_REFUSED,
    HFX_PREEXEC_SERIALIZE_FAILED,
    HFX_PREEXEC_SEND_ERROR,
    HFX_PREEXEC_RECEIVE_EOF,
    HFX_PREEXEC_RECEIVE_ERROR,
    HFX_PREEXEC_SIZE_MISMATCH,
    HFX_PREEXEC_DECODE_REFUSED,
    HFX_PREEXEC_RECEIPT_REFUSED,
    HFX_PREEXEC_DUPLICATE_RESULT,
    HFX_PREEXEC_POST_ABORT,
};

enum hfx_preexecute_transport_state : uint32_t {
    HFX_TRANSPORT_NOT_ATTEMPTED = 0,
    HFX_TRANSPORT_SERIALIZE_FAILED,
    HFX_TRANSPORT_SEND_OPCODE_STARTED,
    HFX_TRANSPORT_SEND_OPCODE_COMPLETE,
    HFX_TRANSPORT_SEND_HEADER_STARTED,
    HFX_TRANSPORT_SEND_HEADER_COMPLETE,
    HFX_TRANSPORT_SEND_BODY_STARTED,
    HFX_TRANSPORT_SEND_BODY_COMPLETE,
    HFX_TRANSPORT_RECV_HEADER_STARTED,
    HFX_TRANSPORT_RECV_HEADER_COMPLETE,
    HFX_TRANSPORT_RECV_BODY_STARTED,
    HFX_TRANSPORT_RECV_BODY_COMPLETE,
    HFX_TRANSPORT_EOF,
    HFX_TRANSPORT_SYSCALL_ERROR,
    HFX_TRANSPORT_DECODE_REFUSED,
};

static bool hfx_preexecute_transport_transition(uint32_t prior, uint32_t next) {
    switch (next) {
        case HFX_TRANSPORT_SERIALIZE_FAILED:
            return prior == HFX_TRANSPORT_NOT_ATTEMPTED;
        case HFX_TRANSPORT_SEND_OPCODE_STARTED:
            return prior == HFX_TRANSPORT_NOT_ATTEMPTED ||
                   prior == HFX_TRANSPORT_RECV_BODY_COMPLETE ||
                   prior == HFX_TRANSPORT_DECODE_REFUSED;
        case HFX_TRANSPORT_SEND_OPCODE_COMPLETE:
            return prior == HFX_TRANSPORT_SEND_OPCODE_STARTED;
        case HFX_TRANSPORT_SEND_HEADER_STARTED:
            return prior == HFX_TRANSPORT_SEND_OPCODE_COMPLETE;
        case HFX_TRANSPORT_SEND_HEADER_COMPLETE:
            return prior == HFX_TRANSPORT_SEND_HEADER_STARTED;
        case HFX_TRANSPORT_SEND_BODY_STARTED:
            return prior == HFX_TRANSPORT_SEND_HEADER_COMPLETE;
        case HFX_TRANSPORT_SEND_BODY_COMPLETE:
            return prior == HFX_TRANSPORT_SEND_BODY_STARTED;
        case HFX_TRANSPORT_RECV_HEADER_STARTED:
            return prior == HFX_TRANSPORT_SEND_BODY_COMPLETE;
        case HFX_TRANSPORT_RECV_HEADER_COMPLETE:
            return prior == HFX_TRANSPORT_RECV_HEADER_STARTED;
        case HFX_TRANSPORT_RECV_BODY_STARTED:
            return prior == HFX_TRANSPORT_RECV_HEADER_COMPLETE;
        case HFX_TRANSPORT_RECV_BODY_COMPLETE:
            return prior == HFX_TRANSPORT_RECV_BODY_STARTED;
        case HFX_TRANSPORT_EOF:
        case HFX_TRANSPORT_SYSCALL_ERROR:
            return prior == HFX_TRANSPORT_SEND_OPCODE_STARTED ||
                   prior == HFX_TRANSPORT_SEND_HEADER_STARTED ||
                   prior == HFX_TRANSPORT_SEND_BODY_STARTED ||
                   prior == HFX_TRANSPORT_RECV_HEADER_STARTED ||
                   prior == HFX_TRANSPORT_RECV_BODY_STARTED;
        case HFX_TRANSPORT_DECODE_REFUSED:
            return prior == HFX_TRANSPORT_RECV_HEADER_COMPLETE ||
                   prior == HFX_TRANSPORT_RECV_BODY_COMPLETE;
        default:
            return false;
    }
}

enum hfx_preexecute_grammar_token : uint8_t {
    HFX_GRAMMAR_BEGIN,
    HFX_GRAMMAR_L42,
    HFX_GRAMMAR_L44_BEGIN,
    HFX_GRAMMAR_REGISTER_PLAN,
    HFX_GRAMMAR_EXCLUDE_PLAN,
    HFX_GRAMMAR_PREPARE,
    HFX_GRAMMAR_COMMIT,
    HFX_GRAMMAR_GRAPH_DECISION,
    HFX_GRAMMAR_TRANSPORT_COMPLETE,
    HFX_GRAMMAR_TRANSPORT_FAIL_2,
    HFX_GRAMMAR_TRANSPORT_FAIL_4,
    HFX_GRAMMAR_TRANSPORT_FAIL_6,
    HFX_GRAMMAR_TRANSPORT_FAIL_8,
    HFX_GRAMMAR_TRANSPORT_FAIL_10,
    HFX_GRAMMAR_TRANSPORT_FAIL_12,
    HFX_GRAMMAR_TRANSPORT_FAIL_14,
    HFX_GRAMMAR_TRANSPORT_FAIL_16,
    HFX_GRAMMAR_TRANSPORT_FAIL_18,
    HFX_GRAMMAR_ABORT,
    HFX_GRAMMAR_END,
};

struct hfx_preexecute_production {
    uint16_t grammar_major;
    uint16_t grammar_minor;
    uint32_t terminal_branch;
    std::array<hfx_preexecute_grammar_token, 12> tokens;
    uint8_t token_count;
};

#define HFX_PRODUCTION(branch, ...) \
    hfx_preexecute_production { 1, 0, branch, { __VA_ARGS__ }, \
        static_cast<uint8_t>(std::initializer_list<hfx_preexecute_grammar_token>{__VA_ARGS__}.size()) }

// Exact, finite, versioned terminal productions. REGISTER_PLAN and
// EXCLUDE_PLAN consume exactly the scheduler-sealed cardinalities. Transport
// tokens consume exactly 10 complete stages or the named exact failing count.
static const std::array<hfx_preexecute_production, 14> HFX_PREEXECUTE_PRODUCTIONS {{
    HFX_PRODUCTION(1, HFX_GRAMMAR_BEGIN, HFX_GRAMMAR_L42, HFX_GRAMMAR_L44_BEGIN,
        HFX_GRAMMAR_REGISTER_PLAN, HFX_GRAMMAR_EXCLUDE_PLAN, HFX_GRAMMAR_PREPARE,
        HFX_GRAMMAR_GRAPH_DECISION, HFX_GRAMMAR_TRANSPORT_COMPLETE,
        HFX_GRAMMAR_TRANSPORT_COMPLETE, HFX_GRAMMAR_COMMIT, HFX_GRAMMAR_END),
    HFX_PRODUCTION(2, HFX_GRAMMAR_BEGIN, HFX_GRAMMAR_L42, HFX_GRAMMAR_L44_BEGIN,
        HFX_GRAMMAR_ABORT),
    HFX_PRODUCTION(5, HFX_GRAMMAR_BEGIN, HFX_GRAMMAR_L42, HFX_GRAMMAR_L44_BEGIN,
        HFX_GRAMMAR_REGISTER_PLAN, HFX_GRAMMAR_EXCLUDE_PLAN,
        HFX_GRAMMAR_ABORT),
    HFX_PRODUCTION(4, HFX_GRAMMAR_BEGIN, HFX_GRAMMAR_L42, HFX_GRAMMAR_L44_BEGIN,
        HFX_GRAMMAR_REGISTER_PLAN, HFX_GRAMMAR_EXCLUDE_PLAN, HFX_GRAMMAR_PREPARE,
        HFX_GRAMMAR_ABORT),
    HFX_PRODUCTION(4, HFX_GRAMMAR_BEGIN, HFX_GRAMMAR_L42, HFX_GRAMMAR_L44_BEGIN,
        HFX_GRAMMAR_REGISTER_PLAN, HFX_GRAMMAR_EXCLUDE_PLAN, HFX_GRAMMAR_PREPARE,
        HFX_GRAMMAR_COMMIT, HFX_GRAMMAR_ABORT),
    HFX_PRODUCTION(3, HFX_GRAMMAR_BEGIN, HFX_GRAMMAR_L42, HFX_GRAMMAR_L44_BEGIN,
        HFX_GRAMMAR_REGISTER_PLAN, HFX_GRAMMAR_EXCLUDE_PLAN, HFX_GRAMMAR_PREPARE,
        HFX_GRAMMAR_GRAPH_DECISION, HFX_GRAMMAR_TRANSPORT_FAIL_2, HFX_GRAMMAR_ABORT),
    HFX_PRODUCTION(3, HFX_GRAMMAR_BEGIN, HFX_GRAMMAR_L42, HFX_GRAMMAR_L44_BEGIN,
        HFX_GRAMMAR_REGISTER_PLAN, HFX_GRAMMAR_EXCLUDE_PLAN, HFX_GRAMMAR_PREPARE,
        HFX_GRAMMAR_GRAPH_DECISION, HFX_GRAMMAR_TRANSPORT_FAIL_4, HFX_GRAMMAR_ABORT),
    HFX_PRODUCTION(3, HFX_GRAMMAR_BEGIN, HFX_GRAMMAR_L42, HFX_GRAMMAR_L44_BEGIN,
        HFX_GRAMMAR_REGISTER_PLAN, HFX_GRAMMAR_EXCLUDE_PLAN, HFX_GRAMMAR_PREPARE,
        HFX_GRAMMAR_GRAPH_DECISION, HFX_GRAMMAR_TRANSPORT_FAIL_6, HFX_GRAMMAR_ABORT),
    HFX_PRODUCTION(3, HFX_GRAMMAR_BEGIN, HFX_GRAMMAR_L42, HFX_GRAMMAR_L44_BEGIN,
        HFX_GRAMMAR_REGISTER_PLAN, HFX_GRAMMAR_EXCLUDE_PLAN, HFX_GRAMMAR_PREPARE,
        HFX_GRAMMAR_GRAPH_DECISION, HFX_GRAMMAR_TRANSPORT_FAIL_8, HFX_GRAMMAR_ABORT),
    HFX_PRODUCTION(3, HFX_GRAMMAR_BEGIN, HFX_GRAMMAR_L42, HFX_GRAMMAR_L44_BEGIN,
        HFX_GRAMMAR_REGISTER_PLAN, HFX_GRAMMAR_EXCLUDE_PLAN, HFX_GRAMMAR_PREPARE,
        HFX_GRAMMAR_GRAPH_DECISION, HFX_GRAMMAR_TRANSPORT_FAIL_10, HFX_GRAMMAR_ABORT),
    HFX_PRODUCTION(3, HFX_GRAMMAR_BEGIN, HFX_GRAMMAR_L42, HFX_GRAMMAR_L44_BEGIN,
        HFX_GRAMMAR_REGISTER_PLAN, HFX_GRAMMAR_EXCLUDE_PLAN, HFX_GRAMMAR_PREPARE,
        HFX_GRAMMAR_GRAPH_DECISION, HFX_GRAMMAR_TRANSPORT_FAIL_12, HFX_GRAMMAR_ABORT),
    HFX_PRODUCTION(3, HFX_GRAMMAR_BEGIN, HFX_GRAMMAR_L42, HFX_GRAMMAR_L44_BEGIN,
        HFX_GRAMMAR_REGISTER_PLAN, HFX_GRAMMAR_EXCLUDE_PLAN, HFX_GRAMMAR_PREPARE,
        HFX_GRAMMAR_GRAPH_DECISION, HFX_GRAMMAR_TRANSPORT_FAIL_14, HFX_GRAMMAR_ABORT),
    HFX_PRODUCTION(3, HFX_GRAMMAR_BEGIN, HFX_GRAMMAR_L42, HFX_GRAMMAR_L44_BEGIN,
        HFX_GRAMMAR_REGISTER_PLAN, HFX_GRAMMAR_EXCLUDE_PLAN, HFX_GRAMMAR_PREPARE,
        HFX_GRAMMAR_GRAPH_DECISION, HFX_GRAMMAR_TRANSPORT_FAIL_16, HFX_GRAMMAR_ABORT),
    HFX_PRODUCTION(3, HFX_GRAMMAR_BEGIN, HFX_GRAMMAR_L42, HFX_GRAMMAR_L44_BEGIN,
        HFX_GRAMMAR_REGISTER_PLAN, HFX_GRAMMAR_EXCLUDE_PLAN, HFX_GRAMMAR_PREPARE,
        HFX_GRAMMAR_GRAPH_DECISION, HFX_GRAMMAR_TRANSPORT_FAIL_18, HFX_GRAMMAR_ABORT),
}};
#undef HFX_PRODUCTION

static bool hfx_preexecute_production_matches(
        const hfx_preexecute_production & production,
        const std::vector<uint32_t> & events,
        const std::vector<uint32_t> & transport_states,
        uint32_t expected_register,
        uint32_t expected_exclude) {
    size_t event_index = 0;
    size_t transport_index = 0;
    const auto consume = [&](uint32_t event, size_t count,
                             size_t & index) -> bool {
        if (count > events.size() - index) return false;
        for (size_t i = 0; i < count; ++i) {
            if (events[index + i] != event) return false;
        }
        index += count;
        return true;
    };
    static constexpr std::array<std::array<uint32_t, 18>, 9> failure_sequences {{
        {{ 2, 13 }},
        {{ 2, 3, 4, 13 }},
        {{ 2, 3, 4, 5, 6, 13 }},
        {{ 2, 3, 4, 5, 6, 7, 8, 13 }},
        {{ 2, 3, 4, 5, 6, 7, 8, 9, 10, 13 }},
        {{ 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 2, 13 }},
        {{ 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 2, 3, 4, 13 }},
        {{ 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 2, 3, 4, 5, 6, 13 }},
        {{ 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 2, 3, 4, 5, 6, 7, 8, 13 }},
    }};
    for (size_t i = 0; i < production.token_count; ++i) {
        const auto token = production.tokens[i];
        size_t count = 1;
        uint32_t event = 0;
        switch (token) {
            case HFX_GRAMMAR_BEGIN:          event = HFX_PREEXEC_BEGIN; break;
            case HFX_GRAMMAR_L42:            event = HFX_PREEXEC_L42_PREPARED; break;
            case HFX_GRAMMAR_L44_BEGIN:      event = HFX_PREEXEC_L44_BEGIN; break;
            case HFX_GRAMMAR_REGISTER_PLAN:  event = HFX_PREEXEC_L44_REGISTER; count = expected_register; break;
            case HFX_GRAMMAR_EXCLUDE_PLAN:   event = HFX_PREEXEC_L44_EXCLUDE; count = expected_exclude; break;
            case HFX_GRAMMAR_PREPARE:        event = HFX_PREEXEC_L44_PREPARE; break;
            case HFX_GRAMMAR_COMMIT:         event = HFX_PREEXEC_L44_COMMIT; break;
            case HFX_GRAMMAR_GRAPH_DECISION: event = HFX_PREEXEC_GRAPH_DECISION; break;
            case HFX_GRAMMAR_ABORT:          event = HFX_PREEXEC_L44_ABORT; break;
            case HFX_GRAMMAR_END:            event = HFX_PREEXEC_END; break;
            default: {
                event = HFX_PREEXEC_TRANSPORT;
                const uint32_t exact = token == HFX_GRAMMAR_TRANSPORT_COMPLETE ? 10 :
                    2 * (static_cast<uint32_t>(token) -
                         static_cast<uint32_t>(HFX_GRAMMAR_TRANSPORT_FAIL_2) + 1);
                count = exact;
                if (exact > transport_states.size() - transport_index) return false;
                if (token == HFX_GRAMMAR_TRANSPORT_COMPLETE) {
                    static constexpr std::array<uint32_t, 10> complete {
                        2, 3, 4, 5, 6, 7, 8, 9, 10, 11
                    };
                    if (!std::equal(complete.begin(), complete.end(),
                                    transport_states.begin() + transport_index)) return false;
                } else {
                    const size_t failure_index =
                        static_cast<size_t>(token - HFX_GRAMMAR_TRANSPORT_FAIL_2);
                    if (!std::equal(
                            failure_sequences[failure_index].begin(),
                            failure_sequences[failure_index].begin() + exact,
                            transport_states.begin() + transport_index)) return false;
                }
                transport_index += exact;
                break;
            }
        }
        if (!consume(event, count, event_index)) return false;
    }
    return event_index == events.size() &&
        transport_index == transport_states.size();
}

static bool hfx_preexecute_requested() {
    const char * value = std::getenv("HALOFPX_PREEXECUTE_AUTHORITY");
    return value != nullptr && std::strcmp(value, "1") == 0;
}

struct hfx_preexecute_publication {
    std::string final_path;
    hfx_digest payload_sha256 {};
    int error_number = 0;
};

static std::string hfx_preexecute_authority_identity(
        const hfx_preexecute_recorder & recorder) {
    return hfx_hex(recorder.attempt_nonce.data(), recorder.attempt_nonce.size()) + "-" +
        std::to_string(recorder.generation) + "-" +
        std::to_string(recorder.client_connection_epoch) +
        (recorder.role == hfx_preexecute_recorder::SERVER ? "-server" : "");
}

static bool hfx_preexecute_publish(
        const hfx_preexecute_recorder & recorder,
        const std::vector<std::string> & records,
        hfx_preexecute_publication * publication = nullptr) {
    const char * path = std::getenv("HALOFPX_PREEXECUTE_AUTHORITY_PATH");
    std::string payload;
    for (const auto & record : records) payload += record;
    const std::string identity = hfx_preexecute_authority_identity(recorder);
    const std::string final_name = identity + ".authority";
    if (publication != nullptr) {
        publication->final_path =
            path != nullptr ? std::string(path) + "/" + final_name : std::string();
        publication->payload_sha256 = hfx_sha256(payload.data(), payload.size());
        publication->error_number = 0;
    }
    const auto fail = [&](int error_number) {
        if (publication != nullptr) {
            publication->error_number = error_number != 0 ? error_number : EIO;
        }
        return false;
    };
    const char * injection = std::getenv("HALOFPX_PREEXECUTE_PUBLISH_INJECT");
    if (recorder.role == hfx_preexecute_recorder::SERVER &&
        injection != nullptr && std::strcmp(injection, "server") == 0) {
        return fail(EIO);
    }
    if (path == nullptr || path[0] != '/' || std::strstr(path, "..") != nullptr ||
        records.empty()) return fail(EINVAL);
    const int directory = open(path, O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    if (directory < 0) return fail(errno);
    struct stat directory_stat {};
    if (fstat(directory, &directory_stat) != 0 || !S_ISDIR(directory_stat.st_mode) ||
        directory_stat.st_uid != geteuid() || (directory_stat.st_mode & 0022) != 0) {
        const int error_number = errno != 0 ? errno : EPERM;
        close(directory);
        return fail(error_number);
    }
    const std::string temporary_name =
        "." + identity + "." + std::to_string(static_cast<uint64_t>(getpid())) + ".tmp";
    const int fd = openat(directory, temporary_name.c_str(),
        O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW, 0600);
    if (fd < 0) {
        const int error_number = errno;
        close(directory);
        return fail(error_number);
    }
    const uint64_t expected_size = payload.size();
    bool valid = hfx_write_all(fd, payload.data(), payload.size());
    struct stat written {};
    valid = valid && fsync(fd) == 0 && fchmod(fd, 0400) == 0 &&
        fsync(fd) == 0 && fstat(fd, &written) == 0 &&
        S_ISREG(written.st_mode) && written.st_uid == geteuid() &&
        (written.st_mode & 0777) == 0400 &&
        static_cast<uint64_t>(written.st_size) == expected_size;
    valid = close(fd) == 0 && valid;
    if (valid) {
        const int reopened = openat(directory, temporary_name.c_str(),
            O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
        struct stat observed {};
        std::string observed_payload(expected_size, '\0');
        size_t read_bytes = 0;
        while (reopened >= 0 && read_bytes < observed_payload.size()) {
            const ssize_t count = read(
                reopened, observed_payload.data() + read_bytes,
                observed_payload.size() - read_bytes);
            if (count < 0 && errno == EINTR) continue;
            if (count <= 0) break;
            read_bytes += static_cast<size_t>(count);
        }
        valid = reopened >= 0 && fstat(reopened, &observed) == 0 &&
            observed.st_dev == written.st_dev && observed.st_ino == written.st_ino &&
            observed.st_uid == geteuid() && (observed.st_mode & 0777) == 0400 &&
            static_cast<uint64_t>(observed.st_size) == expected_size &&
            read_bytes == expected_size && observed_payload == payload;
        if (reopened >= 0) close(reopened);
    }
    // linkat is the atomic no-replace publication point: an existing immutable
    // attempt identity is a collision and is never overwritten.
    valid = valid &&
        linkat(directory, temporary_name.c_str(), directory, final_name.c_str(), 0) == 0;
    if (valid) {
        valid = unlinkat(directory, temporary_name.c_str(), 0) == 0 &&
            fsync(directory) == 0;
        const int reopened = openat(directory, final_name.c_str(),
            O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
        struct stat observed {};
        std::string observed_payload(expected_size, '\0');
        size_t read_bytes = 0;
        while (reopened >= 0 && read_bytes < observed_payload.size()) {
            const ssize_t count = read(
                reopened, observed_payload.data() + read_bytes,
                observed_payload.size() - read_bytes);
            if (count < 0 && errno == EINTR) continue;
            if (count <= 0) break;
            read_bytes += static_cast<size_t>(count);
        }
        valid = valid && reopened >= 0 && fstat(reopened, &observed) == 0 &&
            observed.st_uid == geteuid() && (observed.st_mode & 0777) == 0400 &&
            static_cast<uint64_t>(observed.st_size) == expected_size &&
            read_bytes == expected_size && observed_payload == payload;
        if (reopened >= 0) close(reopened);
    } else {
        unlinkat(directory, temporary_name.c_str(), 0);
        fsync(directory);
    }
    const int error_number = valid ? 0 : (errno != 0 ? errno : EIO);
    close(directory);
    return valid ? true : fail(error_number);
}

enum hfx_server_event_type : uint32_t {
    HFX_SERVER_ADMISSION_ACCEPTED = 1,
    HFX_SERVER_ADMISSION_REFUSED,
    HFX_SERVER_PHYSICAL_PREPARED,
    HFX_SERVER_EXECUTE_INTENT_CONSUMED,
    HFX_SERVER_BACKEND_EXECUTED,
    HFX_SERVER_RECEIPT_PUBLISHED,
    HFX_SERVER_CLOSE,
    HFX_SERVER_ABORT,
};

static const std::array<std::vector<uint32_t>, 5> HFX_SERVER_PRODUCTIONS {{
    { HFX_SERVER_ADMISSION_ACCEPTED, HFX_SERVER_PHYSICAL_PREPARED,
      HFX_SERVER_EXECUTE_INTENT_CONSUMED, HFX_SERVER_BACKEND_EXECUTED,
      HFX_SERVER_RECEIPT_PUBLISHED, HFX_SERVER_CLOSE },
    { HFX_SERVER_ADMISSION_REFUSED, HFX_SERVER_ABORT },
    { HFX_SERVER_ADMISSION_ACCEPTED, HFX_SERVER_ABORT },
    { HFX_SERVER_ADMISSION_ACCEPTED, HFX_SERVER_PHYSICAL_PREPARED,
      HFX_SERVER_ABORT },
    { HFX_SERVER_ADMISSION_ACCEPTED, HFX_SERVER_PHYSICAL_PREPARED,
      HFX_SERVER_EXECUTE_INTENT_CONSUMED, HFX_SERVER_ABORT },
}};

static bool hfx_server_sequence_valid(
        const std::vector<uint32_t> & events, bool terminal) {
    size_t matches = 0;
    for (const auto & production : HFX_SERVER_PRODUCTIONS) {
        const bool match = terminal ? events == production :
            (events.size() < production.size() &&
             std::equal(events.begin(), events.end(), production.begin()));
        matches += match ? 1 : 0;
    }
    return terminal ? matches == 1 : matches >= 1;
}

static bool hfx_server_emit(
        const std::shared_ptr<hfx_preexecute_recorder> & recorder,
        uint32_t type,
        uint32_t reason,
        bool terminal = false) {
    if (!hfx_preexecute_requested()) return true;
    if (!recorder) return false;
    std::lock_guard<std::mutex> lock(recorder->mutex);
    if (recorder->role != hfx_preexecute_recorder::SERVER ||
        recorder->terminal || recorder->generation == 0 ||
        recorder->execution_sequence == 0 ||
        hfx_zero(recorder->attempt_nonce.data(), 32) ||
        type < HFX_SERVER_ADMISSION_ACCEPTED || type > HFX_SERVER_ABORT) {
        return false;
    }
    auto events = recorder->server_events;
    events.push_back(type);
    auto reasons = recorder->server_reasons;
    reasons.push_back(reason);
    uint32_t terminal_branch = 0;
    if (terminal) {
        for (size_t i = 0; i < HFX_SERVER_PRODUCTIONS.size(); ++i) {
            if (events == HFX_SERVER_PRODUCTIONS[i]) {
                if (terminal_branch != 0) return false;
                terminal_branch = static_cast<uint32_t>(i + 1);
            }
        }
        if (terminal_branch == 0 || !hfx_server_sequence_valid(events, true)) {
            GGML_LOG_ERROR(
                "[halofpx-preexecute-server] terminal production refusal events=%zu last=%u\n",
                events.size(), type);
            return false;
        }
    } else {
        if (!hfx_server_sequence_valid(events, false)) return false;
    }
    if (terminal && (hfx_zero(recorder->admission_object_id.data(), 32) ||
                     hfx_zero(recorder->expected_admission_digest.data(), 32) ||
                     hfx_zero(recorder->graph_digest.data(), 32) ||
                     recorder->execute_receipt_state > 1 ||
                     (terminal_branch == 1 &&
                      (recorder->execute_receipt_state != 1 ||
                       hfx_zero(recorder->execute_receipt.data(), 32))) ||
                     (recorder->execute_receipt_state == 0 &&
                      !hfx_zero(recorder->execute_receipt.data(), 32)))) {
        return false;
    }
    std::vector<std::string> pending;
    std::array<uint8_t, 32> chain {};
    if (terminal) {
        std::array<uint8_t, 32> key {};
        if (!hfx_graph_key(key)) return false;
        for (size_t i = 0; i < events.size(); ++i) {
            std::vector<uint8_t> canonical;
            hfx_le<uint16_t>(canonical, 1);
            hfx_le<uint16_t>(canonical, 0);
            hfx_le<uint32_t>(canonical, hfx_preexecute_recorder::SERVER);
            hfx_le<uint32_t>(canonical, events[i]);
            hfx_le<uint32_t>(canonical, reasons[i]);
            hfx_le<uint32_t>(canonical, terminal_branch);
            hfx_le<uint64_t>(canonical, i + 1);
            hfx_le<uint64_t>(canonical, recorder->generation);
            hfx_le<uint64_t>(canonical, recorder->execution_sequence);
            hfx_le<uint64_t>(canonical, recorder->parent_uid);
            hfx_le<uint64_t>(canonical, recorder->split_uid);
            hfx_le<uint64_t>(canonical, recorder->server_connection_epoch);
            hfx_le<uint64_t>(canonical, recorder->allocation_epoch);
            hfx_le<uint32_t>(canonical, recorder->backend_ordinal);
            hfx_le<uint32_t>(canonical, recorder->split_ordinal);
            hfx_le<uint32_t>(canonical, recorder->execute_receipt_state);
            hfx_bytes(canonical, recorder->attempt_nonce.data(), 32);
            hfx_bytes(canonical, recorder->admission_object_id.data(), 32);
            hfx_bytes(canonical, recorder->expected_admission_digest.data(), 32);
            hfx_bytes(canonical, recorder->graph_digest.data(), 32);
            hfx_bytes(canonical, recorder->execute_receipt.data(), 32);
            hfx_bytes(canonical, chain.data(), 32);
            const auto tag =
                hfx_preexecute_hmac(key.data(), canonical.data(), canonical.size());
            pending.push_back(
                std::string("domain=") + HFX_PREEXECUTE_DOMAIN +
                "|role=server|grammar=1.0|record=" +
                hfx_hex(canonical.data(), canonical.size()) +
                "|tag=" + hfx_hex(tag.data(), tag.size()) + "\n");
            chain = tag;
        }
        hfx_preexecute_publication publication {};
        const bool published =
            hfx_preexecute_publish(*recorder, pending, &publication);
        GGML_LOG_INFO(
            "[halofpx-preexecute-server-publication] status=%s attempt=%s admission=%s "
            "sequence=%" PRIu64 " split_uid=%" PRIu64 " split_ordinal=%u backend=%u "
            "path=%s sha256=%s errno=%d\n",
            published ? "present" : "error",
            hfx_hex(recorder->attempt_nonce.data(), 32).c_str(),
            hfx_hex(recorder->admission_object_id.data(), 32).c_str(),
            recorder->execution_sequence, recorder->split_uid,
            recorder->split_ordinal, recorder->backend_ordinal,
            publication.final_path.c_str(),
            hfx_hex(publication.payload_sha256.data(), 32).c_str(),
            publication.error_number);
        if (!published) return false;
    }
    recorder->event_sequence = events.size();
    recorder->chain = chain;
    recorder->pending_records = std::move(pending);
    recorder->server_events = std::move(events);
    recorder->server_reasons = std::move(reasons);
    recorder->terminal_branch = terminal_branch;
    recorder->terminal = terminal;
    return true;
}

static bool hfx_preexecute_emit(
        const std::shared_ptr<hfx_preexecute_recorder> & recorder,
        uint32_t type,
        uint32_t reason,
        uint32_t transport_state = HFX_TRANSPORT_NOT_ATTEMPTED,
        uint8_t opcode = 0,
        uint64_t split_uid = 0,
        uint32_t split_ordinal = UINT32_MAX,
        uint64_t expected = 0,
        uint64_t actual = 0,
        int error_number = 0,
        bool terminal = false) {
    if (!hfx_preexecute_requested()) return true;
    if (recorder == nullptr || recorder->role != hfx_preexecute_recorder::CLIENT) return false;
    std::lock_guard<std::mutex> lock(recorder->mutex);
    if (recorder->terminal || recorder->event_sequence >= HFX_PREEXECUTE_MAX_EVENTS ||
        recorder->generation == 0 || recorder->execution_sequence == 0 ||
        hfx_zero(recorder->attempt_nonce.data(), 32)) return false;
    std::array<uint8_t, 32> key {};
    if (!hfx_graph_key(key)) return false;
    const uint64_t sequence = recorder->event_sequence + 1;
    if (type == 0 || type >= recorder->event_counts.size()) return false;
    auto counts = recorder->event_counts;
    ++counts[type];
    auto transport_phases = recorder->transport_phase;
    if (type == HFX_PREEXEC_TRANSPORT) {
        if (opcode == 0 ||
            !hfx_preexecute_transport_transition(transport_phases[opcode], transport_state)) {
            return false;
        }
        transport_phases[opcode] = transport_state;
    } else if (transport_state != HFX_TRANSPORT_NOT_ATTEMPTED &&
               transport_state != HFX_TRANSPORT_SERIALIZE_FAILED &&
               transport_state != HFX_TRANSPORT_DECODE_REFUSED) {
        return false;
    }
    auto observed_events = recorder->observed_events;
    observed_events.push_back(type);
    auto observed_transport_states = recorder->observed_transport_states;
    if (type == HFX_PREEXEC_TRANSPORT) {
        observed_transport_states.push_back(transport_state);
    }
    if (terminal) {
        const hfx_preexecute_production * matched = nullptr;
        for (const auto & production : HFX_PREEXECUTE_PRODUCTIONS) {
            // Branch 5 represents a completed, non-empty sealed census. With
            // two zero cardinalities its token expansion would be identical
            // to the preserved pre-registration branch 2.
            if (production.terminal_branch == 5 &&
                recorder->expected_register == 0 &&
                recorder->expected_exclude == 0) {
                continue;
            }
            if (hfx_preexecute_production_matches(
                    production, observed_events, observed_transport_states,
                    recorder->expected_register, recorder->expected_exclude)) {
                if (matched != nullptr) return false;
                matched = &production;
            }
        }
        if (matched == nullptr) {
            GGML_LOG_ERROR("[halofpx-preexecute] terminal production refusal "
                "begin=%u l42=%u l44=%u register=%u/%u exclude=%u/%u "
                "prepare=%u commit=%u decision=%u transport=%u abort=%u end=%u\n",
                counts[HFX_PREEXEC_BEGIN], counts[HFX_PREEXEC_L42_PREPARED],
                counts[HFX_PREEXEC_L44_BEGIN], counts[HFX_PREEXEC_L44_REGISTER],
                recorder->expected_register, counts[HFX_PREEXEC_L44_EXCLUDE],
                recorder->expected_exclude, counts[HFX_PREEXEC_L44_PREPARE],
                counts[HFX_PREEXEC_L44_COMMIT], counts[HFX_PREEXEC_GRAPH_DECISION],
                counts[HFX_PREEXEC_TRANSPORT], counts[HFX_PREEXEC_L44_ABORT],
                counts[HFX_PREEXEC_END]);
            return false;
        }
        recorder->terminal_branch = matched->terminal_branch;
    }
    std::vector<uint8_t> canonical;
    canonical.reserve(256);
    hfx_le<uint16_t>(canonical, 1);
    hfx_le<uint16_t>(canonical, 0);
    hfx_le<uint32_t>(canonical, type);
    hfx_le<uint32_t>(canonical, reason);
    hfx_le<uint32_t>(canonical, transport_state);
    hfx_le<uint32_t>(canonical, opcode);
    hfx_le<uint64_t>(canonical, sequence);
    hfx_le<uint64_t>(canonical, recorder->generation);
    hfx_le<uint64_t>(canonical, recorder->execution_sequence);
    hfx_le<uint64_t>(canonical, recorder->parent_uid);
    hfx_le<uint64_t>(canonical, split_uid);
    hfx_le<uint64_t>(canonical, recorder->client_connection_epoch);
    hfx_le<uint64_t>(canonical, recorder->server_connection_epoch);
    hfx_le<uint64_t>(canonical, recorder->allocation_epoch);
    hfx_le<uint32_t>(canonical, recorder->backend_ordinal);
    hfx_le<uint32_t>(canonical, split_ordinal);
    hfx_le<uint64_t>(canonical, expected);
    hfx_le<uint64_t>(canonical, actual);
    hfx_le<uint32_t>(canonical, static_cast<uint32_t>(error_number));
    hfx_le<uint32_t>(canonical, terminal ? recorder->terminal_branch : 0);
    hfx_le<uint32_t>(canonical, recorder->expected_register);
    hfx_le<uint32_t>(canonical, recorder->expected_exclude);
    for (uint32_t count : counts) hfx_le<uint32_t>(canonical, count);
    hfx_bytes(canonical, recorder->attempt_nonce.data(), 32);
    hfx_bytes(canonical, recorder->chain.data(), 32);
    const auto tag = hfx_preexecute_hmac(key.data(), canonical.data(), canonical.size());
    const std::string record =
        std::string("domain=") + HFX_PREEXECUTE_DOMAIN +
        "|record=" + hfx_hex(canonical.data(), canonical.size()) +
        "|tag=" + hfx_hex(tag.data(), tag.size()) + "\n";
    auto pending = recorder->pending_records;
    pending.push_back(record);
    if (terminal && !hfx_preexecute_publish(*recorder, pending)) {
        GGML_LOG_ERROR("[halofpx-preexecute] fatal immutable publication failure "
                       "generation=%" PRIu64 " errno=%d\n",
                       recorder->generation, errno);
        return false;
    }
    recorder->event_sequence = sequence;
    recorder->event_counts = counts;
    recorder->transport_phase = transport_phases;
    recorder->chain = tag;
    recorder->pending_records = std::move(pending);
    recorder->observed_events = std::move(observed_events);
    recorder->observed_transport_states = std::move(observed_transport_states);
    recorder->terminal = terminal;
    return true;
}

bool hfx_rpc_response_diagnostics_requested() {
    const char * enabled = std::getenv("HALOFPX_RPC_RESPONSE_DIAGNOSTICS");
    return enabled != nullptr && std::strcmp(enabled, "1") == 0;
}

bool hfx_rpc_response_event(
        const uint8_t key[32],
        const char * side,
        const char * phase,
        uint8_t opcode,
        uint64_t parent_uid,
        uint64_t split_uid,
        uint64_t exec_sequence,
        uint32_t backend_ordinal,
        const uint8_t attempt_nonce[32],
        const uint8_t connection_epoch[32],
        uint64_t expected,
        uint64_t actual,
        int rc,
        int error_number,
        bool eof,
        uint32_t status) {
    if (!hfx_rpc_response_diagnostics_requested()) return true;
    static std::atomic<uint32_t> event_sequence { 0 };
    const uint32_t sequence = event_sequence.fetch_add(1) + 1;
    if (sequence == 0 || sequence > HFX_RPC_RESPONSE_MAX_EVENTS) return false;
    const char * path = std::getenv("HALOFPX_RPC_RESPONSE_DIAGNOSTICS_PATH");
    if (path == nullptr || path[0] != '/' || std::strstr(path, "..") != nullptr ||
        side == nullptr || phase == nullptr) return false;
    const auto wall = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    const auto mono = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    std::string canonical =
        std::string("domain=") + HFX_RPC_RESPONSE_DOMAIN +
        "|version=1|event=" + std::to_string(sequence) +
        "|side=" + side + "|phase=" + phase +
        "|opcode=" + std::to_string(opcode) +
        "|parent_uid=" + std::to_string(parent_uid) +
        "|split_uid=" + std::to_string(split_uid) +
        "|exec_sequence=" + std::to_string(exec_sequence) +
        "|backend_ordinal=" + std::to_string(backend_ordinal) +
        "|attempt=" + hfx_hex(attempt_nonce, 32) +
        "|connection_epoch=" + hfx_hex(connection_epoch, 32) +
        "|expected=" + std::to_string(expected) +
        "|actual=" + std::to_string(actual) +
        "|rc=" + std::to_string(rc) +
        "|errno=" + std::to_string(error_number) +
        "|eof=" + std::to_string(eof ? 1 : 0) +
        "|status=" + std::to_string(status) +
        "|wall_ns=" + std::to_string(wall) +
        "|mono_ns=" + std::to_string(mono);
    const auto tag = hfx_rpc_response_hmac(key, canonical.data(), canonical.size());
    const std::string record = canonical + "|tag=" + hfx_hex(tag.data(), tag.size()) + "\n";
    const int fd = open(path, O_WRONLY | O_APPEND | O_CREAT | O_CLOEXEC | O_NOFOLLOW, 0600);
    if (fd < 0) return false;
    struct stat st {};
    const bool valid = fstat(fd, &st) == 0 && S_ISREG(st.st_mode) &&
        (st.st_mode & 0777) == 0600 && st.st_uid == geteuid() &&
        hfx_write_all(fd, record.data(), record.size()) && fsync(fd) == 0;
    close(fd);
    return valid;
}

bool hfx_rpc_response_evidence_probe(bool server_side) {
    std::array<uint8_t, 32> key {};
    std::array<uint8_t, 32> attempt {};
    std::array<uint8_t, 32> connection {};
    if (!hfx_rpc_response_diagnostics_requested() || !hfx_graph_key(key)) {
        return false;
    }
    attempt[0] = 0xa1;
    connection[0] = 0xc1;
    const bool written = hfx_rpc_response_event(
        key.data(), server_side ? "server" : "client",
        server_side ? "handler_entry" : "request_opcode",
        static_cast<uint8_t>(RPC_CMD_HALOFPX_GRAPH_AUTH_EXECUTE),
        server_side ? 0 : 1, 2, 1, 0,
        attempt.data(), connection.data(), server_side ? 0 : 1,
        server_side ? 0 : 1, 1, 0, false, 0);
    hfx_wipe(key.data(), key.size());
    return written;
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

extern "C" bool ggml_backend_rpc_halofpx_response_evidence_probe(bool server_side) {
#ifdef GGML_RPC_HALOFPX_LOCAL_STATE
    return hfx_rpc_response_evidence_probe(server_side);
#else
    GGML_UNUSED(server_side);
    return false;
#endif
}

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
        sock->mark_transport_failed();
        return false;
    }
    if (!sock->send_data(&input_size, sizeof(input_size))) {
        sock->mark_transport_failed();
        return false;
    }
    if (!sock->send_data(input, input_size)) {
        sock->mark_transport_failed();
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
        sock->mark_transport_failed();
        return false;
    }
    if (out_size != output_size) {
        sock->mark_transport_failed();
        return false;
    }
    if (!sock->recv_data(output, output_size)) {
        sock->mark_transport_failed();
        return false;
    }
    return true;
}

#ifdef GGML_RPC_HALOFPX_LOCAL_STATE
struct hfx_rpc_response_context {
    uint64_t parent_uid;
    uint64_t split_uid;
    uint64_t exec_sequence;
    uint32_t backend_ordinal;
    const uint8_t * attempt_nonce;
    const uint8_t * connection_epoch;
    const uint8_t * key;
};

static bool send_rpc_cmd_observed(
        socket_ptr sock,
        enum rpc_cmd cmd,
        const void * input,
        size_t input_size,
        void * output,
        size_t output_size,
        const hfx_rpc_response_context & ctx) {
    const auto event = [&](const char * phase, uint64_t expected, const rpc_transport_io_result & io,
                           bool ok, uint32_t status) {
        return hfx_rpc_response_event(
            ctx.key, "client", phase, static_cast<uint8_t>(cmd), ctx.parent_uid, ctx.split_uid,
            ctx.exec_sequence, ctx.backend_ordinal, ctx.attempt_nonce, ctx.connection_epoch,
            expected, io.transferred, ok ? 1 : 0, io.error_number, io.eof, status);
    };
    rpc_transport_io_result io {};
    const uint8_t cmd_byte = cmd;
    bool ok = sock->send_data_observed(&cmd_byte, sizeof(cmd_byte), io);
    if (!event("request_opcode", sizeof(cmd_byte), io, ok, 0) || !ok) return false;
    ok = sock->send_data_observed(&input_size, sizeof(input_size), io);
    if (!event("request_header", sizeof(input_size), io, ok, 0) || !ok) return false;
    ok = sock->send_data_observed(input, input_size, io);
    if (!event("request_body", input_size, io, ok, 0) || !ok) return false;
    uint64_t out_size = 0;
    ok = sock->recv_data_observed(&out_size, sizeof(out_size), io);
    if (!event("response_header", sizeof(out_size), io, ok, 0) || !ok) return false;
    if (out_size != output_size) {
        rpc_transport_io_result mismatch {};
        mismatch.requested = output_size;
        mismatch.transferred = out_size;
        event("response_size_mismatch", output_size, mismatch, false, 1);
        return false;
    }
    ok = sock->recv_data_observed(output, output_size, io);
    if (!event("response_body", output_size, io, ok, 0) || !ok) return false;
    return true;
}

static bool send_rpc_cmd_preexecute(
        socket_ptr sock,
        enum rpc_cmd cmd,
        const void * input,
        size_t input_size,
        void * output,
        size_t output_size,
        const std::shared_ptr<hfx_preexecute_recorder> & recorder,
        uint64_t split_uid,
        uint32_t split_ordinal) {
    if (!hfx_preexecute_requested()) {
        return send_rpc_cmd(sock, cmd, input, input_size, output, output_size);
    }
    const auto stage = [&](uint32_t state, const rpc_transport_io_result & io,
                           uint32_t reason = HFX_PREEXEC_OK) {
        return hfx_preexecute_emit(
            recorder, HFX_PREEXEC_TRANSPORT, reason, state,
            static_cast<uint8_t>(cmd), split_uid, split_ordinal,
            io.requested, io.transferred, io.error_number);
    };
    rpc_transport_io_result io {};
    const uint8_t cmd_byte = cmd;
    io.requested = sizeof(cmd_byte);
    if (!stage(HFX_TRANSPORT_SEND_OPCODE_STARTED, io)) return false;
    bool ok = sock->send_data_observed(&cmd_byte, sizeof(cmd_byte), io);
    if (!stage(ok ? HFX_TRANSPORT_SEND_OPCODE_COMPLETE :
                   HFX_TRANSPORT_SYSCALL_ERROR, io,
               ok ? HFX_PREEXEC_OK : HFX_PREEXEC_SEND_ERROR) || !ok) return false;
    io = {}; io.requested = sizeof(input_size);
    if (!stage(HFX_TRANSPORT_SEND_HEADER_STARTED, io)) return false;
    ok = sock->send_data_observed(&input_size, sizeof(input_size), io);
    if (!stage(ok ? HFX_TRANSPORT_SEND_HEADER_COMPLETE :
                   HFX_TRANSPORT_SYSCALL_ERROR, io,
               ok ? HFX_PREEXEC_OK : HFX_PREEXEC_SEND_ERROR) || !ok) return false;
    io = {}; io.requested = input_size;
    if (!stage(HFX_TRANSPORT_SEND_BODY_STARTED, io)) return false;
    ok = sock->send_data_observed(input, input_size, io);
    if (!stage(ok ? HFX_TRANSPORT_SEND_BODY_COMPLETE :
                   HFX_TRANSPORT_SYSCALL_ERROR, io,
               ok ? HFX_PREEXEC_OK : HFX_PREEXEC_SEND_ERROR) || !ok) return false;
    uint64_t out_size = 0;
    io = {}; io.requested = sizeof(out_size);
    if (!stage(HFX_TRANSPORT_RECV_HEADER_STARTED, io)) return false;
    ok = sock->recv_data_observed(&out_size, sizeof(out_size), io);
    const uint32_t header_state = ok ? HFX_TRANSPORT_RECV_HEADER_COMPLETE :
        (io.eof ? HFX_TRANSPORT_EOF : HFX_TRANSPORT_SYSCALL_ERROR);
    const uint32_t header_reason = ok ? HFX_PREEXEC_OK :
        (io.eof ? HFX_PREEXEC_RECEIVE_EOF : HFX_PREEXEC_RECEIVE_ERROR);
    if (!stage(header_state, io, header_reason) || !ok) return false;
    if (out_size != output_size) {
        io = {}; io.requested = output_size; io.transferred = out_size;
        stage(HFX_TRANSPORT_DECODE_REFUSED, io, HFX_PREEXEC_SIZE_MISMATCH);
        return false;
    }
    io = {}; io.requested = output_size;
    if (!stage(HFX_TRANSPORT_RECV_BODY_STARTED, io)) return false;
    ok = sock->recv_data_observed(output, output_size, io);
    const uint32_t body_state = ok ? HFX_TRANSPORT_RECV_BODY_COMPLETE :
        (io.eof ? HFX_TRANSPORT_EOF : HFX_TRANSPORT_SYSCALL_ERROR);
    const uint32_t body_reason = ok ? HFX_PREEXEC_OK :
        (io.eof ? HFX_PREEXEC_RECEIVE_EOF : HFX_PREEXEC_RECEIVE_ERROR);
    if (!stage(body_state, io, body_reason) || !ok) return false;
    const char * injection = std::getenv("HALOFPX_RPC_TRANSPORT_INJECT");
    unsigned injected_opcode = 0;
    if (injection != nullptr &&
        std::sscanf(injection, "decode_refusal:%u", &injected_opcode) == 1 &&
        injected_opcode == static_cast<unsigned>(cmd) && output_size != 0) {
        static_cast<uint8_t *>(output)[0] ^= 0x80;
    } else if (injection != nullptr &&
               std::sscanf(injection, "receipt_refusal:%u", &injected_opcode) == 1 &&
               injected_opcode == static_cast<unsigned>(cmd) && output_size != 0) {
        static_cast<uint8_t *>(output)[output_size - 1] ^= 0x80;
    }
    return true;
}
#endif

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

#ifdef GGML_RPC_HALOFPX_LOCAL_STATE
static hfx_digest hfx_mutable_chain(
        const hfx_digest & prior,
        const hfx_mutable_update_header & update) {
    const auto wire = hfx_mutable_encode(update, false);
    std::vector<uint8_t> input;
    input.reserve(prior.size() + wire.size());
    input.insert(input.end(), prior.begin(), prior.end());
    input.insert(input.end(), wire.begin(), wire.end());
    return hfx_sha256(input.data(), input.size());
}

static hfx_digest hfx_mutable_semantic_chain(
        const hfx_digest & prior,
        hfx_mutable_update_header update) {
    hfx_set_magic(update.magic, "HFXMUV1\0");
    update.exec_sequence = 0;
    update.mutation_sequence = 0;
    update.cache_hash = 0;
    memset(update.attempt_nonce, 0, 32);
    memset(update.server_nonce, 0, 32);
    memset(update.prior_root, 0, 32);
    memset(update.tag, 0, 32);
    const auto wire = hfx_mutable_encode(update, false);
    std::vector<uint8_t> input;
    input.insert(input.end(), prior.begin(), prior.end());
    input.insert(input.end(), wire.begin(), wire.end());
    return hfx_sha256(input.data(), input.size());
}

static bool hfx_mutable_client_receipt_valid(
        const hfx_mutable_receipt & receipt,
        const hfx_mutable_update_header & request,
        const hfx_digest & expected_root,
        const uint8_t key[32]) {
    return hfx_magic(receipt.magic, "HFXMUR1\0") &&
        receipt.major == HFX_MUTABLE_MAJOR && receipt.minor == HFX_MUTABLE_MINOR &&
        receipt.encoded_size == sizeof(receipt) && receipt.status == 1 &&
        receipt.role == request.role && receipt.role_ordinal == request.role_ordinal &&
        receipt.allocation_ordinal == request.allocation_ordinal &&
        receipt.graph_uid == request.graph_uid &&
        receipt.exec_sequence == request.exec_sequence &&
        receipt.mutation_sequence == request.mutation_sequence &&
        receipt.tensor_relative == request.tensor_relative &&
        receipt.logical_offset == request.logical_offset &&
        receipt.logical_size == request.logical_size &&
        hfx_equal(receipt.attempt_nonce, request.attempt_nonce, 32) &&
        hfx_equal(receipt.server_nonce, request.server_nonce, 32) &&
        hfx_equal(receipt.content_digest, request.content_digest, 32) &&
        hfx_equal(receipt.view_digest, request.view_digest, 32) &&
        hfx_equal(receipt.mutation_root, expected_root.data(), 32) &&
        hfx_mutable_verify(receipt, key);
}

static bool hfx_mutable_client_update(
        ggml_backend_rpc_buffer_context * buffer_ctx,
        ggml_tensor * tensor,
        const void * data,
        size_t offset,
        size_t size,
        bool hash_only,
        uint64_t cache_hash,
    bool * applied = nullptr) {
    std::shared_ptr<hfx_mutable_client_session> session_ptr;
    {
        std::lock_guard<std::mutex> lock(hfx_mutable_mutex);
        auto active = hfx_mutable_active_sockets.find(buffer_ctx->sock.get());
        if (active == hfx_mutable_active_sockets.end()) return false;
        auto sit = hfx_mutable_sessions.find(active->second);
        if (sit == hfx_mutable_sessions.end()) {
            GGML_LOG_ERROR("[halofpx-mutable] update refused reason=session-missing\n");
            return false;
        }
        session_ptr = sit->second;
    }
    if (session_ptr == nullptr) return false;
    std::lock_guard<std::mutex> session_lock(session_ptr->mutex);
    auto & session = *session_ptr;
    const auto * registration = hfx_mutable_find_registration(session.roles, tensor);
    if (registration == nullptr) {
        auto * target_ctx = static_cast<ggml_backend_rpc_buffer_context *>(tensor->buffer->context);
        const uint64_t target_base =
            reinterpret_cast<uint64_t>(ggml_backend_buffer_get_base(tensor->buffer));
        const uint64_t target_address = reinterpret_cast<uint64_t>(tensor->data);
        const uint64_t target_relative =
            target_address >= target_base ? target_address - target_base : UINT64_MAX;
        GGML_LOG_ERROR(
            "[halofpx-mutable] update refused reason=unregistered-tensor "
            "allocation=%u relative=%" PRIu64 " type=%d "
            "ne=%" PRId64 ",%" PRId64 ",%" PRId64 ",%" PRId64 " view=%u\n",
            target_ctx->allocation_ordinal, target_relative,
            static_cast<int>(tensor->type),
            tensor->ne[0], tensor->ne[1], tensor->ne[2], tensor->ne[3],
            tensor->view_src != nullptr ? 1u : 0u);
        return false;
    }
    if (!session.negotiated || session.failed ||
        buffer_ctx->sock.get() != session.sock.get() ||
        session.mutation_sequence >= session.attempt.max_mutations ||
        size == 0 || size > HFX_MUTABLE_MAX_BYTES ||
        tensor->buffer == nullptr || tensor->data == nullptr ||
        !hfx_mutable_role_valid(registration->role)) {
        GGML_LOG_ERROR("[halofpx-mutable] update refused reason=session-or-range\n");
        session.failed = true;
        return false;
    }
    const uint64_t base = reinterpret_cast<uint64_t>(ggml_backend_buffer_get_base(tensor->buffer));
    const uint64_t address = reinterpret_cast<uint64_t>(tensor->data);
    uint64_t logical_end = 0;
    if (address < base || !hfx_add(offset, size, logical_end) ||
        logical_end > ggml_nbytes(tensor) || address - base > UINT64_MAX - offset) {
        GGML_LOG_ERROR("[halofpx-mutable] update refused reason=bounds\n");
        session.failed = true;
        return false;
    }
    hfx_mutable_update_header request {};
    hfx_set_magic(request.magic, hash_only ? "HFXMUH1\0" : "HFXMUS1\0");
    request.major = HFX_MUTABLE_MAJOR;
    request.minor = HFX_MUTABLE_MINOR;
    request.encoded_size = sizeof(request);
    request.role = registration->role;
    request.role_ordinal = registration->role_ordinal;
    request.allocation_ordinal = buffer_ctx->allocation_ordinal;
    request.type = tensor->type;
    request.graph_uid = session.bound_graph_uid != 0 ?
        session.bound_graph_uid : session.attempt.graph_uid;
    request.exec_sequence = session.attempt.execution_sequence;
    request.mutation_sequence = ++session.mutation_sequence;
    request.tensor_relative = address - base;
    request.logical_offset = offset;
    request.logical_size = size;
    request.cache_hash = hash_only ? cache_hash : 0;
    for (size_t i = 0; i < GGML_MAX_DIMS; ++i) {
        if (tensor->ne[i] <= 0 || uint64_t(tensor->ne[i]) > UINT32_MAX || tensor->nb[i] > UINT32_MAX) {
            session.failed = true;
            return false;
        }
        request.ne[i] = static_cast<uint32_t>(tensor->ne[i]);
        request.nb[i] = static_cast<uint32_t>(tensor->nb[i]);
    }
    memcpy(request.attempt_nonce, session.attempt.attempt_nonce, 32);
    memcpy(request.server_nonce, session.server_nonce.data(), 32);
    const auto digest = hfx_sha256(data, size);
    memcpy(request.content_digest, digest.data(), 32);
    std::vector<uint8_t> view_chain;
    for (const ggml_tensor * v = tensor; v; v = v->view_src) {
        hfx_le<uint64_t>(view_chain, v->view_offs);
        if (view_chain.size() > 8 * GGML_MAX_SRC) {
            session.failed = true;
            return false;
        }
    }
    const auto view_digest = hfx_sha256(view_chain.data(), view_chain.size());
    memcpy(request.view_digest, view_digest.data(), 32);
    memcpy(request.prior_root, session.mutation_root.data(), 32);
    if (!hfx_mutable_sign(request, session.key.data())) {
        session.failed = true;
        return false;
    }
    const auto root = hfx_mutable_chain(session.mutation_root, request);
    const auto semantic = hfx_mutable_semantic_chain(session.semantic_root, request);
    const auto header = hfx_mutable_encode(request);
    const rpc_tensor wire_tensor = serialize_tensor(tensor);
    std::vector<uint8_t> wire;
    wire.reserve(header.size() + sizeof(wire_tensor) + (hash_only ? 0 : size));
    wire.insert(wire.end(), header.begin(), header.end());
    const uint8_t * tensor_bytes = reinterpret_cast<const uint8_t *>(&wire_tensor);
    wire.insert(wire.end(), tensor_bytes, tensor_bytes + sizeof(wire_tensor));
    if (!hash_only) {
        const uint8_t * bytes = static_cast<const uint8_t *>(data);
        wire.insert(wire.end(), bytes, bytes + size);
    }
    std::array<uint8_t, sizeof(hfx_mutable_receipt)> response_wire {};
    hfx_mutable_receipt receipt {};
    const rpc_cmd command = hash_only ? RPC_CMD_HALOFPX_MUTABLE_SET_HASH : RPC_CMD_HALOFPX_MUTABLE_SET;
    if (!send_rpc_cmd(buffer_ctx->sock, command, wire.data(), wire.size(),
                      response_wire.data(), response_wire.size()) ||
        !hfx_mutable_decode(response_wire.data(), response_wire.size(), receipt)) {
        GGML_LOG_ERROR("[halofpx-mutable] update refused reason=transport-or-response\n");
        session.failed = true;
        return false;
    }
    if (hash_only && receipt.status == 2) {
        session.set_hash_miss_count++;
        session.mutation_sequence--;
        if (applied) *applied = false;
        return hfx_magic(receipt.magic, "HFXMUR1\0") &&
            receipt.major == HFX_MUTABLE_MAJOR && receipt.minor == HFX_MUTABLE_MINOR &&
            receipt.encoded_size == sizeof(receipt) &&
            receipt.graph_uid == request.graph_uid &&
            receipt.exec_sequence == request.exec_sequence &&
            receipt.mutation_sequence == request.mutation_sequence &&
            receipt.role == request.role &&
            receipt.role_ordinal == request.role_ordinal &&
            receipt.allocation_ordinal == request.allocation_ordinal &&
            receipt.tensor_relative == request.tensor_relative &&
            receipt.logical_offset == request.logical_offset &&
            receipt.logical_size == request.logical_size &&
            hfx_equal(receipt.attempt_nonce, request.attempt_nonce, 32) &&
            hfx_equal(receipt.server_nonce, request.server_nonce, 32) &&
            hfx_equal(receipt.content_digest, request.content_digest, 32) &&
            hfx_equal(receipt.view_digest, request.view_digest, 32) &&
            hfx_mutable_verify(receipt, session.key.data());
    }
    if (!hfx_mutable_client_receipt_valid(receipt, request, root, session.key.data())) {
        GGML_LOG_ERROR("[halofpx-mutable] update refused reason=receipt status=%u\n", receipt.status);
        session.failed = true;
        return false;
    }
    if (applied) *applied = true;
    if (hash_only) session.set_hash_hit_count++;
    else session.set_count++;
    session.mutation_root = root;
    session.semantic_root = semantic;
    hfx_mutable_census_entry entry {};
    entry.role = request.role;
    entry.role_ordinal = request.role_ordinal;
    entry.allocation_ordinal = request.allocation_ordinal;
    entry.type = request.type;
    entry.tensor_relative = request.tensor_relative;
    entry.logical_size = ggml_nbytes(tensor);
    memcpy(entry.ne, request.ne, sizeof(entry.ne));
    memcpy(entry.nb, request.nb, sizeof(entry.nb));
    memcpy(entry.view_digest, request.view_digest, 32);
    session.mutations.push_back(entry);
    return true;
}
static bool hfx_mutable_client_bind(
        hfx_mutable_client_session & session,
        uint64_t graph_uid,
        uint64_t parent_graph_uid,
        uint32_t split_ordinal,
        uint32_t backend_ordinal,
        const uint8_t split_mapping_root[32]);
#endif

static void ggml_backend_rpc_buffer_free_buffer(ggml_backend_buffer_t buffer) {
    ggml_backend_rpc_buffer_context * ctx = (ggml_backend_rpc_buffer_context *)buffer->context;
#ifdef GGML_RPC_HALOFPX_LOCAL_STATE
    {
        std::lock_guard<std::mutex> lock(hfx_mutable_mutex);
        auto active = hfx_mutable_active_sockets.find(ctx->sock.get());
        if (active != hfx_mutable_active_sockets.end()) {
            auto found = hfx_mutable_sessions.find(active->second);
            if (found != hfx_mutable_sessions.end() && found->second &&
                found->second->recorder && !found->second->recorder->terminal &&
                !hfx_preexecute_emit(
                    found->second->recorder, HFX_PREEXEC_L44_ABORT,
                    HFX_PREEXEC_POST_ABORT, HFX_TRANSPORT_NOT_ATTEMPTED,
                    0, 0, UINT32_MAX, 0, 0, 0, true)) {
                GGML_LOG_ERROR(
                    "[halofpx-preexecute] fatal publication failure during buffer teardown\n");
            } else {
                hfx_mutable_close_session_locked(active->second);
            }
        }
    }
#endif
    rpc_msg_free_buffer_req request = {ctx->remote_ptr};
    bool status = !ctx->sock->observed_transport_failed() &&
        send_rpc_cmd(ctx->sock, RPC_CMD_FREE_BUFFER, &request, sizeof(request), nullptr, 0);
    if (!status) {
        GGML_LOG_ERROR(
            "[halofpx-rpc] buffer teardown request refused after connection failure\n");
    }
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
#ifdef GGML_RPC_HALOFPX_LOCAL_STATE
    if (hfx_mutable_requested() && hfx_mutable_session_active(ctx->sock.get())) {
        if (size > HASH_THRESHOLD) {
            bool applied = false;
            const uint64_t hash = fnv_hash(static_cast<const uint8_t *>(data), size);
            const bool valid = hfx_mutable_client_update(ctx, tensor, data, offset, size, true, hash, &applied);
            RPC_STATUS_ASSERT(valid);
            if (applied) return;
        }
        const bool valid = hfx_mutable_client_update(ctx, tensor, data, offset, size, false, 0);
        RPC_STATUS_ASSERT(valid);
        return;
    }
#endif
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

#ifdef GGML_RPC_HALOFPX_LOCAL_STATE
static uint32_t rpc_client_next_allocation_ordinal(const socket_ptr & sock) {
    struct sequence {
        std::weak_ptr<socket_t> owner;
        uint32_t next = 0;
    };
    static std::mutex mutex;
    static std::unordered_map<const socket_t *, sequence> sequences;
    std::lock_guard<std::mutex> lock(mutex);
    sequence & value = sequences[sock.get()];
    const auto owner = value.owner.lock();
    if (owner.get() != sock.get()) {
        value.owner = sock;
        value.next = 0;
    }
    RPC_STATUS_ASSERT(value.next != UINT32_MAX);
    return value.next++;
}
#endif

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
            new ggml_backend_rpc_buffer_context{
                sock, nullptr, response.remote_ptr, buft_ctx->device
#ifdef GGML_RPC_HALOFPX_LOCAL_STATE
                , hfx_graph_requested() ? rpc_client_next_allocation_ordinal(sock) : UINT32_MAX
#endif
            },
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
#ifdef GGML_RPC_HALOFPX_LOCAL_STATE
    {
        std::lock_guard<std::mutex> lock(hfx_mutable_mutex);
        std::vector<uint64_t> closing;
        for (const auto & item : hfx_mutable_sessions) {
            if (item.second && item.second->endpoint == rpc_ctx->endpoint) closing.push_back(item.first);
        }
        for (uint64_t session_id : closing) hfx_mutable_close_session_locked(session_id);
    }
#endif
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

#ifdef GGML_RPC_HALOFPX_LOCAL_STATE
using hfx_graph_storage_resolver = std::function<bool(uint64_t, uint32_t &, uint64_t &)>;

static bool hfx_graph_canonical_digest(
        uint32_t device,
        const uint64_t * nodes,
        uint32_t n_nodes,
        const rpc_tensor * tensors,
        uint32_t n_tensors,
        const hfx_graph_storage_resolver & storage,
        hfx_digest & digest) {
    if (n_nodes == 0 || n_nodes > HFX_GRAPH_AUTH_MAX_NODES ||
        n_tensors == 0 || n_tensors > HFX_GRAPH_AUTH_MAX_TENSORS) return false;
    std::unordered_map<uint64_t, uint32_t> ids;
    ids.reserve(n_tensors);
    for (uint32_t i = 0; i < n_tensors; ++i) {
        if (tensors[i].id == 0 || !ids.emplace(tensors[i].id, i).second) return false;
    }
    std::vector<uint8_t> canonical;
    canonical.reserve(static_cast<size_t>(n_tensors) * 320);
    hfx_bytes(canonical, HFX_GRAPH_AUTH_DOMAIN, sizeof(HFX_GRAPH_AUTH_DOMAIN) - 1);
    hfx_le<uint16_t>(canonical, HFX_GRAPH_AUTH_MAJOR);
    hfx_le<uint16_t>(canonical, HFX_GRAPH_AUTH_MINOR);
    hfx_le<uint32_t>(canonical, device);
    hfx_le<uint32_t>(canonical, n_nodes);
    hfx_le<uint32_t>(canonical, n_tensors);
    for (uint32_t i = 0; i < n_nodes; ++i) {
        const auto it = ids.find(nodes[i]);
        if (it == ids.end()) return false;
        hfx_le<uint32_t>(canonical, it->second);
    }
    for (uint32_t i = 0; i < n_tensors; ++i) {
        const auto & tensor = tensors[i];
        hfx_le<uint32_t>(canonical, i);
        hfx_le<uint32_t>(canonical, tensor.type);
        hfx_le<uint32_t>(canonical, tensor.op);
        hfx_le<int32_t>(canonical, tensor.flags);
        for (uint32_t d = 0; d < GGML_MAX_DIMS; ++d) hfx_le<uint32_t>(canonical, tensor.ne[d]);
        for (uint32_t d = 0; d < GGML_MAX_DIMS; ++d) hfx_le<uint32_t>(canonical, tensor.nb[d]);
        for (uint32_t p = 0; p < GGML_MAX_OP_PARAMS / sizeof(int32_t); ++p) {
            hfx_le<int32_t>(canonical, tensor.op_params[p]);
        }
        uint32_t null_bitmap = 0;
        for (uint32_t s = 0; s < GGML_MAX_SRC; ++s) {
            if (tensor.src[s] == 0) {
                null_bitmap |= UINT32_C(1) << s;
                hfx_le<uint32_t>(canonical, UINT32_MAX);
            } else {
                const auto it = ids.find(tensor.src[s]);
                if (it == ids.end()) return false;
                hfx_le<uint32_t>(canonical, it->second);
            }
        }
        hfx_le<uint32_t>(canonical, null_bitmap);
        if (tensor.view_src == 0) {
            hfx_le<uint32_t>(canonical, UINT32_MAX);
            hfx_le<uint64_t>(canonical, 0);
        } else {
            const auto it = ids.find(tensor.view_src);
            if (it == ids.end()) return false;
            hfx_le<uint32_t>(canonical, it->second);
            hfx_le<uint64_t>(canonical, tensor.view_offs);
        }
        uint64_t relative = 0;
        uint32_t allocation_ordinal = UINT32_MAX;
        if (tensor.buffer != 0 || tensor.data != 0) {
            if (tensor.buffer == 0 || tensor.data == 0 ||
                !storage(tensor.id, allocation_ordinal, relative) ||
                allocation_ordinal == UINT32_MAX) return false;
        }
        hfx_le<uint32_t>(canonical, allocation_ordinal);
        hfx_le<uint64_t>(canonical, relative);
    }
    if (canonical.size() > HFX_GRAPH_AUTH_MAX_GRAPH_BYTES) return false;
    digest = hfx_sha256(canonical.data(), canonical.size());
    return true;
}
#endif

static bool serialize_graph(
        uint32_t device,
        const ggml_cgraph * cgraph,
        std::vector<uint8_t> & output
#ifdef GGML_RPC_HALOFPX_LOCAL_STATE
        , hfx_digest * authority_digest
#endif
        ) {
    if (cgraph->n_nodes <= 0 || static_cast<uint64_t>(cgraph->n_nodes) > UINT32_MAX) return false;
    uint32_t n_nodes = cgraph->n_nodes;
    std::vector<rpc_tensor> tensors;
    std::unordered_set<ggml_tensor*> visited;
    for (uint32_t i = 0; i < n_nodes; i++) {
        add_tensor(cgraph->nodes[i], tensors, visited);
    }
    // serialization format:
    // | device (4 bytes) | n_nodes (4 bytes) | nodes (n_nodes * sizeof(uint64_t) | n_tensors (4 bytes) | tensors (n_tensors * sizeof(rpc_tensor)) |
    if (tensors.empty() || tensors.size() > UINT32_MAX) return false;
    uint32_t n_tensors = static_cast<uint32_t>(tensors.size());
    const uint64_t output_size_u64 = 2*sizeof(uint32_t) +
        static_cast<uint64_t>(n_nodes) * sizeof(uint64_t) + sizeof(uint32_t) +
        static_cast<uint64_t>(n_tensors) * sizeof(rpc_tensor);
    if (output_size_u64 > SIZE_MAX) return false;
    const size_t output_size = static_cast<size_t>(output_size_u64);
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
#ifdef GGML_RPC_HALOFPX_LOCAL_STATE
    if (authority_digest != nullptr) {
        std::vector<uint64_t> node_ids(n_nodes);
        for (uint32_t i = 0; i < n_nodes; ++i) node_ids[i] = reinterpret_cast<uint64_t>(cgraph->nodes[i]);
        const auto resolver = [](uint64_t id, uint32_t & ordinal, uint64_t & relative) {
            const auto * tensor = reinterpret_cast<const ggml_tensor *>(id);
            if (tensor == nullptr || tensor->buffer == nullptr || !ggml_backend_buffer_is_rpc(tensor->buffer)) return false;
            const auto * context = static_cast<const ggml_backend_rpc_buffer_context *>(tensor->buffer->context);
            if (context == nullptr || context->remote_ptr == 0) return false;
            const uint64_t base = reinterpret_cast<uint64_t>(ggml_backend_buffer_get_base(tensor->buffer));
            const uint64_t data = reinterpret_cast<uint64_t>(tensor->data);
            if (base == 0 || data < base) return false;
            ordinal = context->allocation_ordinal;
            relative = data - base;
            return true;
        };
        if (!hfx_graph_canonical_digest(device, node_ids.data(), n_nodes,
                                        tensors.data(), n_tensors, resolver, *authority_digest)) {
            authority_digest->fill(0);
            return false;
        }
    }
#endif
    return true;
}

#ifdef GGML_RPC_HALOFPX_LOCAL_STATE
static bool hfx_graph_negotiate(ggml_backend_rpc_context * ctx, socket_ptr sock) {
    if (ctx->graph_auth_negotiated) return true;
    if (!hfx_graph_requested() || !hfx_graph_key(ctx->graph_auth_key)) return false;
    hfx_graph_auth_caps_req request {};
    hfx_set_magic(request.magic, "HFXGAQ1\0");
    request.major = HFX_GRAPH_AUTH_MAJOR;
    request.minor = HFX_GRAPH_AUTH_MINOR;
    request.encoded_size = sizeof(request);
    if (!hfx_random_all(request.attempt_nonce, sizeof(request.attempt_nonce))) return false;
    memcpy(ctx->graph_auth_attempt_nonce.data(), request.attempt_nonce, sizeof(request.attempt_nonce));
    hfx_graph_sign_record(request, ctx->graph_auth_key.data());
    hfx_graph_auth_caps_rsp response {};
    const auto request_wire = hfx_graph_encode(request);
    std::array<uint8_t, sizeof(response)> response_wire {};
    if (!send_rpc_cmd(sock, RPC_CMD_HALOFPX_GRAPH_AUTH_CAPS,
                      request_wire.data(), request_wire.size(), response_wire.data(), response_wire.size()) ||
        !hfx_graph_decode(response_wire.data(), response_wire.size(), response) ||
        !hfx_magic(response.magic, "HFXGAC1\0") ||
        response.major != HFX_GRAPH_AUTH_MAJOR || response.minor != HFX_GRAPH_AUTH_MINOR ||
        response.encoded_size != sizeof(response) || response.status != 1 ||
        response.max_graph_bytes != HFX_GRAPH_AUTH_MAX_GRAPH_BYTES ||
        response.max_tensors != HFX_GRAPH_AUTH_MAX_TENSORS ||
        response.max_nodes != HFX_GRAPH_AUTH_MAX_NODES ||
        !hfx_equal(response.attempt_nonce, request.attempt_nonce, sizeof(request.attempt_nonce)) ||
        hfx_zero(response.server_nonce, sizeof(response.server_nonce)) ||
        !hfx_graph_verify_record(response, ctx->graph_auth_key.data())) {
        return false;
    }
    memcpy(ctx->graph_auth_server_nonce.data(), response.server_nonce, sizeof(response.server_nonce));
    ctx->graph_auth_sequence = 0;
    ctx->graph_auth_transcript_root = hfx_sha256(HFX_GRAPH_AUTH_DOMAIN, sizeof(HFX_GRAPH_AUTH_DOMAIN) - 1);
    ctx->graph_auth_negotiated = true;
    return true;
}

static bool hfx_graph_receipt_valid(
        const hfx_graph_auth_receipt & response,
        const hfx_graph_auth_header & request,
        const uint8_t key[32],
        uint32_t expected_status = 1) {
    return hfx_magic(response.magic, "HFXGAR1\0") &&
        response.major == HFX_GRAPH_AUTH_MAJOR && response.minor == HFX_GRAPH_AUTH_MINOR &&
        response.encoded_size == sizeof(response) && response.status == expected_status &&
        response.device == request.device && response.graph_uid == request.graph_uid &&
        response.exec_sequence == request.exec_sequence &&
        hfx_equal(response.attempt_nonce, request.attempt_nonce, 32) &&
        hfx_equal(response.server_nonce, request.server_nonce, 32) &&
        hfx_equal(response.graph_digest, request.graph_digest, 32) &&
        hfx_equal(response.transcript_root, request.transcript_root, 32) &&
        hfx_equal(response.admission_object_id, request.admission_object_id, 32) &&
        hfx_equal(response.expected_admission_digest,
                  request.expected_admission_digest, 32) &&
        hfx_graph_verify_record(response, key);
}

static void hfx_graph_make_header(
        hfx_graph_auth_header & header,
        const ggml_backend_rpc_context * ctx,
        uint32_t graph_size,
        uint64_t graph_uid,
        const uint8_t graph_digest[32]) {
    memset(&header, 0, sizeof(header));
    hfx_set_magic(header.magic, "HFXGAX1\0");
    header.major = HFX_GRAPH_AUTH_MAJOR;
    header.minor = HFX_GRAPH_AUTH_MINOR;
    header.encoded_size = sizeof(header);
    header.graph_size = graph_size;
    header.device = ctx->device;
    header.graph_uid = graph_uid;
    header.exec_sequence = ctx->graph_auth_sequence;
    memcpy(header.attempt_nonce, ctx->graph_auth_attempt_nonce.data(), 32);
    memcpy(header.server_nonce, ctx->graph_auth_server_nonce.data(), 32);
    memcpy(header.graph_digest, graph_digest, 32);
    memcpy(header.transcript_root, ctx->graph_auth_transcript_root.data(), 32);
    hfx_graph_sign_record(header, ctx->graph_auth_key.data());
}
#endif

static enum ggml_status ggml_backend_rpc_graph_compute(ggml_backend_t backend, ggml_cgraph * cgraph) {
    ggml_backend_rpc_context * rpc_ctx = (ggml_backend_rpc_context *)backend->context;

    GGML_ASSERT(cgraph->n_nodes > 0);
    bool reuse = cgraph->uid != 0 && rpc_ctx->last_graph_uid == cgraph->uid;
#ifdef GGML_RPC_HALOFPX_LOCAL_STATE
    if (hfx_graph_requested() && rpc_ctx->execution_armed) {
        if (!rpc_ctx->execution_splits_bound ||
            rpc_ctx->execution_parent_graph_uid == 0 ||
            hfx_zero(rpc_ctx->execution_split_mapping_root.data(), 32)) {
            hfx_preexecute_emit(rpc_ctx->preexecute_recorder, HFX_PREEXEC_GRAPH_DECISION,
                                HFX_PREEXEC_GRAPH_UNBOUND);
            return GGML_STATUS_FAILED;
        }
        const auto expected_split = std::find_if(
            rpc_ctx->execution_splits.begin(), rpc_ctx->execution_splits.end(),
            [cgraph](const ggml_backend_rpc_halofpx_split_identity & value) {
                return value.split_graph_uid == cgraph->uid;
            });
        if (expected_split == rpc_ctx->execution_splits.end()) {
            hfx_preexecute_emit(rpc_ctx->preexecute_recorder, HFX_PREEXEC_GRAPH_DECISION,
                                HFX_PREEXEC_SPLIT_MISSING);
            return GGML_STATUS_FAILED;
        }
        auto sock = get_socket(rpc_ctx->endpoint);
        if (sock == nullptr) {
            hfx_preexecute_emit(rpc_ctx->preexecute_recorder, HFX_PREEXEC_GRAPH_DECISION,
                                HFX_PREEXEC_SOCKET_MISSING);
            return GGML_STATUS_FAILED;
        }
        if (!hfx_graph_negotiate(rpc_ctx, sock)) {
            hfx_preexecute_emit(rpc_ctx->preexecute_recorder, HFX_PREEXEC_GRAPH_DECISION,
                                HFX_PREEXEC_CAPS_REFUSED);
            return GGML_STATUS_FAILED;
        }
        std::shared_ptr<hfx_mutable_client_session> session;
        if (hfx_mutable_requested()) {
            {
                std::lock_guard<std::mutex> lock(hfx_mutable_mutex);
                auto active = hfx_mutable_active_sockets.find(sock.get());
                if (active == hfx_mutable_active_sockets.end()) {
                    hfx_preexecute_emit(rpc_ctx->preexecute_recorder,
                        HFX_PREEXEC_GRAPH_DECISION, HFX_PREEXEC_SESSION_MISSING);
                    return GGML_STATUS_FAILED;
                }
                auto it = hfx_mutable_sessions.find(active->second);
                if (it == hfx_mutable_sessions.end()) {
                    hfx_preexecute_emit(rpc_ctx->preexecute_recorder,
                        HFX_PREEXEC_GRAPH_DECISION, HFX_PREEXEC_SESSION_MISSING);
                    return GGML_STATUS_FAILED;
                }
                session = it->second;
            }
            if (session == nullptr) {
                hfx_preexecute_emit(rpc_ctx->preexecute_recorder,
                    HFX_PREEXEC_GRAPH_DECISION, HFX_PREEXEC_SESSION_MISSING);
                return GGML_STATUS_FAILED;
            }
            std::lock_guard<std::mutex> session_lock(session->mutex);
            if (session->failed || !session->prepared ||
                session->attempt.execution_sequence != rpc_ctx->execution_sequence ||
                session->client_connection_epoch != sock->authority_epoch()) {
                hfx_preexecute_emit(rpc_ctx->preexecute_recorder,
                    HFX_PREEXEC_GRAPH_DECISION,
                    session->client_connection_epoch != sock->authority_epoch() ?
                        HFX_PREEXEC_WRONG_CONNECTION : HFX_PREEXEC_SESSION_UNPREPARED);
                return GGML_STATUS_FAILED;
            }
            if (session->bound_graph_uid != cgraph->uid &&
                !hfx_mutable_client_bind(
                    *session, cgraph->uid, rpc_ctx->execution_parent_graph_uid,
                    expected_split->split_ordinal, expected_split->backend_ordinal,
                    rpc_ctx->execution_split_mapping_root.data())) {
                hfx_preexecute_emit(rpc_ctx->preexecute_recorder,
                    HFX_PREEXEC_GRAPH_DECISION, HFX_PREEXEC_BIND_REFUSED);
                return GGML_STATUS_FAILED;
            }
            std::vector<uint8_t> binding;
            hfx_bytes(binding, session->semantic_root.data(), 32);
            hfx_bytes(binding, session->attempt.scheduler_attempt_nonce, 32);
            hfx_le<uint64_t>(binding, rpc_ctx->execution_parent_graph_uid);
            hfx_le<uint64_t>(binding, rpc_ctx->execution_sequence);
            hfx_le<uint32_t>(binding, expected_split->split_ordinal);
            hfx_le<uint32_t>(binding, expected_split->backend_ordinal);
            hfx_le<uint64_t>(binding, expected_split->split_graph_uid);
            hfx_bytes(binding, rpc_ctx->execution_split_mapping_root.data(), 32);
            rpc_ctx->graph_auth_transcript_root = hfx_sha256(binding.data(), binding.size());
            GGML_LOG_INFO("[halofpx-mutable] bound graph uid=%" PRIu64 " exec=%" PRIu64
                          " semantic=%s transcript=%s\n",
                          cgraph->uid, session->attempt.execution_sequence,
                          hfx_hex(session->semantic_root.data(), 32).c_str(),
                          hfx_hex(rpc_ctx->graph_auth_transcript_root.data(), 32).c_str());
        }
        rpc_ctx->graph_auth_sequence = rpc_ctx->execution_sequence;
        hfx_graph_auth_header request {};
        hfx_graph_auth_receipt response {};
        if (reuse) {
            hfx_graph_make_header(request, rpc_ctx, 0, cgraph->uid, rpc_ctx->graph_auth_last_digest.data());
            hfx_set_magic(request.magic, "HFXGRX1\0");
            hfx_graph_sign_record(request, rpc_ctx->graph_auth_key.data());
            const auto request_wire = hfx_graph_encode(request);
            std::array<uint8_t, sizeof(response)> response_wire {};
            if (!hfx_preexecute_emit(
                    rpc_ctx->preexecute_recorder, HFX_PREEXEC_GRAPH_DECISION,
                    HFX_PREEXEC_OK, HFX_TRANSPORT_NOT_ATTEMPTED,
                    static_cast<uint8_t>(RPC_CMD_HALOFPX_GRAPH_AUTH_RECOMPUTE),
                    cgraph->uid, expected_split->split_ordinal) ||
                !send_rpc_cmd_preexecute(sock, RPC_CMD_HALOFPX_GRAPH_AUTH_RECOMPUTE,
                              request_wire.data(), request_wire.size(),
                              response_wire.data(), response_wire.size(),
                              rpc_ctx->preexecute_recorder, cgraph->uid,
                              expected_split->split_ordinal)) {
                return GGML_STATUS_FAILED;
            }
            if (!hfx_graph_decode(response_wire.data(), response_wire.size(), response)) {
                hfx_preexecute_emit(rpc_ctx->preexecute_recorder, HFX_PREEXEC_GRAPH_DECISION,
                    HFX_PREEXEC_DECODE_REFUSED, HFX_TRANSPORT_DECODE_REFUSED,
                    static_cast<uint8_t>(RPC_CMD_HALOFPX_GRAPH_AUTH_RECOMPUTE),
                    cgraph->uid, expected_split->split_ordinal);
                return GGML_STATUS_FAILED;
            }
            if (!hfx_graph_receipt_valid(response, request, rpc_ctx->graph_auth_key.data())) {
                hfx_preexecute_emit(rpc_ctx->preexecute_recorder, HFX_PREEXEC_GRAPH_DECISION,
                    HFX_PREEXEC_RECEIPT_REFUSED, HFX_TRANSPORT_DECODE_REFUSED,
                    static_cast<uint8_t>(RPC_CMD_HALOFPX_GRAPH_AUTH_RECOMPUTE),
                    cgraph->uid, expected_split->split_ordinal);
                return GGML_STATUS_FAILED;
            }
            GGML_LOG_INFO("[halofpx-rpc-graph-auth] client prepared mode=recompute sequence=%" PRIu64
                          " uid=%" PRIu64 " digest=%s\n",
                          request.exec_sequence, request.graph_uid,
                          hfx_hex(request.graph_digest, 32).c_str());
        } else {
            std::vector<uint8_t> graph;
            hfx_digest digest {};
            if (!serialize_graph(rpc_ctx->device, cgraph, graph, &digest) ||
                graph.empty() || graph.size() > HFX_GRAPH_AUTH_MAX_GRAPH_BYTES || hfx_zero(digest.data(), digest.size())) {
                hfx_preexecute_emit(
                    rpc_ctx->preexecute_recorder, HFX_PREEXEC_GRAPH_DECISION,
                    HFX_PREEXEC_SERIALIZE_FAILED, HFX_TRANSPORT_SERIALIZE_FAILED,
                    static_cast<uint8_t>(RPC_CMD_HALOFPX_GRAPH_AUTH_COMPUTE),
                    cgraph->uid, expected_split->split_ordinal);
                return GGML_STATUS_FAILED;
            }
            rpc_ctx->last_graph_uid = cgraph->uid;
            rpc_ctx->graph_auth_last_digest = digest;
            hfx_graph_make_header(request, rpc_ctx, static_cast<uint32_t>(graph.size()), cgraph->uid, digest.data());
            const auto header_wire = hfx_graph_encode(request);
            std::vector<uint8_t> envelope(header_wire.size() + graph.size());
            memcpy(envelope.data(), header_wire.data(), header_wire.size());
            memcpy(envelope.data() + header_wire.size(), graph.data(), graph.size());
            std::array<uint8_t, sizeof(response)> response_wire {};
            if (!hfx_preexecute_emit(
                    rpc_ctx->preexecute_recorder, HFX_PREEXEC_GRAPH_DECISION,
                    HFX_PREEXEC_OK, HFX_TRANSPORT_NOT_ATTEMPTED,
                    static_cast<uint8_t>(RPC_CMD_HALOFPX_GRAPH_AUTH_COMPUTE),
                    cgraph->uid, expected_split->split_ordinal) ||
                !send_rpc_cmd_preexecute(
                    sock, RPC_CMD_HALOFPX_GRAPH_AUTH_COMPUTE,
                    envelope.data(), envelope.size(), response_wire.data(), response_wire.size(),
                    rpc_ctx->preexecute_recorder, cgraph->uid, expected_split->split_ordinal)) {
                return GGML_STATUS_FAILED;
            }
            if (!hfx_graph_decode(response_wire.data(), response_wire.size(), response)) {
                hfx_preexecute_emit(rpc_ctx->preexecute_recorder, HFX_PREEXEC_GRAPH_DECISION,
                    HFX_PREEXEC_DECODE_REFUSED, HFX_TRANSPORT_DECODE_REFUSED,
                    static_cast<uint8_t>(RPC_CMD_HALOFPX_GRAPH_AUTH_COMPUTE),
                    cgraph->uid, expected_split->split_ordinal);
                return GGML_STATUS_FAILED;
            }
            if (!hfx_graph_receipt_valid(response, request, rpc_ctx->graph_auth_key.data())) {
                hfx_preexecute_emit(rpc_ctx->preexecute_recorder, HFX_PREEXEC_GRAPH_DECISION,
                    HFX_PREEXEC_RECEIPT_REFUSED, HFX_TRANSPORT_DECODE_REFUSED,
                    static_cast<uint8_t>(RPC_CMD_HALOFPX_GRAPH_AUTH_COMPUTE),
                    cgraph->uid, expected_split->split_ordinal);
                return GGML_STATUS_FAILED;
            }
            GGML_LOG_INFO("[halofpx-rpc-graph-auth] client prepared mode=compute sequence=%" PRIu64
                          " uid=%" PRIu64 " digest=%s\n",
                          request.exec_sequence, request.graph_uid,
                          hfx_hex(request.graph_digest, 32).c_str());
        }
        hfx_set_magic(request.magic, "HFXGEX1\0");
        request.graph_size = 0;
        if (session) {
            memcpy(request.admission_object_id,
                   session->admission.object_id, 32);
            const auto expected_digest = hfx_sha256(
                &session->expected_admission,
                offsetof(struct ggml_backend_sched_authority_prepared_admission, tag));
            memcpy(request.expected_admission_digest,
                   expected_digest.data(), expected_digest.size());
        }
        hfx_graph_sign_record(request, rpc_ctx->graph_auth_key.data());
        const auto execute_wire = hfx_graph_encode(request);
        std::array<uint8_t, sizeof(response)> execute_response_wire {};
        const hfx_rpc_response_context response_context {
            rpc_ctx->execution_parent_graph_uid,
            request.graph_uid,
            request.exec_sequence,
            expected_split->backend_ordinal,
            request.attempt_nonce,
            request.server_nonce,
            rpc_ctx->graph_auth_key.data(),
        };
        const bool response_observed = hfx_rpc_response_diagnostics_requested();
        const bool response_received = hfx_preexecute_requested()
            ? send_rpc_cmd_preexecute(
                sock, RPC_CMD_HALOFPX_GRAPH_AUTH_EXECUTE,
                execute_wire.data(), execute_wire.size(),
                execute_response_wire.data(), execute_response_wire.size(),
                rpc_ctx->preexecute_recorder, request.graph_uid,
                expected_split->split_ordinal)
            : response_observed
            ? send_rpc_cmd_observed(sock, RPC_CMD_HALOFPX_GRAPH_AUTH_EXECUTE,
                                    execute_wire.data(), execute_wire.size(),
                                    execute_response_wire.data(), execute_response_wire.size(),
                                    response_context)
            : send_rpc_cmd(sock, RPC_CMD_HALOFPX_GRAPH_AUTH_EXECUTE,
                           execute_wire.data(), execute_wire.size(),
                           execute_response_wire.data(), execute_response_wire.size());
        if (!response_received) {
            return GGML_STATUS_FAILED;
        }
        const bool decoded = hfx_graph_decode(
            execute_response_wire.data(), execute_response_wire.size(), response);
        if (response_observed &&
            !hfx_rpc_response_event(
                rpc_ctx->graph_auth_key.data(), "client", "client_decode",
                static_cast<uint8_t>(RPC_CMD_HALOFPX_GRAPH_AUTH_EXECUTE),
                rpc_ctx->execution_parent_graph_uid, request.graph_uid, request.exec_sequence,
                expected_split->backend_ordinal, request.attempt_nonce, request.server_nonce,
                0, 0, decoded ? 1 : 0, 0, false, decoded ? 1U : 0U)) {
            return GGML_STATUS_FAILED;
        }
        if (!decoded) {
            hfx_preexecute_emit(rpc_ctx->preexecute_recorder, HFX_PREEXEC_GRAPH_DECISION,
                HFX_PREEXEC_DECODE_REFUSED, HFX_TRANSPORT_DECODE_REFUSED,
                static_cast<uint8_t>(RPC_CMD_HALOFPX_GRAPH_AUTH_EXECUTE),
                request.graph_uid, expected_split->split_ordinal);
            return GGML_STATUS_FAILED;
        }
        const bool receipt_valid = hfx_graph_receipt_valid(
            response, request, rpc_ctx->graph_auth_key.data(), 2);
        if (response_observed &&
            !hfx_rpc_response_event(
                rpc_ctx->graph_auth_key.data(), "client", "client_receipt_validation",
                static_cast<uint8_t>(RPC_CMD_HALOFPX_GRAPH_AUTH_EXECUTE),
                rpc_ctx->execution_parent_graph_uid, request.graph_uid, request.exec_sequence,
                expected_split->backend_ordinal, request.attempt_nonce, request.server_nonce,
                0, 0, receipt_valid ? 1 : 0, 0, false,
                receipt_valid ? response.status : 0U)) {
            return GGML_STATUS_FAILED;
        }
        if (!receipt_valid) {
            hfx_preexecute_emit(rpc_ctx->preexecute_recorder, HFX_PREEXEC_GRAPH_DECISION,
                HFX_PREEXEC_RECEIPT_REFUSED, HFX_TRANSPORT_DECODE_REFUSED,
                static_cast<uint8_t>(RPC_CMD_HALOFPX_GRAPH_AUTH_EXECUTE),
                request.graph_uid, expected_split->split_ordinal);
            return GGML_STATUS_FAILED;
        }
        GGML_LOG_INFO("[halofpx-rpc-graph-auth] client executed sequence=%" PRIu64
                      " uid=%" PRIu64 " digest=%s\n",
                      request.exec_sequence, request.graph_uid,
                      hfx_hex(request.graph_digest, 32).c_str());
        rpc_ctx->graph_auth_result = {};
        rpc_ctx->graph_auth_result.version = HFX_GRAPH_AUTH_MAJOR;
        rpc_ctx->graph_auth_result.status = response.status;
        rpc_ctx->graph_auth_result.graph_uid = response.graph_uid;
        rpc_ctx->graph_auth_result.execution_sequence = response.exec_sequence;
        memcpy(rpc_ctx->graph_auth_result.graph_digest, response.graph_digest, 32);
        memcpy(rpc_ctx->graph_auth_result.transcript_root, response.transcript_root, 32);
        memcpy(rpc_ctx->graph_auth_result.receipt_tag, response.tag, 32);
        if (std::any_of(
                rpc_ctx->graph_auth_results.begin(), rpc_ctx->graph_auth_results.end(),
                [&response](const ggml_backend_rpc_halofpx_graph_result & value) {
                    return value.graph_uid == response.graph_uid;
                })) {
            return GGML_STATUS_FAILED;
        }
        rpc_ctx->graph_auth_results.push_back(rpc_ctx->graph_auth_result);
        return GGML_STATUS_SUCCESS;
    }
#endif
    if (reuse) {
        rpc_msg_graph_recompute_req request;
        request.device = rpc_ctx->device;
        auto sock = get_socket(rpc_ctx->endpoint);
        bool status = send_rpc_cmd(sock, RPC_CMD_GRAPH_RECOMPUTE, &request, sizeof(request));
        RPC_STATUS_ASSERT(status);
    } else {
        rpc_ctx->last_graph_uid = cgraph->uid;
        std::vector<uint8_t> input;
        bool serialized = serialize_graph(rpc_ctx->device, cgraph, input
#ifdef GGML_RPC_HALOFPX_LOCAL_STATE
                        , nullptr
#endif
        );
        RPC_STATUS_ASSERT(serialized);
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
#ifdef GGML_RPC_HALOFPX_LOCAL_STATE
        /* .graph_auth_negotiated     = */ false,
        /* .graph_auth_sequence       = */ 0,
        /* .graph_auth_key            = */ {},
        /* .graph_auth_attempt_nonce  = */ {},
        /* .graph_auth_server_nonce   = */ {},
        /* .graph_auth_last_digest    = */ {},
        /* .graph_auth_transcript_root= */ {},
        /* .graph_auth_result         = */ {},
        /* .graph_auth_results        = */ {},
        /* .execution_armed           = */ false,
        /* .execution_splits_bound    = */ false,
        /* .execution_mutable_begin_consumed= */ false,
        /* .execution_sequence        = */ 0,
        /* .last_execution_sequence   = */ 0,
        /* .execution_parent_graph_uid= */ 0,
        /* .execution_backend_ordinal = */ UINT32_MAX,
        /* .execution_split_mapping_root= */ {},
        /* .execution_splits          = */ {},
        /* .execution_consumed_split_uids= */ {},
        /* .execution_attempt_nonce   = */ {},
        /* .preexecute_recorder       = */ {},
#endif
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

#ifdef GGML_RPC_HALOFPX_LOCAL_STATE
bool ggml_backend_rpc_halofpx_execution_arm(
        ggml_backend_t backend,
        const uint8_t attempt_nonce[GGML_RPC_HALOFPX_STATE_DIGEST_BYTES],
        uint64_t execution_sequence) {
    if (!hfx_graph_requested() || !hfx_mutable_requested() ||
        !ggml_backend_is_rpc(backend) || attempt_nonce == nullptr ||
        execution_sequence == 0 || hfx_zero(attempt_nonce, 32)) return false;
    auto * ctx = static_cast<ggml_backend_rpc_context *>(backend->context);
    if (ctx->execution_armed || execution_sequence <= ctx->last_execution_sequence) return false;
    ctx->execution_armed = true;
    ctx->execution_sequence = execution_sequence;
    ctx->last_execution_sequence = execution_sequence;
    memcpy(ctx->execution_attempt_nonce.data(), attempt_nonce, 32);
    ctx->graph_auth_result = {};
    ctx->graph_auth_results.clear();
    ctx->execution_splits_bound = false;
    ctx->execution_mutable_begin_consumed = false;
    ctx->execution_parent_graph_uid = 0;
    ctx->execution_backend_ordinal = UINT32_MAX;
    ctx->execution_split_mapping_root.fill(0);
    ctx->execution_splits.clear();
    ctx->execution_consumed_split_uids.clear();
    ctx->preexecute_recorder.reset();
    if (hfx_preexecute_requested()) {
        auto sock = get_socket(ctx->endpoint);
        if (sock == nullptr) {
            ctx->execution_armed = false;
            return false;
        }
        auto recorder = std::make_shared<hfx_preexecute_recorder>();
        recorder->generation =
            hfx_preexecute_next_generation.fetch_add(1, std::memory_order_relaxed);
        if (recorder->generation == 0) {
            recorder->generation =
                hfx_preexecute_next_generation.fetch_add(1, std::memory_order_relaxed);
        }
        recorder->execution_sequence = execution_sequence;
        recorder->client_connection_epoch = sock->authority_epoch();
        memcpy(recorder->attempt_nonce.data(), attempt_nonce, 32);
        ctx->preexecute_recorder = recorder;
        if (!hfx_preexecute_emit(recorder, HFX_PREEXEC_BEGIN, HFX_PREEXEC_OK)) {
            ctx->preexecute_recorder.reset();
            ctx->execution_armed = false;
            return false;
        }
    }
    return true;
}

bool ggml_backend_rpc_halofpx_execution_bind_splits(
        ggml_backend_t backend,
        const uint8_t attempt_nonce[GGML_RPC_HALOFPX_STATE_DIGEST_BYTES],
        uint64_t execution_sequence,
        uint64_t parent_graph_uid,
        const uint8_t split_mapping_root[GGML_RPC_HALOFPX_STATE_DIGEST_BYTES],
        uint32_t backend_ordinal,
        const ggml_backend_rpc_halofpx_split_identity * splits,
        size_t split_count) {
    if (!ggml_backend_is_rpc(backend) || attempt_nonce == nullptr ||
        split_mapping_root == nullptr || splits == nullptr || split_count == 0 ||
        split_count > 64 || parent_graph_uid == 0 || backend_ordinal == UINT32_MAX ||
        hfx_zero(split_mapping_root, 32)) return false;
    auto * ctx = static_cast<ggml_backend_rpc_context *>(backend->context);
    if (!ctx->execution_armed || ctx->execution_splits_bound ||
        ctx->execution_sequence != execution_sequence ||
        !hfx_equal(ctx->execution_attempt_nonce.data(), attempt_nonce, 32)) return false;
    std::unordered_set<uint64_t> uids;
    uint32_t prior_ordinal = 0;
    for (size_t i = 0; i < split_count; ++i) {
        if (splits[i].split_graph_uid == 0 ||
            splits[i].backend_ordinal != backend_ordinal ||
            (i > 0 && splits[i].split_ordinal <= prior_ordinal) ||
            !uids.insert(splits[i].split_graph_uid).second) return false;
        prior_ordinal = splits[i].split_ordinal;
    }
    ctx->execution_parent_graph_uid = parent_graph_uid;
    ctx->execution_backend_ordinal = backend_ordinal;
    memcpy(ctx->execution_split_mapping_root.data(), split_mapping_root, 32);
    ctx->execution_splits.assign(splits, splits + split_count);
    ctx->execution_consumed_split_uids.clear();
    ctx->execution_splits_bound = true;
    if (ctx->preexecute_recorder) {
        ctx->preexecute_recorder->parent_uid = parent_graph_uid;
        ctx->preexecute_recorder->backend_ordinal = backend_ordinal;
        if (!hfx_preexecute_emit(
                ctx->preexecute_recorder, HFX_PREEXEC_L42_PREPARED,
                HFX_PREEXEC_OK, HFX_TRANSPORT_NOT_ATTEMPTED, 0, 0,
                UINT32_MAX, split_count, split_count)) {
            ctx->execution_splits_bound = false;
            return false;
        }
    }
    return true;
}

bool ggml_backend_rpc_halofpx_execution_disarm(
        ggml_backend_t backend,
        const uint8_t attempt_nonce[GGML_RPC_HALOFPX_STATE_DIGEST_BYTES],
        uint64_t execution_sequence) {
    if (!ggml_backend_is_rpc(backend) || attempt_nonce == nullptr) return false;
    auto * ctx = static_cast<ggml_backend_rpc_context *>(backend->context);
    const bool matched = ctx->execution_armed &&
        ctx->execution_sequence == execution_sequence &&
        hfx_equal(ctx->execution_attempt_nonce.data(), attempt_nonce, 32);
    if (ctx->preexecute_recorder && !ctx->preexecute_recorder->terminal) {
        bool completed = false;
        {
            std::lock_guard<std::mutex> recorder_lock(ctx->preexecute_recorder->mutex);
            completed =
                ctx->preexecute_recorder->event_counts[HFX_PREEXEC_L44_PREPARE] == 1 &&
                ctx->preexecute_recorder->event_counts[HFX_PREEXEC_L44_COMMIT] == 1;
        }
        const bool successful_terminal = matched && completed;
        if (!hfx_preexecute_emit(
            ctx->preexecute_recorder,
            successful_terminal ? HFX_PREEXEC_END : HFX_PREEXEC_L44_ABORT,
            successful_terminal ? HFX_PREEXEC_OK : HFX_PREEXEC_POST_ABORT,
            HFX_TRANSPORT_NOT_ATTEMPTED, 0, 0, UINT32_MAX, 0, 0, 0, true)) {
            return false;
        }
    }
    ctx->execution_armed = false;
    ctx->execution_splits_bound = false;
    ctx->execution_mutable_begin_consumed = false;
    ctx->execution_sequence = 0;
    ctx->execution_parent_graph_uid = 0;
    ctx->execution_backend_ordinal = UINT32_MAX;
    ctx->execution_split_mapping_root.fill(0);
    ctx->execution_splits.clear();
    ctx->execution_consumed_split_uids.clear();
    ctx->graph_auth_results.clear();
    ctx->execution_attempt_nonce.fill(0);
    ctx->preexecute_recorder.reset();
    return matched;
}

uint64_t ggml_backend_rpc_halofpx_mutable_graph_uid(ggml_cgraph * graph) {
    if (graph && graph->uid == 0) graph->uid = ggml_graph_next_uid();
    return graph ? graph->uid : 0;
}

bool ggml_backend_rpc_halofpx_mutable_begin(
        ggml_backend_t backend,
        const struct ggml_backend_sched_authority_prepared_admission * scheduler_admission,
        const struct ggml_backend_sched_authority_prepared_admission * expected_admission,
        const ggml_backend_rpc_halofpx_mutable_attempt * attempt,
        ggml_backend_rpc_halofpx_mutable_session * handle) {
    if (!hfx_mutable_requested() || !ggml_backend_is_rpc(backend)) return false;
    auto * ctx = static_cast<ggml_backend_rpc_context *>(backend->context);
    const auto refuse = [&](uint32_t reason) {
        GGML_LOG_ERROR("HaloFPX mutable activation refusal: reason=%u\n", reason);
        return hfx_preexecute_emit(
            ctx->preexecute_recorder, HFX_PREEXEC_L44_BEGIN, reason) && false;
    };
    if (attempt == nullptr || handle == nullptr ||
        attempt->version != HFX_MUTABLE_MAJOR || attempt->reserved != 0 ||
        attempt->graph_uid == 0 || attempt->execution_sequence == 0 ||
        attempt->max_mutations == 0 || attempt->max_mutations > HFX_MUTABLE_MAX_MUTATIONS ||
        attempt->max_census_entries == 0 || attempt->max_census_entries > HFX_MUTABLE_MAX_CENSUS ||
        hfx_zero(attempt->attempt_nonce, 32)) {
        return refuse(HFX_PREEXEC_INVALID_ARGUMENT);
    }
    if (scheduler_admission == nullptr || expected_admission == nullptr ||
        scheduler_admission->major != 3 || scheduler_admission->minor != 0 ||
        scheduler_admission->capabilities != HFX_MUTABLE_CAPABILITIES) {
        return refuse(HFX_PREEXEC_PREPARED_MISSING);
    }
    ggml_backend_rpc_halofpx_mutable_attempt admitted = *attempt;
    admitted.scheduler_execution_sequence = scheduler_admission->execution_sequence;
    memcpy(admitted.scheduler_attempt_nonce, scheduler_admission->attempt_nonce, 32);
    memcpy(admitted.scheduler_transcript_root, scheduler_admission->prepared_root, 32);
    if (!ctx->execution_armed || ctx->execution_mutable_begin_consumed ||
        ctx->execution_sequence != attempt->execution_sequence ||
        !hfx_equal(ctx->execution_attempt_nonce.data(), attempt->attempt_nonce, 32)) {
        return refuse(HFX_PREEXEC_NOT_ARMED);
    }
    auto sock = get_socket(ctx->endpoint);
    hfx_digest key {};
    if (!sock || !hfx_graph_key(key)) {
        return refuse(HFX_PREEXEC_SOCKET_MISSING);
    }
    if (!ggml_backend_sched_authority_verify_prepared_admission(
            scheduler_admission, key.data(), expected_admission) ||
        scheduler_admission->execution_sequence != attempt->execution_sequence ||
        scheduler_admission->parent_graph_uid != attempt->graph_uid ||
        scheduler_admission->backend_ordinal != ctx->execution_backend_ordinal ||
        scheduler_admission->client_connection_epoch != sock->authority_epoch() ||
        !hfx_equal(scheduler_admission->attempt_nonce,
                   attempt->attempt_nonce, 32) ||
        !hfx_equal(scheduler_admission->split_mapping_root,
                   ctx->execution_split_mapping_root.data(), 32)) {
        return refuse(HFX_PREEXEC_PREPARED_MISMATCH);
    }
    if (ctx->preexecute_recorder &&
        !hfx_preexecute_install_expected_census(
            ctx->preexecute_recorder, *scheduler_admission)) {
        return refuse(HFX_PREEXEC_PREPARED_MISMATCH);
    }
    {
        std::lock_guard<std::mutex> lock(hfx_mutable_mutex);
        if (hfx_mutable_active_sockets.find(sock.get()) != hfx_mutable_active_sockets.end()) {
            return refuse(HFX_PREEXEC_SESSION_CONFLICT);
        }
    }
    hfx_mutable_caps request {};
    hfx_set_magic(request.magic, "HFXMAQ3\0");
    request.major = HFX_MUTABLE_MAJOR; request.minor = HFX_MUTABLE_MINOR;
    request.encoded_size = sizeof(request);
    request.capabilities = HFX_MUTABLE_CAPABILITIES;
    request.graph_auth_major = HFX_GRAPH_AUTH_MAJOR;
    request.scheduler_admission_major = scheduler_admission->major;
    request.mutable_major = HFX_MUTABLE_MAJOR;
    request.max_mutations = admitted.max_mutations;
    request.max_census = admitted.max_census_entries;
    request.graph_uid = admitted.graph_uid;
    request.exec_sequence = admitted.execution_sequence;
    request.scheduler_sequence = admitted.scheduler_execution_sequence;
    request.client_connection_epoch = sock->authority_epoch();
    request.server_connection_epoch = scheduler_admission->server_connection_epoch;
    request.allocation_epoch = scheduler_admission->allocation_topology_epoch;
    memcpy(request.attempt_nonce, admitted.attempt_nonce, 32);
    memcpy(request.scheduler_nonce, admitted.scheduler_attempt_nonce, 32);
    memcpy(request.scheduler_root, admitted.scheduler_transcript_root, 32);
    const auto expected_admission_digest = hfx_sha256(
        expected_admission,
        offsetof(struct ggml_backend_sched_authority_prepared_admission, tag));
    memcpy(request.expected_admission_digest,
           expected_admission_digest.data(), expected_admission_digest.size());
    request.scheduler_expected_admission = *expected_admission;
    request.scheduler_admission = *scheduler_admission;
    if (!hfx_mutable_sign(request, key.data())) {
        return refuse(HFX_PREEXEC_CAPS_REFUSED);
    }
    const auto wire = hfx_mutable_encode(request);
    std::array<uint8_t, sizeof(hfx_mutable_caps)> response_wire {};
    hfx_mutable_caps response {};
    if (!send_rpc_cmd(sock, RPC_CMD_HALOFPX_MUTABLE_CAPS, wire.data(), wire.size(),
                      response_wire.data(), response_wire.size())) {
        return refuse(HFX_PREEXEC_CAPS_REFUSED);
    }
    if (!hfx_mutable_decode(response_wire.data(), response_wire.size(), response)) {
        return refuse(HFX_PREEXEC_DECODE_REFUSED);
    }
    const bool receipt_refused =
        !hfx_magic(response.magic, "HFXMAR3\0") ||
        response.major != HFX_MUTABLE_MAJOR || response.minor != HFX_MUTABLE_MINOR ||
        response.encoded_size != sizeof(response) || response.status != 1 ||
        response.max_mutations != admitted.max_mutations ||
        response.max_census != admitted.max_census_entries ||
        response.capabilities != HFX_MUTABLE_CAPABILITIES ||
        response.graph_auth_major != HFX_GRAPH_AUTH_MAJOR ||
        response.scheduler_admission_major != 3 ||
        response.mutable_major != HFX_MUTABLE_MAJOR ||
        response.version_reserved != 0 ||
        response.graph_uid != admitted.graph_uid ||
        response.exec_sequence != admitted.execution_sequence ||
        response.scheduler_sequence != admitted.scheduler_execution_sequence ||
        response.client_connection_epoch != request.client_connection_epoch ||
        response.server_connection_epoch != scheduler_admission->server_connection_epoch ||
        response.allocation_epoch != scheduler_admission->allocation_topology_epoch ||
        !hfx_equal(response.attempt_nonce, admitted.attempt_nonce, 32) ||
        !hfx_equal(response.scheduler_nonce, admitted.scheduler_attempt_nonce, 32) ||
        !hfx_equal(response.scheduler_root, admitted.scheduler_transcript_root, 32) ||
        !hfx_equal(response.expected_admission_digest,
                   request.expected_admission_digest, 32) ||
        std::memcmp(&response.scheduler_expected_admission, expected_admission,
                    sizeof(*expected_admission)) != 0 ||
        std::memcmp(&response.scheduler_admission, scheduler_admission,
                    sizeof(*scheduler_admission)) != 0 ||
        hfx_zero(response.server_nonce, 32) ||
        !hfx_mutable_verify(response, key.data());
    if (receipt_refused) {
        return refuse(HFX_PREEXEC_RECEIPT_REFUSED);
    }
    ctx->execution_mutable_begin_consumed = true;
    std::lock_guard<std::mutex> lock(hfx_mutable_mutex);
    if (hfx_mutable_active_sockets.find(sock.get()) != hfx_mutable_active_sockets.end()) {
        return false;
    }
    auto session = std::make_shared<hfx_mutable_client_session>();
    session->session_id = hfx_mutable_next_session.fetch_add(1, std::memory_order_relaxed);
    if (session->session_id == 0) session->session_id = hfx_mutable_next_session.fetch_add(1, std::memory_order_relaxed);
    session->generation = session->session_id;
    session->connection_epoch = response.server_connection_epoch;
    session->client_connection_epoch = response.client_connection_epoch;
    session->allocation_epoch = response.allocation_epoch;
    session->bound_backend_ordinal = scheduler_admission->backend_ordinal;
    session->sock = sock;
    session->endpoint = ctx->endpoint;
    session->attempt = admitted;
    session->admission = *scheduler_admission;
    session->expected_admission = *expected_admission;
    session->key = key;
    memcpy(session->server_nonce.data(), response.server_nonce, 32);
    session->mutation_root = hfx_sha256(HFX_MUTABLE_DOMAIN, sizeof(HFX_MUTABLE_DOMAIN) - 1);
    session->semantic_root = session->mutation_root;
    session->negotiated = true;
    session->recorder = ctx->preexecute_recorder;
    if (session->recorder) {
        session->recorder->server_connection_epoch = session->connection_epoch;
        session->recorder->allocation_epoch = session->allocation_epoch;
    }
    const uint64_t session_id = session->session_id;
    hfx_mutable_sessions.emplace(session_id, session);
    hfx_mutable_active_sockets.emplace(sock.get(), session_id);
    memset(handle, 0, sizeof(*handle));
    handle->version = HFX_MUTABLE_MAJOR;
    handle->session_id = session_id;
    handle->generation = session_id;
    handle->connection_epoch = session->connection_epoch;
    handle->allocation_epoch = session->allocation_epoch;
    handle->graph_uid = admitted.graph_uid;
    handle->execution_sequence = admitted.execution_sequence;
    memcpy(handle->attempt_nonce, admitted.attempt_nonce, 32);
    if (!hfx_preexecute_emit(
            session->recorder, HFX_PREEXEC_L44_BEGIN, HFX_PREEXEC_OK,
            HFX_TRANSPORT_NOT_ATTEMPTED, static_cast<uint8_t>(RPC_CMD_HALOFPX_MUTABLE_CAPS))) {
        hfx_mutable_close_session_locked(session_id);
        memset(handle, 0, sizeof(*handle));
        return false;
    }
    return true;
}

bool ggml_backend_rpc_halofpx_mutable_negotiate_preflight(
        ggml_backend_t backend,
        uint64_t key_generation,
        ggml_backend_rpc_halofpx_mutable_preflight * preflight) {
    if (!hfx_mutable_requested() || !ggml_backend_is_rpc(backend) ||
        key_generation == 0 || preflight == nullptr) {
        GGML_LOG_ERROR("HaloFPX mutable preflight: invalid entry runtime=%d rpc=%d keygen=%llu output=%d\n",
            hfx_mutable_requested() ? 1 : 0, ggml_backend_is_rpc(backend) ? 1 : 0,
            static_cast<unsigned long long>(key_generation), preflight != nullptr ? 1 : 0);
        return false;
    }
    auto * ctx = static_cast<ggml_backend_rpc_context *>(backend->context);
    auto sock = get_socket(ctx->endpoint);
    hfx_digest key {};
    if (!sock || !hfx_graph_key(key)) {
        GGML_LOG_ERROR("HaloFPX mutable preflight: socket/key socket=%d key=%d\n",
            sock ? 1 : 0, hfx_graph_key(key) ? 1 : 0);
        return false;
    }
    hfx_mutable_preflight_wire request {};
    hfx_set_magic(request.magic, "HFXMPQ3\0");
    request.major = HFX_MUTABLE_MAJOR;
    request.minor = HFX_MUTABLE_MINOR;
    request.encoded_size = sizeof(request);
    request.capabilities = HFX_MUTABLE_CAPABILITIES;
    request.key_generation = key_generation;
    request.client_connection_epoch = sock->authority_epoch();
    if (!hfx_random_all(request.client_nonce, 32)) {
        GGML_LOG_ERROR("HaloFPX mutable preflight: nonce generation\n");
        return false;
    }
    if (!hfx_mutable_sign(request, key.data())) {
        GGML_LOG_ERROR("HaloFPX mutable preflight: request signing size=%zu encoded=%zu\n",
            sizeof(request), hfx_mutable_encode(request, false).size());
        return false;
    }
    const auto request_wire = hfx_mutable_encode(request);
    std::array<uint8_t, sizeof(hfx_mutable_preflight_wire)> response_wire {};
    hfx_mutable_preflight_wire response {};
    if (!send_rpc_cmd(sock, RPC_CMD_HALOFPX_MUTABLE_PREFLIGHT,
                      request_wire.data(), request_wire.size(),
                      response_wire.data(), response_wire.size())) {
        GGML_LOG_ERROR("HaloFPX mutable preflight: transport/refusal\n");
        return false;
    }
    if (!hfx_mutable_decode(response_wire.data(), response_wire.size(), response)) {
        GGML_LOG_ERROR("HaloFPX mutable preflight: response decode\n");
        return false;
    }
    if (
        !hfx_magic(response.magic, "HFXMPR3\0") ||
        response.major != HFX_MUTABLE_MAJOR ||
        response.minor != HFX_MUTABLE_MINOR ||
        response.encoded_size != sizeof(response) || response.status != 1 ||
        response.reserved != 0 ||
        response.capabilities != HFX_MUTABLE_CAPABILITIES ||
        response.key_generation != key_generation ||
        response.client_connection_epoch != request.client_connection_epoch ||
        response.server_connection_epoch == 0 ||
        response.allocation_topology_epoch == 0 ||
        !hfx_equal(response.client_nonce, request.client_nonce, 32) ||
        hfx_zero(response.server_nonce, 32) ||
        !hfx_mutable_verify(response, key.data())) {
        GGML_LOG_ERROR("HaloFPX mutable preflight: authenticated response validation\n");
        return false;
    }
    memset(preflight, 0, sizeof(*preflight));
    preflight->major = response.major;
    preflight->minor = response.minor;
    preflight->encoded_size = sizeof(*preflight);
    preflight->capabilities = response.capabilities;
    preflight->key_generation = response.key_generation;
    preflight->client_connection_epoch = response.client_connection_epoch;
    preflight->server_connection_epoch = response.server_connection_epoch;
    preflight->allocation_topology_epoch = response.allocation_topology_epoch;
    memcpy(preflight->client_nonce, response.client_nonce, 32);
    memcpy(preflight->server_nonce, response.server_nonce, 32);
    memcpy(preflight->receipt_tag, response.tag, 32);
    return true;
}

ggml_backend_rpc_halofpx_mutable_admit_result
ggml_backend_rpc_halofpx_mutable_register_typed(
        const ggml_backend_rpc_halofpx_mutable_session * handle,
        ggml_tensor * tensor,
        ggml_backend_rpc_halofpx_mutable_role role,
        uint32_t role_ordinal,
        const ggml_backend_sched_authority_census_entry * projection) {
    if (!hfx_mutable_requested() || tensor == nullptr ||
        !hfx_mutable_role_valid(role)) {
        return GGML_RPC_HALOFPX_MUTABLE_ADMIT_INVALID_ARGUMENT;
    }
    std::shared_ptr<hfx_mutable_client_session> session;
    {
        std::lock_guard<std::mutex> lock(hfx_mutable_mutex);
        session = hfx_mutable_lookup_locked(handle);
    }
    if (session == nullptr) return GGML_RPC_HALOFPX_MUTABLE_ADMIT_SESSION_MISSING;
    std::lock_guard<std::mutex> session_lock(session->mutex);
    if (session->exclusions.find(tensor) != session->exclusions.end()) {
        return hfx_preexecute_emit(
            session->recorder, HFX_PREEXEC_L44_REGISTER,
            HFX_PREEXEC_DUPLICATE_ROLE) ?
            GGML_RPC_HALOFPX_MUTABLE_ADMIT_ALREADY_REGISTERED :
            GGML_RPC_HALOFPX_MUTABLE_ADMIT_RECORDER_FAILURE;
    }
    ggml_tensor * storage = tensor;
    std::unordered_set<const ggml_tensor *> ancestry;
    while (storage->buffer == nullptr && storage->view_src != nullptr) {
        if (!ancestry.insert(storage).second) {
            return GGML_RPC_HALOFPX_MUTABLE_ADMIT_VIEW_CYCLE;
        }
        storage = storage->view_src;
    }
    if (storage->buffer == nullptr) {
        return hfx_preexecute_emit(
            session->recorder, HFX_PREEXEC_L44_REGISTER,
            HFX_PREEXEC_WRONG_CONNECTION) ?
            GGML_RPC_HALOFPX_MUTABLE_ADMIT_STORAGE_ABSENT :
            GGML_RPC_HALOFPX_MUTABLE_ADMIT_RECORDER_FAILURE;
    }
    if (!ggml_backend_buffer_is_rpc(storage->buffer)) {
        return hfx_preexecute_emit(
            session->recorder, HFX_PREEXEC_L44_REGISTER,
            HFX_PREEXEC_WRONG_CONNECTION) ?
            GGML_RPC_HALOFPX_MUTABLE_ADMIT_STORAGE_NOT_RPC :
            GGML_RPC_HALOFPX_MUTABLE_ADMIT_RECORDER_FAILURE;
    }
    auto * bctx = static_cast<ggml_backend_rpc_buffer_context *>(storage->buffer->context);
    if (bctx->sock.get() != session->sock.get()) {
        return hfx_preexecute_emit(
            session->recorder, HFX_PREEXEC_L44_REGISTER,
            HFX_PREEXEC_WRONG_CONNECTION) ?
            GGML_RPC_HALOFPX_MUTABLE_ADMIT_WRONG_SOCKET :
            GGML_RPC_HALOFPX_MUTABLE_ADMIT_RECORDER_FAILURE;
    }
    if (projection != nullptr &&
        (projection->resolved != 1 || projection->runtime_tensor != tensor ||
         projection->disposition != GGML_BACKEND_SCHED_CENSUS_REGISTER ||
         projection->role != static_cast<uint32_t>(role) ||
         projection->role_ordinal != role_ordinal ||
         projection->destination_backend_ordinal != session->bound_backend_ordinal ||
         projection->rpc_connection_epoch != session->client_connection_epoch ||
         projection->rpc_device == UINT32_MAX ||
         hfx_zero(projection->logical_tensor_identity, 32) ||
         hfx_zero(projection->storage_tensor_identity, 32) ||
         hfx_zero(projection->runtime_semantic_identity, 32) ||
         hfx_zero(projection->rpc_endpoint_identity, 32))) {
        return GGML_RPC_HALOFPX_MUTABLE_ADMIT_CONFLICTING_DUPLICATE;
    }
    hfx_mutable_registration value {};
    value.role = static_cast<uint32_t>(role);
    value.role_ordinal = role_ordinal;
    if (projection != nullptr) {
        memcpy(value.logical_tensor_identity.data(), projection->logical_tensor_identity, 32);
        memcpy(value.storage_tensor_identity.data(), projection->storage_tensor_identity, 32);
        memcpy(value.runtime_semantic_identity.data(), projection->runtime_semantic_identity, 32);
        memcpy(value.rpc_endpoint_identity.data(), projection->rpc_endpoint_identity, 32);
        value.rpc_connection_epoch = projection->rpc_connection_epoch;
        value.destination_backend_ordinal = projection->destination_backend_ordinal;
        value.rpc_device = projection->rpc_device;
    }
    auto [it, inserted] = session->roles.emplace(tensor, value);
    if (!inserted) {
        return hfx_preexecute_emit(
            session->recorder, HFX_PREEXEC_L44_REGISTER,
            HFX_PREEXEC_DUPLICATE_ROLE) ?
            (hfx_mutable_registration_equal(it->second, value) ?
                GGML_RPC_HALOFPX_MUTABLE_ADMIT_EXACT_DUPLICATE :
                GGML_RPC_HALOFPX_MUTABLE_ADMIT_CONFLICTING_DUPLICATE) :
            GGML_RPC_HALOFPX_MUTABLE_ADMIT_RECORDER_FAILURE;
    }
    session->projected_census.emplace_back(tensor, value);
    return hfx_preexecute_emit(
        session->recorder, HFX_PREEXEC_L44_REGISTER,
        HFX_PREEXEC_OK, HFX_TRANSPORT_NOT_ATTEMPTED, 0, 0, UINT32_MAX,
        role_ordinal, role_ordinal) ?
        GGML_RPC_HALOFPX_MUTABLE_ADMIT_SUCCESS :
        GGML_RPC_HALOFPX_MUTABLE_ADMIT_RECORDER_FAILURE;
}

bool ggml_backend_rpc_halofpx_mutable_register(
        const ggml_backend_rpc_halofpx_mutable_session * handle,
        ggml_tensor * tensor,
        ggml_backend_rpc_halofpx_mutable_role role,
        uint32_t role_ordinal) {
    return ggml_backend_rpc_halofpx_mutable_register_typed(
        handle, tensor, role, role_ordinal, nullptr) ==
        GGML_RPC_HALOFPX_MUTABLE_ADMIT_SUCCESS;
}

ggml_backend_rpc_halofpx_mutable_admit_result
ggml_backend_rpc_halofpx_resolve_storage_identity(
        ggml_backend_t destination_backend,
        ggml_tensor * tensor,
        ggml_backend_rpc_halofpx_storage_identity * identity) {
    if (!hfx_mutable_requested() || tensor == nullptr || identity == nullptr) {
        return GGML_RPC_HALOFPX_MUTABLE_ADMIT_INVALID_ARGUMENT;
    }
    if (!ggml_backend_is_rpc(destination_backend)) {
        return GGML_RPC_HALOFPX_MUTABLE_ADMIT_WRONG_DESTINATION_BACKEND;
    }
    std::unordered_set<const ggml_tensor *> ancestry;
    ggml_tensor * storage = nullptr;
    for (ggml_tensor * current = tensor; current != nullptr;
            current = current->view_src) {
        if (!ancestry.insert(current).second) {
            return GGML_RPC_HALOFPX_MUTABLE_ADMIT_VIEW_CYCLE;
        }
        if (current->buffer != nullptr) storage = current;
    }
    if (storage == nullptr || storage->buffer == nullptr) {
        return GGML_RPC_HALOFPX_MUTABLE_ADMIT_STORAGE_ABSENT;
    }
    if (!ggml_backend_buffer_is_rpc(storage->buffer)) {
        return GGML_RPC_HALOFPX_MUTABLE_ADMIT_STORAGE_NOT_RPC;
    }
    auto * backend_ctx =
        static_cast<ggml_backend_rpc_context *>(destination_backend->context);
    auto * buffer_ctx =
        static_cast<ggml_backend_rpc_buffer_context *>(storage->buffer->context);
    auto destination_socket = get_socket(backend_ctx->endpoint);
    if (destination_socket == nullptr) {
        return GGML_RPC_HALOFPX_MUTABLE_ADMIT_WRONG_ENDPOINT;
    }
    if (buffer_ctx == nullptr ||
        buffer_ctx->sock.get() != destination_socket.get()) {
        return GGML_RPC_HALOFPX_MUTABLE_ADMIT_WRONG_SOCKET;
    }
    if (buffer_ctx->device != backend_ctx->device) {
        return GGML_RPC_HALOFPX_MUTABLE_ADMIT_WRONG_DEVICE;
    }
    memset(identity, 0, sizeof(*identity));
    identity->version = 1;
    identity->device = buffer_ctx->device;
    identity->connection_epoch = destination_socket->authority_epoch();
    std::vector<uint8_t> endpoint_material;
    hfx_le<uint32_t>(endpoint_material, backend_ctx->device);
    endpoint_material.insert(
        endpoint_material.end(), backend_ctx->endpoint.begin(),
        backend_ctx->endpoint.end());
    const auto endpoint_digest = hfx_sha256(
        endpoint_material.data(), endpoint_material.size());
    memcpy(identity->endpoint_identity, endpoint_digest.data(), 32);
    std::vector<uint8_t> storage_material;
    hfx_bytes(storage_material, identity->endpoint_identity, 32);
    hfx_le<uint64_t>(storage_material, identity->connection_epoch);
    hfx_le<uint64_t>(storage_material, buffer_ctx->remote_ptr);
    const uintptr_t base = reinterpret_cast<uintptr_t>(
        ggml_backend_buffer_get_base(storage->buffer));
    const uintptr_t data = reinterpret_cast<uintptr_t>(storage->data);
    if (data < base) return GGML_RPC_HALOFPX_MUTABLE_ADMIT_STORAGE_ABSENT;
    hfx_le<uint64_t>(storage_material, static_cast<uint64_t>(data - base));
    const auto storage_digest = hfx_sha256(
        storage_material.data(), storage_material.size());
    memcpy(identity->storage_identity, storage_digest.data(), 32);
    uint64_t logical_address = reinterpret_cast<uint64_t>(tensor->data);
    if (logical_address == 0) {
        uint64_t view_offset = 0;
        for (const ggml_tensor * current = tensor;
                current != storage && current != nullptr;
                current = current->view_src) {
            if (!hfx_add(view_offset, current->view_offs, view_offset)) {
                return GGML_RPC_HALOFPX_MUTABLE_ADMIT_INVALID_ARGUMENT;
            }
        }
        if (!hfx_add(
                reinterpret_cast<uint64_t>(storage->data),
                view_offset, logical_address)) {
            return GGML_RPC_HALOFPX_MUTABLE_ADMIT_INVALID_ARGUMENT;
        }
    }
    if (logical_address < base) {
        return GGML_RPC_HALOFPX_MUTABLE_ADMIT_INVALID_ARGUMENT;
    }
    std::vector<uint8_t> semantic_material;
    hfx_bytes(semantic_material, identity->storage_identity, 32);
    hfx_le<uint64_t>(semantic_material, logical_address - base);
    hfx_le<uint32_t>(semantic_material, static_cast<uint32_t>(tensor->type));
    hfx_le<uint64_t>(semantic_material, ggml_nbytes(tensor));
    for (size_t i = 0; i < GGML_MAX_DIMS; ++i) {
        if (tensor->ne[i] <= 0) {
            return GGML_RPC_HALOFPX_MUTABLE_ADMIT_INVALID_ARGUMENT;
        }
        hfx_le<uint64_t>(semantic_material, static_cast<uint64_t>(tensor->ne[i]));
        hfx_le<uint64_t>(semantic_material, static_cast<uint64_t>(tensor->nb[i]));
    }
    const auto semantic_digest = hfx_sha256(
        semantic_material.data(), semantic_material.size());
    memcpy(identity->runtime_semantic_identity, semantic_digest.data(), 32);
    return GGML_RPC_HALOFPX_MUTABLE_ADMIT_SUCCESS;
}

ggml_backend_rpc_halofpx_mutable_admit_result
ggml_backend_rpc_halofpx_mutable_exclude_typed(
        const ggml_backend_rpc_halofpx_mutable_session * handle,
        ggml_tensor * tensor,
        ggml_backend_rpc_halofpx_exclusion exclusion,
        uint32_t exclusion_ordinal,
        const ggml_backend_sched_authority_census_entry * projection) {
    if (!hfx_mutable_requested()) {
        return GGML_RPC_HALOFPX_MUTABLE_ADMIT_INVALID_ARGUMENT;
    }
    if (tensor == nullptr ||
        (exclusion != GGML_RPC_HALOFPX_EXCLUDE_IMMUTABLE_MODEL_WEIGHT &&
         exclusion != GGML_RPC_HALOFPX_EXCLUDE_LOCAL_STATE_PAYLOAD)) {
        return GGML_RPC_HALOFPX_MUTABLE_ADMIT_INVALID_ARGUMENT;
    }
    std::shared_ptr<hfx_mutable_client_session> session;
    {
        std::lock_guard<std::mutex> lock(hfx_mutable_mutex);
        session = hfx_mutable_lookup_locked(handle);
    }
    if (session == nullptr) return GGML_RPC_HALOFPX_MUTABLE_ADMIT_SESSION_MISSING;
    std::lock_guard<std::mutex> session_lock(session->mutex);
    if (session->roles.find(tensor) != session->roles.end()) {
        return hfx_preexecute_emit(
            session->recorder, HFX_PREEXEC_L44_EXCLUDE,
            HFX_PREEXEC_DUPLICATE_ROLE) ?
            GGML_RPC_HALOFPX_MUTABLE_ADMIT_ALREADY_REGISTERED :
            GGML_RPC_HALOFPX_MUTABLE_ADMIT_RECORDER_FAILURE;
    }
    ggml_tensor * storage = tensor;
    std::unordered_set<const ggml_tensor *> ancestry;
    while (storage->buffer == nullptr && storage->view_src != nullptr) {
        if (!ancestry.insert(storage).second) {
            return GGML_RPC_HALOFPX_MUTABLE_ADMIT_VIEW_CYCLE;
        }
        storage = storage->view_src;
    }
    if (storage->buffer == nullptr) {
        return hfx_preexecute_emit(
            session->recorder, HFX_PREEXEC_L44_EXCLUDE,
            HFX_PREEXEC_WRONG_CONNECTION) ?
            GGML_RPC_HALOFPX_MUTABLE_ADMIT_STORAGE_ABSENT :
            GGML_RPC_HALOFPX_MUTABLE_ADMIT_RECORDER_FAILURE;
    }
    if (!ggml_backend_buffer_is_rpc(storage->buffer)) {
        return hfx_preexecute_emit(
            session->recorder, HFX_PREEXEC_L44_EXCLUDE,
            HFX_PREEXEC_WRONG_CONNECTION) ?
            GGML_RPC_HALOFPX_MUTABLE_ADMIT_STORAGE_NOT_RPC :
            GGML_RPC_HALOFPX_MUTABLE_ADMIT_RECORDER_FAILURE;
    }
    auto * bctx = static_cast<ggml_backend_rpc_buffer_context *>(storage->buffer->context);
    if (bctx->sock.get() != session->sock.get()) {
        return hfx_preexecute_emit(
            session->recorder, HFX_PREEXEC_L44_EXCLUDE,
            HFX_PREEXEC_WRONG_CONNECTION) ?
            GGML_RPC_HALOFPX_MUTABLE_ADMIT_WRONG_SOCKET :
            GGML_RPC_HALOFPX_MUTABLE_ADMIT_RECORDER_FAILURE;
    }
    if (projection != nullptr &&
        (projection->resolved != 1 || projection->runtime_tensor != tensor ||
         projection->disposition != GGML_BACKEND_SCHED_CENSUS_EXCLUDE ||
         projection->role != static_cast<uint32_t>(exclusion) ||
         projection->role_ordinal != exclusion_ordinal ||
         projection->destination_backend_ordinal != session->bound_backend_ordinal ||
         projection->rpc_connection_epoch != session->client_connection_epoch ||
         projection->rpc_device == UINT32_MAX ||
         hfx_zero(projection->logical_tensor_identity, 32) ||
         hfx_zero(projection->storage_tensor_identity, 32) ||
         hfx_zero(projection->runtime_semantic_identity, 32) ||
         hfx_zero(projection->rpc_endpoint_identity, 32))) {
        return GGML_RPC_HALOFPX_MUTABLE_ADMIT_CONFLICTING_DUPLICATE;
    }
    hfx_mutable_registration value {};
    value.role = HFX_MUTABLE_EXCLUSION_BASE + static_cast<uint32_t>(exclusion);
    value.role_ordinal = exclusion_ordinal;
    if (projection != nullptr) {
        memcpy(value.logical_tensor_identity.data(), projection->logical_tensor_identity, 32);
        memcpy(value.storage_tensor_identity.data(), projection->storage_tensor_identity, 32);
        memcpy(value.runtime_semantic_identity.data(), projection->runtime_semantic_identity, 32);
        memcpy(value.rpc_endpoint_identity.data(), projection->rpc_endpoint_identity, 32);
        value.rpc_connection_epoch = projection->rpc_connection_epoch;
        value.destination_backend_ordinal = projection->destination_backend_ordinal;
        value.rpc_device = projection->rpc_device;
    }
    auto [it, inserted] = session->exclusions.emplace(tensor, value);
    const bool exact =
        !inserted && hfx_mutable_registration_equal(it->second, value);
    const auto result = inserted ? GGML_RPC_HALOFPX_MUTABLE_ADMIT_SUCCESS :
        exact ? GGML_RPC_HALOFPX_MUTABLE_ADMIT_EXACT_DUPLICATE :
                GGML_RPC_HALOFPX_MUTABLE_ADMIT_CONFLICTING_DUPLICATE;
    if (inserted) session->projected_census.emplace_back(tensor, value);
    return hfx_preexecute_emit(
        session->recorder, HFX_PREEXEC_L44_EXCLUDE,
        inserted || exact ? HFX_PREEXEC_OK : HFX_PREEXEC_DUPLICATE_ROLE,
        HFX_TRANSPORT_NOT_ATTEMPTED, 0, 0, UINT32_MAX,
        exclusion_ordinal, exclusion_ordinal) ? result :
        GGML_RPC_HALOFPX_MUTABLE_ADMIT_RECORDER_FAILURE;
}

bool ggml_backend_rpc_halofpx_mutable_exclude(
        const ggml_backend_rpc_halofpx_mutable_session * handle,
        ggml_tensor * tensor,
        ggml_backend_rpc_halofpx_exclusion exclusion,
        uint32_t exclusion_ordinal) {
    const auto result = ggml_backend_rpc_halofpx_mutable_exclude_typed(
        handle, tensor, exclusion, exclusion_ordinal, nullptr);
    return result == GGML_RPC_HALOFPX_MUTABLE_ADMIT_SUCCESS ||
        result == GGML_RPC_HALOFPX_MUTABLE_ADMIT_EXACT_DUPLICATE;
}

static void hfx_mutable_collect_graph(
        ggml_tensor * tensor,
        std::unordered_set<ggml_tensor *> & seen,
        std::vector<ggml_tensor *> & leaves) {
    if (tensor == nullptr || !seen.insert(tensor).second) return;
    if (tensor->view_src) hfx_mutable_collect_graph(tensor->view_src, seen, leaves);
    bool has_src = false;
    for (ggml_tensor * src : tensor->src) {
        if (src) { has_src = true; hfx_mutable_collect_graph(src, seen, leaves); }
    }
    if (!has_src) leaves.push_back(tensor);
}

static bool hfx_mutable_client_bind(
        hfx_mutable_client_session & session,
        uint64_t graph_uid,
        uint64_t parent_graph_uid,
        uint32_t split_ordinal,
        uint32_t backend_ordinal,
        const uint8_t split_mapping_root[32]) {
    if (!session.prepared || graph_uid == 0 || parent_graph_uid == 0 ||
        split_ordinal == UINT32_MAX || backend_ordinal == UINT32_MAX ||
        split_mapping_root == nullptr || hfx_zero(split_mapping_root, 32)) return false;
    hfx_mutable_commit_header request {};
    hfx_set_magic(request.magic, "HFXMCB1\0");
    request.major = HFX_MUTABLE_MAJOR; request.minor = HFX_MUTABLE_MINOR;
    request.encoded_size = sizeof(request);
    request.census_count = session.census_count;
    request.mutation_count = session.mutation_sequence;
    request.graph_uid = graph_uid;
    request.exec_sequence = session.attempt.execution_sequence;
    request.parent_graph_uid = parent_graph_uid;
    request.split_ordinal = split_ordinal;
    request.backend_ordinal = backend_ordinal;
    memcpy(request.attempt_nonce, session.attempt.attempt_nonce, 32);
    memcpy(request.server_nonce, session.server_nonce.data(), 32);
    memcpy(request.scheduler_nonce, session.attempt.scheduler_attempt_nonce, 32);
    memcpy(request.scheduler_root, session.attempt.scheduler_transcript_root, 32);
    memcpy(request.mutation_root, session.mutation_root.data(), 32);
    memcpy(request.split_mapping_root, split_mapping_root, 32);
    if (!hfx_mutable_sign(request, session.key.data())) return false;
    const auto wire = hfx_mutable_encode(request);
    std::array<uint8_t, sizeof(hfx_mutable_commit_header)> response_wire {};
    hfx_mutable_commit_header response {};
    if (!send_rpc_cmd(session.sock, RPC_CMD_HALOFPX_MUTABLE_BIND,
                      wire.data(), wire.size(), response_wire.data(), response_wire.size()) ||
        !hfx_mutable_decode(response_wire.data(), response_wire.size(), response) ||
        !hfx_magic(response.magic, "HFXMBR1\0") ||
        response.graph_uid != graph_uid || response.exec_sequence != request.exec_sequence ||
        response.parent_graph_uid != parent_graph_uid ||
        response.split_ordinal != split_ordinal ||
        response.backend_ordinal != backend_ordinal ||
        !hfx_equal(response.split_mapping_root, split_mapping_root, 32) ||
        !hfx_equal(response.mutation_root, request.mutation_root, 32) ||
        !hfx_mutable_verify(response, session.key.data())) return false;
    session.bound_graph_uid = graph_uid;
    session.bound_parent_graph_uid = parent_graph_uid;
    session.bound_split_ordinal = split_ordinal;
    session.bound_backend_ordinal = backend_ordinal;
    memcpy(session.split_mapping_root.data(), split_mapping_root, 32);
    return true;
}

static bool hfx_mutable_client_finish(
        const ggml_backend_rpc_halofpx_mutable_session * handle,
        ggml_cgraph * graph,
        ggml_backend_rpc_halofpx_mutable_result * result,
        bool prepare) {
    if (result != nullptr) memset(result, 0, sizeof(*result));
    if (!hfx_mutable_requested() || graph == nullptr || result == nullptr) return false;
    std::shared_ptr<hfx_mutable_client_session> session_ptr;
    {
        std::lock_guard<std::mutex> lock(hfx_mutable_mutex);
        session_ptr = hfx_mutable_lookup_locked(handle);
    }
    if (session_ptr == nullptr) return false;
    std::lock_guard<std::mutex> session_lock(session_ptr->mutex);
    auto & session = *session_ptr;
    auto sock = session.sock;
    std::vector<hfx_mutable_census_entry> census;
    std::unordered_set<uint64_t> role_keys;
    if (session.projected_census.size() !=
            session.roles.size() + session.exclusions.size()) {
        session.failed = true;
        return false;
    }
    for (const auto & projection : session.projected_census) {
        ggml_tensor * tensor = const_cast<ggml_tensor *>(projection.first);
        const auto & authority = projection.second;
        ggml_tensor * storage = tensor;
        uint64_t inherited_offset = 0;
        std::unordered_set<const ggml_tensor *> ancestry;
        while (storage->buffer == nullptr && storage->view_src != nullptr) {
            if (!ancestry.insert(storage).second) {
                session.failed = true; return false;
            }
            if (!hfx_add(inherited_offset, storage->view_offs, inherited_offset)) {
                session.failed = true; return false;
            }
            storage = storage->view_src;
        }
        if (storage->buffer == nullptr ||
            !ggml_backend_buffer_is_rpc(storage->buffer)) {
            session.failed = true; return false;
        }
        auto * bctx = static_cast<ggml_backend_rpc_buffer_context *>(storage->buffer->context);
        if (bctx->sock.get() != sock.get()) {
            session.failed = true; return false;
        }
        const uint64_t key = (uint64_t(authority.role) << 32) | authority.role_ordinal;
        if (!role_keys.insert(key).second || census.size() >= session.attempt.max_census_entries) {
            session.failed = true; return false;
        }
        hfx_mutable_census_entry entry {};
        entry.role = authority.role; entry.role_ordinal = authority.role_ordinal;
        memcpy(entry.logical_tensor_identity, authority.logical_tensor_identity.data(), 32);
        memcpy(entry.storage_tensor_identity, authority.storage_tensor_identity.data(), 32);
        memcpy(entry.runtime_semantic_identity, authority.runtime_semantic_identity.data(), 32);
        memcpy(entry.rpc_endpoint_identity, authority.rpc_endpoint_identity.data(), 32);
        entry.rpc_connection_epoch = authority.rpc_connection_epoch;
        entry.destination_backend_ordinal = authority.destination_backend_ordinal;
        entry.rpc_device = authority.rpc_device;
        entry.allocation_ordinal = bctx->allocation_ordinal; entry.type = tensor->type;
        const uint64_t base = reinterpret_cast<uint64_t>(ggml_backend_buffer_get_base(storage->buffer));
        uint64_t address = reinterpret_cast<uint64_t>(tensor->data);
        if (address == 0) {
            const uint64_t storage_address = reinterpret_cast<uint64_t>(storage->data);
            if (!hfx_add(storage_address, inherited_offset, address)) {
                session.failed = true; return false;
            }
        }
        if (address < base) { session.failed = true; return false; }
        entry.tensor_relative = address - base; entry.logical_size = ggml_nbytes(tensor);
        for (size_t i = 0; i < GGML_MAX_DIMS; ++i) {
            if (tensor->ne[i] <= 0 || uint64_t(tensor->ne[i]) > UINT32_MAX || tensor->nb[i] > UINT32_MAX) {
                session.failed = true; return false;
            }
            entry.ne[i] = tensor->ne[i]; entry.nb[i] = tensor->nb[i];
        }
        std::vector<uint8_t> view;
        for (const ggml_tensor * v = tensor; v; v = v->view_src) {
            hfx_le<uint64_t>(view, v->view_offs);
            if (view.size() > 8 * GGML_MAX_SRC) { session.failed = true; return false; }
        }
        const auto vd = hfx_sha256(view.data(), view.size());
        memcpy(entry.view_digest, vd.data(), 32);
        census.push_back(entry);
        GGML_LOG_INFO("[halofpx-mutable] census role=%u ordinal=%u allocation=%u relative=%" PRIu64
                      " bytes=%" PRIu64 "\n",
                      entry.role, entry.role_ordinal, entry.allocation_ordinal,
                      entry.tensor_relative, entry.logical_size);
    }
    if (census.empty()) { session.failed = true; return false; }
    uint32_t logical_mutable_count = 0;
    uint32_t logical_exclusion_count = 0;
    const auto logical_expected_census = hfx_mutable_logical_expected_census(
        census, logical_mutable_count, logical_exclusion_count);
    if (logical_mutable_count != session.admission.logical_expected_mutable_count ||
        logical_exclusion_count != session.admission.logical_expected_exclusion_count ||
        !hfx_equal(logical_expected_census.data(),
                   session.admission.logical_expected_census_root, 32)) {
        session.failed = true;
        return false;
    }
    std::vector<uint8_t> census_wire;
    for (const auto & entry : census) {
        const auto encoded = hfx_mutable_encode(entry);
        census_wire.insert(census_wire.end(), encoded.begin(), encoded.end());
    }
    const auto census_root = hfx_sha256(census_wire.data(), census_wire.size());
    hfx_mutable_commit_header request {};
    hfx_set_magic(request.magic, prepare ? "HFXMCP1\0" : "HFXMCC1\0");
    request.major = HFX_MUTABLE_MAJOR; request.minor = HFX_MUTABLE_MINOR;
    request.encoded_size = sizeof(request) + census_wire.size();
    request.census_count = census.size();
    request.mutation_count = session.mutation_sequence;
    request.graph_uid = session.bound_graph_uid != 0 ?
        session.bound_graph_uid : session.attempt.graph_uid;
    request.exec_sequence = session.attempt.execution_sequence;
    memcpy(request.attempt_nonce, session.attempt.attempt_nonce, 32);
    memcpy(request.server_nonce, session.server_nonce.data(), 32);
    memcpy(request.scheduler_nonce, session.attempt.scheduler_attempt_nonce, 32);
    memcpy(request.scheduler_root, session.attempt.scheduler_transcript_root, 32);
    memcpy(request.mutation_root, session.mutation_root.data(), 32);
    memcpy(request.census_root, census_root.data(), 32);
    if (!hfx_mutable_sign(request, session.key.data())) return false;
    const auto header_wire = hfx_mutable_encode(request);
    std::vector<uint8_t> wire = header_wire;
    wire.insert(wire.end(), census_wire.begin(), census_wire.end());
    std::array<uint8_t, sizeof(hfx_mutable_commit_header)> response_wire {};
    hfx_mutable_commit_header response {};
    if (!send_rpc_cmd(sock, RPC_CMD_HALOFPX_MUTABLE_COMMIT, wire.data(), wire.size(),
                      response_wire.data(), response_wire.size())) {
        result->status = 101; session.failed = true; return false;
    }
    if (!hfx_mutable_decode(response_wire.data(), response_wire.size(), response)) {
        result->status = 102; session.failed = true; return false;
    }
    if (!hfx_magic(response.magic, prepare ? "HFXMPR1\0" : "HFXMCR1\0")) {
        result->status = 103; session.failed = true; return false;
    }
    if (response.encoded_size != sizeof(response)) {
        result->status = 104; session.failed = true; return false;
    }
    if (response.census_count != request.census_count) {
        result->status = 105; session.failed = true; return false;
    }
    if (response.mutation_count != request.mutation_count) {
        result->status = 106; session.failed = true; return false;
    }
    if (response.graph_uid != request.graph_uid) {
        result->status = 107; session.failed = true; return false;
    }
    if (response.exec_sequence != request.exec_sequence) {
        result->status = 108; session.failed = true; return false;
    }
    if (!hfx_equal(response.mutation_root, request.mutation_root, 32)) {
        result->status = 109; session.failed = true; return false;
    }
    if (!hfx_equal(response.census_root, request.census_root, 32)) {
        result->status = 110; session.failed = true; return false;
    }
    if (!hfx_mutable_verify(response, session.key.data())) {
        result->status = 111; session.failed = true; return false;
    }
    memset(result, 0, sizeof(*result));
    result->version = HFX_MUTABLE_MAJOR; result->status = 1;
    result->mutation_count = request.mutation_count; result->census_count = request.census_count;
    result->set_count = session.set_count;
    result->set_hash_hit_count = session.set_hash_hit_count;
    result->set_hash_miss_count = session.set_hash_miss_count;
    result->graph_uid = request.graph_uid; result->execution_sequence = request.exec_sequence;
    memcpy(result->mutation_root, request.mutation_root, 32);
    memcpy(result->semantic_root, session.semantic_root.data(), 32);
    memcpy(result->census_root, request.census_root, 32);
    memcpy(result->receipt_tag, response.tag, 32);
    session.prepared = true;
    session.committed = !prepare;
    session.census_count = request.census_count;
    if (session.recorder) {
        if (!hfx_preexecute_emit(
                session.recorder,
                prepare ? HFX_PREEXEC_L44_PREPARE : HFX_PREEXEC_L44_COMMIT,
                HFX_PREEXEC_OK)) {
            session.failed = true;
            return false;
        }
    }
    return true;
}

bool ggml_backend_rpc_halofpx_mutable_prepare(
        const ggml_backend_rpc_halofpx_mutable_session * handle,
        ggml_cgraph * graph,
        ggml_backend_rpc_halofpx_mutable_result * result) {
    return hfx_mutable_client_finish(handle, graph, result, true);
}

bool ggml_backend_rpc_halofpx_mutable_commit(
        const ggml_backend_rpc_halofpx_mutable_session * handle,
        ggml_cgraph * graph,
        ggml_backend_rpc_halofpx_mutable_result * result) {
    return hfx_mutable_client_finish(handle, graph, result, false);
}

bool ggml_backend_rpc_halofpx_mutable_abort(ggml_backend_rpc_halofpx_mutable_session * handle) {
    if (handle == nullptr) return false;
    std::lock_guard<std::mutex> lock(hfx_mutable_mutex);
    auto session = hfx_mutable_lookup_locked(handle, true);
    if (session == nullptr) return false;
    if (!session->committed && session->recorder &&
        !hfx_preexecute_emit(
            session->recorder, HFX_PREEXEC_L44_ABORT, HFX_PREEXEC_POST_ABORT,
            HFX_TRANSPORT_NOT_ATTEMPTED, 0, 0, UINT32_MAX, 0, 0, 0, true)) {
        return false;
    }
    const uint64_t session_id = session->session_id;
    hfx_mutable_close_session_locked(session_id);
    memset(handle, 0, sizeof(*handle));
    return true;
}

bool ggml_backend_rpc_halofpx_graph_result_get(
        ggml_backend_t backend,
        ggml_backend_rpc_halofpx_graph_result * result) {
    ggml_backend_rpc_halofpx_graph_result_reason reason =
        GGML_RPC_HALOFPX_GRAPH_RESULT_OK;
    return ggml_backend_rpc_halofpx_graph_result_inspect(backend, result, &reason);
}

bool ggml_backend_rpc_halofpx_graph_result_inspect(
        ggml_backend_t backend,
        ggml_backend_rpc_halofpx_graph_result * result,
        ggml_backend_rpc_halofpx_graph_result_reason * reason) {
    if (reason == nullptr) return false;
    *reason = GGML_RPC_HALOFPX_GRAPH_RESULT_OK;
    if (backend == nullptr || result == nullptr) {
        *reason = GGML_RPC_HALOFPX_GRAPH_RESULT_INVALID_ARGUMENT;
        return false;
    }
    if (!ggml_backend_is_rpc(backend)) {
        *reason = GGML_RPC_HALOFPX_GRAPH_RESULT_NOT_RPC;
        return false;
    }
    const auto * context = static_cast<const ggml_backend_rpc_context *>(backend->context);
    if (context == nullptr) {
        *reason = GGML_RPC_HALOFPX_GRAPH_RESULT_CONTEXT_MISSING;
        return false;
    }
    if (context->graph_auth_result.status != 2) {
        *reason = GGML_RPC_HALOFPX_GRAPH_RESULT_STATUS_NOT_EXECUTED;
        return false;
    }
    if (context->graph_auth_result.graph_uid == 0) {
        *reason = GGML_RPC_HALOFPX_GRAPH_RESULT_GRAPH_UID_ZERO;
        return false;
    }
    if (context->graph_auth_result.execution_sequence == 0) {
        *reason = GGML_RPC_HALOFPX_GRAPH_RESULT_EXECUTION_SEQUENCE_ZERO;
        return false;
    }
    *result = context->graph_auth_result;
    return true;
}

bool ggml_backend_rpc_halofpx_graph_result_for_split(
        ggml_backend_t backend,
        uint64_t parent_graph_uid,
        const uint8_t split_mapping_root[GGML_RPC_HALOFPX_STATE_DIGEST_BYTES],
        uint64_t split_graph_uid,
        uint32_t split_ordinal,
        uint32_t backend_ordinal,
        uint64_t execution_sequence,
        ggml_backend_rpc_halofpx_graph_result * result,
        ggml_backend_rpc_halofpx_graph_result_reason * reason) {
    if (reason == nullptr) return false;
    *reason = GGML_RPC_HALOFPX_GRAPH_RESULT_INVALID_ARGUMENT;
    if (!ggml_backend_is_rpc(backend) || split_mapping_root == nullptr ||
        result == nullptr || parent_graph_uid == 0 || split_graph_uid == 0 ||
        backend_ordinal == UINT32_MAX ||
        execution_sequence == 0) return false;
    auto * ctx = static_cast<ggml_backend_rpc_context *>(backend->context);
    if (ctx == nullptr) {
        *reason = GGML_RPC_HALOFPX_GRAPH_RESULT_CONTEXT_MISSING;
        return false;
    }
    if (!ctx->execution_armed || !ctx->execution_splits_bound ||
        ctx->execution_parent_graph_uid != parent_graph_uid ||
        ctx->execution_backend_ordinal != backend_ordinal ||
        ctx->execution_sequence != execution_sequence ||
        !hfx_equal(ctx->execution_split_mapping_root.data(), split_mapping_root, 32)) {
        *reason = GGML_RPC_HALOFPX_GRAPH_RESULT_STATUS_NOT_EXECUTED;
        return false;
    }
    if (ctx->execution_consumed_split_uids.count(split_graph_uid) != 0) {
        *reason = GGML_RPC_HALOFPX_GRAPH_RESULT_STATUS_NOT_EXECUTED;
        return false;
    }
    const auto found = std::find_if(
        ctx->graph_auth_results.begin(), ctx->graph_auth_results.end(),
        [&ctx, split_graph_uid, split_ordinal, backend_ordinal](
                const ggml_backend_rpc_halofpx_graph_result & value) {
            if (value.graph_uid != split_graph_uid) return false;
            return std::any_of(
                ctx->execution_splits.begin(), ctx->execution_splits.end(),
                [split_graph_uid, split_ordinal, backend_ordinal](
                        const ggml_backend_rpc_halofpx_split_identity & split) {
                    return split.split_graph_uid == split_graph_uid &&
                        split.split_ordinal == split_ordinal &&
                        split.backend_ordinal == backend_ordinal;
                });
        });
    if (found == ctx->graph_auth_results.end()) {
        *reason = GGML_RPC_HALOFPX_GRAPH_RESULT_GRAPH_UID_ZERO;
        return false;
    }
    *result = *found;
    ctx->execution_consumed_split_uids.insert(split_graph_uid);
    *reason = GGML_RPC_HALOFPX_GRAPH_RESULT_OK;
    return true;
}

bool ggml_backend_rpc_halofpx_mutable_test_inject(
        const ggml_backend_rpc_halofpx_mutable_session * handle,
        ggml_tensor * tensor,
        ggml_backend_rpc_halofpx_mutable_test_case test_case,
        uint32_t * exact_handler_status) {
    const char * enabled = std::getenv("HALOFPX_RPC_MUTABLE_AUTH_TEST");
    if (enabled == nullptr || strcmp(enabled, "1") != 0 || tensor == nullptr ||
        exact_handler_status == nullptr ||
        test_case < GGML_RPC_HALOFPX_MUTABLE_TEST_MALFORMED ||
        test_case > GGML_RPC_HALOFPX_MUTABLE_TEST_WRONG_VIEW) return false;
    std::shared_ptr<hfx_mutable_client_session> session;
    {
        std::lock_guard<std::mutex> lock(hfx_mutable_mutex);
        session = hfx_mutable_lookup_locked(handle);
    }
    if (session == nullptr) return false;
    std::lock_guard<std::mutex> session_lock(session->mutex);
    if (session == nullptr || session->mutation_sequence != 0) return false;
    auto role = session->roles.find(tensor);
    if (role == session->roles.end() || tensor->buffer == nullptr || tensor->data == nullptr ||
        !ggml_backend_buffer_is_rpc(tensor->buffer)) return false;
    auto * bctx = static_cast<ggml_backend_rpc_buffer_context *>(tensor->buffer->context);
    if (bctx->sock.get() != session->sock.get()) return false;
    const size_t size = ggml_nbytes(tensor);
    if (size == 0 || size > HFX_MUTABLE_MAX_BYTES) return false;
    std::vector<uint8_t> payload(size);
    ggml_backend_tensor_get(tensor, payload.data(), 0, size);
    hfx_mutable_update_header request {};
    hfx_set_magic(request.magic, "HFXMUS1\0");
    request.major = HFX_MUTABLE_MAJOR; request.minor = HFX_MUTABLE_MINOR;
    request.encoded_size = sizeof(request);
    request.role = role->second.role; request.role_ordinal = role->second.role_ordinal;
    request.allocation_ordinal = bctx->allocation_ordinal; request.type = tensor->type;
    request.graph_uid = session->attempt.graph_uid;
    request.exec_sequence = session->attempt.execution_sequence;
    request.mutation_sequence = 1;
    const uint64_t base = reinterpret_cast<uint64_t>(ggml_backend_buffer_get_base(tensor->buffer));
    const uint64_t address = reinterpret_cast<uint64_t>(tensor->data);
    if (address < base) return false;
    request.tensor_relative = address - base; request.logical_size = size;
    for (size_t i = 0; i < GGML_MAX_DIMS; ++i) {
        if (tensor->ne[i] <= 0 || uint64_t(tensor->ne[i]) > UINT32_MAX || tensor->nb[i] > UINT32_MAX) return false;
        request.ne[i] = tensor->ne[i]; request.nb[i] = tensor->nb[i];
    }
    memcpy(request.attempt_nonce, session->attempt.attempt_nonce, 32);
    memcpy(request.server_nonce, session->server_nonce.data(), 32);
    const auto content = hfx_sha256(payload.data(), payload.size());
    memcpy(request.content_digest, content.data(), 32);
    std::vector<uint8_t> view;
    for (const ggml_tensor * item = tensor; item; item = item->view_src) hfx_le<uint64_t>(view, item->view_offs);
    const auto view_digest = hfx_sha256(view.data(), view.size());
    memcpy(request.view_digest, view_digest.data(), 32);
    memcpy(request.prior_root, session->mutation_root.data(), 32);
    if (!hfx_mutable_sign(request, session->key.data())) return false;
    switch (test_case) {
        case GGML_RPC_HALOFPX_MUTABLE_TEST_MALFORMED: request.encoded_size--; break;
        case GGML_RPC_HALOFPX_MUTABLE_TEST_TAMPERED: request.tag[0] ^= 1; break;
        case GGML_RPC_HALOFPX_MUTABLE_TEST_DUPLICATE_SEQUENCE: request.mutation_sequence = 0; break;
        case GGML_RPC_HALOFPX_MUTABLE_TEST_OUT_OF_BOUNDS: request.logical_offset = size; break;
        case GGML_RPC_HALOFPX_MUTABLE_TEST_WRONG_VIEW: request.view_digest[0] ^= 1; break;
    }
    if (test_case != GGML_RPC_HALOFPX_MUTABLE_TEST_TAMPERED &&
        !hfx_mutable_sign(request, session->key.data())) return false;
    const auto header = hfx_mutable_encode(request);
    const rpc_tensor wire_tensor = serialize_tensor(tensor);
    std::vector<uint8_t> wire(header.begin(), header.end());
    const auto * tensor_wire = reinterpret_cast<const uint8_t *>(&wire_tensor);
    wire.insert(wire.end(), tensor_wire, tensor_wire + sizeof(wire_tensor));
    wire.insert(wire.end(), payload.begin(), payload.end());
    std::array<uint8_t, sizeof(hfx_mutable_receipt)> response_wire {};
    hfx_mutable_receipt response {};
    if (!send_rpc_cmd(session->sock, RPC_CMD_HALOFPX_MUTABLE_SET, wire.data(), wire.size(),
                      response_wire.data(), response_wire.size()) ||
        !hfx_mutable_decode(response_wire.data(), response_wire.size(), response)) return false;
    *exact_handler_status = response.status;
    return hfx_magic(response.magic, "HFXMUR1\0") &&
        response.major == HFX_MUTABLE_MAJOR && response.minor == HFX_MUTABLE_MINOR &&
        response.encoded_size == sizeof(response) && response.status == 0 &&
        response.graph_uid == request.graph_uid &&
        response.exec_sequence == request.exec_sequence &&
        response.mutation_sequence == request.mutation_sequence &&
        hfx_equal(response.attempt_nonce, request.attempt_nonce, 32) &&
        hfx_equal(response.server_nonce, request.server_nonce, 32) &&
        hfx_mutable_verify(response, session->key.data());
}

bool ggml_backend_rpc_halofpx_mutable_test_commit_omit_unmutated_leaf(
        const ggml_backend_rpc_halofpx_mutable_session * handle) {
    const char * enabled = std::getenv("HALOFPX_RPC_MUTABLE_AUTH_TEST");
    if (enabled == nullptr || strcmp(enabled, "1") != 0) return false;
    std::shared_ptr<hfx_mutable_client_session> session;
    {
        std::lock_guard<std::mutex> lock(hfx_mutable_mutex);
        session = hfx_mutable_lookup_locked(handle);
    }
    if (session == nullptr) return false;
    std::lock_guard<std::mutex> session_lock(session->mutex);
    if (session == nullptr || session->committed || session->mutations.empty() ||
        session->mutations.size() >= session->roles.size() + session->exclusions.size()) return false;
    std::vector<hfx_mutable_census_entry> census = session->mutations;
    std::sort(census.begin(), census.end(), [](const auto & a, const auto & b) {
        return std::tie(a.role, a.role_ordinal) < std::tie(b.role, b.role_ordinal);
    });
    std::vector<uint8_t> census_wire;
    for (const auto & entry : census) {
        const auto encoded = hfx_mutable_encode(entry);
        census_wire.insert(census_wire.end(), encoded.begin(), encoded.end());
    }
    const auto census_root = hfx_sha256(census_wire.data(), census_wire.size());
    hfx_mutable_commit_header request {};
    hfx_set_magic(request.magic, "HFXMCP1\0");
    request.major = HFX_MUTABLE_MAJOR; request.minor = HFX_MUTABLE_MINOR;
    request.encoded_size = sizeof(request) + census_wire.size();
    request.census_count = census.size(); request.mutation_count = session->mutation_sequence;
    request.graph_uid = session->attempt.graph_uid;
    request.exec_sequence = session->attempt.execution_sequence;
    memcpy(request.attempt_nonce, session->attempt.attempt_nonce, 32);
    memcpy(request.server_nonce, session->server_nonce.data(), 32);
    memcpy(request.scheduler_nonce, session->attempt.scheduler_attempt_nonce, 32);
    memcpy(request.scheduler_root, session->attempt.scheduler_transcript_root, 32);
    memcpy(request.mutation_root, session->mutation_root.data(), 32);
    memcpy(request.census_root, census_root.data(), 32);
    if (!hfx_mutable_sign(request, session->key.data())) return false;
    const auto header = hfx_mutable_encode(request);
    std::vector<uint8_t> wire(header.begin(), header.end());
    wire.insert(wire.end(), census_wire.begin(), census_wire.end());
    std::array<uint8_t, sizeof(hfx_mutable_commit_header)> response_wire {};
    hfx_mutable_commit_header response {};
    if (!send_rpc_cmd(session->sock, RPC_CMD_HALOFPX_MUTABLE_COMMIT,
                      wire.data(), wire.size(), response_wire.data(), response_wire.size()) ||
        !hfx_mutable_decode(response_wire.data(), response_wire.size(), response) ||
        !hfx_magic(response.magic, "HFXMPR1\0") ||
        !hfx_equal(response.census_root, request.census_root, 32) ||
        !hfx_mutable_verify(response, session->key.data())) return false;
    session->prepared = true;
    session->census_count = request.census_count;
    return true;
}
#else
bool ggml_backend_rpc_halofpx_execution_arm(ggml_backend_t, const uint8_t *, uint64_t) { return false; }
bool ggml_backend_rpc_halofpx_execution_disarm(ggml_backend_t, const uint8_t *, uint64_t) { return false; }
bool ggml_backend_rpc_halofpx_execution_bind_splits(
        ggml_backend_t, const uint8_t *, uint64_t, uint64_t, const uint8_t *,
        uint32_t, const ggml_backend_rpc_halofpx_split_identity *, size_t) { return false; }
uint64_t ggml_backend_rpc_halofpx_mutable_graph_uid(ggml_cgraph *) { return 0; }
bool ggml_backend_rpc_halofpx_mutable_begin(ggml_backend_t, const struct ggml_backend_sched_authority_prepared_admission *, const struct ggml_backend_sched_authority_prepared_admission *, const ggml_backend_rpc_halofpx_mutable_attempt *, ggml_backend_rpc_halofpx_mutable_session *) { return false; }
ggml_backend_rpc_halofpx_mutable_admit_result ggml_backend_rpc_halofpx_mutable_register_typed(const ggml_backend_rpc_halofpx_mutable_session *, ggml_tensor *, ggml_backend_rpc_halofpx_mutable_role, uint32_t, const ggml_backend_sched_authority_census_entry *) { return GGML_RPC_HALOFPX_MUTABLE_ADMIT_INVALID_ARGUMENT; }
bool ggml_backend_rpc_halofpx_mutable_register(const ggml_backend_rpc_halofpx_mutable_session *, ggml_tensor *, ggml_backend_rpc_halofpx_mutable_role, uint32_t) { return false; }
ggml_backend_rpc_halofpx_mutable_admit_result ggml_backend_rpc_halofpx_mutable_exclude_typed(const ggml_backend_rpc_halofpx_mutable_session *, ggml_tensor *, ggml_backend_rpc_halofpx_exclusion, uint32_t, const ggml_backend_sched_authority_census_entry *) { return GGML_RPC_HALOFPX_MUTABLE_ADMIT_INVALID_ARGUMENT; }
bool ggml_backend_rpc_halofpx_mutable_exclude(const ggml_backend_rpc_halofpx_mutable_session *, ggml_tensor *, ggml_backend_rpc_halofpx_exclusion, uint32_t) { return false; }
ggml_backend_rpc_halofpx_mutable_admit_result ggml_backend_rpc_halofpx_resolve_storage_identity(ggml_backend_t, ggml_tensor *, ggml_backend_rpc_halofpx_storage_identity *) { return GGML_RPC_HALOFPX_MUTABLE_ADMIT_INVALID_ARGUMENT; }
bool ggml_backend_rpc_halofpx_mutable_prepare(const ggml_backend_rpc_halofpx_mutable_session *, ggml_cgraph *, ggml_backend_rpc_halofpx_mutable_result *) { return false; }
bool ggml_backend_rpc_halofpx_mutable_commit(const ggml_backend_rpc_halofpx_mutable_session *, ggml_cgraph *, ggml_backend_rpc_halofpx_mutable_result *) { return false; }
bool ggml_backend_rpc_halofpx_mutable_abort(ggml_backend_rpc_halofpx_mutable_session *) { return false; }
bool ggml_backend_rpc_halofpx_graph_result_get(ggml_backend_t, ggml_backend_rpc_halofpx_graph_result *) { return false; }
bool ggml_backend_rpc_halofpx_graph_result_inspect(
        ggml_backend_t, ggml_backend_rpc_halofpx_graph_result *,
        ggml_backend_rpc_halofpx_graph_result_reason * reason) {
    if (reason != nullptr) *reason = GGML_RPC_HALOFPX_GRAPH_RESULT_NOT_RPC;
    return false;
}
bool ggml_backend_rpc_halofpx_graph_result_for_split(
        ggml_backend_t, uint64_t, const uint8_t *, uint64_t, uint32_t, uint32_t, uint64_t,
        ggml_backend_rpc_halofpx_graph_result *,
        ggml_backend_rpc_halofpx_graph_result_reason * reason) {
    if (reason != nullptr) *reason = GGML_RPC_HALOFPX_GRAPH_RESULT_NOT_RPC;
    return false;
}
bool ggml_backend_rpc_halofpx_mutable_test_inject(const ggml_backend_rpc_halofpx_mutable_session *, ggml_tensor *, ggml_backend_rpc_halofpx_mutable_test_case, uint32_t *) { return false; }
bool ggml_backend_rpc_halofpx_mutable_test_commit_omit_unmutated_leaf(const ggml_backend_rpc_halofpx_mutable_session *) { return false; }
#endif

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

struct hfx_state_diag_component {
    hfx_state_object_component object {};
    uint32_t buffer_group = 0;
    uint32_t reserved = 0;
    uint64_t range_begin = 0;
    uint64_t range_end = 0;
};

struct hfx_state_diag_summary {
    uint8_t phase[8] {};
    uint32_t component_count = 0;
    uint32_t reserved = 0;
    uint64_t component_bytes = 0;
    uint8_t aggregate[32] {};
    uint8_t merkle_root[32] {};
};

static hfx_digest hfx_state_diag_merkle(std::vector<hfx_digest> level) {
    if (level.empty()) return hfx_sha256(nullptr, 0);
    while (level.size() > 1) {
        std::vector<hfx_digest> next;
        next.reserve((level.size() + 1) / 2);
        for (size_t i = 0; i < level.size(); i += 2) {
            std::array<uint8_t, 64> pair {};
            memcpy(pair.data(), level[i].data(), 32);
            memcpy(pair.data() + 32, level[std::min(i + 1, level.size() - 1)].data(), 32);
            next.push_back(hfx_sha256(pair.data(), pair.size()));
        }
        level = std::move(next);
    }
    return level[0];
}

static bool hfx_state_log_component_digest(
        const char * phase,
        const std::vector<hfx_state_object_component> & components,
        const hfx_state_component_wire * addressed,
        const std::unordered_set<ggml_backend_buffer_t> & buffers,
        const uint8_t control_key[32]) {
    if (!hfx_state_diagnostics_enabled()) return true;
    if (!phase || !addressed || !control_key || components.size() > HFX_STATE_MAX_COMPONENTS) return false;
    const auto aggregate = hfx_sha256(
        components.data(), components.size() * sizeof(components[0]));
    std::unordered_map<uint64_t, uint32_t> groups;
    std::vector<hfx_digest> leaves;
    leaves.reserve(components.size());
    uint64_t total = 0;
    for (size_t i = 0; i < components.size(); ++i) {
        const auto buffer_value = addressed[i].buffer;
        auto inserted = groups.emplace(buffer_value, static_cast<uint32_t>(groups.size()));
        auto buffer = reinterpret_cast<ggml_backend_buffer_t>(buffer_value);
        if (buffer == nullptr || buffers.find(buffer) == buffers.end()) return false;
        const uint64_t base = reinterpret_cast<uint64_t>(ggml_backend_buffer_get_base(buffer));
        uint64_t begin = 0;
        uint64_t end = 0;
        if (addressed[i].data < base ||
            !hfx_add(addressed[i].data - base, addressed[i].offset, begin) ||
            !hfx_add(begin, addressed[i].size, end) ||
            end > ggml_backend_buffer_get_size(buffer) ||
            !hfx_add(total, addressed[i].size, total)) return false;
        hfx_state_diag_component diagnostic {};
        diagnostic.object = components[i];
        diagnostic.buffer_group = inserted.first->second;
        diagnostic.range_begin = begin;
        diagnostic.range_end = end;
        const auto leaf = hfx_sha256(&diagnostic, sizeof(diagnostic));
        leaves.push_back(leaf);
        GGML_LOG_INFO(
            "[halofpx-state-diag-component] phase=%s ordinal=%u kind=%u type=%u "
            "ne=%u,%u,%u,%u nb=%u,%u,%u,%u view_offset=%" PRIu64 " size=%" PRIu64 " "
            "label_sha256=%s content_sha256=%s buffer_group=%u range=%" PRIu64 ":%" PRIu64 " leaf_sha256=%s\n",
            phase, diagnostic.object.descriptor.ordinal, diagnostic.object.descriptor.kind,
            diagnostic.object.descriptor.type,
            diagnostic.object.descriptor.ne[0], diagnostic.object.descriptor.ne[1],
            diagnostic.object.descriptor.ne[2], diagnostic.object.descriptor.ne[3],
            diagnostic.object.descriptor.nb[0], diagnostic.object.descriptor.nb[1],
            diagnostic.object.descriptor.nb[2], diagnostic.object.descriptor.nb[3],
            diagnostic.object.descriptor.offset, diagnostic.object.descriptor.size,
            hfx_hex(diagnostic.object.descriptor.label_digest, 32).c_str(),
            hfx_hex(diagnostic.object.content_digest, 32).c_str(), diagnostic.buffer_group,
            diagnostic.range_begin, diagnostic.range_end, hfx_hex(leaf.data(), leaf.size()).c_str());
    }
    const auto merkle = hfx_state_diag_merkle(leaves);
    hfx_state_diag_summary summary {};
    snprintf(reinterpret_cast<char *>(summary.phase), sizeof(summary.phase), "%s", phase);
    summary.component_count = static_cast<uint32_t>(components.size());
    summary.component_bytes = total;
    memcpy(summary.aggregate, aggregate.data(), aggregate.size());
    memcpy(summary.merkle_root, merkle.data(), merkle.size());
    const auto tag = hfx_hmac(control_key, &summary, sizeof(summary));
    GGML_LOG_INFO(
        "[halofpx-state-diag] phase=%s components=%zu bytes=%" PRIu64
        " descriptor_content_sha256=%s merkle_sha256=%s auth_tag=%s\n",
        phase, components.size(), total, hfx_hex(aggregate.data(), aggregate.size()).c_str(),
        hfx_hex(merkle.data(), merkle.size()).c_str(), hfx_hex(tag.data(), tag.size()).c_str());
    return true;
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

struct hfx_mutable_server_session {
    hfx_mutable_caps caps {};
    hfx_digest mutation_root {};
    hfx_digest semantic_root {};
    uint64_t mutation_sequence = 0;
    uint32_t census_count = 0;
    uint64_t bound_parent_graph_uid = 0;
    uint32_t bound_split_ordinal = UINT32_MAX;
    uint32_t bound_backend_ordinal = UINT32_MAX;
    hfx_digest split_mapping_root {};
    std::vector<hfx_mutable_census_entry> mutations;
    std::vector<hfx_mutable_census_entry> census;
    uint32_t planned_operation = 0;
    bool admitted = false;
    bool prepared = false;
    bool committed = false;
    bool consumed = false;
};
#endif

class rpc_server {
public:
    rpc_server(std::vector<ggml_backend_t> all_backends, const char * cache_dir
#ifdef GGML_RPC_HALOFPX_LOCAL_STATE
            , const hfx_state_server_config_owned * hfx_state_config,
              uint64_t connection_epoch
#endif
            )
        : backends(std::move(all_backends)), cache_dir(cache_dir)
#ifdef GGML_RPC_HALOFPX_LOCAL_STATE
        , hfx_state_config(hfx_state_config ? std::optional<hfx_state_server_config_owned>(*hfx_state_config) : std::nullopt)
        , connection_epoch(connection_epoch)
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
    bool graph_compute(const std::vector<uint8_t> & input
#ifdef GGML_RPC_HALOFPX_LOCAL_STATE
                       , hfx_digest * reconstructed_digest = nullptr,
                       const uint8_t * expected_digest = nullptr,
                       bool execute = true
#endif
                       );
    bool graph_recompute(const rpc_msg_graph_recompute_req & request);
#ifdef GGML_RPC_HALOFPX_LOCAL_STATE
    bool hfx_graph_caps(const hfx_graph_auth_caps_req & request, hfx_graph_auth_caps_rsp & response);
    bool hfx_graph_compute(const std::vector<uint8_t> & input, hfx_graph_auth_receipt & response);
    bool hfx_graph_response_published();
    bool hfx_graph_recompute(const hfx_graph_auth_header & request, hfx_graph_auth_receipt & response);
    bool hfx_graph_execute(const hfx_graph_auth_header & request, hfx_graph_auth_receipt & response);
    bool hfx_graph_response_event(const hfx_graph_auth_header & request, const char * phase,
                                  uint64_t expected, const rpc_transport_io_result & io,
                                  bool ok, uint32_t status);
    void hfx_graph_invalidate();
    void hfx_graph_invalidate_prepared();
    void hfx_graph_discard_lineage();
    bool hfx_mutable_caps(const hfx_mutable_caps & request, hfx_mutable_caps & response);
    bool hfx_mutable_preflight(
        const hfx_mutable_preflight_wire & request,
        hfx_mutable_preflight_wire & response);
    bool hfx_mutable_decode_refusal(const std::vector<uint8_t> & wire);
    bool hfx_mutable_set(const std::vector<uint8_t> & input, bool hash_only, hfx_mutable_receipt & response);
    bool hfx_mutable_refusal(const std::vector<uint8_t> & input, hfx_mutable_receipt & response);
    bool hfx_mutable_commit(const std::vector<uint8_t> & input, hfx_mutable_commit_header & response);
    bool hfx_mutable_bind(const hfx_mutable_commit_header & request, hfx_mutable_commit_header & response);
    bool hfx_mutable_graph_census_valid(const ggml_cgraph * graph) const;
    bool hfx_mutable_epoch_valid() const;
#endif
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
#ifdef GGML_RPC_HALOFPX_LOCAL_STATE
        uint64_t auth_uid = 0;
        hfx_digest auth_digest {};
        hfx_digest auth_transcript {};
        bool auth_prepared = false;
#endif
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
#ifdef GGML_RPC_HALOFPX_LOCAL_STATE
    std::unordered_map<ggml_backend_buffer_t, uint32_t> buffer_allocation_ordinals;
    std::unordered_map<ggml_backend_buffer_t, uint32_t> buffer_device_ordinals;
    uint32_t next_buffer_allocation_ordinal = 0;
    uint64_t allocation_topology_epoch = 1;
    uint64_t connection_epoch = 0;
#endif
    // store the last computed graph for each backend
    std::vector<stored_graph> stored_graphs;
#ifdef GGML_RPC_HALOFPX_LOCAL_STATE
    std::optional<hfx_state_server_config_owned> hfx_state_config;
    std::optional<hfx_state_pending_attempt> hfx_state_pending;
    bool hfx_graph_negotiated = false;
    uint64_t hfx_graph_sequence = 0;
    std::array<uint8_t, 32> hfx_graph_key_value {};
    hfx_digest hfx_graph_attempt_nonce {};
    hfx_digest hfx_graph_server_nonce {};
    hfx_mutable_server_session hfx_mutable_session {};
    std::shared_ptr<hfx_preexecute_recorder> preexecute_recorder;
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
#ifdef GGML_RPC_HALOFPX_LOCAL_STATE
        if (hfx_graph_requested()) {
            if (next_buffer_allocation_ordinal == UINT32_MAX ||
                !buffer_allocation_ordinals.emplace(buffer, next_buffer_allocation_ordinal++).second) {
                ggml_backend_buffer_free(buffer);
                buffers.erase(buffer);
                response = {};
                return false;
            }
            buffer_device_ordinals.emplace(buffer, dev_id);
            if (allocation_topology_epoch == UINT64_MAX) {
                ggml_backend_buffer_free(buffer);
                buffers.erase(buffer);
                buffer_allocation_ordinals.erase(buffer);
                buffer_device_ordinals.erase(buffer);
                response = {};
                return false;
            }
            ++allocation_topology_epoch;
            if (hfx_mutable_session.admitted) {
                if (preexecute_recorder && !preexecute_recorder->terminal) {
                    if (!hfx_server_emit(
                        preexecute_recorder, HFX_SERVER_ABORT,
                        HFX_PREEXEC_WRONG_ALLOCATION_EPOCH, true)) return false;
                }
                hfx_mutable_session = {};
            }
        }
#endif
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
#ifdef GGML_RPC_HALOFPX_LOCAL_STATE
    buffer_allocation_ordinals.erase(buffer);
    buffer_device_ordinals.erase(buffer);
    if (allocation_topology_epoch == UINT64_MAX) return false;
    ++allocation_topology_epoch;
    if (hfx_mutable_session.admitted) {
        if (preexecute_recorder && !preexecute_recorder->terminal) {
            if (!hfx_server_emit(
                preexecute_recorder, HFX_SERVER_ABORT,
                HFX_PREEXEC_WRONG_ALLOCATION_EPOCH, true)) return false;
        }
        hfx_mutable_session = {};
    }
#endif
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

bool rpc_server::graph_compute(const std::vector<uint8_t> & input
#ifdef GGML_RPC_HALOFPX_LOCAL_STATE
        , hfx_digest * reconstructed_digest,
        const uint8_t * expected_digest,
        bool execute
#endif
        ) {
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
#ifdef GGML_RPC_HALOFPX_LOCAL_STATE
    if (reconstructed_digest != nullptr && (n_nodes == 0 || n_nodes > HFX_GRAPH_AUTH_MAX_NODES)) return false;
#endif
    if (input.size() < 2*sizeof(uint32_t) + n_nodes*sizeof(uint64_t) + sizeof(uint32_t)) {
        return false;
    }
    const uint64_t * nodes = (const uint64_t *)src;
    src += n_nodes*sizeof(uint64_t);
    uint32_t n_tensors;
    memcpy(&n_tensors, src, sizeof(n_tensors));
    src += sizeof(n_tensors);
    const uint64_t expected_size = 2*sizeof(uint32_t) +
        static_cast<uint64_t>(n_nodes)*sizeof(uint64_t) + sizeof(uint32_t) +
        static_cast<uint64_t>(n_tensors)*sizeof(rpc_tensor);
    if (input.size() < expected_size) return false;
#ifdef GGML_RPC_HALOFPX_LOCAL_STATE
    if (reconstructed_digest != nullptr && expected_size != input.size()) return false;
#endif
#ifdef GGML_RPC_HALOFPX_LOCAL_STATE
    if (reconstructed_digest != nullptr && (n_tensors == 0 || n_tensors > HFX_GRAPH_AUTH_MAX_TENSORS)) return false;
#endif
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
#ifdef GGML_RPC_HALOFPX_LOCAL_STATE
    if (reconstructed_digest != nullptr) {
        std::unordered_map<const ggml_tensor *, uint64_t> reverse;
        reverse.reserve(tensor_map.size());
        for (const auto & entry : tensor_map) {
            if (!reverse.emplace(entry.second, entry.first).second) return false;
        }
        std::vector<const ggml_tensor *> derived_order;
        std::unordered_set<const ggml_tensor *> derived_seen;
        std::function<bool(const ggml_tensor *)> visit = [&](const ggml_tensor * tensor) {
            if (tensor == nullptr || derived_seen.find(tensor) != derived_seen.end()) return true;
            derived_seen.insert(tensor);
            for (uint32_t s = 0; s < GGML_MAX_SRC; ++s) {
                if (!visit(tensor->src[s])) return false;
            }
            if (!visit(tensor->view_src)) return false;
            if (reverse.find(tensor) == reverse.end()) return false;
            derived_order.push_back(tensor);
            return true;
        };
        std::vector<uint64_t> derived_nodes;
        derived_nodes.reserve(n_nodes);
        for (uint32_t i = 0; i < n_nodes; ++i) {
            if (graph->nodes[i] == nullptr || !visit(graph->nodes[i])) return false;
            const auto node_id = reverse.find(graph->nodes[i]);
            if (node_id == reverse.end()) return false;
            derived_nodes.push_back(node_id->second);
        }
        if (derived_order.size() != n_tensors || tensor_map.size() != n_tensors) return false;
        std::vector<rpc_tensor> reconstructed;
        reconstructed.reserve(n_tensors);
        for (uint32_t i = 0; i < n_tensors; ++i) {
            const ggml_tensor * actual = derived_order[i];
            const auto actual_id = reverse.find(actual);
            if (actual_id == reverse.end() || actual_id->second != tensors[i].id) return false;
            rpc_tensor record {};
            record.id = actual_id->second;
            record.type = actual->type;
            record.buffer = reinterpret_cast<uint64_t>(actual->buffer);
            for (uint32_t d = 0; d < GGML_MAX_DIMS; ++d) {
                record.ne[d] = actual->ne[d];
                record.nb[d] = actual->nb[d];
            }
            record.op = actual->op;
            memcpy(record.op_params, actual->op_params, sizeof(record.op_params));
            record.flags = actual->flags;
            for (uint32_t s = 0; s < GGML_MAX_SRC; ++s) {
                if (actual->src[s] != nullptr) {
                    const auto source = reverse.find(actual->src[s]);
                    if (source == reverse.end()) return false;
                    record.src[s] = source->second;
                }
            }
            if (actual->view_src != nullptr) {
                const auto view = reverse.find(actual->view_src);
                if (view == reverse.end()) return false;
                record.view_src = view->second;
                record.view_offs = actual->view_offs;
            }
            record.data = reinterpret_cast<uint64_t>(actual->data);
            reconstructed.push_back(record);
        }
        const auto resolver = [this, &tensor_map](uint64_t id, uint32_t & ordinal, uint64_t & relative) {
            const auto found = tensor_map.find(id);
            if (found == tensor_map.end() || found->second == nullptr ||
                found->second->buffer == nullptr || found->second->data == nullptr) return false;
            const uint64_t base = reinterpret_cast<uint64_t>(ggml_backend_buffer_get_base(found->second->buffer));
            const uint64_t data = reinterpret_cast<uint64_t>(found->second->data);
            if (base == 0 || data < base) return false;
            const auto allocation = buffer_allocation_ordinals.find(found->second->buffer);
            if (allocation == buffer_allocation_ordinals.end()) return false;
            ordinal = allocation->second;
            relative = data - base;
            return true;
        };
        if (!hfx_graph_canonical_digest(device, derived_nodes.data(), n_nodes, reconstructed.data(), n_tensors,
                                        resolver, *reconstructed_digest)) return false;
        if (expected_digest != nullptr &&
            !hfx_equal(reconstructed_digest->data(), expected_digest, reconstructed_digest->size())) return false;
    }
#endif
    stored_graphs[device].graph = graph;
#ifdef GGML_RPC_HALOFPX_LOCAL_STATE
    if (execute) {
#endif
        ggml_status status = ggml_backend_graph_compute(backends[device], graph);
        GGML_ASSERT(status == GGML_STATUS_SUCCESS && "Unsuccessful graph computations are not supported with RPC");
#ifdef GGML_RPC_HALOFPX_LOCAL_STATE
    }
#endif
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

#ifdef GGML_RPC_HALOFPX_LOCAL_STATE
bool rpc_server::hfx_graph_caps(
        const hfx_graph_auth_caps_req & request,
        hfx_graph_auth_caps_rsp & response) {
    hfx_graph_invalidate();
    if (!hfx_graph_requested() || !hfx_graph_key(hfx_graph_key_value) ||
        !hfx_magic(request.magic, "HFXGAQ1\0") ||
        request.major != HFX_GRAPH_AUTH_MAJOR || request.minor != HFX_GRAPH_AUTH_MINOR ||
        request.encoded_size != sizeof(request) ||
        hfx_zero(request.attempt_nonce, sizeof(request.attempt_nonce)) ||
        !hfx_graph_verify_record(request, hfx_graph_key_value.data())) return false;
    memset(&response, 0, sizeof(response));
    hfx_set_magic(response.magic, "HFXGAC1\0");
    response.major = HFX_GRAPH_AUTH_MAJOR;
    response.minor = HFX_GRAPH_AUTH_MINOR;
    response.encoded_size = sizeof(response);
    response.status = 1;
    response.max_graph_bytes = HFX_GRAPH_AUTH_MAX_GRAPH_BYTES;
    response.max_tensors = HFX_GRAPH_AUTH_MAX_TENSORS;
    response.max_nodes = HFX_GRAPH_AUTH_MAX_NODES;
    memcpy(response.attempt_nonce, request.attempt_nonce, 32);
    if (!hfx_random_all(response.server_nonce, sizeof(response.server_nonce))) return false;
    memcpy(hfx_graph_attempt_nonce.data(), request.attempt_nonce, 32);
    memcpy(hfx_graph_server_nonce.data(), response.server_nonce, 32);
    hfx_graph_sequence = 0;
    hfx_graph_negotiated = true;
    return hfx_graph_sign_record(response, hfx_graph_key_value.data());
}

static bool hfx_graph_server_header_valid(
        const hfx_graph_auth_header & request,
        const std::array<uint8_t, 32> & key,
        const hfx_digest & attempt_nonce,
        const hfx_digest & server_nonce,
        uint64_t expected_sequence,
        const char expected_magic[8]) {
    return hfx_magic(request.magic, expected_magic) &&
        request.major == HFX_GRAPH_AUTH_MAJOR && request.minor == HFX_GRAPH_AUTH_MINOR &&
        request.encoded_size == sizeof(request) &&
        request.graph_uid != 0 &&
        request.exec_sequence != 0 && request.exec_sequence == expected_sequence &&
        hfx_equal(request.attempt_nonce, attempt_nonce.data(), 32) &&
        hfx_equal(request.server_nonce, server_nonce.data(), 32) &&
        !hfx_zero(request.graph_digest, 32) &&
        !hfx_zero(request.transcript_root, 32) &&
        hfx_graph_verify_record(request, key.data());
}

static bool hfx_graph_server_receipt(
        hfx_graph_auth_receipt & response,
        const hfx_graph_auth_header & request,
        const uint8_t key[32]) {
    memset(&response, 0, sizeof(response));
    hfx_set_magic(response.magic, "HFXGAR1\0");
    response.major = HFX_GRAPH_AUTH_MAJOR;
    response.minor = HFX_GRAPH_AUTH_MINOR;
    response.encoded_size = sizeof(response);
    response.status = 1;
    response.device = request.device;
    response.graph_uid = request.graph_uid;
    response.exec_sequence = request.exec_sequence;
    memcpy(response.attempt_nonce, request.attempt_nonce, 32);
    memcpy(response.server_nonce, request.server_nonce, 32);
    memcpy(response.graph_digest, request.graph_digest, 32);
    memcpy(response.transcript_root, request.transcript_root, 32);
    memcpy(response.admission_object_id, request.admission_object_id, 32);
    memcpy(response.expected_admission_digest,
           request.expected_admission_digest, 32);
    return hfx_graph_sign_record(response, key);
}

bool rpc_server::hfx_graph_compute(
        const std::vector<uint8_t> & input,
        hfx_graph_auth_receipt & response) {
    if (!hfx_graph_negotiated || input.size() < sizeof(hfx_graph_auth_header)) return false;
    hfx_graph_auth_header request {};
    if (!hfx_graph_decode(input.data(), sizeof(request), request)) return false;
    if (request.graph_size == 0 || request.graph_size > HFX_GRAPH_AUTH_MAX_GRAPH_BYTES ||
        input.size() != sizeof(request) + request.graph_size ||
        !hfx_graph_server_header_valid(request, hfx_graph_key_value,
                                       hfx_graph_attempt_nonce, hfx_graph_server_nonce,
                                       hfx_graph_sequence + 1, "HFXGAX1\0")) return false;
    if (hfx_mutable_requested()) {
        if (!hfx_mutable_session.admitted || !hfx_mutable_session.prepared ||
            hfx_mutable_session.caps.graph_uid != request.graph_uid ||
            hfx_mutable_session.caps.exec_sequence != request.exec_sequence) return false;
        std::vector<uint8_t> binding;
        hfx_bytes(binding, hfx_mutable_session.semantic_root.data(), 32);
        hfx_bytes(binding, hfx_mutable_session.caps.scheduler_nonce, 32);
        hfx_le<uint64_t>(binding, hfx_mutable_session.bound_parent_graph_uid);
        hfx_le<uint64_t>(binding, request.exec_sequence);
        hfx_le<uint32_t>(binding, hfx_mutable_session.bound_split_ordinal);
        hfx_le<uint32_t>(binding, hfx_mutable_session.bound_backend_ordinal);
        hfx_le<uint64_t>(binding, request.graph_uid);
        hfx_bytes(binding, hfx_mutable_session.split_mapping_root.data(), 32);
        const auto expected = hfx_sha256(binding.data(), binding.size());
        if (!hfx_equal(expected.data(), request.transcript_root, 32)) return false;
    }
    std::vector<uint8_t> graph(input.begin() + sizeof(request), input.end());
    if (graph.size() < sizeof(uint32_t)) return false;
    uint32_t wire_device = UINT32_MAX;
    memcpy(&wire_device, graph.data(), sizeof(wire_device));
    if (wire_device != request.device || wire_device >= stored_graphs.size()) return false;
    hfx_digest reconstructed {};
    if (!graph_compute(graph, &reconstructed, request.graph_digest, false) ||
        !hfx_equal(reconstructed.data(), request.graph_digest, reconstructed.size())) return false;
    if (hfx_mutable_requested() && !hfx_mutable_graph_census_valid(stored_graphs[wire_device].graph)) {
        if (!hfx_graph_server_receipt(response, request, hfx_graph_key_value.data())) return false;
        response.status = 3;
        return hfx_graph_sign_record(response, hfx_graph_key_value.data());
    }
    auto & stored = stored_graphs[wire_device];
    stored.auth_uid = request.graph_uid;
    stored.auth_digest = reconstructed;
    memcpy(stored.auth_transcript.data(), request.transcript_root, 32);
    stored.auth_prepared = true;
    if (hfx_mutable_requested()) {
        hfx_mutable_session.planned_operation =
            GGML_BACKEND_SCHED_OPERATION_AUTHENTICATED_EXECUTE;
    }
    hfx_graph_sequence = request.exec_sequence;
    GGML_LOG_INFO("[halofpx-rpc-graph-auth] server prepared mode=compute sequence=%" PRIu64
                  " uid=%" PRIu64 " digest=%s\n",
                  request.exec_sequence, request.graph_uid,
                  hfx_hex(reconstructed.data(), reconstructed.size()).c_str());
    return hfx_graph_server_receipt(response, request, hfx_graph_key_value.data());
}

bool rpc_server::hfx_graph_recompute(
        const hfx_graph_auth_header & request,
        hfx_graph_auth_receipt & response) {
    if (!hfx_graph_negotiated || request.graph_size != 0 ||
        request.device >= stored_graphs.size() ||
        !hfx_graph_server_header_valid(request, hfx_graph_key_value,
                                       hfx_graph_attempt_nonce, hfx_graph_server_nonce,
                                       hfx_graph_sequence + 1, "HFXGRX1\0")) return false;
    if (hfx_mutable_requested()) {
        if (!hfx_mutable_session.admitted || !hfx_mutable_session.prepared ||
            hfx_mutable_session.caps.graph_uid != request.graph_uid ||
            hfx_mutable_session.caps.exec_sequence != request.exec_sequence) return false;
        std::vector<uint8_t> binding;
        hfx_bytes(binding, hfx_mutable_session.semantic_root.data(), 32);
        hfx_bytes(binding, hfx_mutable_session.caps.scheduler_nonce, 32);
        hfx_le<uint64_t>(binding, hfx_mutable_session.bound_parent_graph_uid);
        hfx_le<uint64_t>(binding, request.exec_sequence);
        hfx_le<uint32_t>(binding, hfx_mutable_session.bound_split_ordinal);
        hfx_le<uint32_t>(binding, hfx_mutable_session.bound_backend_ordinal);
        hfx_le<uint64_t>(binding, request.graph_uid);
        hfx_bytes(binding, hfx_mutable_session.split_mapping_root.data(), 32);
        const auto expected = hfx_sha256(binding.data(), binding.size());
        if (!hfx_equal(expected.data(), request.transcript_root, 32)) return false;
    }
    const auto & stored = stored_graphs[request.device];
    if (stored.graph == nullptr || request.graph_uid == 0 ||
        request.graph_uid != stored.auth_uid ||
        !hfx_equal(request.graph_digest, stored.auth_digest.data(), 32) ||
        !hfx_equal(request.transcript_root, stored.auth_transcript.data(), 32) ||
        (hfx_mutable_requested() && !hfx_mutable_graph_census_valid(stored.graph))) return false;
    stored_graphs[request.device].auth_prepared = true;
    if (hfx_mutable_requested()) {
        hfx_mutable_session.planned_operation =
            GGML_BACKEND_SCHED_OPERATION_AUTHENTICATED_RECOMPUTE;
    }
    hfx_graph_sequence = request.exec_sequence;
    GGML_LOG_INFO("[halofpx-rpc-graph-auth] server prepared mode=recompute sequence=%" PRIu64
                  " uid=%" PRIu64 " digest=%s\n",
                  request.exec_sequence, request.graph_uid,
                  hfx_hex(request.graph_digest, 32).c_str());
    return hfx_graph_server_receipt(response, request, hfx_graph_key_value.data());
}

bool rpc_server::hfx_graph_execute(
        const hfx_graph_auth_header & request,
        hfx_graph_auth_receipt & response) {
    rpc_transport_io_result no_io {};
    if (!hfx_graph_response_event(request, "handler_entry", 0, no_io, true, 0)) return false;
    const bool header_valid = hfx_graph_negotiated && request.graph_size == 0 &&
        request.device < stored_graphs.size() &&
        hfx_graph_server_header_valid(request, hfx_graph_key_value,
                                      hfx_graph_attempt_nonce, hfx_graph_server_nonce,
                                      hfx_graph_sequence, "HFXGEX1\0");
    const bool mutable_valid = !hfx_mutable_requested() ||
        (hfx_mutable_session.admitted &&
         hfx_mutable_session.prepared &&
         !hfx_mutable_session.consumed &&
         hfx_mutable_session.planned_operation != 0 &&
         hfx_mutable_session.planned_operation ==
            hfx_mutable_session.caps.scheduler_admission.allowed_operation &&
         !hfx_zero(request.admission_object_id, 32) &&
         !hfx_zero(request.expected_admission_digest, 32) &&
         hfx_equal(request.admission_object_id,
                   hfx_mutable_session.caps.scheduler_admission.object_id, 32) &&
         hfx_equal(request.expected_admission_digest,
                   hfx_mutable_session.caps.expected_admission_digest, 32) &&
         hfx_mutable_session.caps.scheduler_admission.state ==
            GGML_BACKEND_SCHED_ADMISSION_PREPARED &&
         hfx_mutable_session.caps.scheduler_admission.server_connection_epoch ==
            connection_epoch &&
         hfx_mutable_session.caps.scheduler_admission.allocation_topology_epoch ==
            allocation_topology_epoch &&
         ggml_backend_sched_authority_verify_prepared_admission(
            &hfx_mutable_session.caps.scheduler_admission,
            hfx_graph_key_value.data(),
            &hfx_mutable_session.caps.scheduler_expected_admission));
    const bool handler_valid = header_valid && mutable_valid &&
        stored_graphs[request.device].auth_prepared &&
        stored_graphs[request.device].graph != nullptr &&
        request.graph_uid == stored_graphs[request.device].auth_uid &&
        hfx_equal(request.graph_digest, stored_graphs[request.device].auth_digest.data(), 32) &&
        hfx_equal(request.transcript_root, stored_graphs[request.device].auth_transcript.data(), 32);
    if (!hfx_graph_response_event(request, "handler_validation", 0, no_io,
                                  handler_valid, handler_valid ? 1U : 0U)) return false;
    if (!handler_valid) {
        if (preexecute_recorder && !preexecute_recorder->terminal) {
            hfx_server_emit(preexecute_recorder, HFX_SERVER_ABORT,
                            HFX_PREEXEC_BIND_REFUSED, true);
        }
        return false;
    }
    if (hfx_mutable_requested()) {
        memcpy(preexecute_recorder->execute_receipt.data(), request.tag, 32);
        memcpy(preexecute_recorder->graph_digest.data(), request.graph_digest, 32);
        preexecute_recorder->execute_receipt_state = 1;
    }
    auto & stored = stored_graphs[request.device];
    // The connection-local server owns this state and dispatches commands
    // serially. This transition is therefore atomic with respect to every
    // authenticated request on the connection and occurs immediately before
    // the only mutating backend execution call.
    if (hfx_mutable_requested()) {
        if (!hfx_server_emit(preexecute_recorder,
                             HFX_SERVER_EXECUTE_INTENT_CONSUMED,
                             HFX_PREEXEC_OK)) {
            return false;
        }
        hfx_mutable_session.consumed = true;
    }
    stored.auth_prepared = false;
    ggml_status status = ggml_backend_graph_compute(backends[request.device], stored.graph);
    if (!hfx_graph_response_event(request, "backend_complete", 0, no_io,
                                  status == GGML_STATUS_SUCCESS, static_cast<uint32_t>(status))) return false;
    if (status != GGML_STATUS_SUCCESS) {
        if (preexecute_recorder && !preexecute_recorder->terminal) {
            hfx_server_emit(preexecute_recorder, HFX_SERVER_ABORT,
                            HFX_PREEXEC_RECEIPT_REFUSED, true);
        }
        return false;
    }
    if (hfx_mutable_requested() &&
        !hfx_server_emit(preexecute_recorder,
                         HFX_SERVER_BACKEND_EXECUTED,
                         HFX_PREEXEC_OK)) {
        return false;
    }
    GGML_LOG_INFO("[halofpx-rpc-graph-auth] server executed sequence=%" PRIu64
                  " uid=%" PRIu64 " digest=%s\n",
                  request.exec_sequence, request.graph_uid,
                  hfx_hex(request.graph_digest, 32).c_str());
    bool signed_receipt = hfx_graph_server_receipt(
        response, request, hfx_graph_key_value.data());
    if (signed_receipt) {
        response.status = 2;
        signed_receipt = hfx_graph_sign_record(response, hfx_graph_key_value.data());
    }
    if (!hfx_graph_response_event(request, "receipt_construction", 0, no_io,
                                  signed_receipt, signed_receipt ? 1U : 0U)) return false;
    if (!signed_receipt) {
        if (preexecute_recorder && !preexecute_recorder->terminal) {
            hfx_server_emit(preexecute_recorder, HFX_SERVER_ABORT,
                            HFX_PREEXEC_RECEIPT_REFUSED, true);
        }
        return false;
    }
    if (!hfx_graph_response_event(request, "handler_exit", 0, no_io,
                                  signed_receipt, signed_receipt ? 2U : 1U)) return false;
    return signed_receipt;
}

bool rpc_server::hfx_graph_response_published() {
    return !hfx_mutable_requested() ||
        (hfx_server_emit(preexecute_recorder,
                         HFX_SERVER_RECEIPT_PUBLISHED,
                         HFX_PREEXEC_OK) &&
         hfx_server_emit(preexecute_recorder,
                         HFX_SERVER_CLOSE,
                         HFX_PREEXEC_OK, true));
}

bool rpc_server::hfx_graph_response_event(
        const hfx_graph_auth_header & request,
        const char * phase,
        uint64_t expected,
        const rpc_transport_io_result & io,
        bool ok,
        uint32_t status) {
    return hfx_rpc_response_event(
        hfx_graph_key_value.data(), "server", phase,
        static_cast<uint8_t>(RPC_CMD_HALOFPX_GRAPH_AUTH_EXECUTE),
        0, request.graph_uid, request.exec_sequence, request.device,
        request.attempt_nonce, request.server_nonce, expected, io.transferred,
        ok ? 1 : 0, io.error_number, io.eof, status);
}

void rpc_server::hfx_graph_invalidate() {
    hfx_graph_negotiated = false;
    hfx_graph_sequence = 0;
    hfx_graph_attempt_nonce.fill(0);
    hfx_graph_server_nonce.fill(0);
    hfx_graph_discard_lineage();
}

void rpc_server::hfx_graph_discard_lineage() {
    for (auto & stored : stored_graphs) {
        stored.auth_uid = 0;
        stored.auth_digest.fill(0);
        stored.auth_transcript.fill(0);
        stored.auth_prepared = false;
    }
}

void rpc_server::hfx_graph_invalidate_prepared() {
    for (auto & stored : stored_graphs) stored.auth_prepared = false;
}

bool rpc_server::hfx_mutable_preflight(
        const hfx_mutable_preflight_wire & request,
        hfx_mutable_preflight_wire & response) {
    if (!hfx_mutable_requested()) {
        GGML_LOG_ERROR("HaloFPX mutable preflight refusal: runtime_off\n");
        return false;
    }
    if (!hfx_graph_key(hfx_graph_key_value)) {
        GGML_LOG_ERROR("HaloFPX mutable preflight refusal: key\n");
        return false;
    }
    if (
        !hfx_magic(request.magic, "HFXMPQ3\0") ||
        request.major != HFX_MUTABLE_MAJOR ||
        request.minor != HFX_MUTABLE_MINOR ||
        request.encoded_size != sizeof(request) || request.status != 0 ||
        request.reserved != 0 ||
        request.capabilities != HFX_MUTABLE_CAPABILITIES ||
        request.key_generation == 0 ||
        request.client_connection_epoch == 0 ||
        request.server_connection_epoch != 0 ||
        request.allocation_topology_epoch != 0 ||
        hfx_zero(request.client_nonce, 32) ||
        !hfx_zero(request.server_nonce, 32) ||
        !hfx_mutable_verify(request, hfx_graph_key_value.data())) {
        GGML_LOG_ERROR(
            "HaloFPX mutable preflight refusal: request magic=%d version=%u.%u size=%u/%zu "
            "status=%u reserved=%u caps=%llu/%llu keygen=%llu client_epoch=%llu "
            "server_epoch=%llu allocation_epoch=%llu client_nonce_zero=%d "
            "server_nonce_zero=%d hmac=%d\n",
            hfx_magic(request.magic, "HFXMPQ3\0") ? 1 : 0,
            request.major, request.minor, request.encoded_size, sizeof(request),
            request.status, request.reserved,
            static_cast<unsigned long long>(request.capabilities),
            static_cast<unsigned long long>(HFX_MUTABLE_CAPABILITIES),
            static_cast<unsigned long long>(request.key_generation),
            static_cast<unsigned long long>(request.client_connection_epoch),
            static_cast<unsigned long long>(request.server_connection_epoch),
            static_cast<unsigned long long>(request.allocation_topology_epoch),
            hfx_zero(request.client_nonce, 32) ? 1 : 0,
            hfx_zero(request.server_nonce, 32) ? 1 : 0,
            hfx_mutable_verify(request, hfx_graph_key_value.data()) ? 1 : 0);
        return false;
    }
    response = request;
    hfx_set_magic(response.magic, "HFXMPR3\0");
    response.status = 1;
    response.server_connection_epoch = connection_epoch;
    response.allocation_topology_epoch = allocation_topology_epoch;
    if (!hfx_random_all(response.server_nonce, 32) ||
        !hfx_mutable_sign(response, hfx_graph_key_value.data())) return false;
    // Strictly non-mutating: no session, recorder-success, admission state,
    // graph lineage, tensor, allocation, or execution authority is changed.
    return true;
}

bool rpc_server::hfx_mutable_caps(
        const ::hfx_mutable_caps & request,
        ::hfx_mutable_caps & response) {
    hfx_mutable_session = {};
    preexecute_recorder.reset();
    if (hfx_preexecute_requested() && request.exec_sequence != 0 &&
        !hfx_zero(request.attempt_nonce, 32)) {
        preexecute_recorder = std::make_shared<hfx_preexecute_recorder>();
        preexecute_recorder->role = hfx_preexecute_recorder::SERVER;
        preexecute_recorder->generation =
            hfx_preexecute_next_generation.fetch_add(1, std::memory_order_relaxed);
        if (preexecute_recorder->generation == 0) {
            preexecute_recorder->generation =
                hfx_preexecute_next_generation.fetch_add(1, std::memory_order_relaxed);
        }
        preexecute_recorder->execution_sequence = request.exec_sequence;
        preexecute_recorder->parent_uid = request.graph_uid;
        preexecute_recorder->client_connection_epoch = request.client_connection_epoch;
        preexecute_recorder->server_connection_epoch = connection_epoch;
        preexecute_recorder->allocation_epoch = allocation_topology_epoch;
        preexecute_recorder->backend_ordinal =
            request.scheduler_admission.backend_ordinal;
        for (uint32_t i = 0;
             i < request.scheduler_admission.split_count && i < 64; ++i) {
            const auto & split = request.scheduler_admission.ordered_splits[i];
            if (split.backend_ordinal == preexecute_recorder->backend_ordinal) {
                preexecute_recorder->split_uid = split.split_graph_uid;
                preexecute_recorder->split_ordinal = split.split_ordinal;
                break;
            }
        }
        memcpy(preexecute_recorder->attempt_nonce.data(), request.attempt_nonce, 32);
        memcpy(preexecute_recorder->admission_object_id.data(),
               request.scheduler_admission.object_id, 32);
        memcpy(preexecute_recorder->expected_admission_digest.data(),
               request.expected_admission_digest, 32);
        memcpy(preexecute_recorder->graph_digest.data(),
               request.scheduler_admission.prepared_graph_digest, 32);
    }
    const bool key_loaded = hfx_graph_key(hfx_graph_key_value);
    const auto independently_expected_digest = hfx_sha256(
        &request.scheduler_expected_admission,
        offsetof(struct ggml_backend_sched_authority_prepared_admission, tag));
    const bool rejected =
        !hfx_mutable_requested() || !key_loaded ||
        !hfx_magic(request.magic, "HFXMAQ3\0") ||
        request.major != HFX_MUTABLE_MAJOR || request.minor != HFX_MUTABLE_MINOR ||
        request.encoded_size != sizeof(request) || request.status != 0 || request.reserved != 0 ||
        request.capabilities != HFX_MUTABLE_CAPABILITIES ||
        request.graph_auth_major != HFX_GRAPH_AUTH_MAJOR ||
        request.scheduler_admission_major != 3 ||
        request.mutable_major != HFX_MUTABLE_MAJOR ||
        request.version_reserved != 0 ||
        request.max_mutations == 0 || request.max_mutations > HFX_MUTABLE_MAX_MUTATIONS ||
        request.max_census == 0 || request.max_census > HFX_MUTABLE_MAX_CENSUS ||
        request.graph_uid == 0 || request.exec_sequence == 0 || request.scheduler_sequence == 0 ||
        hfx_zero(request.attempt_nonce, 32) || hfx_zero(request.scheduler_nonce, 32) ||
        hfx_zero(request.scheduler_root, 32) ||
        hfx_zero(request.expected_admission_digest, 32) ||
        !hfx_equal(request.expected_admission_digest,
                   independently_expected_digest.data(), 32) ||
        !hfx_zero(request.scheduler_expected_admission.tag, 32) ||
        request.client_connection_epoch == 0 ||
        request.server_connection_epoch != connection_epoch ||
        request.allocation_epoch != allocation_topology_epoch ||
        !hfx_zero(request.server_nonce, 32) ||
        request.scheduler_admission.client_connection_epoch !=
            request.client_connection_epoch ||
        request.scheduler_admission.server_connection_epoch != connection_epoch ||
        request.scheduler_admission.allocation_topology_epoch !=
            allocation_topology_epoch ||
        request.scheduler_admission.execution_sequence != request.exec_sequence ||
        request.scheduler_admission.parent_graph_uid != request.graph_uid ||
        !hfx_equal(request.scheduler_admission.attempt_nonce,
                   request.attempt_nonce, 32) ||
        !ggml_backend_sched_authority_verify_prepared_admission(
            &request.scheduler_admission, hfx_graph_key_value.data(),
            &request.scheduler_expected_admission) ||
        !hfx_mutable_verify(request, hfx_graph_key_value.data());
    if (rejected) {
        if (preexecute_recorder) {
            hfx_server_emit(preexecute_recorder,
                            HFX_SERVER_ADMISSION_REFUSED,
                            HFX_PREEXEC_CAPS_REFUSED);
            hfx_server_emit(preexecute_recorder, HFX_SERVER_ABORT,
                            HFX_PREEXEC_CAPS_REFUSED, true);
        }
        return false;
    }
    if (preexecute_recorder &&
        !hfx_preexecute_install_expected_census(
            preexecute_recorder, request.scheduler_admission)) {
        hfx_server_emit(preexecute_recorder,
                        HFX_SERVER_ADMISSION_REFUSED,
                        HFX_PREEXEC_PREPARED_MISMATCH);
        hfx_server_emit(preexecute_recorder, HFX_SERVER_ABORT,
                        HFX_PREEXEC_PREPARED_MISMATCH, true);
        return false;
    }
    response = request;
    hfx_set_magic(response.magic, "HFXMAR3\0");
    response.status = 1;
    response.server_connection_epoch = connection_epoch;
    response.allocation_epoch = allocation_topology_epoch;
    if (!hfx_random_all(response.server_nonce, 32)) return false;
    if (!hfx_mutable_sign(response, hfx_graph_key_value.data())) return false;
    hfx_mutable_session.caps = response;
    hfx_mutable_session.mutation_root = hfx_sha256(HFX_MUTABLE_DOMAIN, sizeof(HFX_MUTABLE_DOMAIN) - 1);
    hfx_mutable_session.semantic_root = hfx_mutable_session.mutation_root;
    hfx_mutable_session.admitted = true;
    if (preexecute_recorder) {
        preexecute_recorder->server_connection_epoch = response.server_connection_epoch;
        preexecute_recorder->allocation_epoch = response.allocation_epoch;
        if (!hfx_server_emit(preexecute_recorder,
                             HFX_SERVER_ADMISSION_ACCEPTED,
                             HFX_PREEXEC_OK)) {
            hfx_mutable_session = {};
            preexecute_recorder.reset();
            return false;
        }
    }
    return true;
}

bool rpc_server::hfx_mutable_decode_refusal(const std::vector<uint8_t> & wire) {
    if (!hfx_preexecute_requested() || wire.empty() ||
        !hfx_graph_key(hfx_graph_key_value)) return false;
    preexecute_recorder = std::make_shared<hfx_preexecute_recorder>();
    preexecute_recorder->generation =
        hfx_preexecute_next_generation.fetch_add(1, std::memory_order_relaxed);
    if (preexecute_recorder->generation == 0) {
        preexecute_recorder->generation =
            hfx_preexecute_next_generation.fetch_add(1, std::memory_order_relaxed);
    }
    preexecute_recorder->execution_sequence = UINT64_MAX;
    preexecute_recorder->server_connection_epoch = connection_epoch;
    preexecute_recorder->allocation_epoch = allocation_topology_epoch;
    const auto identity = hfx_preexecute_hmac(
        hfx_graph_key_value.data(), wire.data(), wire.size());
    preexecute_recorder->attempt_nonce = identity;
    return hfx_preexecute_emit(
               preexecute_recorder, HFX_PREEXEC_BEGIN, HFX_PREEXEC_OK) &&
        hfx_preexecute_emit(
               preexecute_recorder, HFX_PREEXEC_L44_BEGIN,
               HFX_PREEXEC_DECODE_REFUSED, HFX_TRANSPORT_DECODE_REFUSED,
               static_cast<uint8_t>(RPC_CMD_HALOFPX_MUTABLE_CAPS)) &&
        hfx_preexecute_emit(
               preexecute_recorder, HFX_PREEXEC_L44_ABORT,
               HFX_PREEXEC_DECODE_REFUSED, HFX_TRANSPORT_DECODE_REFUSED,
               static_cast<uint8_t>(RPC_CMD_HALOFPX_MUTABLE_CAPS),
               0, UINT32_MAX, wire.size(), wire.size(), 0, true);
}

bool rpc_server::hfx_mutable_epoch_valid() const {
    return hfx_mutable_session.admitted &&
        hfx_mutable_session.caps.server_connection_epoch == connection_epoch &&
        hfx_mutable_session.caps.allocation_epoch == allocation_topology_epoch;
}

bool rpc_server::hfx_mutable_set(
        const std::vector<uint8_t> & input,
        bool hash_only,
        hfx_mutable_receipt & response) {
    const size_t fixed = sizeof(hfx_mutable_update_header) + sizeof(rpc_tensor);
    if (!hfx_mutable_epoch_valid() || hfx_mutable_session.committed ||
        input.size() < fixed || (hash_only ? input.size() != fixed : input.size() <= fixed)) {
        GGML_LOG_ERROR(
            "[halofpx-mutable] set shape/session refusal admitted=%u committed=%u "
            "hash=%u input=%zu fixed=%zu\n",
            hfx_mutable_session.admitted ? 1u : 0u,
            hfx_mutable_session.committed ? 1u : 0u,
            hash_only ? 1u : 0u, input.size(), fixed);
        return false;
    }
    hfx_mutable_update_header request {};
    if (!hfx_mutable_decode(input.data(), sizeof(request), request) ||
        !hfx_magic(request.magic, hash_only ? "HFXMUH1\0" : "HFXMUS1\0") ||
        request.major != HFX_MUTABLE_MAJOR || request.minor != HFX_MUTABLE_MINOR ||
        request.encoded_size != sizeof(request) || !hfx_mutable_role_valid(request.role) ||
        request.type >= GGML_TYPE_COUNT || request.graph_uid != hfx_mutable_session.caps.graph_uid ||
        request.exec_sequence != hfx_mutable_session.caps.exec_sequence ||
        request.mutation_sequence != hfx_mutable_session.mutation_sequence + 1 ||
        request.mutation_sequence > hfx_mutable_session.caps.max_mutations ||
        request.logical_size == 0 || request.logical_size > HFX_MUTABLE_MAX_BYTES ||
        hfx_zero(request.content_digest, 32) ||
        !hfx_equal(request.attempt_nonce, hfx_mutable_session.caps.attempt_nonce, 32) ||
        !hfx_equal(request.server_nonce, hfx_mutable_session.caps.server_nonce, 32) ||
        !hfx_equal(request.prior_root, hfx_mutable_session.mutation_root.data(), 32) ||
        !hfx_mutable_verify(request, hfx_graph_key_value.data())) {
        GGML_LOG_ERROR("[halofpx-mutable] set header refusal seq=%" PRIu64 "\n", request.mutation_sequence); return false;
    }
    rpc_tensor wire_tensor {};
    memcpy(&wire_tensor, input.data() + sizeof(request), sizeof(wire_tensor));
    ggml_init_params params { ggml_tensor_overhead(), nullptr, true };
    ggml_context_ptr context { ggml_init(params) };
    if (!context) return false;
    ggml_tensor * tensor = deserialize_tensor(context.get(), &wire_tensor);
    if (!tensor || !tensor->buffer || buffers.find(tensor->buffer) == buffers.end()) {
        GGML_LOG_ERROR("[halofpx-mutable] set tensor refusal\n"); return false;
    }
    const auto oit = buffer_allocation_ordinals.find(tensor->buffer);
    if (oit == buffer_allocation_ordinals.end() || oit->second != request.allocation_ordinal ||
        tensor->type != request.type) {
        GGML_LOG_ERROR("[halofpx-mutable] set allocation refusal expected=%u actual=%u\n",
                       request.allocation_ordinal, oit == buffer_allocation_ordinals.end() ? UINT32_MAX : oit->second);
        return false;
    }
    for (size_t i = 0; i < GGML_MAX_DIMS; ++i) {
        if (tensor->ne[i] != request.ne[i] || tensor->nb[i] != request.nb[i]) {
            GGML_LOG_ERROR("[halofpx-mutable] set layout refusal dim=%zu\n", i); return false;
        }
    }
    std::vector<uint8_t> server_view_chain;
    for (const ggml_tensor * v = tensor; v; v = v->view_src) {
        hfx_le<uint64_t>(server_view_chain, v->view_offs);
        if (server_view_chain.size() > 8 * GGML_MAX_SRC) return false;
    }
    const auto server_view_digest = hfx_sha256(server_view_chain.data(), server_view_chain.size());
    if (!hfx_equal(server_view_digest.data(), request.view_digest, 32)) {
        GGML_LOG_ERROR("[halofpx-mutable] set view refusal\n"); return false;
    }
    const uint64_t base = reinterpret_cast<uint64_t>(ggml_backend_buffer_get_base(tensor->buffer));
    const uint64_t address = reinterpret_cast<uint64_t>(tensor->data);
    uint64_t end = 0;
    if (address < base || address - base != request.tensor_relative ||
        !hfx_add(request.logical_offset, request.logical_size, end) ||
        end > ggml_nbytes(tensor)) {
        GGML_LOG_ERROR("[halofpx-mutable] set range refusal rel=%" PRIu64 " expected=%" PRIu64 "\n",
                       address - base, request.tensor_relative); return false;
    }
    const uint8_t * material = nullptr;
    std::vector<uint8_t> cached;
    if (hash_only) {
        if (!get_cached_file(request.cache_hash, cached)) {
            memset(&response, 0, sizeof(response));
            hfx_set_magic(response.magic, "HFXMUR1\0");
            response.major = HFX_MUTABLE_MAJOR; response.minor = HFX_MUTABLE_MINOR;
            response.encoded_size = sizeof(response); response.status = 2;
            response.role = request.role; response.role_ordinal = request.role_ordinal;
            response.allocation_ordinal = request.allocation_ordinal;
            response.graph_uid = request.graph_uid; response.exec_sequence = request.exec_sequence;
            response.mutation_sequence = request.mutation_sequence;
            response.tensor_relative = request.tensor_relative;
            response.logical_offset = request.logical_offset;
            response.logical_size = request.logical_size;
            memcpy(response.attempt_nonce, request.attempt_nonce, 32);
            memcpy(response.server_nonce, request.server_nonce, 32);
            memcpy(response.content_digest, request.content_digest, 32);
            memcpy(response.view_digest, request.view_digest, 32);
            return hfx_mutable_sign(response, hfx_graph_key_value.data());
        }
        if (cached.size() != request.logical_size) return false;
        material = cached.data();
    } else {
        if (input.size() != fixed + request.logical_size) return false;
        material = input.data() + fixed;
    }
    const auto supplied = hfx_sha256(material, request.logical_size);
    if (!hfx_equal(supplied.data(), request.content_digest, 32)) {
        GGML_LOG_ERROR("[halofpx-mutable] set supplied digest refusal\n"); return false;
    }
    ggml_backend_tensor_set(tensor, material, request.logical_offset, request.logical_size);
    const auto dit = buffer_device_ordinals.find(tensor->buffer);
    if (dit == buffer_device_ordinals.end() || dit->second >= backends.size()) {
        GGML_LOG_ERROR("[halofpx-mutable] set owner refusal\n"); return false;
    }
    ggml_backend_t owner = backends[dit->second];
    ggml_backend_synchronize(owner);
    std::vector<uint8_t> applied(request.logical_size);
    ggml_backend_tensor_get(tensor, applied.data(), request.logical_offset, request.logical_size);
    const auto applied_digest = hfx_sha256(applied.data(), applied.size());
    if (!hfx_equal(applied_digest.data(), request.content_digest, 32)) {
        GGML_LOG_ERROR("[halofpx-mutable] set applied digest refusal\n"); return false;
    }
    if (!hash_only && cache_dir && request.logical_size > HASH_THRESHOLD) {
        char hash_str[17];
        snprintf(hash_str, sizeof(hash_str), "%016" PRIx64, fnv_hash(material, request.logical_size));
        std::ofstream ofs(fs::path(cache_dir) / hash_str, std::ios::binary);
        ofs.write(reinterpret_cast<const char *>(material), request.logical_size);
        if (!ofs) return false;
    }
    const auto root = hfx_mutable_chain(hfx_mutable_session.mutation_root, request);
    const auto semantic = hfx_mutable_semantic_chain(hfx_mutable_session.semantic_root, request);
    memset(&response, 0, sizeof(response));
    hfx_set_magic(response.magic, "HFXMUR1\0");
    response.major = HFX_MUTABLE_MAJOR; response.minor = HFX_MUTABLE_MINOR;
    response.encoded_size = sizeof(response); response.status = 1;
    response.role = request.role; response.role_ordinal = request.role_ordinal;
    response.allocation_ordinal = request.allocation_ordinal;
    response.graph_uid = request.graph_uid; response.exec_sequence = request.exec_sequence;
    response.mutation_sequence = request.mutation_sequence;
    response.tensor_relative = request.tensor_relative;
    response.logical_offset = request.logical_offset; response.logical_size = request.logical_size;
    memcpy(response.attempt_nonce, request.attempt_nonce, 32);
    memcpy(response.server_nonce, request.server_nonce, 32);
    memcpy(response.content_digest, applied_digest.data(), 32);
    memcpy(response.view_digest, server_view_digest.data(), 32);
    memcpy(response.mutation_root, root.data(), 32);
    if (!hfx_mutable_sign(response, hfx_graph_key_value.data())) return false;
    hfx_mutable_session.mutation_root = root;
    hfx_mutable_session.semantic_root = semantic;
    hfx_mutable_session.mutation_sequence = request.mutation_sequence;
    hfx_mutable_census_entry entry {};
    entry.role = request.role; entry.role_ordinal = request.role_ordinal;
    entry.allocation_ordinal = request.allocation_ordinal; entry.type = request.type;
    entry.tensor_relative = request.tensor_relative; entry.logical_size = ggml_nbytes(tensor);
    memcpy(entry.ne, request.ne, sizeof(entry.ne)); memcpy(entry.nb, request.nb, sizeof(entry.nb));
    memcpy(entry.view_digest, server_view_digest.data(), 32);
    hfx_mutable_session.mutations.push_back(entry);
    return true;
}

bool rpc_server::hfx_mutable_refusal(
        const std::vector<uint8_t> & input,
        hfx_mutable_receipt & response) {
    if (!hfx_mutable_session.admitted ||
        input.size() < sizeof(hfx_mutable_update_header) + sizeof(rpc_tensor)) return false;
    hfx_mutable_update_header request {};
    if (!hfx_mutable_decode(input.data(), sizeof(request), request) ||
        !hfx_equal(request.attempt_nonce, hfx_mutable_session.caps.attempt_nonce, 32) ||
        !hfx_equal(request.server_nonce, hfx_mutable_session.caps.server_nonce, 32)) return false;
    memset(&response, 0, sizeof(response));
    hfx_set_magic(response.magic, "HFXMUR1\0");
    response.major = HFX_MUTABLE_MAJOR;
    response.minor = HFX_MUTABLE_MINOR;
    response.encoded_size = sizeof(response);
    response.status = 0;
    response.role = request.role;
    response.role_ordinal = request.role_ordinal;
    response.allocation_ordinal = request.allocation_ordinal;
    response.graph_uid = request.graph_uid;
    response.exec_sequence = request.exec_sequence;
    response.mutation_sequence = request.mutation_sequence;
    response.tensor_relative = request.tensor_relative;
    response.logical_offset = request.logical_offset;
    response.logical_size = request.logical_size;
    memcpy(response.attempt_nonce, request.attempt_nonce, 32);
    memcpy(response.server_nonce, request.server_nonce, 32);
    memcpy(response.content_digest, request.content_digest, 32);
    memcpy(response.view_digest, request.view_digest, 32);
    memcpy(response.mutation_root, hfx_mutable_session.mutation_root.data(), 32);
    return hfx_mutable_sign(response, hfx_graph_key_value.data());
}

bool rpc_server::hfx_mutable_commit(
        const std::vector<uint8_t> & input,
        hfx_mutable_commit_header & response) {
    if (!hfx_mutable_epoch_valid() || hfx_mutable_session.committed ||
        input.size() < sizeof(hfx_mutable_commit_header)) {
        GGML_LOG_ERROR("[halofpx-mutable] commit session/shape refusal\n");
        return false;
    }
    hfx_mutable_commit_header request {};
    if (!hfx_mutable_decode(input.data(), sizeof(request), request)) return false;
    const bool prepare = hfx_magic(request.magic, "HFXMCP1\0");
    if ((!prepare && !hfx_magic(request.magic, "HFXMCC1\0")) ||
        (!prepare && !hfx_mutable_session.prepared) ||
        request.major != HFX_MUTABLE_MAJOR || request.minor != HFX_MUTABLE_MINOR ||
        request.encoded_size != input.size() ||
        request.census_count == 0 || request.census_count > hfx_mutable_session.caps.max_census ||
        request.mutation_count != hfx_mutable_session.mutation_sequence ||
        request.graph_uid != hfx_mutable_session.caps.graph_uid ||
        request.exec_sequence != hfx_mutable_session.caps.exec_sequence ||
        !hfx_equal(request.attempt_nonce, hfx_mutable_session.caps.attempt_nonce, 32) ||
        !hfx_equal(request.server_nonce, hfx_mutable_session.caps.server_nonce, 32) ||
        !hfx_equal(request.scheduler_nonce, hfx_mutable_session.caps.scheduler_nonce, 32) ||
        !hfx_equal(request.scheduler_root, hfx_mutable_session.caps.scheduler_root, 32) ||
        !hfx_equal(request.mutation_root, hfx_mutable_session.mutation_root.data(), 32) ||
        !hfx_mutable_verify(request, hfx_graph_key_value.data()) ||
        input.size() != sizeof(request) + size_t(request.census_count) * sizeof(hfx_mutable_census_entry)) {
        GGML_LOG_ERROR("[halofpx-mutable] commit header refusal prepare=%u mutations=%u server=%" PRIu64 "\n",
            prepare ? 1u : 0u, request.mutation_count, hfx_mutable_session.mutation_sequence);
        GGML_LOG_ERROR(
            "[halofpx-mutable] commit checks version=%u size=%u census=%u graph=%u exec=%u "
            "attempt=%u server_nonce=%u scheduler_nonce=%u scheduler_root=%u mutation_root=%u "
            "tag=%u wire=%u\n",
            request.major == HFX_MUTABLE_MAJOR && request.minor == HFX_MUTABLE_MINOR,
            request.encoded_size == input.size(),
            request.census_count != 0 && request.census_count <= hfx_mutable_session.caps.max_census,
            request.graph_uid == hfx_mutable_session.caps.graph_uid,
            request.exec_sequence == hfx_mutable_session.caps.exec_sequence,
            hfx_equal(request.attempt_nonce, hfx_mutable_session.caps.attempt_nonce, 32),
            hfx_equal(request.server_nonce, hfx_mutable_session.caps.server_nonce, 32),
            hfx_equal(request.scheduler_nonce, hfx_mutable_session.caps.scheduler_nonce, 32),
            hfx_equal(request.scheduler_root, hfx_mutable_session.caps.scheduler_root, 32),
            hfx_equal(request.mutation_root, hfx_mutable_session.mutation_root.data(), 32),
            hfx_mutable_verify(request, hfx_graph_key_value.data()),
            input.size() == sizeof(request) +
                size_t(request.census_count) * sizeof(hfx_mutable_census_entry));
        return false;
    }
    std::vector<hfx_mutable_census_entry> census(request.census_count);
    const auto & sealed = hfx_mutable_session.caps.scheduler_admission;
    std::unordered_set<uint64_t> census_role_keys;
    for (uint32_t i = 0; i < request.census_count; ++i) {
        const uint8_t * p = input.data() + sizeof(request) + i * sizeof(hfx_mutable_census_entry);
        if (!hfx_mutable_decode(p, sizeof(hfx_mutable_census_entry), census[i]) ||
            !hfx_mutable_census_role_valid(census[i].role) ||
            hfx_zero(census[i].logical_tensor_identity, 32) ||
            hfx_zero(census[i].storage_tensor_identity, 32) ||
            hfx_zero(census[i].runtime_semantic_identity, 32) ||
            hfx_zero(census[i].rpc_endpoint_identity, 32) ||
            census[i].rpc_connection_epoch != sealed.client_connection_epoch ||
            census[i].destination_backend_ordinal != sealed.backend_ordinal ||
            census[i].rpc_device == UINT32_MAX) return false;
        const uint64_t role_key =
            (uint64_t(census[i].role) << 32) | census[i].role_ordinal;
        if (!census_role_keys.insert(role_key).second) return false;
    }
    std::vector<uint8_t> census_wire(input.begin() + sizeof(request), input.end());
    const auto root = hfx_sha256(census_wire.data(), census_wire.size());
    if (!hfx_equal(root.data(), request.census_root, 32)) return false;
    uint32_t logical_mutable_count = 0;
    uint32_t logical_exclusion_count = 0;
    const auto logical_expected_census = hfx_mutable_logical_expected_census(
        census, logical_mutable_count, logical_exclusion_count);
    if (logical_mutable_count != sealed.logical_expected_mutable_count ||
        logical_exclusion_count != sealed.logical_expected_exclusion_count ||
        !hfx_equal(logical_expected_census.data(),
                   sealed.logical_expected_census_root, 32)) {
        GGML_LOG_ERROR("[halofpx-mutable] logical/physical census reconciliation refusal\n");
        return false;
    }
    for (const auto & mutation : hfx_mutable_session.mutations) {
        const auto found = std::find_if(census.begin(), census.end(), [&](const auto & entry) {
            return entry.role == mutation.role && entry.role_ordinal == mutation.role_ordinal &&
                entry.allocation_ordinal == mutation.allocation_ordinal &&
                entry.type == mutation.type && entry.tensor_relative == mutation.tensor_relative &&
                entry.logical_size == mutation.logical_size &&
                hfx_equal(entry.view_digest, mutation.view_digest, 32) &&
                memcmp(entry.ne, mutation.ne, sizeof(entry.ne)) == 0 &&
                memcmp(entry.nb, mutation.nb, sizeof(entry.nb)) == 0;
        });
        if (found == census.end()) {
            GGML_LOG_ERROR("[halofpx-mutable] commit mutation/census refusal role=%u ordinal=%u\n",
                mutation.role, mutation.role_ordinal);
            return false;
        }
    }
    response = request;
    hfx_set_magic(response.magic, prepare ? "HFXMPR1\0" : "HFXMCR1\0");
    response.encoded_size = sizeof(response);
    if (!hfx_mutable_sign(response, hfx_graph_key_value.data())) return false;
    hfx_mutable_session.prepared = true;
    hfx_mutable_session.committed = !prepare;
    hfx_mutable_session.census_count = request.census_count;
    hfx_mutable_session.census = census;
    if (prepare && preexecute_recorder &&
        !hfx_server_emit(preexecute_recorder,
                         HFX_SERVER_PHYSICAL_PREPARED,
                         HFX_PREEXEC_OK)) {
        return false;
    }
    return true;
}

bool rpc_server::hfx_mutable_bind(
        const hfx_mutable_commit_header & request,
        hfx_mutable_commit_header & response) {
    const auto & sealed = hfx_mutable_session.caps.scheduler_admission;
    const auto split_match = std::find_if(
        sealed.ordered_splits,
        sealed.ordered_splits + sealed.split_count,
        [&request](const auto & split) {
            return split.split_graph_uid == request.graph_uid &&
                split.split_ordinal == request.split_ordinal &&
                split.backend_ordinal == request.backend_ordinal;
        });
    if (!hfx_mutable_epoch_valid() || !hfx_mutable_session.prepared ||
        !hfx_magic(request.magic, "HFXMCB1\0") ||
        request.major != HFX_MUTABLE_MAJOR || request.minor != HFX_MUTABLE_MINOR ||
        request.encoded_size != sizeof(request) || request.graph_uid == 0 ||
        request.parent_graph_uid == 0 || request.split_ordinal == UINT32_MAX ||
        request.backend_ordinal == UINT32_MAX || hfx_zero(request.split_mapping_root, 32) ||
        request.parent_graph_uid != sealed.parent_graph_uid ||
        request.backend_ordinal != sealed.backend_ordinal ||
        split_match == sealed.ordered_splits + sealed.split_count ||
        !hfx_equal(request.split_mapping_root, sealed.split_mapping_root, 32) ||
        request.exec_sequence != hfx_mutable_session.caps.exec_sequence ||
        request.mutation_count != hfx_mutable_session.mutation_sequence ||
        request.census_count != hfx_mutable_session.census_count ||
        !hfx_equal(request.attempt_nonce, hfx_mutable_session.caps.attempt_nonce, 32) ||
        !hfx_equal(request.server_nonce, hfx_mutable_session.caps.server_nonce, 32) ||
        !hfx_equal(request.scheduler_nonce, hfx_mutable_session.caps.scheduler_nonce, 32) ||
        !hfx_equal(request.scheduler_root, hfx_mutable_session.caps.scheduler_root, 32) ||
        !hfx_equal(request.mutation_root, hfx_mutable_session.mutation_root.data(), 32) ||
        !hfx_mutable_verify(request, hfx_graph_key_value.data())) return false;
    hfx_mutable_session.caps.graph_uid = request.graph_uid;
    hfx_mutable_session.bound_parent_graph_uid = request.parent_graph_uid;
    hfx_mutable_session.bound_split_ordinal = request.split_ordinal;
    hfx_mutable_session.bound_backend_ordinal = request.backend_ordinal;
    if (preexecute_recorder) {
        preexecute_recorder->split_uid = request.graph_uid;
        preexecute_recorder->split_ordinal = request.split_ordinal;
        preexecute_recorder->backend_ordinal = request.backend_ordinal;
    }
    memcpy(hfx_mutable_session.split_mapping_root.data(), request.split_mapping_root, 32);
    response = request;
    hfx_set_magic(response.magic, "HFXMBR1\0");
    return hfx_mutable_sign(response, hfx_graph_key_value.data());
}

bool rpc_server::hfx_mutable_graph_census_valid(const ggml_cgraph * graph) const {
    if (!hfx_mutable_epoch_valid() || !hfx_mutable_session.prepared || graph == nullptr ||
        hfx_mutable_session.census.size() != hfx_mutable_session.census_count) return false;
    std::unordered_set<const ggml_tensor *> seen;
    std::vector<const ggml_tensor *> tensors;
    std::function<void(const ggml_tensor *)> collect = [&](const ggml_tensor * tensor) {
        if (tensor == nullptr || !seen.insert(tensor).second) return;
        if (tensor->view_src) collect(tensor->view_src);
        bool has_src = false;
        for (const ggml_tensor * src : tensor->src) {
            if (src) {
                has_src = true;
                collect(src);
            }
        }
        if (!has_src) tensors.push_back(tensor);
    };
    for (int i = 0; i < graph->n_nodes; ++i) collect(graph->nodes[i]);
    if (tensors.size() != hfx_mutable_session.census.size()) return false;
    std::unordered_set<const ggml_tensor *> matched_tensors;
    for (const auto & entry : hfx_mutable_session.census) {
        const ggml_tensor * matched = nullptr;
        for (const ggml_tensor * tensor : tensors) {
            if (matched_tensors.count(tensor) != 0) continue;
            const ggml_tensor * storage = tensor;
            uint64_t inherited = 0;
            while (storage->buffer == nullptr && storage->view_src != nullptr) {
                if (!hfx_add(inherited, storage->view_offs, inherited)) return false;
                storage = storage->view_src;
            }
            if (storage->buffer == nullptr) continue;
            const auto ordinal = buffer_allocation_ordinals.find(storage->buffer);
            if (ordinal == buffer_allocation_ordinals.end() || ordinal->second != entry.allocation_ordinal ||
                tensor->type != entry.type || ggml_nbytes(tensor) != entry.logical_size) continue;
            const uint64_t base = reinterpret_cast<uint64_t>(ggml_backend_buffer_get_base(storage->buffer));
            uint64_t address = reinterpret_cast<uint64_t>(tensor->data);
            if (address == 0) {
                if (!hfx_add(reinterpret_cast<uint64_t>(storage->data), inherited, address)) return false;
            }
            if (address < base || address - base != entry.tensor_relative) continue;
            bool layout = true;
            for (size_t i = 0; i < GGML_MAX_DIMS; ++i) {
                layout = layout && tensor->ne[i] == entry.ne[i] && tensor->nb[i] == entry.nb[i];
            }
            if (!layout) continue;
            std::vector<uint8_t> view;
            for (const ggml_tensor * v = tensor; v; v = v->view_src) {
                hfx_le<uint64_t>(view, v->view_offs);
                if (view.size() > 8 * GGML_MAX_SRC) return false;
            }
            const auto digest = hfx_sha256(view.data(), view.size());
            if (hfx_equal(digest.data(), entry.view_digest, 32)) { matched = tensor; break; }
        }
        if (!matched || !matched_tensors.insert(matched).second) return false;
    }
    return matched_tensors.size() == tensors.size();
}

extern "C" uint32_t ggml_backend_rpc_halofpx_graph_auth_self_test(void) {
    uint32_t passed = 0;
    std::array<uint8_t, 32> key {};
    for (size_t i = 0; i < key.size(); ++i) key[i] = static_cast<uint8_t>(i + 1);
    hfx_graph_auth_caps_req caps {};
    hfx_set_magic(caps.magic, "HFXGAQ1\0");
    caps.major = HFX_GRAPH_AUTH_MAJOR;
    caps.minor = HFX_GRAPH_AUTH_MINOR;
    caps.encoded_size = sizeof(caps);
    memset(caps.attempt_nonce, 0x5a, sizeof(caps.attempt_nonce));
    hfx_graph_sign_record(caps, key.data());
    if (hfx_graph_verify_record(caps, key.data())) passed |= 1U << 0;
    caps.attempt_nonce[0] ^= 1;
    if (!hfx_graph_verify_record(caps, key.data())) passed |= 1U << 1;

    hfx_graph_auth_header header {};
    hfx_set_magic(header.magic, "HFXGAX1\0");
    header.major = HFX_GRAPH_AUTH_MAJOR;
    header.minor = HFX_GRAPH_AUTH_MINOR;
    header.encoded_size = sizeof(header);
    header.graph_size = 64;
    header.device = 0;
    header.graph_uid = 7;
    header.exec_sequence = 3;
    memset(header.attempt_nonce, 1, 32);
    memset(header.server_nonce, 2, 32);
    memset(header.graph_digest, 3, 32);
    memset(header.transcript_root, 4, 32);
    hfx_graph_sign_record(header, key.data());
    hfx_digest attempt {};
    hfx_digest server {};
    memcpy(attempt.data(), header.attempt_nonce, 32);
    memcpy(server.data(), header.server_nonce, 32);
    if (hfx_graph_server_header_valid(header, key, attempt, server, 3, "HFXGAX1\0")) passed |= 1U << 2;
    if (!hfx_graph_server_header_valid(header, key, attempt, server, 2, "HFXGAX1\0")) passed |= 1U << 3;
    header.graph_uid ^= 1;
    if (!hfx_graph_server_header_valid(header, key, attempt, server, 3, "HFXGAX1\0")) passed |= 1U << 4;

    rpc_tensor tensor {};
    tensor.id = 1;
    tensor.type = GGML_TYPE_F32;
    tensor.ne[0] = tensor.ne[1] = tensor.ne[2] = tensor.ne[3] = 1;
    tensor.nb[0] = 4;
    tensor.nb[1] = tensor.nb[2] = tensor.nb[3] = 4;
    const uint64_t node = 1;
    hfx_digest digest {};
    const auto no_storage = [](uint64_t, uint32_t &, uint64_t &) { return false; };
    if (hfx_graph_canonical_digest(0, &node, 1, &tensor, 1, no_storage, digest)) passed |= 1U << 5;
    tensor.src[0] = 99;
    if (!hfx_graph_canonical_digest(0, &node, 1, &tensor, 1, no_storage, digest)) passed |= 1U << 6;
    tensor.src[0] = 0;
    rpc_tensor duplicate[2] { tensor, tensor };
    if (!hfx_graph_canonical_digest(0, &node, 1, duplicate, 2, no_storage, digest)) passed |= 1U << 7;
    const uint64_t unknown_node = 77;
    if (!hfx_graph_canonical_digest(0, &unknown_node, 1, &tensor, 1, no_storage, digest)) passed |= 1U << 8;
    if (!hfx_graph_canonical_digest(0, &node, HFX_GRAPH_AUTH_MAX_NODES + 1,
                                    &tensor, 1, no_storage, digest)) passed |= 1U << 9;
    hfx_graph_auth_receipt receipt {};
    header.graph_uid ^= 1;
    hfx_graph_sign_record(header, key.data());
    if (hfx_graph_server_receipt(receipt, header, key.data()) &&
        hfx_graph_receipt_valid(receipt, header, key.data())) passed |= 1U << 10;
    receipt.graph_digest[0] ^= 1;
    if (!hfx_graph_receipt_valid(receipt, header, key.data())) passed |= 1U << 11;
    const auto caps_wire = hfx_graph_encode(caps);
    hfx_graph_auth_caps_req caps_roundtrip {};
    if (hfx_graph_decode(caps_wire.data(), caps_wire.size(), caps_roundtrip) &&
        memcmp(&caps, &caps_roundtrip, sizeof(caps)) == 0) passed |= 1U << 12;
    std::vector<uint8_t> trailing = caps_wire;
    trailing.push_back(0);
    if (!hfx_graph_decode(trailing.data(), trailing.size(), caps_roundtrip)) passed |= 1U << 13;
    header.major = HFX_GRAPH_AUTH_MAJOR + 1;
    hfx_graph_sign_record(header, key.data());
    if (!hfx_graph_server_header_valid(header, key, attempt, server, 3, "HFXGAX1\0")) passed |= 1U << 14;
    tensor.buffer = 1;
    tensor.data = 1;
    const auto storage_one = [](uint64_t, uint32_t & ordinal, uint64_t & relative) {
        ordinal = 1;
        relative = 0;
        return true;
    };
    const auto storage_two = [](uint64_t, uint32_t & ordinal, uint64_t & relative) {
        ordinal = 2;
        relative = 0;
        return true;
    };
    hfx_digest digest_one {};
    hfx_digest digest_two {};
    if (hfx_graph_canonical_digest(0, &node, 1, &tensor, 1, storage_one, digest_one) &&
        hfx_graph_canonical_digest(0, &node, 1, &tensor, 1, storage_two, digest_two) &&
        !hfx_equal(digest_one.data(), digest_two.data(), digest_one.size())) passed |= 1U << 15;
    header.major = HFX_GRAPH_AUTH_MAJOR;
    header.graph_uid = 7;
    header.exec_sequence = 0;
    hfx_graph_sign_record(header, key.data());
    if (!hfx_graph_server_header_valid(header, key, attempt, server, 0, "HFXGAX1\0")) passed |= 1U << 16;
    header.exec_sequence = 1;
    header.graph_uid = 0;
    hfx_graph_sign_record(header, key.data());
    if (!hfx_graph_server_header_valid(header, key, attempt, server, 1, "HFXGAX1\0")) passed |= 1U << 17;
    return passed;
}

extern "C" uint32_t ggml_backend_rpc_halofpx_mutable_auth_self_test(void) {
    uint32_t passed = 0;
    hfx_digest key {};
    for (size_t i = 0; i < key.size(); ++i) key[i] = static_cast<uint8_t>(i + 3);
    hfx_mutable_caps caps {};
    hfx_set_magic(caps.magic, "HFXMCQ2\0");
    caps.major = HFX_MUTABLE_MAJOR; caps.minor = HFX_MUTABLE_MINOR;
    caps.encoded_size = sizeof(caps); caps.max_mutations = 8; caps.max_census = 8;
    caps.graph_uid = 9; caps.exec_sequence = 1; caps.scheduler_sequence = 4;
    memset(caps.attempt_nonce, 1, 32); memset(caps.scheduler_nonce, 2, 32);
    memset(caps.scheduler_root, 3, 32);
    if (hfx_mutable_sign(caps, key.data()) && hfx_mutable_verify(caps, key.data())) passed |= 1U << 0;
    auto wire = hfx_mutable_encode(caps);
    hfx_mutable_caps decoded {};
    if (hfx_mutable_decode(wire.data(), wire.size(), decoded) &&
        memcmp(&caps, &decoded, sizeof(caps)) == 0) passed |= 1U << 1;
    wire.push_back(0);
    if (!hfx_mutable_decode(wire.data(), wire.size(), decoded)) passed |= 1U << 2;
    caps.graph_uid ^= 1;
    if (!hfx_mutable_verify(caps, key.data())) passed |= 1U << 3;
    caps.graph_uid ^= 1;
    hfx_mutable_update_header update {};
    hfx_set_magic(update.magic, "HFXMUS1\0");
    update.major = HFX_MUTABLE_MAJOR; update.minor = HFX_MUTABLE_MINOR;
    update.encoded_size = sizeof(update); update.role = GGML_RPC_HALOFPX_MUTABLE_TOKEN;
    update.allocation_ordinal = 2; update.type = GGML_TYPE_I32;
    update.graph_uid = 9; update.exec_sequence = 1; update.mutation_sequence = 1;
    update.logical_size = 4; update.ne[0] = update.ne[1] = update.ne[2] = update.ne[3] = 1;
    update.nb[0] = update.nb[1] = update.nb[2] = update.nb[3] = 4;
    memset(update.attempt_nonce, 1, 32); memset(update.server_nonce, 4, 32);
    memset(update.content_digest, 5, 32); memset(update.prior_root, 6, 32);
    if (hfx_mutable_sign(update, key.data()) && hfx_mutable_verify(update, key.data())) passed |= 1U << 4;
    const auto root1 = hfx_mutable_chain(hfx_sha256(HFX_MUTABLE_DOMAIN, sizeof(HFX_MUTABLE_DOMAIN) - 1), update);
    update.mutation_sequence = 2; hfx_mutable_sign(update, key.data());
    const auto root2 = hfx_mutable_chain(root1, update);
    if (!hfx_equal(root1.data(), root2.data(), 32)) passed |= 1U << 5;
    update.role = 0; hfx_mutable_sign(update, key.data());
    if (!hfx_mutable_role_valid(update.role)) passed |= 1U << 6;
    update.role = GGML_RPC_HALOFPX_MUTABLE_SCHEDULER_COPY + 1;
    if (!hfx_mutable_role_valid(update.role)) passed |= 1U << 7;
    hfx_mutable_census_entry a {};
    a.role = GGML_RPC_HALOFPX_MUTABLE_TOKEN; a.role_ordinal = 0;
    hfx_mutable_census_entry b = a;
    if (std::tie(a.role, a.role_ordinal) >= std::tie(b.role, b.role_ordinal)) passed |= 1U << 8;
    uint64_t end = 0;
    if (!hfx_add(UINT64_MAX, 1, end)) passed |= 1U << 9;
    hfx_mutable_receipt receipt {};
    hfx_set_magic(receipt.magic, "HFXMUR1\0");
    receipt.major = HFX_MUTABLE_MAJOR; receipt.minor = HFX_MUTABLE_MINOR;
    receipt.encoded_size = sizeof(receipt); receipt.status = 1;
    receipt.role = GGML_RPC_HALOFPX_MUTABLE_TOKEN; receipt.allocation_ordinal = 2;
    receipt.graph_uid = 9; receipt.exec_sequence = 1; receipt.mutation_sequence = 1;
    receipt.logical_size = 4; memset(receipt.attempt_nonce, 1, 32);
    memset(receipt.server_nonce, 4, 32); memset(receipt.content_digest, 5, 32);
    memcpy(receipt.mutation_root, root1.data(), 32);
    if (hfx_mutable_sign(receipt, key.data()) && hfx_mutable_verify(receipt, key.data())) passed |= 1U << 10;
    receipt.content_digest[0] ^= 1;
    if (!hfx_mutable_verify(receipt, key.data())) passed |= 1U << 11;
    hfx_mutable_update_header semantic_a {};
    hfx_set_magic(semantic_a.magic, "HFXMUS1\0");
    semantic_a.major = HFX_MUTABLE_MAJOR; semantic_a.minor = HFX_MUTABLE_MINOR;
    semantic_a.encoded_size = sizeof(semantic_a);
    semantic_a.role = GGML_RPC_HALOFPX_MUTABLE_CAUSAL_MASK;
    semantic_a.type = GGML_TYPE_F32; semantic_a.graph_uid = 11;
    semantic_a.exec_sequence = 1; semantic_a.mutation_sequence = 1;
    semantic_a.logical_size = 16; semantic_a.ne[0] = 4;
    semantic_a.ne[1] = semantic_a.ne[2] = semantic_a.ne[3] = 1;
    semantic_a.nb[0] = 4; semantic_a.nb[1] = semantic_a.nb[2] = semantic_a.nb[3] = 16;
    memset(semantic_a.content_digest, 7, 32);
    const auto seed = hfx_sha256(HFX_MUTABLE_DOMAIN, sizeof(HFX_MUTABLE_DOMAIN) - 1);
    const auto semantic_set = hfx_mutable_semantic_chain(seed, semantic_a);
    auto semantic_hash = semantic_a;
    hfx_set_magic(semantic_hash.magic, "HFXMUH1\0");
    semantic_hash.cache_hash = 99;
    semantic_hash.exec_sequence = 2;
    semantic_hash.mutation_sequence = 4;
    if (hfx_equal(semantic_set.data(),
                  hfx_mutable_semantic_chain(seed, semantic_hash).data(), 32)) passed |= 1U << 12;
    semantic_hash.content_digest[0] ^= 1;
    if (!hfx_equal(semantic_set.data(),
                   hfx_mutable_semantic_chain(seed, semantic_hash).data(), 32)) passed |= 1U << 13;
    hfx_mutable_census_entry census {};
    census.role = GGML_RPC_HALOFPX_MUTABLE_SELECTED_KV; census.role_ordinal = 3;
    census.allocation_ordinal = 4; census.type = GGML_TYPE_Q8_0;
    census.tensor_relative = 64; census.logical_size = 1088;
    census.ne[0] = 32; census.ne[1] = census.ne[2] = census.ne[3] = 1;
    census.nb[0] = 34; census.nb[1] = census.nb[2] = census.nb[3] = 34;
    memset(census.view_digest, 8, 32);
    auto census_wire = hfx_mutable_encode(census);
    hfx_mutable_census_entry census_roundtrip {};
    if (hfx_mutable_decode(census_wire.data(), census_wire.size(), census_roundtrip) &&
        memcmp(&census, &census_roundtrip, sizeof(census)) == 0) passed |= 1U << 14;
    census_wire.push_back(0);
    if (!hfx_mutable_decode(census_wire.data(), census_wire.size(), census_roundtrip)) passed |= 1U << 15;
    std::unordered_set<uint64_t> roles;
    const uint64_t role_key = (uint64_t(census.role) << 32) | census.role_ordinal;
    roles.insert(role_key);
    if (!roles.insert(role_key).second) passed |= 1U << 16;
    uint64_t bounded = 0;
    if (hfx_add(HFX_MUTABLE_MAX_BYTES - 1, 1, bounded) && bounded == HFX_MUTABLE_MAX_BYTES) passed |= 1U << 17;
    return passed;
}

extern "C" bool ggml_backend_rpc_halofpx_preexecute_publication_self_test(void) {
    const auto make_recorder = [](uint64_t generation) {
        auto recorder = std::make_shared<hfx_preexecute_recorder>();
        recorder->generation = generation;
        recorder->execution_sequence = 1;
        recorder->client_connection_epoch = UINT64_C(0x67);
        for (size_t i = 0; i < recorder->attempt_nonce.size(); ++i) {
            recorder->attempt_nonce[i] =
                static_cast<uint8_t>(generation + i);
        }
        return recorder;
    };
    struct ggml_backend_sched_authority_prepared_admission admission {};
    admission.logical_expected_mutable_count = 2;
    admission.logical_expected_exclusion_count = 1;

    auto complete = make_recorder(UINT64_C(0x67));
    const bool complete_abort =
        hfx_preexecute_emit(complete, HFX_PREEXEC_BEGIN, HFX_PREEXEC_OK) &&
        hfx_preexecute_emit(complete, HFX_PREEXEC_L42_PREPARED, HFX_PREEXEC_OK) &&
        hfx_preexecute_install_expected_census(complete, admission) &&
        hfx_preexecute_emit(complete, HFX_PREEXEC_L44_BEGIN, HFX_PREEXEC_OK) &&
        hfx_preexecute_emit(complete, HFX_PREEXEC_L44_REGISTER, HFX_PREEXEC_OK) &&
        hfx_preexecute_emit(complete, HFX_PREEXEC_L44_REGISTER, HFX_PREEXEC_OK) &&
        hfx_preexecute_emit(complete, HFX_PREEXEC_L44_EXCLUDE, HFX_PREEXEC_OK) &&
        hfx_preexecute_emit(complete, HFX_PREEXEC_L44_ABORT,
                            HFX_PREEXEC_POST_ABORT,
                            HFX_TRANSPORT_NOT_ATTEMPTED, 0, 0, UINT32_MAX,
                            0, 0, 0, true);

    auto wrong = make_recorder(UINT64_C(0x68));
    admission.logical_expected_mutable_count = 1;
    admission.logical_expected_exclusion_count = 1;
    const bool wrong_counts_refused =
        hfx_preexecute_emit(wrong, HFX_PREEXEC_BEGIN, HFX_PREEXEC_OK) &&
        hfx_preexecute_emit(wrong, HFX_PREEXEC_L42_PREPARED, HFX_PREEXEC_OK) &&
        hfx_preexecute_install_expected_census(wrong, admission) &&
        hfx_preexecute_emit(wrong, HFX_PREEXEC_L44_BEGIN, HFX_PREEXEC_OK) &&
        hfx_preexecute_emit(wrong, HFX_PREEXEC_L44_REGISTER, HFX_PREEXEC_OK) &&
        hfx_preexecute_emit(wrong, HFX_PREEXEC_L44_REGISTER, HFX_PREEXEC_OK) &&
        hfx_preexecute_emit(wrong, HFX_PREEXEC_L44_EXCLUDE, HFX_PREEXEC_OK) &&
        !hfx_preexecute_emit(wrong, HFX_PREEXEC_L44_ABORT,
                             HFX_PREEXEC_POST_ABORT,
                             HFX_TRANSPORT_NOT_ATTEMPTED, 0, 0, UINT32_MAX,
                             0, 0, 0, true);

    auto pre_registration = make_recorder(UINT64_C(0x69));
    const bool pre_registration_abort =
        hfx_preexecute_emit(pre_registration, HFX_PREEXEC_BEGIN, HFX_PREEXEC_OK) &&
        hfx_preexecute_emit(
            pre_registration, HFX_PREEXEC_L42_PREPARED, HFX_PREEXEC_OK) &&
        hfx_preexecute_emit(pre_registration, HFX_PREEXEC_L44_BEGIN,
                            HFX_PREEXEC_PREPARED_MISMATCH) &&
        hfx_preexecute_emit(pre_registration, HFX_PREEXEC_L44_ABORT,
                            HFX_PREEXEC_POST_ABORT,
                            HFX_TRANSPORT_NOT_ATTEMPTED, 0, 0, UINT32_MAX,
                            0, 0, 0, true);
    const std::vector<uint32_t> server_success {
        HFX_SERVER_ADMISSION_ACCEPTED, HFX_SERVER_PHYSICAL_PREPARED,
        HFX_SERVER_EXECUTE_INTENT_CONSUMED, HFX_SERVER_BACKEND_EXECUTED,
        HFX_SERVER_RECEIPT_PUBLISHED, HFX_SERVER_CLOSE,
    };
    const std::vector<uint32_t> server_refusal {
        HFX_SERVER_ADMISSION_REFUSED, HFX_SERVER_ABORT,
    };
    auto missing = server_success;
    missing.erase(missing.begin() + 2);
    auto reordered = server_success;
    std::swap(reordered[1], reordered[2]);
    auto duplicate = server_success;
    duplicate.insert(duplicate.begin() + 2, HFX_SERVER_PHYSICAL_PREPARED);
    auto post_terminal = server_success;
    post_terminal.push_back(HFX_SERVER_ABORT);
    const std::vector<uint32_t> unknown {
        HFX_SERVER_ADMISSION_ACCEPTED, UINT32_MAX,
    };
    const bool server_grammar =
        hfx_server_sequence_valid(server_success, true) &&
        hfx_server_sequence_valid(server_refusal, true) &&
        !hfx_server_sequence_valid(missing, true) &&
        !hfx_server_sequence_valid(reordered, true) &&
        !hfx_server_sequence_valid(duplicate, true) &&
        !hfx_server_sequence_valid(post_terminal, true) &&
        !hfx_server_sequence_valid(unknown, false);
    return complete_abort && wrong_counts_refused &&
        pre_registration_abort && server_grammar;
}
#endif

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
    if (ok) ok = hfx_state_log_component_digest(
        "capture", stored, components, buffers, hfx_state_config->control_key.data());
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
    if (ok) ok = hfx_state_log_component_digest(
        "stage", stored, components, buffers, hfx_state_config->control_key.data());
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
        const uint64_t type_size = ggml_type_size(src->type);
        const uint64_t block_size = ggml_blck_size(src->type);
        uint64_t elements_u64 = 0;
        if (type_size == 0 || block_size == 0 ||
            hfx_state_pending->staged[i].size % type_size != 0 ||
            !hfx_mul(hfx_state_pending->staged[i].size / type_size, block_size, elements_u64) ||
            elements_u64 == 0 || elements_u64 > INT64_MAX) { ok = false; break; }
        const int64_t elements = static_cast<int64_t>(elements_u64);
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
        if (ok) ok = hfx_state_log_component_digest(
            "apply", applied_components, live, buffers, hfx_state_config->control_key.data());
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
        , hfx_state_config, sock->authority_epoch()
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
        if (cmd == RPC_CMD_ALLOC_BUFFER || cmd == RPC_CMD_FREE_BUFFER ||
            cmd == RPC_CMD_GRAPH_COMPUTE || cmd == RPC_CMD_INIT_TENSOR || cmd == RPC_CMD_GRAPH_RECOMPUTE) {
            server.hfx_graph_discard_lineage();
        } else if (cmd == RPC_CMD_BUFFER_CLEAR || cmd == RPC_CMD_SET_TENSOR ||
                   cmd == RPC_CMD_SET_TENSOR_HASH || cmd == RPC_CMD_COPY_TENSOR) {
            server.hfx_graph_invalidate_prepared();
        }
        if (cmd == RPC_CMD_ALLOC_BUFFER || cmd == RPC_CMD_FREE_BUFFER || cmd == RPC_CMD_BUFFER_CLEAR ||
            cmd == RPC_CMD_SET_TENSOR || cmd == RPC_CMD_SET_TENSOR_HASH || cmd == RPC_CMD_COPY_TENSOR ||
            cmd == RPC_CMD_GRAPH_COMPUTE || cmd == RPC_CMD_INIT_TENSOR || cmd == RPC_CMD_GRAPH_RECOMPUTE ||
            cmd == RPC_CMD_HALOFPX_GRAPH_AUTH_COMPUTE || cmd == RPC_CMD_HALOFPX_GRAPH_AUTH_RECOMPUTE ||
            cmd == RPC_CMD_HALOFPX_GRAPH_AUTH_EXECUTE) {
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
#ifdef GGML_RPC_HALOFPX_LOCAL_STATE
            case RPC_CMD_HALOFPX_GRAPH_AUTH_CAPS: {
                hfx_graph_auth_caps_req request {};
                hfx_graph_auth_caps_rsp response {};
                std::vector<uint8_t> request_wire;
                if (!recv_msg_bounded(sock, request_wire, sizeof(request)) ||
                    !hfx_graph_decode(request_wire.data(), request_wire.size(), request) ||
                    !server.hfx_graph_caps(request, response)) return;
                const auto response_wire = hfx_graph_encode(response);
                if (!send_msg(sock, response_wire.data(), response_wire.size())) return;
                break;
            }
            case RPC_CMD_HALOFPX_GRAPH_AUTH_COMPUTE: {
                std::vector<uint8_t> input;
                hfx_graph_auth_receipt response {};
                if (!recv_msg_bounded(sock, input,
                                      sizeof(hfx_graph_auth_header) + HFX_GRAPH_AUTH_MAX_GRAPH_BYTES) ||
                    !server.hfx_graph_compute(input, response)) return;
                const auto response_wire = hfx_graph_encode(response);
                if (!send_msg(sock, response_wire.data(), response_wire.size())) return;
                break;
            }
            case RPC_CMD_HALOFPX_GRAPH_AUTH_RECOMPUTE: {
                hfx_graph_auth_header request {};
                hfx_graph_auth_receipt response {};
                std::vector<uint8_t> request_wire;
                if (!recv_msg_bounded(sock, request_wire, sizeof(request)) ||
                    !hfx_graph_decode(request_wire.data(), request_wire.size(), request) ||
                    !server.hfx_graph_recompute(request, response)) return;
                const auto response_wire = hfx_graph_encode(response);
                if (!send_msg(sock, response_wire.data(), response_wire.size())) return;
                break;
            }
            case RPC_CMD_HALOFPX_GRAPH_AUTH_EXECUTE: {
                hfx_graph_auth_header request {};
                hfx_graph_auth_receipt response {};
                std::vector<uint8_t> request_wire;
                if (!recv_msg_bounded(sock, request_wire, sizeof(request)) ||
                    !hfx_graph_decode(request_wire.data(), request_wire.size(), request) ||
                    !server.hfx_graph_execute(request, response)) return;
                const auto response_wire = hfx_graph_encode(response);
                rpc_transport_io_result io {};
                const uint64_t response_size = response_wire.size();
                bool sent = sock->send_data_observed(&response_size, sizeof(response_size), io);
                if (!server.hfx_graph_response_event(request, "response_header_publish",
                                                     sizeof(response_size), io, sent, 0) || !sent) return;
                sent = sock->send_data_observed(response_wire.data(), response_wire.size(), io);
                if (!server.hfx_graph_response_event(request, "response_body_publish",
                                                     response_wire.size(), io, sent, 0) ||
                    !sent || !server.hfx_graph_response_published()) return;
                break;
            }
            case RPC_CMD_HALOFPX_MUTABLE_CAPS: {
                std::vector<uint8_t> wire;
                hfx_mutable_caps request {};
                hfx_mutable_caps response {};
                if (!recv_msg_bounded(sock, wire, sizeof(request))) return;
                const char * injection = std::getenv("HALOFPX_RPC_SERVER_INJECT");
                if (injection != nullptr &&
                    std::strcmp(injection, "mutable_caps_decode") == 0 &&
                    !wire.empty()) wire.pop_back();
                if (!hfx_mutable_decode(wire.data(), wire.size(), request)) {
                    server.hfx_mutable_decode_refusal(wire);
                    return;
                }
                if (
                    !server.hfx_mutable_caps(request, response)) return;
                auto output = hfx_mutable_encode(response);
                if (injection != nullptr &&
                    std::strcmp(injection, "mutable_caps_receipt") == 0 &&
                    !output.empty()) output.back() ^= 0x80;
                if (!send_msg(sock, output.data(), output.size())) return;
                break;
            }
            case RPC_CMD_HALOFPX_MUTABLE_PREFLIGHT: {
                std::vector<uint8_t> wire;
                hfx_mutable_preflight_wire request {};
                hfx_mutable_preflight_wire response {};
                if (!recv_msg_bounded(sock, wire, sizeof(request)) ||
                    !hfx_mutable_decode(wire.data(), wire.size(), request) ||
                    !server.hfx_mutable_preflight(request, response)) return;
                const auto output = hfx_mutable_encode(response);
                if (!send_msg(sock, output.data(), output.size())) return;
                break;
            }
            case RPC_CMD_HALOFPX_MUTABLE_SET:
            case RPC_CMD_HALOFPX_MUTABLE_SET_HASH: {
                std::vector<uint8_t> wire;
                hfx_mutable_receipt response {};
                const size_t limit = sizeof(hfx_mutable_update_header) + sizeof(rpc_tensor) +
                    (cmd == RPC_CMD_HALOFPX_MUTABLE_SET ? HFX_MUTABLE_MAX_BYTES : 0);
                if (!recv_msg_bounded(sock, wire, limit)) return;
                if (!server.hfx_mutable_set(wire, cmd == RPC_CMD_HALOFPX_MUTABLE_SET_HASH, response) &&
                    !server.hfx_mutable_refusal(wire, response)) return;
                const auto output = hfx_mutable_encode(response);
                if (!send_msg(sock, output.data(), output.size())) return;
                break;
            }
            case RPC_CMD_HALOFPX_MUTABLE_COMMIT: {
                std::vector<uint8_t> wire;
                hfx_mutable_commit_header response {};
                const size_t limit = sizeof(hfx_mutable_commit_header) +
                    size_t(HFX_MUTABLE_MAX_CENSUS) * sizeof(hfx_mutable_census_entry);
                if (!recv_msg_bounded(sock, wire, limit) ||
                    !server.hfx_mutable_commit(wire, response)) return;
                const auto output = hfx_mutable_encode(response);
                if (!send_msg(sock, output.data(), output.size())) return;
                break;
            }
            case RPC_CMD_HALOFPX_MUTABLE_BIND: {
                std::vector<uint8_t> wire;
                hfx_mutable_commit_header request {};
                hfx_mutable_commit_header response {};
                if (!recv_msg_bounded(sock, wire, sizeof(request)) ||
                    !hfx_mutable_decode(wire.data(), wire.size(), request) ||
                    !server.hfx_mutable_bind(request, response)) return;
                const auto output = hfx_mutable_encode(response);
                if (!send_msg(sock, output.data(), output.size())) return;
                break;
            }
#endif
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
