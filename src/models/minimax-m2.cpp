#include "models.h"
#include "llama-adapter.h"

#include <algorithm>
#include <charconv>
#include <cinttypes>
#include <cstdlib>
#include <cstring>
#include <string_view>

static int halofpx_minimax_m2_shadow_layer_from_env() {
    const char * raw = std::getenv("HALOFPX_MINIMAX_M2_EXPERT_SHADOW_LAYER");
    if (raw == nullptr) {
        return -1;
    }
    const std::string_view value(raw);
    if (value.empty()) {
        throw std::runtime_error("HALOFPX_MINIMAX_M2_EXPERT_SHADOW_LAYER must be a strict decimal layer index");
    }
    int layer = -1;
    const auto result = std::from_chars(value.data(), value.data() + value.size(), layer);
    if (result.ec != std::errc() || result.ptr != value.data() + value.size() || layer < 0) {
        throw std::runtime_error("HALOFPX_MINIMAX_M2_EXPERT_SHADOW_LAYER must be a strict non-negative decimal layer index");
    }
    return layer;
}

static bool halofpx_minimax_m2_shadow_compute_from_env() {
    const char * raw = std::getenv("HALOFPX_MINIMAX_M2_EXPERT_SHADOW_COMPUTE");
    if (raw == nullptr) {
        return false;
    }
    if (std::strcmp(raw, "1") != 0) {
        throw std::runtime_error("HALOFPX_MINIMAX_M2_EXPERT_SHADOW_COMPUTE must be exactly 1 when present");
    }
    return true;
}

static bool halofpx_minimax_m2_peer_half_load_from_env() {
    const char * raw = std::getenv("HALOFPX_MINIMAX_M2_EXPERT_PEER_HALF_LOAD");
    if (raw == nullptr) {
        return false;
    }
    if (std::strcmp(raw, "1") != 0) {
        throw std::runtime_error("HALOFPX_MINIMAX_M2_EXPERT_PEER_HALF_LOAD must be exactly 1 when present");
    }
    return true;
}

static bool halofpx_device_backend_is(ggml_backend_dev_t dev, const char * backend_name) {
    ggml_backend_reg_t reg = dev == nullptr ? nullptr : ggml_backend_dev_backend_reg(dev);
    const char * name = reg == nullptr ? nullptr : ggml_backend_reg_name(reg);
    return name != nullptr && std::strcmp(name, backend_name) == 0;
}

void llama_model_minimax_m2::load_arch_hparams(llama_model_loader & ml) {
    ml.get_key(LLM_KV_ATTENTION_LAYERNORM_RMS_EPS,  hparams.f_norm_rms_eps);
    ml.get_key(LLM_KV_EXPERT_FEED_FORWARD_LENGTH,   hparams.n_ff_exp);
    ml.get_key(LLM_KV_EXPERT_GATING_FUNC,           hparams.expert_gating_func, false);

    switch (hparams.n_layer) {
        case 62: type = LLM_TYPE_230B_A10B; break;
        default: type = LLM_TYPE_UNKNOWN;
    }
}

