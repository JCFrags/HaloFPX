#include "common.cuh"
#include "mmq.cuh"
#include "quantize.cuh"
#include "mmid.cuh"
#include "rocmfpx-ffn-q8-reuse.h"
#include "rocmfpx-moe-q8-reuse.h"
#include "rocmfpx-qkv-q8-reuse.h"

static void ggml_cuda_mul_mat_q_switch_type(ggml_backend_cuda_context & ctx, const mmq_args & args, cudaStream_t stream) {
    switch (args.type_x) {
        case GGML_TYPE_Q1_0:
            mul_mat_q_case<GGML_TYPE_Q1_0>(ctx, args, stream);
            break;
        case GGML_TYPE_Q4_0:
            mul_mat_q_case<GGML_TYPE_Q4_0>(ctx, args, stream);
            break;
        case GGML_TYPE_Q4_1:
            mul_mat_q_case<GGML_TYPE_Q4_1>(ctx, args, stream);
            break;
        case GGML_TYPE_Q5_0:
            mul_mat_q_case<GGML_TYPE_Q5_0>(ctx, args, stream);
            break;
        case GGML_TYPE_Q5_1:
            mul_mat_q_case<GGML_TYPE_Q5_1>(ctx, args, stream);
            break;
        case GGML_TYPE_Q8_0:
            mul_mat_q_case<GGML_TYPE_Q8_0>(ctx, args, stream);
            break;
        case GGML_TYPE_MXFP4:
            mul_mat_q_case<GGML_TYPE_MXFP4>(ctx, args, stream);
            break;
        case GGML_TYPE_Q4_0_ROCMFP4:
            mul_mat_q_case<GGML_TYPE_Q4_0_ROCMFP4>(ctx, args, stream);
            break;
        case GGML_TYPE_Q4_0_ROCMFP4_FAST:
            mul_mat_q_case<GGML_TYPE_Q4_0_ROCMFP4_FAST>(ctx, args, stream);
            break;
        case GGML_TYPE_Q3_0_ROCMFPX:
            mul_mat_q_case<GGML_TYPE_Q3_0_ROCMFPX>(ctx, args, stream);
            break;
        case GGML_TYPE_Q2_0_ROCMFPX:
            mul_mat_q_case<GGML_TYPE_Q2_0_ROCMFPX>(ctx, args, stream);
            break;
        case GGML_TYPE_Q6_0_ROCMFPX:
            mul_mat_q_case<GGML_TYPE_Q6_0_ROCMFPX>(ctx, args, stream);
            break;
        case GGML_TYPE_Q8_0_ROCMFPX:
            mul_mat_q_case<GGML_TYPE_Q8_0_ROCMFPX>(ctx, args, stream);
            break;
        case GGML_TYPE_NVFP4:
            mul_mat_q_case<GGML_TYPE_NVFP4>(ctx, args, stream);
            break;
        case GGML_TYPE_Q2_K:
            mul_mat_q_case<GGML_TYPE_Q2_K>(ctx, args, stream);
            break;
        case GGML_TYPE_Q3_K:
            mul_mat_q_case<GGML_TYPE_Q3_K>(ctx, args, stream);
            break;
        case GGML_TYPE_Q4_K:
            mul_mat_q_case<GGML_TYPE_Q4_K>(ctx, args, stream);
            break;
        case GGML_TYPE_Q5_K:
            mul_mat_q_case<GGML_TYPE_Q5_K>(ctx, args, stream);
            break;
        case GGML_TYPE_Q6_K:
            mul_mat_q_case<GGML_TYPE_Q6_K>(ctx, args, stream);
            break;
        case GGML_TYPE_IQ2_XXS:
            mul_mat_q_case<GGML_TYPE_IQ2_XXS>(ctx, args, stream);
            break;
        case GGML_TYPE_IQ2_XS:
            mul_mat_q_case<GGML_TYPE_IQ2_XS>(ctx, args, stream);
            break;
        case GGML_TYPE_IQ2_S:
            mul_mat_q_case<GGML_TYPE_IQ2_S>(ctx, args, stream);
            break;
        case GGML_TYPE_IQ3_XXS:
            mul_mat_q_case<GGML_TYPE_IQ3_XXS>(ctx, args, stream);
            break;
        case GGML_TYPE_IQ3_S:
            mul_mat_q_case<GGML_TYPE_IQ3_S>(ctx, args, stream);
            break;
        case GGML_TYPE_IQ1_S:
            mul_mat_q_case<GGML_TYPE_IQ1_S>(ctx, args, stream);
            break;
        case GGML_TYPE_IQ4_XS:
            mul_mat_q_case<GGML_TYPE_IQ4_XS>(ctx, args, stream);
            break;
        case GGML_TYPE_IQ4_NL:
            mul_mat_q_case<GGML_TYPE_IQ4_NL>(ctx, args, stream);
            break;
        default:
            GGML_ABORT("fatal error");
            break;
    }
}

