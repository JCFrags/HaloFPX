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

// Runtime-default-off authority for mutable tensors used by one admitted RPC
// execution. Roles are supplied by the source call site which materializes the
// tensor; names, sizes, and GGML_TENSOR_FLAG_INPUT are never role authority.
typedef enum ggml_backend_rpc_halofpx_mutable_role {
    GGML_RPC_HALOFPX_MUTABLE_TOKEN = 1,
    GGML_RPC_HALOFPX_MUTABLE_INPUT_EMBEDDING,
    GGML_RPC_HALOFPX_MUTABLE_ABSOLUTE_POSITION,
    GGML_RPC_HALOFPX_MUTABLE_RELATIVE_POSITION,
    GGML_RPC_HALOFPX_MUTABLE_SEQUENCE_ID,
    GGML_RPC_HALOFPX_MUTABLE_KV_WRITE_INDEX,
    GGML_RPC_HALOFPX_MUTABLE_KV_CELL,
    GGML_RPC_HALOFPX_MUTABLE_CAUSAL_MASK,
    GGML_RPC_HALOFPX_MUTABLE_OUTPUT_ID,
    GGML_RPC_HALOFPX_MUTABLE_OUTPUT_MAP,
    GGML_RPC_HALOFPX_MUTABLE_SELECTED_KV,
    GGML_RPC_HALOFPX_MUTABLE_ARCHITECTURE_INPUT,
    GGML_RPC_HALOFPX_MUTABLE_SCHEDULER_COPY,
} ggml_backend_rpc_halofpx_mutable_role;

typedef enum ggml_backend_rpc_halofpx_exclusion {
    GGML_RPC_HALOFPX_EXCLUDE_IMMUTABLE_MODEL_WEIGHT = 1,
    GGML_RPC_HALOFPX_EXCLUDE_LOCAL_STATE_PAYLOAD = 2,
} ggml_backend_rpc_halofpx_exclusion;

typedef struct ggml_backend_rpc_halofpx_mutable_attempt {
    uint32_t version;
    uint32_t max_mutations;
    uint32_t max_census_entries;
    uint32_t reserved;
    uint64_t graph_uid;
    uint64_t execution_sequence;
    uint64_t scheduler_execution_sequence;
    uint8_t attempt_nonce[GGML_RPC_HALOFPX_STATE_DIGEST_BYTES];
    uint8_t scheduler_attempt_nonce[GGML_RPC_HALOFPX_STATE_DIGEST_BYTES];
    uint8_t scheduler_transcript_root[GGML_RPC_HALOFPX_STATE_DIGEST_BYTES];
} ggml_backend_rpc_halofpx_mutable_attempt;

typedef struct ggml_backend_rpc_halofpx_mutable_result {
    uint32_t version;
    uint32_t mutation_count;
    uint32_t census_count;
    uint32_t status;
    uint32_t set_count;
    uint32_t set_hash_hit_count;
    uint32_t set_hash_miss_count;
    uint32_t reserved;
    uint64_t graph_uid;
    uint64_t execution_sequence;
    uint8_t mutation_root[GGML_RPC_HALOFPX_STATE_DIGEST_BYTES];
    uint8_t semantic_root[GGML_RPC_HALOFPX_STATE_DIGEST_BYTES];
    uint8_t census_root[GGML_RPC_HALOFPX_STATE_DIGEST_BYTES];
    uint8_t receipt_tag[GGML_RPC_HALOFPX_STATE_DIGEST_BYTES];
} ggml_backend_rpc_halofpx_mutable_result;

// Value handle for one admitted mutable-authority lifetime. It contains no
// pointer. Every operation revalidates all fields against private session
// state; closed and stale values are permanently invalid.
typedef struct ggml_backend_rpc_halofpx_mutable_session {
    uint32_t version;
    uint32_t reserved;
    uint64_t session_id;
    uint64_t generation;
    uint64_t connection_epoch;
    uint64_t graph_uid;
    uint64_t execution_sequence;
    uint8_t attempt_nonce[GGML_RPC_HALOFPX_STATE_DIGEST_BYTES];
} ggml_backend_rpc_halofpx_mutable_session;

typedef struct ggml_backend_rpc_halofpx_graph_result {
    uint32_t version;
    uint32_t status;
    uint64_t graph_uid;
    uint64_t execution_sequence;
    uint8_t graph_digest[GGML_RPC_HALOFPX_STATE_DIGEST_BYTES];
    uint8_t transcript_root[GGML_RPC_HALOFPX_STATE_DIGEST_BYTES];
    uint8_t receipt_tag[GGML_RPC_HALOFPX_STATE_DIGEST_BYTES];
} ggml_backend_rpc_halofpx_graph_result;

