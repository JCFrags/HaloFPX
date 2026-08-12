---
section_id: "33"
title: "Attention and KV facts"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["ggml-org/llama.cpp", "charlie12345/ROCmFPX"]
  software_versions: ["788e07d", "a5605a7"]
  hardware_revisions: []
related_sections: ["29", "30", "35", "61"]
---

# Attention variants

| Variant | State implication | Sharding implication |
|---|---|---|
| MHA | each query head has K/V head; largest ordinary KV among equal head dimensions | head sharding natural but attention output needs reduction/merge |
| GQA/MQA | several/all Q heads share fewer K/V heads | shard by KV group; never split a shared K/V head without an explicit collective |
| MLA | stores compressed latent/decoupled RoPE state rather than ordinary full per-head K/V | shard by model's latent projection/head contract; ordinary GQA formula is invalid |
| sliding-window + global | local layers need bounded history semantically; global layers remain token-linear | ownership and persistence record per-layer window/global role and retained positions |
| recurrent/SSM hybrid | recurrent layers hold fixed-size recurrent/convolution state while attention layers hold token-indexed KV | serialize two state classes and define rollback separately; see Section 35 |

RoPE parameters (base, scaling type/factor, original context, per-layer rotary dimensions and positions) are part of cached-key semantics. Changing them invalidates persisted K state.

# Ordinary KV sizing

For a uniform ordinary GQA model, raw payload bytes per token per sequence are:

`sum_layers(n_kv_heads[layer] * (head_dim_k[layer] * bytes_K + head_dim_v[layer] * bytes_V))`.

Multiply by cached tokens and sequences/streams. Quant block overhead, padding/alignment, stream separation, scheduler buffers, recurrent state and temporary FA workspaces are additional. For block types use `ggml_row_size`, not nominal bits.

**[VERIFIED]** At pinned upstream, K views use dimensions `[head_dim_k, n_head_kv, n_kv, streams]` with row-size-derived strides. V views are constructed from the layer V tensor and may have architecture/backend-specific layout handling [S33-03]. The cache logs actual K/V buffer bytes; that log is stronger evidence than the formula.

# Runtime cache types

| Tree | Parsed cache types at pinned commit |
|---|---|
| upstream llama.cpp | `f32`, `f16`, `bf16`, `q8_0`, `q4_0`, `q4_1`, `iq4_nl`, `q5_0`, `q5_1` |
| ROCmFPX additions | `q4_0_rocmfp4`, `q4_0_rocmfp4_fast`, `q3_0_rocmfpx`, `q6_0_rocmfpx`, `q8_0_rocmfpx`, `turbo3`, `turbo4` (names from `ggml_type_name`) |

The custom ROCmFP weight-family cache types being parseable does not prove they are quality-safe for all architectures.

# FlashAttention and shifting

- FlashAttention is an exact tiled attention algorithm in the paper; a backend implementation can still have type/shape constraints and fall back [S33-04].
- Upstream exposes `--flash-attn on|off|auto`; actual dispatch must be observed.
- **[VERIFIED]** current cache shift is disabled for architectures whose position scheme cannot use the global shift assumptions (for example per-layer RoPE dimensions or multiple positions per embedding) [S33-03].
- Sliding-window semantics do not imply physical memory reduction unless implementation allocation/eviction proves it.

# TurboQuant boundary

TurboQuant is an online vector quantization method described in an ICLR 2026 paper [S33-05]. **[VERIFIED]** Turbo3/4 are absent from pinned upstream's cache-type list and present in the ROCmFPX fork. The fork documents asymmetric `q8_0` K + `turbo4` V and opt-in boundary-layer K protection, but its performance/quality claims are not HaloFPX measurements [S33-06].