void ggml_cuda_mul_mat_q(
        ggml_backend_cuda_context & ctx, const ggml_tensor * src0, const ggml_tensor * src1, const ggml_tensor * ids, ggml_tensor * dst) {
    GGML_ASSERT(        src1->type == GGML_TYPE_F32);
    GGML_ASSERT(        dst->type  == GGML_TYPE_F32);
    GGML_ASSERT(!ids || ids->type  == GGML_TYPE_I32); // Optional, used for batched GGML_MUL_MAT_ID.

    GGML_TENSOR_BINARY_OP_LOCALS;

    cudaStream_t stream = ctx.stream();
    const int cc = ggml_cuda_info().devices[ggml_cuda_get_device()].cc;

    const size_t ts_src0 = ggml_type_size(src0->type);
    const size_t ts_src1 = ggml_type_size(src1->type);
    const size_t ts_dst  = ggml_type_size(dst->type);

    GGML_ASSERT(        nb00       == ts_src0);
    GGML_ASSERT(        nb10       == ts_src1);
    GGML_ASSERT(        nb0        == ts_dst);
    GGML_ASSERT(!ids || ids->nb[0] == ggml_type_size(ids->type));

    const char  * src0_d = (const char  *) src0->data;
    const float * src1_d = (const float *) src1->data;
    float       *  dst_d = (float       *)  dst->data;

    // If src0 is a temporary compute buffer, clear any potential padding.
    if (ggml_backend_buffer_get_usage(src0->buffer) == GGML_BACKEND_BUFFER_USAGE_COMPUTE) {
        const size_t size_data  = ggml_nbytes(src0);
        const size_t size_alloc = ggml_backend_buffer_get_alloc_size(src0->buffer, src0);
        if (size_alloc > size_data) {
            GGML_ASSERT(ggml_is_contiguously_allocated(src0));
            GGML_ASSERT(!src0->view_src);
            CUDA_CHECK(cudaMemsetAsync((char *) src0->data + size_data, 0, size_alloc - size_data, stream));
        }
    }

    const int64_t ne10_padded = GGML_PAD(ne10, MATRIX_ROW_PADDING);

    const int64_t s01 = src0->nb[1] / ts_src0;
    const int64_t s1  =  dst->nb[1] / ts_dst;
    const int64_t s02 = src0->nb[2] / ts_src0;
    const int64_t s2  =  dst->nb[2] / ts_dst;
    const int64_t s03 = src0->nb[3] / ts_src0;
    const int64_t s3  =  dst->nb[3] / ts_dst;

    const bool use_stream_k = (GGML_CUDA_CC_IS_NVIDIA(cc) && ggml_cuda_highest_compiled_arch(cc) >= GGML_CUDA_CC_VOLTA)
                            || GGML_CUDA_CC_IS_CDNA(cc);

    // TODO: tighter pool buffer size vs q8 path
    const bool use_native_fp4 = blackwell_mma_available(cc) && (src0->type == GGML_TYPE_MXFP4 || src0->type == GGML_TYPE_NVFP4);

    if (!ids) {
        const size_t nbytes_src1_q8_1 = ne13*ne12 * ne11*ne10_padded * sizeof(block_q8_1)/QK8_1 +
            get_mmq_x_max_host(cc)*sizeof(block_q8_1_mmq);
        ggml_cuda_pool_alloc<char> src1_q8_1(ctx.pool(), nbytes_src1_q8_1);

        {
            const int64_t s11 = src1->nb[1] / ts_src1;
            const int64_t s12 = src1->nb[2] / ts_src1;
            const int64_t s13 = src1->nb[3] / ts_src1;
            if (use_native_fp4) {
                static_assert(sizeof(block_fp4_mmq) == 4 * sizeof(block_q8_1));
                quantize_mmq_fp4_cuda(src1_d, nullptr, src1_q8_1.get(), src0->type, ne10, s11, s12, s13, ne10_padded,
                                        ne11, ne12, ne13, stream);

            } else {
#if defined(GGML_HIP_ROCMFPX_QKV_Q8_REUSE)
                if (halofpx_rocmfpx_qkv_q8_reuse_type(src0->type)) {
                    ctx.halofpx_qkv_q8_conversions_submitted.fetch_add(1, std::memory_order_relaxed);
                }
#endif
                quantize_mmq_q8_1_cuda(src1_d, nullptr, src1_q8_1.get(), src0->type, ne10, s11, s12, s13, ne10_padded,
                                       ne11, ne12, ne13, stream);
            }
            CUDA_CHECK(cudaGetLastError());
        }

        // Stride depends on quantization format
        const int64_t s12 = use_native_fp4 ?
                                ne11 * ne10_padded * sizeof(block_fp4_mmq) / (QK_K * sizeof(int)) :  // block_fp4_mmq holds 256 values
                                ne11 * ne10_padded * sizeof(block_q8_1) / (QK8_1 * sizeof(int));
        const int64_t s13 = ne12*s12;

        const mmq_args args = {
            src0_d, src0->type, (const int *) src1_q8_1.ptr, nullptr, nullptr, dst_d,
            ne00, ne01, ne1, s01, ne11, s1,
            ne02, ne12, s02, s12, s2,
            ne03, ne13, s03, s13, s3,
            use_stream_k, ne1};
#if defined(GGML_HIP_ROCMFPX_QKV_Q8_REUSE)
        if (halofpx_rocmfpx_qkv_q8_reuse_type(src0->type)) {
            ctx.halofpx_qkv_mmq_submissions.fetch_add(1, std::memory_order_relaxed);
        }
#endif
        ggml_cuda_mul_mat_q_switch_type(ctx, args, stream);
        return;
    }

    GGML_ASSERT(ne13 == 1);
    GGML_ASSERT(nb12 % nb11 == 0);
    GGML_ASSERT(nb2  % nb1  == 0);

    const int64_t n_expert_used = ids->ne[0];
    const int64_t ne_get_rows = ne12 * n_expert_used;
    GGML_ASSERT(ne1 == n_expert_used);

    ggml_cuda_pool_alloc<int32_t> ids_src1(ctx.pool(), ne_get_rows);
    ggml_cuda_pool_alloc<int32_t> ids_dst(ctx.pool(), ne_get_rows);
    ggml_cuda_pool_alloc<int32_t> expert_bounds(ctx.pool(), ne02 + 1);

#if defined(GGML_HIP_ROCMFPX_MOE_Q8_REUSE)
    // HALOFPX_MOE_Q8_REUSE_LEGACY_ID_METRICS_BEGIN
#endif
    {
        GGML_ASSERT(ids->nb[0] == ggml_element_size(ids));
        const int si1  = ids->nb[1] / ggml_element_size(ids);
        const int sis1 = nb12 / nb11;

#if defined(GGML_HIP_ROCMFPX_MOE_Q8_REUSE)
        if (halofpx_rocmfpx_moe_q8_reuse_type(src0->type)) {
            ctx.halofpx_moe_ids_helper_submissions.fetch_add(1, std::memory_order_relaxed);
        }
#endif
        ggml_cuda_launch_mm_ids_helper((const int32_t *) ids->data, ids_src1.get(), ids_dst.get(), expert_bounds.get(),
            ne02, ne12, n_expert_used, ne11, si1, sis1, stream);
        CUDA_CHECK(cudaGetLastError());
    }

    const size_t nbytes_src1_q8_1 = ne12*n_expert_used*ne10_padded * sizeof(block_q8_1)/QK8_1 +
        get_mmq_x_max_host(cc)*sizeof(block_q8_1_mmq);
    ggml_cuda_pool_alloc<char> src1_q8_1(ctx.pool(), nbytes_src1_q8_1);

    const int64_t ne11_flat = ne12*n_expert_used;
    const int64_t ne12_flat = 1;
    const int64_t ne13_flat = 1;

    {
        const int64_t s11 = src1->nb[1] / ts_src1;
        const int64_t s12 = src1->nb[2] / ts_src1;
        const int64_t s13 = src1->nb[3] / ts_src1;

        if (use_native_fp4) {
            quantize_mmq_fp4_cuda(src1_d, ids_src1.get(), src1_q8_1.get(), src0->type, ne10, s11, s12, s13,
                                    ne10_padded, ne11_flat, ne12_flat, ne13_flat, stream);
        } else {
#if defined(GGML_HIP_ROCMFPX_MOE_Q8_REUSE)
            if (halofpx_rocmfpx_moe_q8_reuse_type(src0->type)) {
                ctx.halofpx_moe_q8_conversions_submitted.fetch_add(1, std::memory_order_relaxed);
            }
#endif
            quantize_mmq_q8_1_cuda(src1_d, ids_src1.get(), src1_q8_1.get(), src0->type, ne10, s11, s12, s13,
                                   ne10_padded, ne11_flat, ne12_flat, ne13_flat, stream);
        }
        CUDA_CHECK(cudaGetLastError());
    }

    static_assert(QK_K == 8 * QK_MXFP4, "QK_K needs to be 8 * QK_MXFP4");
    const int64_t s12 = use_native_fp4 ? ne11 * ne10_padded * sizeof(block_fp4_mmq) / (QK_K * sizeof(int)) :
                                         ne11 * ne10_padded * sizeof(block_q8_1) / (QK8_1 * sizeof(int));
    const int64_t s13 = ne12*s12;

    // Note that ne02 is used instead of ne12 because the number of y channels determines the z dimension of the CUDA grid.
    const mmq_args args = {
        src0_d, src0->type, (const int *) src1_q8_1.get(), ids_dst.get(), expert_bounds.get(), dst_d,
        ne00, ne01, ne_get_rows, s01, ne_get_rows, s1,
        ne02, ne02, s02, s12, s2,
        ne03, ne13, s03, s13, s3,
        use_stream_k, ne12};

#if defined(GGML_HIP_ROCMFPX_MOE_Q8_REUSE)
    if (halofpx_rocmfpx_moe_q8_reuse_type(src0->type)) {
        ctx.halofpx_moe_mmq_submissions.fetch_add(1, std::memory_order_relaxed);
    }
    // HALOFPX_MOE_Q8_REUSE_LEGACY_ID_METRICS_END
#endif
    ggml_cuda_mul_mat_q_switch_type(ctx, args, stream);
}

