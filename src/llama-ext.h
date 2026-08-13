#pragma once

// this is a staging header for new llama.cpp API
// breaking changes and C++ are allowed. everything here should be considered WIP
// try as much as possible to not include this header in the rest of the codebase

#include "llama.h"

#include <cstdint>
#include <map>
#include <string>
#include <vector>

// Process a batch of tokens while temporarily limiting the physical
// micro-batch size for this call. A value of 0 uses the context default.
// The logical decode remains a single call and retains all output rows.
LLAMA_API int32_t llama_decode_with_ubatch(
        struct llama_context * ctx,
          struct llama_batch   batch,
                   uint32_t     n_ubatch);

// Reserve a new compute graph. It is valid until the next call to llama_graph_reserve.
LLAMA_API struct ggml_cgraph * llama_graph_reserve(
        struct llama_context * ctx,
        uint32_t n_tokens,
        uint32_t n_seqs,
        uint32_t n_outputs);

// One generation-bound view of a host-visible output row. The view is borrowed
// and remains valid only until the next graph submission or output-buffer
// mutation on the same context. A false return and UNAVAILABLE source are the
// explicit fail-closed result for an absent or incoherent row.
enum llama_output_row_source : uint32_t {
    LLAMA_OUTPUT_ROW_SOURCE_UNAVAILABLE    = 0,
    LLAMA_OUTPUT_ROW_SOURCE_RAW            = 1,
    LLAMA_OUTPUT_ROW_SOURCE_SAMPLED_TOKEN  = 2,
    LLAMA_OUTPUT_ROW_SOURCE_SAMPLED_LOGITS = 3,
    LLAMA_OUTPUT_ROW_SOURCE_SAMPLED_PROBS  = 4,
};

struct llama_output_row_view {
    llama_output_row_source source = LLAMA_OUTPUT_ROW_SOURCE_UNAVAILABLE;
    int32_t requested_index = 0;
    int32_t resolved_row = -1;
    uint64_t generation = 0;

    const float * logits = nullptr;
    const float * probs = nullptr;
    const llama_token * candidates = nullptr;

    uint32_t logits_count = 0;
    uint32_t probs_count = 0;
    uint32_t candidates_count = 0;
    llama_token sampled_token = LLAMA_TOKEN_NULL;
    // Index into this view's candidate tuple; -1 for token-only or absent rows.
    int32_t sampled_candidate_index = -1;
};

LLAMA_API bool llama_get_output_row_view(
        struct llama_context * ctx,
        int32_t idx,
        bool prefer_sampled,
        bool token_only,
        struct llama_output_row_view * view);

// Per-context canary observability. Counts include output-result barriers and
// forced public llama_synchronize() calls, but exclude unrelated lifecycle
// barriers such as state save/load. Reading the counters does not synchronize.
struct llama_output_sync_stats {
    uint64_t generation = 0;
    uint64_t completed_barriers = 0;
    uint64_t reused_barriers = 0;
    uint64_t graph_submissions = 0;
    uint64_t output_transfers = 0;
};

LLAMA_API bool llama_get_output_sync_stats(
        const struct llama_context * ctx,
        struct llama_output_sync_stats * stats);

// Get the default ggml_type for a given ftype.
LLAMA_API ggml_type llama_ftype_get_default_type(llama_ftype ftype);

struct quantize_state_impl;

LLAMA_API quantize_state_impl * llama_quant_init(
        const llama_model * model,
        const llama_model_quantize_params * params);

LLAMA_API void llama_quant_free(quantize_state_impl * qs);

// Descriptor for constructing a mock model for quantization testing.
struct llama_quant_model_desc {
    const char * architecture;
    uint32_t n_embd;
    uint32_t n_ff;
    uint32_t n_layer;
    uint32_t n_head;
    uint32_t n_head_kv;
    uint32_t n_expert;
    uint32_t n_embd_head_k;
    uint32_t n_embd_head_v;
};

// Create a mock model from a metadata descriptor (for testing).
// The returned model must be freed with llama_model_free().
LLAMA_API llama_model * llama_quant_model_from_metadata(const llama_quant_model_desc * desc);

// Returns true if this tensor should be quantized (based on name, dims, params).
LLAMA_API bool llama_quant_tensor_allows_quantization(
        const quantize_state_impl * qs,
        const ggml_tensor * tensor);

// Compute quantization type assignments for a list of tensors.
// All tensors should be quantizable (use llama_quant_tensor_allows_quantization to filter).
// result_types: caller-allocated array of n_tensors elements, filled with assigned types.
LLAMA_API void llama_quant_compute_types(
        quantize_state_impl * qs,
        llama_ftype ftype,
        ggml_tensor ** tensors,
        ggml_type * result_types,
        size_t n_tensors);

