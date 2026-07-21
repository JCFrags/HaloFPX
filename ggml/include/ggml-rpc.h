#pragma once

#include "ggml-backend.h"

#ifdef  __cplusplus
extern "C" {
#endif

#define RPC_PROTO_MAJOR_VERSION    4
#define RPC_PROTO_MINOR_VERSION    0
#define RPC_PROTO_PATCH_VERSION    1

#ifdef  __cplusplus
static_assert(GGML_OP_COUNT == 102, "GGML_OP_COUNT has changed - update RPC_PROTO_PATCH_VERSION");
#endif

#define GGML_RPC_MAX_SERVERS       16

#define GGML_RPC_HALOFPX_STATE_DIGEST_BYTES       32
#define GGML_RPC_HALOFPX_STATE_KEY_BYTES          32
#define GGML_RPC_HALOFPX_STATE_MAX_COMPONENTS   4096
#define GGML_RPC_HALOFPX_STATE_MAX_REQUEST   1048576

typedef enum ggml_backend_rpc_halofpx_state_status {
    GGML_RPC_HALOFPX_STATE_DISABLED = 0,
    GGML_RPC_HALOFPX_STATE_STORED,
    GGML_RPC_HALOFPX_STATE_READY,
    GGML_RPC_HALOFPX_STATE_APPLIED,
    GGML_RPC_HALOFPX_STATE_ABORTED,
    GGML_RPC_HALOFPX_STATE_MISS,
    GGML_RPC_HALOFPX_STATE_REJECTED,
    GGML_RPC_HALOFPX_STATE_STORAGE_ERROR,
    GGML_RPC_HALOFPX_STATE_APPLY_ERROR,
} ggml_backend_rpc_halofpx_state_status;

typedef enum ggml_backend_rpc_halofpx_component_kind {
    GGML_RPC_HALOFPX_COMPONENT_ATTENTION_K = 1,
    GGML_RPC_HALOFPX_COMPONENT_ATTENTION_V = 2,
} ggml_backend_rpc_halofpx_component_kind;

typedef struct ggml_backend_rpc_halofpx_state_server_config {
    const char * root;
    uint32_t logical_rank;
    uint32_t world_size;
    uint64_t key_generation;
    uint8_t control_key[GGML_RPC_HALOFPX_STATE_KEY_BYTES];
    uint8_t channel_binding[GGML_RPC_HALOFPX_STATE_DIGEST_BYTES];
} ggml_backend_rpc_halofpx_state_server_config;

typedef struct ggml_backend_rpc_halofpx_state_identity {
    uint64_t key_generation;
    uint64_t generation;
    uint64_t token_count;
    uint64_t token_boundary;
    uint32_t world_size;
    uint32_t logical_rank;
    uint8_t model_digest[GGML_RPC_HALOFPX_STATE_DIGEST_BYTES];
    uint8_t compatibility_root[GGML_RPC_HALOFPX_STATE_DIGEST_BYTES];
    uint8_t plan_digest[GGML_RPC_HALOFPX_STATE_DIGEST_BYTES];
    uint8_t topology_digest[GGML_RPC_HALOFPX_STATE_DIGEST_BYTES];
    uint8_t placement_digest[GGML_RPC_HALOFPX_STATE_DIGEST_BYTES];
    uint8_t checkpoint_digest[GGML_RPC_HALOFPX_STATE_DIGEST_BYTES];
    uint8_t token_prefix_digest[GGML_RPC_HALOFPX_STATE_DIGEST_BYTES];
    uint8_t component_manifest_digest[GGML_RPC_HALOFPX_STATE_DIGEST_BYTES];
    uint8_t attempt_nonce[GGML_RPC_HALOFPX_STATE_DIGEST_BYTES];
    uint8_t channel_binding[GGML_RPC_HALOFPX_STATE_DIGEST_BYTES];
} ggml_backend_rpc_halofpx_state_identity;

typedef struct ggml_backend_rpc_halofpx_state_component {
    struct ggml_tensor * tensor;
    uint32_t ordinal;
    uint32_t kind;
    uint64_t offset;
    uint64_t size;
    uint8_t label_digest[GGML_RPC_HALOFPX_STATE_DIGEST_BYTES];
} ggml_backend_rpc_halofpx_state_component;

typedef struct ggml_backend_rpc_halofpx_state_result {
    ggml_backend_rpc_halofpx_state_status status;
    uint32_t logical_rank;
    uint64_t generation;
    uint64_t verified_bytes;
    uint32_t verified_components;
    uint8_t object_digest[GGML_RPC_HALOFPX_STATE_DIGEST_BYTES];
    uint8_t worker_nonce[GGML_RPC_HALOFPX_STATE_DIGEST_BYTES];
} ggml_backend_rpc_halofpx_state_result;

// backend API
GGML_BACKEND_API ggml_backend_t ggml_backend_rpc_init(const char * endpoint, uint32_t device);
GGML_BACKEND_API bool ggml_backend_is_rpc(ggml_backend_t backend);

GGML_BACKEND_API ggml_backend_buffer_type_t ggml_backend_rpc_buffer_type(const char * endpoint, uint32_t device);

GGML_BACKEND_API void ggml_backend_rpc_get_device_memory(const char * endpoint, uint32_t device, size_t * free, size_t * total);

GGML_BACKEND_API void ggml_backend_rpc_start_server(const char * endpoint, const char * cache_dir,
                                                    size_t n_threads, size_t n_devices, ggml_backend_dev_t * devices);

// L11 successor canary. These entry points are inert unless the RPC backend is
// compiled with GGML_RPC_HALOFPX_LOCAL_STATE and the worker receives a complete
// operator-owned server configuration. Requests contain descriptors only;
// tensor payload remains on the worker.
GGML_BACKEND_API void ggml_backend_rpc_start_server_with_halofpx_state(
        const char * endpoint, const char * cache_dir,
        size_t n_threads, size_t n_devices, ggml_backend_dev_t * devices,
        const ggml_backend_rpc_halofpx_state_server_config * state_config);

GGML_BACKEND_API ggml_backend_rpc_halofpx_state_result ggml_backend_rpc_halofpx_state_capture(
        const ggml_backend_rpc_halofpx_state_identity * identity,
        const ggml_backend_rpc_halofpx_state_component * components,
        size_t component_count,
        const uint8_t control_key[GGML_RPC_HALOFPX_STATE_KEY_BYTES]);

GGML_BACKEND_API ggml_backend_rpc_halofpx_state_result ggml_backend_rpc_halofpx_state_stage(
        const ggml_backend_rpc_halofpx_state_identity * identity,
        const ggml_backend_rpc_halofpx_state_component * components,
        size_t component_count,
        const uint8_t expected_object_digest[GGML_RPC_HALOFPX_STATE_DIGEST_BYTES],
        const uint8_t control_key[GGML_RPC_HALOFPX_STATE_KEY_BYTES]);

GGML_BACKEND_API ggml_backend_rpc_halofpx_state_result ggml_backend_rpc_halofpx_state_commit_apply(
        const ggml_backend_rpc_halofpx_state_identity * identity,
        const ggml_backend_rpc_halofpx_state_component * live_components,
        size_t component_count,
        const uint8_t expected_object_digest[GGML_RPC_HALOFPX_STATE_DIGEST_BYTES],
        const uint8_t worker_nonce[GGML_RPC_HALOFPX_STATE_DIGEST_BYTES],
        const uint8_t control_key[GGML_RPC_HALOFPX_STATE_KEY_BYTES]);

GGML_BACKEND_API ggml_backend_rpc_halofpx_state_result ggml_backend_rpc_halofpx_state_abort(
        const ggml_backend_rpc_halofpx_state_identity * identity,
        const uint8_t worker_nonce[GGML_RPC_HALOFPX_STATE_DIGEST_BYTES],
        const uint8_t control_key[GGML_RPC_HALOFPX_STATE_KEY_BYTES]);

GGML_BACKEND_API bool ggml_backend_rpc_halofpx_state_sha256(
        const void * data, size_t size,
        uint8_t digest[GGML_RPC_HALOFPX_STATE_DIGEST_BYTES]);

GGML_BACKEND_API ggml_backend_reg_t ggml_backend_rpc_reg(void);
GGML_BACKEND_API ggml_backend_reg_t ggml_backend_rpc_add_server(const char * endpoint);

#ifdef  __cplusplus
}
#endif