#if defined(GGML_HIP_ROCMFPX_FFN_Q8_REUSE)
// HALOFPX_FFN_Q8_REUSE_PAIR_BEGIN
void ggml_cuda_mul_mat_q_rocmfpx_pair(
        ggml_backend_cuda_context & ctx,
        const ggml_tensor * src0_a,
        const ggml_tensor * src0_b,
        const ggml_tensor * src1,
        ggml_tensor * dst_a,
        ggml_tensor * dst_b) {
    GGML_ASSERT(src0_a && src0_b && src1 && dst_a && dst_b);
    GGML_ASSERT(halofpx_rocmfpx_ffn_q8_reuse_type(src0_a->type));
    GGML_ASSERT(src0_a->type == src0_b->type);
    GGML_ASSERT(ggml_are_same_shape(src0_a, src0_b));
    GGML_ASSERT(ggml_are_same_stride(src0_a, src0_b));
    GGML_ASSERT(src1->type == GGML_TYPE_F32);
    GGML_ASSERT(dst_a->type == GGML_TYPE_F32 && dst_b->type == GGML_TYPE_F32);
    GGML_ASSERT(ggml_are_same_shape(dst_a, dst_b));
    GGML_ASSERT(ggml_are_same_stride(dst_a, dst_b));

    cudaStream_t stream = ctx.stream();
    const int cc = ggml_cuda_info().devices[ggml_cuda_get_device()].cc;

    const size_t ts_src1 = ggml_type_size(src1->type);
    GGML_ASSERT(src1->nb[0] == ts_src1);

    const int64_t ne10 = src1->ne[0];
    const int64_t ne11 = src1->ne[1];
    const int64_t ne12 = src1->ne[2];
    const int64_t ne13 = src1->ne[3];
    const int64_t ne10_padded = GGML_PAD(ne10, MATRIX_ROW_PADDING);

    const size_t nbytes_src1_q8_1 = ne13*ne12 * ne11*ne10_padded * sizeof(block_q8_1)/QK8_1 +
        get_mmq_x_max_host(cc)*sizeof(block_q8_1_mmq);
    ggml_cuda_pool_alloc<char> src1_q8_1(ctx.pool(), nbytes_src1_q8_1);

    const int64_t src1_s11 = src1->nb[1] / ts_src1;
    const int64_t src1_s12 = src1->nb[2] / ts_src1;
    const int64_t src1_s13 = src1->nb[3] / ts_src1;

    // This is the sole activation conversion in the paired path. Both MMQs
    // consume the allocation below on the same execution stream before release.
    quantize_mmq_q8_1_cuda(
        (const float *) src1->data, nullptr, src1_q8_1.get(), src0_a->type,
        ne10, src1_s11, src1_s12, src1_s13, ne10_padded, ne11, ne12, ne13, stream);
    CUDA_CHECK(cudaGetLastError());

    const int64_t q8_s12 = ne11 * ne10_padded * sizeof(block_q8_1) / (QK8_1 * sizeof(int));
    const int64_t q8_s13 = ne12 * q8_s12;
    const bool use_stream_k = (GGML_CUDA_CC_IS_NVIDIA(cc) &&
                               ggml_cuda_highest_compiled_arch(cc) >= GGML_CUDA_CC_VOLTA) ||
                              GGML_CUDA_CC_IS_CDNA(cc);

    const auto clear_compute_padding = [stream](const ggml_tensor * weight) {
        if (ggml_backend_buffer_get_usage(weight->buffer) != GGML_BACKEND_BUFFER_USAGE_COMPUTE) {
            return;
        }
        const size_t size_data  = ggml_nbytes(weight);
        const size_t size_alloc = ggml_backend_buffer_get_alloc_size(weight->buffer, weight);
        if (size_alloc > size_data) {
            GGML_ASSERT(ggml_is_contiguously_allocated(weight));
            GGML_ASSERT(!weight->view_src);
            CUDA_CHECK(cudaMemsetAsync((char *) weight->data + size_data, 0, size_alloc - size_data, stream));
        }
    };

    const auto launch = [&](const ggml_tensor * weight, ggml_tensor * dst) {
        const size_t ts_weight = ggml_type_size(weight->type);
        const size_t ts_dst    = ggml_type_size(dst->type);
        GGML_ASSERT(weight->nb[0] == ts_weight);
        GGML_ASSERT(dst->nb[0] == ts_dst);

        const int64_t weight_s01 = weight->nb[1] / ts_weight;
        const int64_t weight_s02 = weight->nb[2] / ts_weight;
        const int64_t weight_s03 = weight->nb[3] / ts_weight;
        const int64_t dst_s1     = dst->nb[1] / ts_dst;
        const int64_t dst_s2     = dst->nb[2] / ts_dst;
        const int64_t dst_s3     = dst->nb[3] / ts_dst;

        clear_compute_padding(weight);

        const mmq_args args = {
            (const char *) weight->data, weight->type, (const int *) src1_q8_1.get(), nullptr, nullptr,
            (float *) dst->data,
            weight->ne[0], weight->ne[1], dst->ne[1], weight_s01, ne11, dst_s1,
            weight->ne[2], ne12, weight_s02, q8_s12, dst_s2,
            weight->ne[3], ne13, weight_s03, q8_s13, dst_s3,
            use_stream_k, dst->ne[1]};
        ggml_cuda_mul_mat_q_switch_type(ctx, args, stream);
    };

    launch(src0_a, dst_a);
    launch(src0_b, dst_b);
}
// HALOFPX_FFN_Q8_REUSE_PAIR_END
#endif

