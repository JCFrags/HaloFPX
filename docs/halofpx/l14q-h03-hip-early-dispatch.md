# L14Q-H03 HIP quantized-KV early dispatch

Status: **qualified as a default-off experiment; performance promotion remains open**

H03 is a target-native refinement of the already admitted H01 standard
quantized-KV HIP decode path. H01 computed exact eligibility before Turbo
routing, then constructed unused pre-dequant pool guards and called the generic
selector again before reaching the typed tile kernel. H03 dispatches the
already-selected typed tile kernel immediately after the existing Turbo batched
and fused-VEC decisions.

The existing compile-time gate remains
`GGML_HIP_QUANT_KV_FATTN_TILE=OFF` by default. Ineligible standard types,
prompt batches, wrong dimensions or GQA, ROCmFPX, ROCmFP4, TurboQuant, Vulkan,
and feature-off builds keep their prior routes. H03 adds no public option,
server surface, RPC change, persistent write, dependency, or donor expression.

## Exact authority and builds

The source parent is HaloFPX commit
`174a59e1dc5ecd52a7ed47ad401c01c33c22eca5`, tree
`1abc035b05c3e3073586b001066497c94efe73bf`. The unmodified parent source digest
on both nodes is
`6daa39e63456f0483d286562ae6016683913b4b4d00b5c872cfaa4cbe00d69a5`;
the H03 source digest is
`d41c9d4d88229d8b548b6de9b6d8a1d07b86d02b588f43290b5c7021c72de8f2`.

Fresh OFF, H01, and H03 builds were produced on both Linux Strix Halo nodes
with the P07 matched tuple: Release, gfx1151, HIP/Vulkan/RPC, forced MMQ, no
VMM, WebUI off, and AMD ROCm Clang at
`/opt/rocm/lib/llvm/bin/clang++`. Every variant's `CMakeCache.txt`, RPC binary,
and server binary is byte-identical across nodes.

| Variant | Gate | RPC SHA-256 | Server SHA-256 |
| --- | --- | --- | --- |
| Feature off | OFF | `314007aecaedbc42ce2a13d595a776519838dde6a4231093cc24ade15ae39cb6` | `520cb2922a9cefccd83a1461bcbc437e5fb715f5f92dfe6ad83ddbf266bd31ff` |
| H01 | ON | `1afab1bf3bacc8909744156ddac580fbe30fd356b1cdf9cd18aee44779715109` | `1dac0c023ba8f8508c610465f6b3ad363d1f8ff0874b24c3622eda304e5e5f19` |
| H03 | ON | `2e762490be65a60484f3f1db01cd7c9182f8933c6f5248020a867ef0698ae3ac` | `33bdd369b5914700ecc03a3a8fc3ad97f2c2c1af6de6ac55a2a4a68036e68c36` |

## Exact-model screen

The primary artifact remains repository
`rcmorano/saricles-MiniMax-M2.7-REAP-172B-A10B-ROCMFPX`, revision
`dba517197f2854f3d362529e13abddcdcad6c10b`, file
`saricles-MiniMax-M2.7-REAP-172B-A10B-Q6_0_ROCMFPX_AGENT.gguf`, size
`159873097824` bytes, SHA-256
`96506ada918e60ca9a9cfde8a5437790e4453401a6a3e236e3f55e7bac3aaea6`.

The runtime and request match P07. Execution order was OFF, H01, H03, H03,
H01, OFF. Each block used one excluded warmup and three retained requests. All
18 retained requests returned HTTP 200 with 1129 prompt tokens, 128 generated
tokens, and decoded-content SHA-256
`3c9bcc5624f4a1d6558c51be89e2443ffba4b2206d10b632c89b7ef64fdd026f`.

| Variant | Prompt tok/s, mean +/- SD | Generation tok/s, mean +/- SD | End-to-end ms, mean +/- SD |
| --- | ---: | ---: | ---: |
| Feature off | 203.7854 +/- 0.1560 | 16.65588 +/- 0.03143 | 13228.914 +/- 15.612 |
| H01 | 203.8338 +/- 0.1340 | 16.64739 +/- 0.02669 | 13231.565 +/- 13.625 |
| H03 | 203.7337 +/- 0.1914 | 16.66448 +/- 0.01548 | 13226.420 +/- 6.063 |

H03 generation is favorable by `+0.10267%` versus H01 and `+0.05164%` versus
feature off. End-to-end time is favorable by `-0.03888%` versus H01 and
`-0.01885%` versus feature off. Prompt processing is adverse by `-0.04912%`
versus H01 and `-0.02536%` versus feature off, although H03 is decode-only and
all approximate 95% intervals cross zero.

This bounded screen supports retaining H03 as an experiment: generation has a
favorable point estimate against H01 and is not adverse against feature off,
while prompt behavior is statistically indistinguishable. It does not prove a
speedup or final non-inferiority. H03 remains default-off; final G9/G10 and the
greater-than-30-tok/s stretch objective remain open.

## Rollback, provenance, and next seam

The known-good nimo-2 RPC worker was restored before the nimo-1 server. Both
services are enabled and active with zero restarts, original binary hashes, and
nimo-1 HTTP 200 health. Raw evidence is hash-manifested and bundled on both
nodes. All five immutable reference repositories remain clean; no Git remote,
donor code, GPL llama-ai code, CachyLLama code, model mutation, WebUI, notice,
or SBOM change entered H03.

The next high-leverage performance seam is not another L14Q permutation. P06e
identified a nonzero Q6 expert view failure; source analysis localized it to
packed logical offsets being applied directly to expanded ROCmFPX device
storage. That backend defect should be corrected and qualified before opening
rank-local expert execution. Canonical preparation Wiki section 33 still
describes the older `37ff5e4f` / L14Q-T01 state; synchronizing that section with
the later HaloFPX milestones remains release-documentation debt and does not
authorize default-on promotion.
