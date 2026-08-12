---
section_id: "33"
title: "Attention Variants, KV Layouts, FlashAttention, and TurboQuant"
status: "needs-machine-validation"
last_verified: "2026-07-19"
applies_to:
  repositories: ["ggml-org/llama.cpp", "charlie12345/ROCmFPX"]
  software_versions: ["llama.cpp 788e07d", "ROCmFPX a5605a7", "HaloFPX 37ff5e4f"]
  hardware_revisions: ["gfx1151 L14Q-T01 test coverage qualified; runtime optimization pending"]
related_sections: ["29", "30", "35", "42", "57", "61"]
---

# Attention and persistent state

KV layout is architecture-, layer-, cache-type-, backend-, context-, slot-, and topology-dependent. **[VERIFIED]** Current upstream accepts F32/F16/BF16/Q8_0/Q4_0/Q4_1/IQ4_NL/Q5_0/Q5_1 K/V types; the ROCmFPX fork additionally exposes ROCmFP4/FPX and Turbo3/4 types [S33-01, S33-02].

**[VERIFIED]** A preserved Strix Halo candidate branch removes redundant standard-Q8_0 KV dequantization in two distinct paths: Vulkan coopmat1 prompt processing and HIP tile-kernel decode. Its exact patch ranges do not apply cleanly to pinned ROCmFPX `61f2f2d...`, so it is source evidence for an attributed manual port, not a cherry-pick [S33-09, S33-11]. **[SECONDARY]** The linked report includes an independent MiniMax M2.7 230B Q3_K_S Vulkan reproduction with faster long-context prompt processing and extra scratch-memory use; this has not been reproduced on nimo-1/nimo-2 [S33-10].

**[VERIFIED]** Stored model-weight quantization and runtime K/V-cache quantization are independent configuration dimensions. Therefore the large MiniMax UD-Q6_K_XL workload need not use ROCmFPX-formatted weights to exercise standard Q8_0/Q4_0 K/V optimizations. The candidate does not yet cover ROCmFPX-only K/V types [S33-01, S33-02, S33-09].

**[VERIFIED]** HaloFPX L14Q-T01 `37ff5e4f6ab48ed7d8b0ea2fda05a6304091ae2b` is a target-native, test-only expansion of the existing `FLASH_ATTN_EXT` inventory. It adds 20 positive standard Q8_0/Q8_0 and Q4_0/Q4_0 cases at head dimensions 128 and 256 across KV lengths 255/256/257, GQA ratio 8, and single-/multi-batch shapes, plus one explicit ROCm unsupported case at head dimension 160. On each of nimo-1 and nimo-2, CPU, ROCm, and Vulkan completed 200/200 focused positive executions with zero failures; both nodes also produced identical zero-failure inherited full-inventory counts [S33-12]. The independent P3 and similarity reviews found no copied donor code, comment, table, loop structure, dependency, kernel, or build change. Excluded CRLF launcher artifacts remain retained as excluded evidence rather than being represented as product failures.

L14Q-T01 makes no performance, latency, memory, runtime-optimization, kernel-dispatch, or zero-regression claim. It admits neither the donor HIP tile path nor the Vulkan coopmat1 path, does not change TurboQuant/ROCmFPX routing, and does not close the broader backend/shape question in OQ33-02 [S33-12].

**[RECOMMENDATION]** Persist typed rank-local state with exact layout descriptors. Never treat a byte count or `.bin` blob as sufficient compatibility identity.

## Research split

- Completed now: formulas, upstream/fork type lists, current KV tensor view, shift constraints, FlashAttention/TurboQuant source boundaries, exact preservation/source inspection of the Strix Halo shared-dequant candidate, and qualified target-native L14Q-T01 standard-Q8_0/Q4_0 backend-op coverage on both nodes.
- Machine work: actual buffer sizes/layouts, FA dispatch/fallback, cache-type quality/speed, shift/restore, long-context and distributed ownership on both nodes.
- Contingent: default K/V types, asymmetric TurboQuant, boundary protection, per-layer sharding and persistent-cache format.