#if defined(GGML_HIP_ROCMFPX_MOE_Q8_REUSE)
// HALOFPX_MOE_Q8_REUSE_PAIR_BEGIN
void ggml_cuda_mul_mat_id_q_rocmfpx_pair(
        ggml_backend_cuda_context & ctx,
        const ggml_tensor * src0_a,
        const ggml_tensor * src0_b,
        const ggml_tensor * src1,
        const ggml_tensor * ids,
        ggml_tensor * dst_a,
        ggml_tensor * dst_b) {
    GGML_ASSERT(src0_a && src0_b && src1 && ids && dst_a && dst_b);
    GGML_ASSERT(ctx.curr_stream_no == 0);
    GGML_ASSERT(halofpx_rocmfpx_moe_q8_reuse_type(src0_a->type));
    GGML_ASSERT(src0_a->type == src0_b->type);
    GGML_ASSERT(ggml_are_same_shape(src0_a, src0_b));
    GGML_ASSERT(ggml_are_same_stride(src0_a, src0_b));
    GGML_ASSERT(src1->type == GGML_TYPE_F32);
    GGML_ASSERT(ids->type == GGML_TYPE_I32);
    GGML_ASSERT(dst_a->type == GGML_TYPE_F32 && dst_b->type == GGML_TYPE_F32);
    GGML_ASSERT(ggml_are_same_shape(dst_a, dst_b));
    GGML_ASSERT(ggml_are_same_stride(dst_a, dst_b));

    const size_t ts_src1 = ggml_type_size(src1->type);
    const size_t ts_ids  = ggml_type_size(ids->type);
    GGML_ASSERT(src1->nb[0] == ts_src1);
    GGML_ASSERT(ids->nb[0] == ts_ids);
    GGML_ASSERT(src1->ne[3] == 1);
    GGML_ASSERT(src1->nb[2] % src1->nb[1] == 0);
    GGML_ASSERT(dst_a->nb[2] % dst_a->nb[1] == 0);
    GGML_ASSERT(dst_b->nb[2] % dst_b->nb[1] == 0);

    const int64_t n_experts    = src0_a->ne[2];
    const int64_t n_expert_used = ids->ne[0];
    const int64_t n_tokens      = src1->ne[2];
    const int64_t ne_get_rows   = n_tokens * n_expert_used;
    GGML_ASSERT(n_experts == src0_b->ne[2]);
    GGML_ASSERT(src1->ne[1] == n_expert_used);
    GGML_ASSERT(ids->ne[1] == n_tokens);
    GGML_ASSERT(dst_a->ne[1] == n_expert_used && dst_b->ne[1] == n_expert_used);
    GGML_ASSERT(dst_a->ne[2] == n_tokens && dst_b->ne[2] == n_tokens);

    cudaStream_t stream = ctx.stream();
    const int cc = ggml_cuda_info().devices[ctx.device].cc;

    ggml_cuda_pool_alloc<int32_t> ids_src1(ctx.pool(), ne_get_rows);
    ggml_cuda_pool_alloc<int32_t> ids_dst(ctx.pool(), ne_get_rows);
    ggml_cuda_pool_alloc<int32_t> expert_bounds(ctx.pool(), n_experts + 1);

    const int ids_s1 = ids->nb[1] / ts_ids;
    const int src1_sis1 = src1->nb[2] / src1->nb[1];
    ctx.halofpx_moe_ids_helper_submissions.fetch_add(1, std::memory_order_relaxed);
    ggml_cuda_launch_mm_ids_helper(
        (const int32_t *) ids->data,
        ids_src1.get(),
        ids_dst.get(),
        expert_bounds.get(),
        static_cast<int>(n_experts),
        static_cast<int>(n_tokens),
        static_cast<int>(n_expert_used),
        static_cast<int>(src1->ne[1]),
        ids_s1,
        src1_sis1,
        stream);
    CUDA_CHECK(cudaGetLastError());

    const int64_t ne10 = src1->ne[0];
    const int64_t ne10_padded = GGML_PAD(ne10, MATRIX_ROW_PADDING);
    const size_t nbytes_src1_q8_1 =
        n_tokens*n_expert_used*ne10_padded * sizeof(block_q8_1)/QK8_1 +
        get_mmq_x_max_host(cc)*sizeof(block_q8_1_mmq);
    ggml_cuda_pool_alloc<char> src1_q8_1(ctx.pool(), nbytes_src1_q8_1);

    const int64_t src1_s11 = src1->nb[1] / ts_src1;
    const int64_t src1_s12 = src1->nb[2] / ts_src1;
    const int64_t src1_s13 = src1->nb[3] / ts_src1;
    ctx.halofpx_moe_q8_conversions_submitted.fetch_add(1, std::memory_order_relaxed);
    quantize_mmq_q8_1_cuda(
        (const float *) src1->data,
        ids_src1.get(),
        src1_q8_1.get(),
        src0_a->type,
        ne10,
        src1_s11,
        src1_s12,
        src1_s13,
        ne10_padded,
        ne_get_rows,
        1,
        1,
        stream);
    CUDA_CHECK(cudaGetLastError());

    const int64_t q8_s12 = src1->ne[1] * ne10_padded * sizeof(block_q8_1) /
                           (QK8_1 * sizeof(int));
    const int64_t q8_s13 = src1->ne[2] * q8_s12;
    const bool use_stream_k = (GGML_CUDA_CC_IS_NVIDIA(cc) &&
                               ggml_cuda_highest_compiled_arch(cc) >= GGML_CUDA_CC_VOLTA) ||
                              GGML_CUDA_CC_IS_CDNA(cc);

    const auto clear_compute_padding = [stream](const ggml_tensor * weight) {
        if (ggml_backend_buffer_get_usage(weight->buffer) != GGML_BACKEND_BUFFER_USAGE_COMPUTE) {
            return;
        }
        const size_t size_data  = ggml_nbytes(weight);
        const size_t size_alloc = ggml_backend_buffer_get_alloc_size(weight->buffer, weight);
        if (size_alloc > size_data) {
            GGML_ASSERT(ggml_is_contiguously_allocated(weight));
            GGML_ASSERT(!weight->view_src);
            CUDA_CHECK(cudaMemsetAsync((char *) weight->data + size_data, 0, size_alloc - size_data, stream));
        }
    };

    const auto launch = [&](const ggml_tensor * weight, ggml_tensor * dst) {
        const size_t ts_weight = ggml_type_size(weight->type);
        const size_t ts_dst    = ggml_type_size(dst->type);
        GGML_ASSERT(weight->nb[0] == ts_weight);
        GGML_ASSERT(dst->nb[0] == ts_dst);

        clear_compute_padding(weight);
        const mmq_args args = {
            (const char *) weight->data,
            weight->type,
            (const int *) src1_q8_1.get(),
            ids_dst.get(),
            expert_bounds.get(),
            (float *) dst->data,
            weight->ne[0],
            weight->ne[1],
            ne_get_rows,
            static_cast<int64_t>(weight->nb[1] / ts_weight),
            ne_get_rows,
            static_cast<int64_t>(dst->nb[1] / ts_dst),
            weight->ne[2],
            weight->ne[2],
            static_cast<int64_t>(weight->nb[2] / ts_weight),
            q8_s12,
            static_cast<int64_t>(dst->nb[2] / ts_dst),
            weight->ne[3],
            src1->ne[3],
            static_cast<int64_t>(weight->nb[3] / ts_weight),
            q8_s13,
            static_cast<int64_t>(dst->nb[3] / ts_dst),
            use_stream_k,
            n_tokens,
        };
        ctx.halofpx_moe_mmq_submissions.fetch_add(1, std::memory_order_relaxed);
        ggml_cuda_mul_mat_q_switch_type(ctx, args, stream);
    };

    launch(src0_a, dst_a);
    launch(src0_b, dst_b);
    ctx.halofpx_moe_pair_dispatches.fetch_add(1, std::memory_order_relaxed);
}
// HALOFPX_MOE_Q8_REUSE_PAIR_END
#endif