//
// device memory querying
//

// "memory" as in physical memory for a buffer type, in bytes
struct llama_memory_breakdown_data {
    size_t model   = 0; // memory allocated for the model
    size_t context = 0; // memory allocated for the context
    size_t compute = 0; // memory allocated for temporary compute buffers

    size_t total() const {
        return model + context + compute;
    }
};

struct llama_device_memory_data {
    int64_t total;
    int64_t free;
    llama_memory_breakdown_data mb;
};

// TODO: convert to C-style data structure
using llama_memory_breakdown = std::map<ggml_backend_buffer_type_t, llama_memory_breakdown_data>;

LLAMA_API int32_t llama_model_n_expert (const struct llama_model * model);
LLAMA_API int32_t llama_model_n_devices(const struct llama_model * model);

LLAMA_API ggml_backend_dev_t llama_model_get_device(const struct llama_model * model, int i);

LLAMA_API llama_memory_breakdown llama_get_memory_breakdown(const struct llama_context * ctx);

// Exact model-weight allocation plan captured from the architecture loader's
// real per-buffer-type GGML contexts. Intended for no_alloc diagnostics only.
struct llama_model_allocation_tensor {
    std::string name;
    std::string type;
    uint64_t logical_bytes = 0;
    uint64_t source_bytes = 0;
    uint64_t source_offset = 0;
    int32_t layer = -1;
    bool view = false;
    bool source_known = false;
};

struct llama_model_allocation_group {
    std::string buffer_type;
    std::string device;
    std::string backend;
    uint64_t request_bytes = 0;
    std::vector<llama_model_allocation_tensor> tensors;
};

struct llama_model_allocation_plan {
    bool no_alloc = false;
    bool use_mmap = false;
    bool complete = false;
    bool arithmetic_ok = false;
    uint64_t source_tensor_count = 0;
    uint64_t source_tensor_bytes = 0;
    uint64_t created_tensor_count = 0;
    uint64_t unknown_created_tensors = 0;
    uint64_t unaccounted_source_tensors = 0;
    std::vector<llama_model_allocation_group> groups;
};

LLAMA_API llama_model_allocation_plan llama_model_get_allocation_plan(const struct llama_model * model);

//
// pre-norm embeddings (hidden state before the final output norm)
//

// Set whether the context outputs pre-norm embeddings.
// If masked == true, output only rows where batch.logits != 0.
// If masked == false, output embeddings for all tokens in the batch regardless of batch.logits.
LLAMA_API void llama_set_embeddings_pre_norm(struct llama_context * ctx, bool value, bool masked = false);
LLAMA_API void llama_set_mtp_source(struct llama_context * ctx, struct llama_context * src);
LLAMA_API int32_t llama_model_n_embd_pre_norm(const struct llama_model * model);

// Select which appended NextN block the DECODER_MTP graph runs (offset past
// the trunk: il = n_layer() + offset). Used by the speculative NextN driver to
// chain multiple trained NextN heads. Default 0 (first head).
LLAMA_API void llama_set_nextn_layer_offset(struct llama_context * ctx, int32_t offset);

// Select the speculative NextN step used to validate reusable MTP graph inputs.
// This is process-thread state rather than context state and defaults to zero.
LLAMA_API void llama_set_mtp_speculative_step(int32_t step);

// mirrors:
// LLAMA_API float * llama_get_embeddings(struct llama_context * ctx);
LLAMA_API float * llama_get_embeddings_pre_norm    (struct llama_context * ctx);

// LLAMA_API float * llama_get_embeddings_ith(struct llama_context * ctx, int32_t i);
LLAMA_API float * llama_get_embeddings_pre_norm_ith(struct llama_context * ctx, int32_t i);

// Set whether the context outputs the input embeddings of a specific layer
LLAMA_API void llama_set_embeddings_layer_inp(struct llama_context * ctx, uint32_t lid, bool value);

// mirrors:
// LLAMA_API float * llama_get_embeddings(struct llama_context * ctx);
LLAMA_API float * llama_get_embeddings_layer_inp(struct llama_context * ctx, uint32_t lid);

LLAMA_API llama_context * llama_get_ctx_other(struct llama_context * ctx);

//
// model/context data extraction
//

// returns pointer to the target-model layer indices
LLAMA_API const int32_t * llama_model_target_layer_ids  (const struct llama_model * model);
// returns the number of extracted layers from target model
LLAMA_API uint32_t        llama_model_target_layer_ids_n(const struct llama_model * model);
