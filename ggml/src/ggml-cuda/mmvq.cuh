#include "common.cuh"

#define MMVQ_MAX_BATCH_SIZE 8 // Max. batch size for which to use MMVQ kernels.

bool ggml_cuda_should_use_mmvq(enum ggml_type type, int cc, int64_t ne11);

// Returns the maximum batch size for which MMVQ should be used for MUL_MAT_ID,
// based on the quantization type and GPU architecture (compute capability).
int get_mmvq_mmid_max_batch(ggml_type type, int cc);

void ggml_cuda_mul_mat_vec_q(ggml_backend_cuda_context & ctx,
    const ggml_tensor * src0, const ggml_tensor * src1, const ggml_tensor * ids, ggml_tensor * dst, const ggml_cuda_mm_fusion_args_host * fusion = nullptr);

void ggml_cuda_op_mul_mat_vec_q(
    ggml_backend_cuda_context & ctx,
    const ggml_tensor * src0, const ggml_tensor * src1, ggml_tensor * dst, const char * src0_dd_i, const float * src1_ddf_i,
    const char * src1_ddq_i, float * dst_dd_i, const int64_t row_low, const int64_t row_high, const int64_t src1_ncols,
    const int64_t src1_padded_row_size, cudaStream_t stream);

#if defined(HALOFPX_MINIMAX_M2_Q6_PRIVATE_HIP_CANARY)
bool ggml_cuda_halofpx_minimax_m2_q6_owned(
    ggml_backend_cuda_context & ctx, ggml_backend_buffer_type_t expected_buft,
    const ggml_tensor * weights, const ggml_tensor * global_ids, const ggml_tensor * activations,
    ggml_tensor * compact_ids, ggml_tensor * compact_activations, ggml_tensor * compact_output,
    ggml_tensor * scattered_output, ggml_tensor * trace, int expert_base);
#endif