#if defined(GGML_HIP_ROCMFPX_QKV_Q8_REUSE)
// HALOFPX_QKV_Q8_REUSE_TRIPLE_BEGIN
void ggml_cuda_mul_mat_q_rocmfpx_triple(
        ggml_backend_cuda_context & ctx,
        const ggml_tensor * src0_q,
        const ggml_tensor * src0_k,
        const ggml_tensor * src0_v,
        const ggml_tensor * src1,
        ggml_tensor * dst_q,
        ggml_tensor * dst_k,
        ggml_tensor * dst_v) {
    GGML_ASSERT(src0_q && src0_k && src0_v && src1 && dst_q && dst_k && dst_v);
    GGML_ASSERT(ctx.curr_stream_no == 0);
    GGML_ASSERT(halofpx_rocmfpx_qkv_q8_reuse_type(src0_q->type));
    GGML_ASSERT(src0_q->type == src0_k->type && src0_q->type == src0_v->type);
    GGML_ASSERT(src1->type == GGML_TYPE_F32);
    GGML_ASSERT(dst_q->type == GGML_TYPE_F32 && dst_k->type == GGML_TYPE_F32 && dst_v->type == GGML_TYPE_F32);

    const std::array<const ggml_tensor *, 3> weights = { src0_q, src0_k, src0_v };
    const std::array<ggml_tensor *, 3> outputs = { dst_q, dst_k, dst_v };
    for (size_t i = 0; i < weights.size(); ++i) {
        GGML_ASSERT(weights[i]->ne[0] == src1->ne[0]);
        GGML_ASSERT(outputs[i]->ne[0] == weights[i]->ne[1]);
        GGML_ASSERT(outputs[i]->ne[1] == src1->ne[1]);
        GGML_ASSERT(outputs[i]->ne[2] == src1->ne[2] / weights[i]->ne[2]);
        GGML_ASSERT(outputs[i]->ne[3] == src1->ne[3] / weights[i]->ne[3]);
    }

    cudaStream_t stream = ctx.stream();
    const int cc = ggml_cuda_info().devices[ctx.device].cc;
    const size_t ts_src1 = ggml_type_size(src1->type);
    GGML_ASSERT(src1->nb[0] == ts_src1);

    const int64_t ne10 = src1->ne[0];
    const int64_t ne11 = src1->ne[1];
    const int64_t ne12 = src1->ne[2];
    const int64_t ne13 = src1->ne[3];
    const int64_t ne10_padded = GGML_PAD(ne10, MATRIX_ROW_PADDING);

    const size_t nbytes_src1_q8_1 = ne13*ne12 * ne11*ne10_padded * sizeof(block_q8_1)/QK8_1 +
        get_mmq_x_max_host(cc)*sizeof(block_q8_1_mmq);
    ggml_cuda_pool_alloc<char> src1_q8_1(ctx.pool(), nbytes_src1_q8_1);

    const int64_t src1_s11 = src1->nb[1] / ts_src1;
    const int64_t src1_s12 = src1->nb[2] / ts_src1;
    const int64_t src1_s13 = src1->nb[3] / ts_src1;

    // This is the only activation conversion in the triple path. All three
    // consumers are submitted on stream zero before the pool allocation dies.
    ctx.halofpx_qkv_q8_conversions_submitted.fetch_add(1, std::memory_order_relaxed);
    quantize_mmq_q8_1_cuda(
        (const float *) src1->data, nullptr, src1_q8_1.get(), src0_q->type,
        ne10, src1_s11, src1_s12, src1_s13, ne10_padded, ne11, ne12, ne13, stream);
    CUDA_CHECK(cudaGetLastError());

    const int64_t q8_s12 = ne11 * ne10_padded * sizeof(block_q8_1) / (QK8_1 * sizeof(int));
    const int64_t q8_s13 = ne12 * q8_s12;
    const bool use_stream_k = (GGML_CUDA_CC_IS_NVIDIA(cc) &&
                               ggml_cuda_highest_compiled_arch(cc) >= GGML_CUDA_CC_VOLTA) ||
                              GGML_CUDA_CC_IS_CDNA(cc);

    const auto clear_compute_padding = [stream](const ggml_tensor * weight) {
        if (ggml_backend_buffer_get_usage(weight->buffer) != GGML_BACKEND_BUFFER_USAGE_COMPUTE) {
            return;
        }
        const size_t size_data  = ggml_nbytes(weight);
        const size_t size_alloc = ggml_backend_buffer_get_alloc_size(weight->buffer, weight);
        if (size_alloc > size_data) {
            GGML_ASSERT(ggml_is_contiguously_allocated(weight));
            GGML_ASSERT(!weight->view_src);
            CUDA_CHECK(cudaMemsetAsync((char *) weight->data + size_data, 0, size_alloc - size_data, stream));
        }
    };

    const auto launch = [&](const ggml_tensor * weight, ggml_tensor * dst) {
        const size_t ts_weight = ggml_type_size(weight->type);
        const size_t ts_dst    = ggml_type_size(dst->type);
        GGML_ASSERT(weight->nb[0] == ts_weight);
        GGML_ASSERT(dst->nb[0] == ts_dst);

        const int64_t weight_s01 = weight->nb[1] / ts_weight;
        const int64_t weight_s02 = weight->nb[2] / ts_weight;
        const int64_t weight_s03 = weight->nb[3] / ts_weight;
        const int64_t dst_s1     = dst->nb[1] / ts_dst;
        const int64_t dst_s2     = dst->nb[2] / ts_dst;
        const int64_t dst_s3     = dst->nb[3] / ts_dst;

        clear_compute_padding(weight);
        const mmq_args args = {
            (const char *) weight->data, weight->type, (const int *) src1_q8_1.get(), nullptr, nullptr,
            (float *) dst->data,
            weight->ne[0], weight->ne[1], dst->ne[1], weight_s01, ne11, dst_s1,
            weight->ne[2], ne12, weight_s02, q8_s12, dst_s2,
            weight->ne[3], ne13, weight_s03, q8_s13, dst_s3,
            use_stream_k, dst->ne[1]};
        ctx.halofpx_qkv_mmq_submissions.fetch_add(1, std::memory_order_relaxed);
        ggml_cuda_mul_mat_q_switch_type(ctx, args, stream);
    };

    launch(src0_q, dst_q);
    launch(src0_k, dst_k);
    launch(src0_v, dst_v);
    ctx.halofpx_qkv_triple_dispatches.fetch_add(1, std::memory_order_relaxed);
}
// HALOFPX_QKV_Q8_REUSE_TRIPLE_END
#endif