typedef enum ggml_backend_rpc_halofpx_mutable_test_case {
    GGML_RPC_HALOFPX_MUTABLE_TEST_MALFORMED = 1,
    GGML_RPC_HALOFPX_MUTABLE_TEST_TAMPERED,
    GGML_RPC_HALOFPX_MUTABLE_TEST_DUPLICATE_SEQUENCE,
    GGML_RPC_HALOFPX_MUTABLE_TEST_OUT_OF_BOUNDS,
    GGML_RPC_HALOFPX_MUTABLE_TEST_WRONG_VIEW,
} ggml_backend_rpc_halofpx_mutable_test_case;

// backend API
GGML_BACKEND_API ggml_backend_t ggml_backend_rpc_init(const char * endpoint, uint32_t device);
GGML_BACKEND_API bool ggml_backend_is_rpc(ggml_backend_t backend);
GGML_BACKEND_API bool ggml_backend_rpc_halofpx_execution_arm(
        ggml_backend_t backend,
        const uint8_t attempt_nonce[GGML_RPC_HALOFPX_STATE_DIGEST_BYTES],
        uint64_t execution_sequence);
GGML_BACKEND_API bool ggml_backend_rpc_halofpx_execution_disarm(
        ggml_backend_t backend,
        const uint8_t attempt_nonce[GGML_RPC_HALOFPX_STATE_DIGEST_BYTES],
        uint64_t execution_sequence);

GGML_BACKEND_API ggml_backend_buffer_type_t ggml_backend_rpc_buffer_type(const char * endpoint, uint32_t device);

GGML_BACKEND_API void ggml_backend_rpc_get_device_memory(const char * endpoint, uint32_t device, size_t * free, size_t * total);

GGML_BACKEND_API bool ggml_backend_rpc_halofpx_mutable_begin(
        ggml_backend_t backend,
        ggml_backend_sched_t scheduler,
        const ggml_backend_rpc_halofpx_mutable_attempt * attempt,
        ggml_backend_rpc_halofpx_mutable_session * session);

GGML_BACKEND_API uint64_t ggml_backend_rpc_halofpx_mutable_graph_uid(
        struct ggml_cgraph * graph);

GGML_BACKEND_API bool ggml_backend_rpc_halofpx_mutable_register(
        const ggml_backend_rpc_halofpx_mutable_session * session,
        struct ggml_tensor * tensor,
        ggml_backend_rpc_halofpx_mutable_role role,
        uint32_t role_ordinal);

GGML_BACKEND_API bool ggml_backend_rpc_halofpx_mutable_exclude(
        const ggml_backend_rpc_halofpx_mutable_session * session,
        struct ggml_tensor * tensor,
        ggml_backend_rpc_halofpx_exclusion exclusion,
        uint32_t exclusion_ordinal);

GGML_BACKEND_API bool ggml_backend_rpc_halofpx_mutable_prepare(
        const ggml_backend_rpc_halofpx_mutable_session * session,
        struct ggml_cgraph * graph,
        ggml_backend_rpc_halofpx_mutable_result * result);

GGML_BACKEND_API bool ggml_backend_rpc_halofpx_mutable_commit(
        const ggml_backend_rpc_halofpx_mutable_session * session,
        struct ggml_cgraph * graph,
        ggml_backend_rpc_halofpx_mutable_result * result);

GGML_BACKEND_API bool ggml_backend_rpc_halofpx_mutable_abort(
        ggml_backend_rpc_halofpx_mutable_session * session);
GGML_BACKEND_API bool ggml_backend_rpc_halofpx_graph_result_get(
        ggml_backend_t backend,
        ggml_backend_rpc_halofpx_graph_result * result);
// Qualification-only negative injection. Requires
// HALOFPX_RPC_MUTABLE_AUTH_TEST=1 and drives the real server SET handler.
GGML_BACKEND_API bool ggml_backend_rpc_halofpx_mutable_test_inject(
        const ggml_backend_rpc_halofpx_mutable_session * session,
        struct ggml_tensor * tensor,
        ggml_backend_rpc_halofpx_mutable_test_case test_case,
        uint32_t * exact_handler_status);
GGML_BACKEND_API bool ggml_backend_rpc_halofpx_mutable_test_commit_omit_unmutated_leaf(
        const ggml_backend_rpc_halofpx_mutable_session * session);
GGML_BACKEND_API uint32_t ggml_backend_rpc_halofpx_mutable_auth_self_test(void);

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