void llama_model_minimax_m2::load_arch_tensors(llama_model_loader & ml) {
    LLAMA_LOAD_LOCALS;

    const int shadow_layer = halofpx_minimax_m2_shadow_layer_from_env();
    const bool shadow_compute = halofpx_minimax_m2_shadow_compute_from_env();
    const bool peer_half_load = halofpx_minimax_m2_peer_half_load_from_env();
    if (shadow_compute && shadow_layer < 0) {
        throw std::runtime_error("HALOFPX MiniMax-M2 expert shadow compute requires HALOFPX_MINIMAX_M2_EXPERT_SHADOW_LAYER");
    }
    if (peer_half_load && (!shadow_compute || shadow_layer < 0)) {
        throw std::runtime_error("HaloFPX MiniMax-M2 peer half-load requires shadow placement and compute");
    }
    ggml_backend_dev_t shadow_peer = nullptr;
    if (shadow_layer >= 0) {
        if (n_layer != 62 || n_embd != 3072 || hparams.n_ff_exp != 1536 || n_ff != 1536 ||
                n_expert != 192 || n_expert_used != 8) {
            throw std::runtime_error("HaloFPX MiniMax-M2 expert shadow rejected: model tuple is not exactly 62/3072/1536/192/top8");
        }
        if (shadow_layer >= n_layer) {
            throw std::runtime_error("HaloFPX MiniMax-M2 expert shadow rejected: layer index is out of range");
        }
        if (params.split_mode != LLAMA_SPLIT_MODE_LAYER) {
            throw std::runtime_error("HaloFPX MiniMax-M2 expert shadow rejected: split mode must be layer");
        }
        if (devices.size() != 2) {
            throw std::runtime_error("HaloFPX MiniMax-M2 expert shadow rejected: exactly two devices are required");
        }

        ggml_backend_dev_t local_rocm = nullptr;
        for (const auto & device : devices) {
            if (device.is_meta) {
                throw std::runtime_error("HaloFPX MiniMax-M2 expert shadow rejected: meta/tensor devices are forbidden");
            }
            if (halofpx_device_backend_is(device.dev, "ROCm")) {
                if (local_rocm != nullptr) {
                    throw std::runtime_error("HaloFPX MiniMax-M2 expert shadow rejected: multiple ROCm devices are ambiguous");
                }
                local_rocm = device.dev;
            } else if (halofpx_device_backend_is(device.dev, "RPC")) {
                if (shadow_peer != nullptr) {
                    throw std::runtime_error("HaloFPX MiniMax-M2 expert shadow rejected: multiple RPC devices are ambiguous");
                }
                shadow_peer = device.dev;
            } else {
                throw std::runtime_error("HaloFPX MiniMax-M2 expert shadow rejected: only one ROCm and one RPC device are admitted");
            }
        }
        if (local_rocm == nullptr || shadow_peer == nullptr || dev_layer(shadow_layer) != local_rocm) {
            throw std::runtime_error("HaloFPX MiniMax-M2 expert shadow rejected: designated layer must be owned by the local ROCm device");
        }
        halofpx_expert_shadow_layer = shadow_layer;
        halofpx_expert_shadow_compute = shadow_compute;
        halofpx_expert_peer_half_load = peer_half_load;
    }

    tok_embd = create_tensor(tn(LLM_TENSOR_TOKEN_EMBD, "weight"), {n_embd, n_vocab}, 0);

    // output
    output_norm = create_tensor(tn(LLM_TENSOR_OUTPUT_NORM, "weight"), {n_embd}, 0);
    output      = create_tensor(tn(LLM_TENSOR_OUTPUT,      "weight"), {n_embd, n_vocab}, 0);

    for (int i = 0; i < n_layer; ++i) {
        auto & layer = layers[i];

        create_tensor_qkv(layer, i, n_embd, n_embd_head_k * n_head, n_embd_gqa, n_embd_gqa, 0);
        layer.wo = create_tensor(tn(LLM_TENSOR_ATTN_OUT, "weight", i), { n_embd_head_k * n_head, n_embd }, 0);

        layer.attn_norm = create_tensor(tn(LLM_TENSOR_ATTN_NORM, "weight", i), {n_embd}, 0);
        layer.attn_q_norm = create_tensor(tn(LLM_TENSOR_ATTN_Q_NORM, "weight", i), {n_embd_head_k * n_head}, 0);
        layer.attn_k_norm = create_tensor(tn(LLM_TENSOR_ATTN_K_NORM, "weight", i), {n_embd_k_gqa}, 0);

        layer.ffn_norm = create_tensor(tn(LLM_TENSOR_FFN_NORM, "weight", i), {n_embd}, 0);

        layer.ffn_gate_inp = create_tensor(tn(LLM_TENSOR_FFN_GATE_INP, "weight", i), {n_embd, n_expert}, 0);
        layer.ffn_gate_exps = create_tensor(tn(LLM_TENSOR_FFN_GATE_EXPS, "weight", i), {n_embd, n_ff,   n_expert}, 0);
        layer.ffn_down_exps = create_tensor(tn(LLM_TENSOR_FFN_DOWN_EXPS, "weight", i), {n_ff,   n_embd, n_expert}, 0);
        layer.ffn_up_exps   = create_tensor(tn(LLM_TENSOR_FFN_UP_EXPS,   "weight", i), {n_embd, n_ff,   n_expert}, 0);
        layer.ffn_exp_probs_b = create_tensor(tn(LLM_TENSOR_FFN_EXP_PROBS_B, "bias", i), {n_expert}, 0);

        if (i == shadow_layer) {
            if (layer.ffn_gate_exps->type != GGML_TYPE_Q6_0_ROCMFPX ||
                    layer.ffn_down_exps->type != GGML_TYPE_Q6_0_ROCMFPX ||
                    layer.ffn_up_exps->type != GGML_TYPE_Q6_0_ROCMFPX) {
                throw std::runtime_error(format(
                        "HaloFPX MiniMax-M2 expert shadow rejected: expert tensor types are gate=%s down=%s up=%s, expected Q6_0_ROCMFPX",
                        ggml_type_name(layer.ffn_gate_exps->type),
                        ggml_type_name(layer.ffn_down_exps->type),
                        ggml_type_name(layer.ffn_up_exps->type)));
            }
            if (peer_half_load) {
                layer.ffn_gate_exps_shadow_peer = create_tensor_source_slice_on_device(
                        ml, tn(LLM_TENSOR_FFN_GATE_EXPS, "weight", i), {n_embd, n_ff, 96}, 96, shadow_peer);
                layer.ffn_down_exps_shadow_peer = create_tensor_source_slice_on_device(
                        ml, tn(LLM_TENSOR_FFN_DOWN_EXPS, "weight", i), {n_ff, n_embd, 96}, 96, shadow_peer);
                layer.ffn_up_exps_shadow_peer = create_tensor_source_slice_on_device(
                        ml, tn(LLM_TENSOR_FFN_UP_EXPS, "weight", i), {n_embd, n_ff, 96}, 96, shadow_peer);
            } else {
                layer.ffn_gate_exps_shadow_peer = create_tensor_on_device(
                        ml, tn(LLM_TENSOR_FFN_GATE_EXPS, "weight", i), {n_embd, n_ff, n_expert}, TENSOR_DUPLICATED, shadow_peer);
                layer.ffn_down_exps_shadow_peer = create_tensor_on_device(
                        ml, tn(LLM_TENSOR_FFN_DOWN_EXPS, "weight", i), {n_ff, n_embd, n_expert}, TENSOR_DUPLICATED, shadow_peer);
                layer.ffn_up_exps_shadow_peer = create_tensor_on_device(
                        ml, tn(LLM_TENSOR_FFN_UP_EXPS, "weight", i), {n_embd, n_ff, n_expert}, TENSOR_DUPLICATED, shadow_peer);
            }
            if (layer.ffn_gate_exps_shadow_peer->type != GGML_TYPE_Q6_0_ROCMFPX ||
                    layer.ffn_down_exps_shadow_peer->type != GGML_TYPE_Q6_0_ROCMFPX ||
                    layer.ffn_up_exps_shadow_peer->type != GGML_TYPE_Q6_0_ROCMFPX) {
                throw std::runtime_error("HaloFPX MiniMax-M2 expert shadow rejected: duplicated peer tensors changed type");
            }
            exclude_tensor_from_lookup(layer.ffn_gate_exps_shadow_peer);
            exclude_tensor_from_lookup(layer.ffn_down_exps_shadow_peer);
            exclude_tensor_from_lookup(layer.ffn_up_exps_shadow_peer);
            if (shadow_compute) {
                LLAMA_LOG_INFO("HaloFPX %s: admitted rank-local 96-expert Q6 shadow compute for layer %d on peer %s; peer storage=%s and authoritative output remains the full local MoE result\n",
                        peer_half_load ? "P06h" : "P06g", shadow_layer, ggml_backend_dev_name(shadow_peer),
                        peer_half_load ? "physical-upper-half" : "replicated-full-view");
            } else {
                LLAMA_LOG_INFO("HaloFPX P06d: admitted placement-only Q6 expert shadow for layer %d on peer %s; authoritative graph unchanged\n",
                        shadow_layer, ggml_backend_dev_name(shadow_peer));
            }
        }
    }
}