void ggml_cuda_op_mul_mat_q(
    ggml_backend_cuda_context & ctx,
    const ggml_tensor * src0, const ggml_tensor * src1, ggml_tensor * dst, const char * src0_dd_i, const float * src1_ddf_i,
    const char * src1_ddq_i, float * dst_dd_i, const int64_t row_low, const int64_t row_high, const int64_t src1_ncols,
    const int64_t src1_padded_row_size, cudaStream_t stream) {

    const int64_t ne00 = src0->ne[0];

    const int64_t ne10 = src1->ne[0];
    const int64_t ne11 = src1->ne[1];
    GGML_ASSERT(ne10 % QK8_1 == 0);

    const int64_t ne0 = dst->ne[0];

    const int64_t row_diff = row_high - row_low;
    const int64_t stride01 = ne00 / ggml_blck_size(src0->type);

    const int id = ggml_cuda_get_device();
    const int cc = ggml_cuda_info().devices[id].cc;

    // the main device has a larger memory buffer to hold the results from all GPUs
    // nrows_dst == nrows of the matrix that the kernel writes into
    const int64_t nrows_dst = id == ctx.device ? ne0 : row_diff;

    // The stream-k decomposition is only faster for recent NVIDIA GPUs.
    // Also its fixup needs to allocate a temporary buffer in the memory pool.
    // There are multiple parallel CUDA streams for src1_ncols != ne11 which would introduce a race condition for this buffer.
    const bool use_stream_k = ((GGML_CUDA_CC_IS_NVIDIA(cc) && ggml_cuda_highest_compiled_arch(cc) >= GGML_CUDA_CC_VOLTA)
                            || GGML_CUDA_CC_IS_CDNA(cc))
                            && src1_ncols == ne11;
    const mmq_args args = {
        src0_dd_i, src0->type, (const int *) src1_ddq_i, nullptr, nullptr, dst_dd_i,
        ne00, row_diff, src1_ncols, stride01, ne11, nrows_dst,
        1, 1, 0, 0, 0,
        1, 1, 0, 0, 0,
        use_stream_k, src1_ncols};

    ggml_cuda_mul_mat_q_switch_type(ctx, args, stream);

    GGML_UNUSED_VARS(src1, dst, src1_ddf_i, src1_padded_row_size);
}

