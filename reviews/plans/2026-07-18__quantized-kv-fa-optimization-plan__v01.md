---
type: plan-addendum
status: approved-by-owner-for-planning
created: 2026-07-18
target: HaloFPX ROCmFPX integration fork
risk: high
parent_plan: 2026-07-17__rocmfpx-llama-ai-fork-plan__draft__v03.md
---

# L14Q quantized-KV FlashAttention optimization lane

## Decision

Add this work to the existing plan as **L14Q**, an independent optional backend
lane under L14. Preserve evidence now. Do not interrupt or mix it into an
in-flight HaloKV persistence milestone. Implementation may begin from a clean,
reviewed HaloFPX anchor after the current in-flight milestone closes and G2/G3
baseline/feature-off authority is retained. It must finish before final G9
performance and G10 release acceptance if it is to ship in that release.

It is not a dependency of persistence correctness and must not delay a correct
cache implementation merely because optimization evidence is incomplete. It is
also not deferred to Phase 2: single-node long-context efficiency is a direct
input to the 200-230 GB model baseline from which multi-node speedups are judged.

## Scope and order

1. Promote exact candidate files/commits to P3 provenance; preserve MIT
   attribution and dependency closure.
2. Establish matched unmodified-HaloFPX baselines for standard Q8_0 and Q4_0 K/V
   on both target nodes, separating prompt processing from generation.
3. Manually port the HIP tile dequant-on-load path behind a narrow, bisectable
   change. Do not overwrite ROCmFPX TurboQuant/FA selection.
4. Independently port the Vulkan coopmat1 shared-dequant scratch/fallback path.
5. Qualify each lane separately, then together only if both are admitted.
6. Consider ROCmFPX-only K/V types in a later L14Q extension; they are not part
   of the initial patch or its claims.

The large MiniMax UD-Q6_K_XL weights are an explicit primary workload even
though they are not ROCmFPX-formatted: runtime `-ctk/-ctv` types are independent
of the stored weight quantization. The smaller ROCmFPX MiniMax remains a second
regression/performance workload.

## Mandatory gates

- exact source/commit/file provenance and license review;
- no direct patch application: both captured ranges fail clean application to
  ROCmFPX `61f2f2d7bc4955e9bca821095ef69125837133b5`;
- default behavior and feature-off output/help/build equivalence;
- inherited and new `FLASH_ATTN_EXT` coverage for head dimensions 128/256,
  relevant GQA ratios including MiniMax's observed shape, standard Q8_0/Q4_0,
  boundary context lengths, and fallback paths;
- deterministic greedy output or stricter accepted numerical oracle;
- repeated, interleaved A/B trials with identical model hash, context, prompt,
  batch/ubatch, KV types, FA mode, backend, clocks/power/thermal state, and node;
- prompt-processing and token-generation rates reported separately with raw
  samples, variance, peak memory/scratch, and failure/fallback observations;
- no statistically supported slowdown in any admitted baseline cell; ambiguous
  results do not pass and the prior implementation remains the default;
- long-context MiniMax cells must retain sufficient memory headroom for the
  model, KV cache, graph, scratch, server, and OS;
- independent review and an immediate per-lane revert/disable path.

## Promotion outcomes

- **Admit HIP only**, **admit Vulkan only**, or **admit both** when their own
  gates pass.
- **Defer** a lane if benefit is workload-specific, memory headroom is unsafe,
  upstream design is still too volatile, or evidence is inconclusive.
- **Reject/revert** on correctness failure or any accepted non-regression miss.

Evidence: [candidate intake](../../sources/repositories/candidate-intake/2026-07-18-strix-halo-quant-kv/README.md) and [knowledge synthesis](../../knowledge/strix-halo-quantized-kv-flash-attention.md).