std::unique_ptr<llm_graph_context> llama_model_minimax_m2::build_arch_graph(const llm_graph_params & params) const {
    return std::make_unique<graph>(*this, params);
}

static void halofpx_minimax_m2_shadow_oracle(
        ggml_tensor * dst, int ith, int nth, void * userdata) {
    GGML_UNUSED(nth);
    if (ith != 0) {
        return;
    }

    auto * telemetry = static_cast<llama_model_minimax_m2::halofpx_shadow_telemetry *>(userdata);
    const ggml_tensor * authoritative = dst->src[0];
    const ggml_tensor * local         = dst->src[1];
    const ggml_tensor * peer          = dst->src[2];
    const ggml_tensor * selected      = dst->src[3];

    if (dst->type != GGML_TYPE_F32 || authoritative->type != GGML_TYPE_F32 ||
            local->type != GGML_TYPE_F32 || peer->type != GGML_TYPE_F32 ||
            selected->type != GGML_TYPE_I32 ||
            !ggml_is_contiguous(dst) || !ggml_is_contiguous(authoritative) ||
            !ggml_is_contiguous(local) || !ggml_is_contiguous(peer) || !ggml_is_contiguous(selected)) {
        GGML_ABORT("HaloFPX P06e shadow oracle received an unsupported tensor layout");
    }

    const int64_t n = ggml_nelements(authoritative);
    if (ggml_nelements(local) != n || ggml_nelements(peer) != n || ggml_nelements(dst) != n) {
        GGML_ABORT("HaloFPX P06e shadow oracle received mismatched branch shapes");
    }

    const float * authoritative_data = static_cast<const float *>(authoritative->data);
    const float * local_data         = static_cast<const float *>(local->data);
    const float * peer_data          = static_cast<const float *>(peer->data);
    float * dst_data                 = static_cast<float *>(dst->data);

    double squared_error = 0.0;
    double squared_reference = 0.0;
    double local_squared = 0.0;
    double peer_squared = 0.0;
    double scaled_max_error = 0.0;
    bool finite = true;
    for (int64_t i = 0; i < n; ++i) {
        const double a = authoritative_data[i];
        const double l = local_data[i];
        const double p = peer_data[i];
        const double s = l + p;
        finite = finite && std::isfinite(a) && std::isfinite(l) && std::isfinite(p) && std::isfinite(s);
        const double error = s - a;
        squared_error += error*error;
        squared_reference += a*a;
        local_squared += l*l;
        peer_squared += p*p;
        scaled_max_error = std::max(scaled_max_error, std::abs(error)/(1.0 + std::abs(a)));
    }

    uint64_t local_selected = 0;
    uint64_t peer_selected = 0;
    const int32_t * selected_data = static_cast<const int32_t *>(selected->data);
    for (int64_t i = 0; i < ggml_nelements(selected); ++i) {
        if (selected_data[i] < 0 || selected_data[i] >= 192) {
            GGML_ABORT("HaloFPX P06e shadow oracle received an out-of-domain expert id");
        }
        if (selected_data[i] < 96) {
            ++local_selected;
        } else {
            ++peer_selected;
        }
    }

    const double nmse = squared_error/std::max(squared_reference, 1.0e-30);
    const double local_l2 = std::sqrt(local_squared);
    const double peer_l2 = std::sqrt(peer_squared);
    const bool passed = finite && nmse <= 1.0e-6 && scaled_max_error <= 1.0e-3;

    uint64_t evaluation = 0;
    {
        std::lock_guard<std::mutex> lock(telemetry->mutex);
        evaluation = ++telemetry->evaluations;
        telemetry->failures += passed ? 0 : 1;
        telemetry->local_selected += local_selected;
        telemetry->peer_selected += peer_selected;
        telemetry->local_l2_sum += local_l2;
        telemetry->peer_l2_sum += peer_l2;
    }

    if (!passed) {
        LLAMA_LOG_ERROR("HaloFPX P06e: shadow oracle FAILED eval=%" PRIu64 " finite=%d nmse=%.9g scaled_max_error=%.9g local_selected=%" PRIu64 " peer_selected=%" PRIu64 " local_l2=%.9g peer_l2=%.9g\n",
                evaluation, finite, nmse, scaled_max_error, local_selected, peer_selected, local_l2, peer_l2);
        GGML_ABORT("HaloFPX P06e shadow oracle rejected divergent expert output");
    }

    std::memcpy(dst_data, authoritative_data, ggml_nbytes(authoritative));
    if (evaluation <= 4 || (evaluation & (evaluation - 1)) == 0) {
        LLAMA_LOG_INFO("HaloFPX P06e: shadow oracle passed eval=%" PRIu64 " nmse=%.9g scaled_max_error=%.9g local_selected=%" PRIu64 " peer_selected=%" PRIu64 " local_l2=%.9g peer_l2=%.9g\n",
                evaluation, nmse, scaled_max_error, local_selected, peer_selected, local_l2, peer_l2);
    }
}