bool ggml_cuda_should_use_mmq(enum ggml_type type, int cc, int64_t ne11, int64_t n_experts) {
#ifdef GGML_CUDA_FORCE_CUBLAS
    return false;
#endif // GGML_CUDA_FORCE_CUBLAS

    bool mmq_supported;

    switch (type) {
        case GGML_TYPE_Q1_0:
        case GGML_TYPE_Q4_0:
        case GGML_TYPE_Q4_1:
        case GGML_TYPE_Q5_0:
        case GGML_TYPE_Q5_1:
        case GGML_TYPE_Q8_0:
        case GGML_TYPE_MXFP4:
        case GGML_TYPE_Q4_0_ROCMFP4:
        case GGML_TYPE_Q4_0_ROCMFP4_FAST:
        case GGML_TYPE_Q3_0_ROCMFPX:
        case GGML_TYPE_Q2_0_ROCMFPX:
        case GGML_TYPE_Q6_0_ROCMFPX:
        case GGML_TYPE_Q8_0_ROCMFPX:
        case GGML_TYPE_NVFP4:
        case GGML_TYPE_Q2_K:
        case GGML_TYPE_Q3_K:
        case GGML_TYPE_Q4_K:
        case GGML_TYPE_Q5_K:
        case GGML_TYPE_Q6_K:
        case GGML_TYPE_IQ2_XXS:
        case GGML_TYPE_IQ2_XS:
        case GGML_TYPE_IQ2_S:
        case GGML_TYPE_IQ3_XXS:
        case GGML_TYPE_IQ3_S:
        case GGML_TYPE_IQ1_S:
        case GGML_TYPE_IQ4_XS:
        case GGML_TYPE_IQ4_NL:
            mmq_supported = true;
            break;
        default:
            mmq_supported = false;
            break;
    }

    if (!mmq_supported) {
        return false;
    }

    if (turing_mma_available(cc)) {
        return true;
    }

    if (ggml_cuda_highest_compiled_arch(cc) < GGML_CUDA_CC_DP4A) {
        return false;
    }

#ifdef GGML_CUDA_FORCE_MMQ
    return true;
#endif //GGML_CUDA_FORCE_MMQ

    if (GGML_CUDA_CC_IS_NVIDIA(cc)) {
        return !fp16_mma_hardware_available(cc) || ne11 < MMQ_DP4A_MAX_BATCH_SIZE;
    }

    if (amd_mfma_available(cc)) {
        // As of ROCM 7.0 rocblas/tensile performs very poorly on CDNA3 and hipblaslt (via ROCBLAS_USE_HIPBLASLT)
        // performs better but is currently suffering from a crash on this architecture.
        // TODO: Revisit when hipblaslt is fixed on CDNA3
        if (GGML_CUDA_CC_IS_CDNA3(cc)) {
            return true;
        }
        if (n_experts > 64 || ne11 <= 128) {
            return true;
        }
        if (type == GGML_TYPE_Q4_0 || type == GGML_TYPE_Q4_1 || type == GGML_TYPE_Q5_0 || type == GGML_TYPE_Q5_1) {
            return true;
        }
        if (ne11 <= 256 && (type == GGML_TYPE_Q4_K || type == GGML_TYPE_Q5_K)) {
            return true;
        }
        return false;
    }

    if (amd_wmma_available(cc)) {
        if (GGML_CUDA_CC_IS_RDNA3(cc)) {
            // High expert counts are almost always better on MMQ due to
            //     the synchronization overhead in the cuBLAS/hipBLAS path:
            // https://github.com/ggml-org/llama.cpp/pull/18202
            if (n_experts >= 64) {
                return true;
            }

            // For some quantization types MMQ can have lower peak TOPS than hipBLAS
            //     so it's only faster for sufficiently small batch sizes:
            switch (type) {
                case GGML_TYPE_Q2_K:
                    return ne11 <= 128;
                case GGML_TYPE_Q6_K:
                    return ne11 <= (GGML_CUDA_CC_IS_RDNA3_0(cc) ? 128 : 256);
                case GGML_TYPE_IQ2_XS:
                case GGML_TYPE_IQ2_S:
                    return GGML_CUDA_CC_IS_RDNA3_5(cc) || ne11 <= 128;
                default:
                    return true;
            }
        }

        // For RDNA4 MMQ is consistently faster than dequantization + hipBLAS:
        // https://github.com/ggml-org/llama.cpp/pull/18537#issuecomment-3706422301
        return true;
    }

    // gfx900 (Vega 10) lacks native dp4a, so MMQ is emulated and loses to
    // dequant + hipBLAS for dense matrices; keep MMQ only for MoE, where the
    // hipBLAS path is much slower.
    if (cc == GGML_CUDA_CC_VEGA) {
        return n_experts > 0;
    }

    return (!GGML_CUDA_CC_IS_CDNA(cc)) || ne11 < MMQ_DP4A_MAX_BATCH_SIZE;
}
