# Strix Halo quantized-KV FlashAttention optimization

Status: high-value candidate; source-inspected, not locally benchmarked or admitted.

## What it changes

The candidate removes repeated dequantization of runtime K/V cache blocks in two
different backend paths:

- Vulkan coopmat1 prompt processing dequantizes/transposes Q8_0 cache data once
  into shared F16 scratch and reuses it across query-head workgroups. It falls
  back when the required scratch exceeds the device storage-buffer limit.
- HIP/ROCm quantized decode routes eligible GQA shapes to the tile kernel, which
  loads/dequantizes K/V once per tile instead of repeating work for each query
  head that shares the same KV head.

The two lanes have different workloads and risks. Vulkan evidence is chiefly
prompt-processing speed plus scratch-memory cost; HIP evidence is chiefly
generation speed. They must be reviewed, ported, and promoted independently.

## Weight format is not the KV-cache format

Model weights and runtime KV state are separate tensors and separately typed.
The large MiniMax model can remain an ordinary UD-Q6_K_XL GGUF while the server
uses standard `-ctk q8_0 -ctv q8_0` (or another admitted K/V pair). The patch's
standard Q8_0/Q4_0 path can therefore apply without ROCmFPX-formatted weights.

This distinction also limits the claim: the published patch does not implement
ROCmFPX-only K/V types. Extending it to ROCmFP4/FPX/Turbo K/V is a later,
type-specific experiment, even when the smaller MiniMax model uses ROCmFPX
weights.

## Evidence assessment

- **[VERIFIED]** The preserved branch and patches implement the mechanisms above
  at `a18067a85e986f7798f43d98345ed5b86b55cf88`.
- **[SECONDARY]** The author reports strong long-context improvements on a
  gfx1151 Qwen model and provides raw matched receipts in the repository docs.
- **[SECONDARY]** A Reddit commenter reports a Vulkan reproduction on MiniMax
  M2.7 230B-A10B Q3_K_S with Q8_0 K/V, including 23-61% prompt-processing gains
  across 16k-64k and a smaller gain at 128k, flat decode, all tested Vulkan
  backend cases passing, and additional scratch-memory use.
- **[OPEN]** No retained local nimo-1/nimo-2 result yet proves either benefit,
  correctness, memory fit, or non-regression for our large UD-Q6_K_XL model.
- **[OPEN]** Upstream Vulkan PR #25494 is still open; the HIP change is not an
  accepted upstream patch.

## Admission rule

Do not bulk-merge the branch. Both patch snapshots fail `git apply --check`
against the pinned ROCmFPX base, and ROCmFPX has custom attention/TurboQuant
routing that must remain intact. Use two default-off, bisectable manual-port
lanes after P3 provenance:

1. HIP Q8_0/Q4_0 decode lane.
2. Vulkan coopmat1 Q8_0 prompt-processing/fallback lane.

Each lane must pass the inherited backend-op suite, deterministic output/logit
checks, sanitizer/static checks where applicable, feature-off equivalence, and
matched repeated nimo benchmarks. Promotion requires no statistically supported
regression in any admitted baseline cell. Measure peak resident/scratch memory,
prefill and decode separately, and include long contexts where redundancy grows.

Source intake: [candidate receipt](../sources/repositories/candidate-intake/2026-07-18-strix-halo-quant-kv/README.md).
Plan insertion: [L14Q decision](../reviews/plans/2026-07-18__quantized-kv-fa-optimization-plan__v01.md).