llama_model_minimax_m2::graph::graph(const llama_model & model, const llm_graph_params & params) : llm_graph_context(params) {
    const auto & minimax_model = static_cast<const llama_model_minimax_m2 &>(model);
    if (minimax_model.halofpx_expert_shadow_compute &&
            (params.gtype != LLM_GRAPH_TYPE_DEFAULT || params.cparams.embeddings)) {
        throw std::runtime_error("HaloFPX MiniMax-M2 expert shadow compute admits only the default generation graph");
    }
    const int64_t n_embd_head = hparams.n_embd_head_v();

    GGML_ASSERT(n_embd_head == hparams.n_embd_head_k());
    // GGML_ASSERT(n_embd_head == n_rot); this is wrong in case of minimax, head_dim = 128, n_rot = 64

    ggml_tensor * cur;
    ggml_tensor * inpL;

    inpL = build_inp_embd(model.tok_embd);

    ggml_tensor * inp_pos = build_inp_pos();
    auto inp_attn = build_attn_inp_kv();
    ggml_tensor * inp_out_ids = build_inp_out_ids();

    for (int il = 0; il < n_layer; ++il) {
        ggml_tensor * inpSA = inpL;

        cur = inpL;

        // self_attention
        {
            cur = build_norm(inpL, model.layers[il].attn_norm, NULL, LLM_NORM_RMS, il);
            cb(cur, "attn_norm", il);

            // compute Q and K and RoPE them
            ggml_tensor * Qcur = build_lora_mm(model.layers[il].wq, cur);
            cb(Qcur, "Qcur", il);

            ggml_tensor * Kcur = build_lora_mm(model.layers[il].wk, cur);
            cb(Kcur, "Kcur", il);

            ggml_tensor * Vcur = build_lora_mm(model.layers[il].wv, cur);
            cb(Vcur, "Vcur", il);

            Qcur = build_norm(Qcur, model.layers[il].attn_q_norm, NULL,
                    LLM_NORM_RMS, il);
            cb(Qcur, "Qcur_normed", il);

            Kcur = build_norm(Kcur, model.layers[il].attn_k_norm, NULL,
                    LLM_NORM_RMS, il);
            cb(Kcur, "Kcur_normed", il);

            Qcur = ggml_reshape_3d(ctx0, Qcur, n_embd_head, n_head,    n_tokens);
            Kcur = ggml_reshape_3d(ctx0, Kcur, n_embd_head, n_head_kv, n_tokens);
            Vcur = ggml_reshape_3d(ctx0, Vcur, n_embd_head, n_head_kv, n_tokens);

            Qcur = ggml_rope_ext(
                ctx0, Qcur, inp_pos, nullptr,
                n_rot, rope_type, n_ctx_orig, freq_base, freq_scale,
                ext_factor, attn_factor, beta_fast, beta_slow
                );

            Kcur = ggml_rope_ext(
                ctx0, Kcur, inp_pos, nullptr,
                n_rot, rope_type, n_ctx_orig, freq_base, freq_scale,
                ext_factor, attn_factor, beta_fast, beta_slow
                );

            cb(Qcur, "Qcur", il);
            cb(Kcur, "Kcur", il);
            cb(Vcur, "Vcur", il);

            cur = build_attn(inp_attn,
                    model.layers[il].wo, NULL, model.layers[il].wo_s,
                    Qcur, Kcur, Vcur, nullptr, nullptr, nullptr, 1.0f/sqrtf(float(n_embd_head)), il);
        }

        if (il == n_layer - 1 && inp_out_ids) {
            cur   = ggml_get_rows(ctx0,   cur, inp_out_ids);
            inpSA = ggml_get_rows(ctx0, inpSA, inp_out_ids);
        }

        ggml_tensor * ffn_inp = ggml_add(ctx0, cur, inpSA);
        cb(ffn_inp, "ffn_inp", il);

        // MoE branch
        cur = build_norm(ffn_inp,
                model.layers[il].ffn_norm, NULL,
                LLM_NORM_RMS, il);
        cb(cur, "ffn_norm", il);

        ggml_tensor * moe_inp = cur;
        llm_moe_routing routing;
        cur = build_moe_ffn(cur,
                model.layers[il].ffn_gate_inp,
                model.layers[il].ffn_up_exps,
                model.layers[il].ffn_gate_exps,
                model.layers[il].ffn_down_exps,
                model.layers[il].ffn_exp_probs_b,
                n_expert, n_expert_used,
                LLM_FFN_SILU, true,
                hparams.expert_weights_scale,
                (llama_expert_gating_func_type) hparams.expert_gating_func,
                il,
                nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
                il == minimax_model.halofpx_expert_shadow_layer && minimax_model.halofpx_expert_shadow_compute ? &routing : nullptr);
        cb(cur, "ffn_moe_out", il);

        if (il == minimax_model.halofpx_expert_shadow_layer && minimax_model.halofpx_expert_shadow_compute &&
                n_tokens == 1) {
            if (n_outputs != 1 || ubatch.n_seqs != 1 || ubatch.n_seqs_unq != 1 || loras == nullptr || !loras->empty() ||
                    (cvec != nullptr && cvec->tensor_for(il) != nullptr)) {
                throw std::runtime_error("HaloFPX MiniMax-M2 expert shadow compute requires one output, one token, one sequence, and no adapters");
            }
            if (routing.selected_experts == nullptr || routing.weights == nullptr ||
                    model.layers[il].ffn_gate_exps_shadow_peer == nullptr ||
                    model.layers[il].ffn_down_exps_shadow_peer == nullptr ||
                    model.layers[il].ffn_up_exps_shadow_peer == nullptr) {
                throw std::runtime_error("HaloFPX MiniMax-M2 expert shadow compute is missing admitted placement or routing state");
            }

            ggml_tensor * mask_ids_f32 = ggml_cast(ctx0, routing.selected_experts, GGML_TYPE_F32);
            ggml_tensor * local_mask = ggml_step(ctx0, ggml_scale_bias(ctx0, mask_ids_f32, -1.0f, 95.5f));
            ggml_tensor * peer_mask  = ggml_scale_bias(ctx0, local_mask, -1.0f, 1.0f);
            local_mask = ggml_reshape_3d(ctx0, local_mask, 1, n_expert_used, 1);
            peer_mask  = ggml_reshape_3d(ctx0, peer_mask, 1, n_expert_used, 1);
            ggml_tensor * local_weights = ggml_mul(ctx0, routing.weights, local_mask);
            ggml_tensor * peer_weights  = ggml_mul(ctx0, routing.weights, peer_mask);
            ggml_tensor * local_ids_f32 = ggml_cast(ctx0, routing.selected_experts, GGML_TYPE_F32);
            ggml_tensor * peer_ids_f32  = ggml_cast(ctx0, routing.selected_experts, GGML_TYPE_F32);
            ggml_tensor * local_ids = ggml_cast(
                    ctx0, ggml_clamp(ctx0, local_ids_f32, 0.0f, 95.0f), GGML_TYPE_I32);
            ggml_tensor * peer_ids = ggml_cast(
                    ctx0, ggml_clamp(ctx0, ggml_scale_bias(ctx0, peer_ids_f32, 1.0f, -96.0f), 0.0f, 95.0f),
                    GGML_TYPE_I32);
            cb(local_weights, "halofpx_shadow_local_weights", il);
            cb(peer_weights, "halofpx_shadow_peer_weights", il);
            cb(local_ids, "halofpx_shadow_local_ids", il);
            cb(peer_ids, "halofpx_shadow_peer_ids", il);

            auto routed_half = [&](ggml_tensor * up_exps, ggml_tensor * gate_exps, ggml_tensor * down_exps,
                                   ggml_tensor * ids, ggml_tensor * weights, bool peer_branch) {
                ggml_tensor * branch_in = ggml_reshape_3d(ctx0, moe_inp, n_embd, 1, 1);
                ggml_tensor * up = build_lora_mm_id(up_exps, branch_in, ids);
                ggml_tensor * gate = build_lora_mm_id(gate_exps, branch_in, ids);
                ggml_tensor * activated = ggml_swiglu_split(ctx0, gate, up);
                ggml_tensor * experts = build_lora_mm_id(down_exps, activated, ids);
                experts = ggml_mul(ctx0, experts, weights);
                cb(up, peer_branch ? "halofpx_shadow_peer_up" : "halofpx_shadow_local_up", il);
                cb(gate, peer_branch ? "halofpx_shadow_peer_gate" : "halofpx_shadow_local_gate", il);
                cb(activated, peer_branch ? "halofpx_shadow_peer_swiglu" : "halofpx_shadow_local_swiglu", il);
                cb(experts, peer_branch ? "halofpx_shadow_peer_weighted" : "halofpx_shadow_local_weighted", il);
                ggml_tensor * out = ggml_view_2d(ctx0, experts, n_embd, 1, experts->nb[2], 0);
                for (int64_t i = 1; i < n_expert_used; ++i) {
                    out = ggml_add(ctx0, out, ggml_view_2d(ctx0, experts, n_embd, 1, experts->nb[2], i*experts->nb[1]));
                }
                cb(out, peer_branch ? "halofpx_shadow_peer_out" : "halofpx_shadow_local_out", il);
                return out;
            };

            const auto & layer = model.layers[il];
            auto expert_view = [&](ggml_tensor * experts, bool peer_branch) {
                GGML_ASSERT(experts != nullptr && experts->type == GGML_TYPE_Q6_0_ROCMFPX);
                if (peer_branch && minimax_model.halofpx_expert_peer_half_load) {
                    GGML_ASSERT(experts->ne[2] == 96 && n_expert == 192);
                    return experts;
                }
                GGML_ASSERT(experts->ne[2] == n_expert && n_expert == 192);
                const size_t offset = peer_branch ? 96 * experts->nb[2] : 0;
                return ggml_view_3d(ctx0, experts, experts->ne[0], experts->ne[1], 96,
                        experts->nb[1], experts->nb[2], offset);
            };

            ggml_tensor * peer_out = routed_half(
                    expert_view(layer.ffn_up_exps_shadow_peer, true),
                    expert_view(layer.ffn_gate_exps_shadow_peer, true),
                    expert_view(layer.ffn_down_exps_shadow_peer, true),
                    peer_ids, peer_weights, true);
            ggml_tensor * local_out = routed_half(
                    expert_view(layer.ffn_up_exps, false),
                    expert_view(layer.ffn_gate_exps, false),
                    expert_view(layer.ffn_down_exps, false),
                    local_ids, local_weights, false);

            ggml_tensor * oracle_args[] = { cur, local_out, peer_out, routing.selected_experts };
            cur = ggml_custom_4d(ctx0, GGML_TYPE_F32, cur->ne[0], cur->ne[1], cur->ne[2], cur->ne[3],
                    oracle_args, 4, halofpx_minimax_m2_shadow_oracle, 1,
                    &minimax_model.halofpx_expert_shadow_telemetry);
            ggml_backend_sched_set_tensor_backend(sched, cur, backend_cpu);
            cb(cur, "halofpx_shadow_oracle", il);
        }

        cur = ggml_add(ctx0, cur, ffn_inp);

        cur = build_cvec(cur, il);
        cb(cur, "l_out", il);

        // input for next layer
        inpL = cur;
    }

    cur = inpL;

    cur = build_norm(cur,
            model.output_norm, NULL,
            LLM_NORM_RMS, -1);

    cb(cur, "result_norm", -1);
    res->t_embd = cur;

    // lm_head
    cur = build_lora_mm(model.output, cur);

    cb(cur, "result_output", -1);
    res->t_logits = cur;

    ggml_build_forward_expand(gf, cur);
}
