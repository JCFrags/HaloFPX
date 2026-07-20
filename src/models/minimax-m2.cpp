#include "models.h"

#include <charconv>
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
            layer.ffn_gate_exps_shadow_peer = create_tensor_on_device(
                    ml, tn(LLM_TENSOR_FFN_GATE_EXPS, "weight", i), {n_embd, n_ff, n_expert}, TENSOR_DUPLICATED, shadow_peer);
            layer.ffn_down_exps_shadow_peer = create_tensor_on_device(
                    ml, tn(LLM_TENSOR_FFN_DOWN_EXPS, "weight", i), {n_ff, n_embd, n_expert}, TENSOR_DUPLICATED, shadow_peer);
            layer.ffn_up_exps_shadow_peer = create_tensor_on_device(
                    ml, tn(LLM_TENSOR_FFN_UP_EXPS, "weight", i), {n_embd, n_ff, n_expert}, TENSOR_DUPLICATED, shadow_peer);
            if (layer.ffn_gate_exps_shadow_peer->type != GGML_TYPE_Q6_0_ROCMFPX ||
                    layer.ffn_down_exps_shadow_peer->type != GGML_TYPE_Q6_0_ROCMFPX ||
                    layer.ffn_up_exps_shadow_peer->type != GGML_TYPE_Q6_0_ROCMFPX) {
                throw std::runtime_error("HaloFPX MiniMax-M2 expert shadow rejected: duplicated peer tensors changed type");
            }
            exclude_tensor_from_lookup(layer.ffn_gate_exps_shadow_peer);
            exclude_tensor_from_lookup(layer.ffn_down_exps_shadow_peer);
            exclude_tensor_from_lookup(layer.ffn_up_exps_shadow_peer);
            LLAMA_LOG_INFO("HaloFPX P06d: admitted placement-only Q6 expert shadow for layer %d on peer %s; authoritative graph unchanged\n",
                    shadow_layer, ggml_backend_dev_name(shadow_peer));
        }
    }
}

std::unique_ptr<llm_graph_context> llama_model_minimax_m2::build_arch_graph(const llm_graph_params & params) const {
    return std::make_unique<graph>(*this, params);
}

llama_model_minimax_m2::graph::graph(const llama_model & model, const llm_graph_params & params) : llm_graph_context(params) {
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
                il);
        cb(cur, "ffn_moe_out", il);

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
