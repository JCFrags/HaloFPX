# L14Q-H01 HIP standard quantized-KV FlashAttention decode port plan

Status: **AUTHORIZED by independently approved runtime P3; implementation pending.**

This is the smallest target-native patch plan for the first HIP runtime slice.
It does not contain a kernel port. Preparation commit
`4f0a2749c2b3c23dc3d45ea25a380ed2a274dfc2` independently admits the bounded `L14Q-H01` clean-reimplementation
unit described here. No captured patch was applied and no donor source was
copied or mechanically translated.

## Exact anchors

- HaloFPX parent: `7e505d202147fb97955b35a15094e3c23029d4bb`, tree
  `6b268262270a201a2a914070b6286c8f10c35d7c`.
- Locked ROCmFPX base: `61f2f2d7bc4955e9bca821095ef69125837133b5`,
  tree `0a35143f33a7b99a81c824fa8ffd8f743f7ae0dd`.
- Preserved source: `Nathanw1014/llama.cpp`, branch
  `strix-halo-fa-fixes`, head
  `a18067a85e986f7798f43d98345ed5b86b55cf88`, tree
  `130e9cac828f8d8ef877d87ea9c192e24b07c9af`.
- Runtime-requirement commit:
  `2a24abc639763332c0ff32cbc03a78f669ae03a0`, parent
  `6b03608e63f48c9371bf5f00423da413ac0288de`, tree
  `c6419edc0771043f8c15d359f72901def1b60fb0`, authored by Nathan
  Wilson `<nath.flagman151@passmail.com>` on `2026-07-17T08:30:04Z`.
- Preserved bundle SHA-256:
  `79c61718bd60ecce3e5ea3919fb18d8c709b3d26032c5fe9d4f24055ade3bc3f`.
- Preserved combined HIP/test patch SHA-256:
  `66ddb0e33301ab7231c736be318483e90cfba813701fbeb5269f547a65c4f42d`.
- Source license: MIT; preserved license blob
  `e7dca554bcb802f98408383a864404e3aa4eacca`, SHA-256
  `bcd8ec749126d45cb06737d0690295d73df4b6e7e194205bcf91190368f27285`.

The runtime commit changes six donor paths and reports 201 insertions and 45
deletions. That donor delta is evidence for the behavior, not a patch source.
The direct-cherry-pick roster remains empty.

| Runtime path | Donor parent blob | Donor result blob | HaloFPX parent blob |
|---|---|---|---|
| `ggml/src/ggml-cuda/fattn-tile.cu` | `c8281497d14895f70aa6cbd2c1698c31ff89d345` | `339e38dbc968e019a087b9fd0cd9ff388c875711` | `c8281497d14895f70aa6cbd2c1698c31ff89d345` |
| `ggml/src/ggml-cuda/fattn-tile.cuh` | `3e07a9f7e04faa763d6850cd17faff3cbbc8fd33` | `78ff3d01644862eb856aaf9829516ef8e0ed2eb0` | `3ddeeeae53c09bf3b94fa6ce24ececa5f5cea1e0` |
| `ggml/src/ggml-cuda/fattn.cu` | `00ffacf2992104e94af25e6b4091b5c55fc94dff` | `fcceb18a21012093912556d204d7f72612b97c00` | `49768ca7e9fd24673834d0bcc479c10aa547807f` |
| `ggml/src/ggml-cuda/template-instances/fattn-tile-instance-dkq64-dv64.cu` | `5caffac0467d880be9dcf5e7808f319f661f9808` | `da5e2a46ec39509837cd88f9edaec6b35fc95840` | `5caffac0467d880be9dcf5e7808f319f661f9808` |
| `ggml/src/ggml-cuda/template-instances/fattn-tile-instance-dkq128-dv128.cu` | `1da18105508acc35cf2013640ecf3b319944a139` | `b229c4d6e9ac45ca0000245fead303b523eda875` | `1da18105508acc35cf2013640ecf3b319944a139` |
| `ggml/src/ggml-cuda/template-instances/fattn-tile-instance-dkq256-dv256.cu` | `bc65c723eca9df5323feb38c4a5c792ce31dafbc` | `c25ddd8064ee9c3334e39f6133bd0a913d2b17c2` | `bc65c723eca9df5323feb38c4a5c792ce31dafbc` |

The differing HaloFPX blobs in `fattn-tile.cuh` and `fattn.cu` are the concrete
reason this is a manual target-native design, not a patch transcription. The
D64 donor unit is recorded for provenance but intentionally excluded from H01.

## Approved P3 authority

Preparation record
`reviews/local-work/2026-07-19__l14q-hip-tile-quant-kv-runtime-p3-candidate__review__v01.md`
at commit `4f0a2749c2b3c23dc3d45ea25a380ed2a274dfc2` covers the runtime commit, all six source paths and
parent/result blobs, authorship, MIT attribution, dependency closure,
notice/SBOM/distribution consequences, target-owned treatment, tests, rollback,
and independent reviewer approval. It authorizes only a clean, target-native
implementation of the following behavioral requirement:

> For eligible HIP decode with symmetric standard Q8_0 or Q4_0 K/V, stage a
> bounded K/V tile in the compute representation once per cooperative tile and
> reuse it across the admitted GQA heads, without materializing a full-cache F16
> copy.

The record forbids patch application, donor line/comment/name/
loop/table translation, ROCmFPX-only or Turbo K/V formats, Vulkan changes, and
any direct source import. Post-change similarity review against the exact donor
runtime blobs remains a promotion gate.

## Narrow v1 contract

The first implementation is compile-time only and defaults off:

`GGML_HIP_QUANT_KV_FATTN_TILE=OFF`

When off, selector behavior, generated HIP code, help text, API behavior, and
runtime output must remain equivalent to the parent. There is no CLI or service
switch in H01.

When on, the new path is eligible only when every condition holds:

- backend is HIP and the existing FlashAttention path is already admitted;
- K and V are symmetric `GGML_TYPE_Q8_0` or symmetric `GGML_TYPE_Q4_0`;
- head dimension is 128 or 256 with equal K/V head dimensions;
- `Q->ne[1] == 1` (decode only);
- GQA ratio is exactly 8 for v1;
- the existing target-owned `gqa_opt_applies` contract is true, including mask,
  zero ALiBi bias, padded K/V length, and compatible strides; and
- the existing tile configuration for the shape is nonzero.

Every other type, dimension, ratio, prompt batch, unsupported layout, or failed
precondition follows the existing selector and fallback unchanged. Mixed
Q8_0/Q4_0 pairs are excluded. ROCmFP4, ROCmFPX, and Turbo types are excluded.

## Exact target patch map

1. `ggml/CMakeLists.txt`
   - Add `GGML_HIP_QUANT_KV_FATTN_TILE` beside the other HIP options with
     default `OFF`.
2. `ggml/src/ggml-hip/CMakeLists.txt`
   - Define `GGML_HIP_QUANT_KV_FATTN_TILE` only for the HIP backend when the
     option is enabled. Do not alter the source list when it is off.
3. `ggml/src/ggml-cuda/fattn-tile.cuh`
   - Under `GGML_USE_HIP && GGML_HIP_QUANT_KV_FATTN_TILE`, add a target-owned,
     bounded tile loader that uses existing standard Q8_0/Q4_0 target type
     traits to populate the current half/float shared tile.
   - Carry K/V type parameters only through the tile load/iteration/kernel
     chain required by the admitted shapes. Preserve the existing F16 template
     defaults and byte-for-byte off-path preprocessing.
   - Derive `launch_fattn`'s `need_f16_K` and `need_f16_V` booleans from the
     admitted type pair so the enabled standard-quantized path does not allocate
     a full-cache F16 temporary. All non-admitted types retain `true`.
   - Keep bounds checks, synchronization, shared-memory size, accumulation,
     softmax, mask, and output code target-owned and unchanged except where a
     typed input is strictly required.
4. `ggml/src/ggml-cuda/fattn-tile.cu`
   - Under the same macro, dispatch only the four admitted combinations:
     D128/Q8_0, D128/Q4_0, D256/Q8_0, and D256/Q4_0. The existing untyped/F16
     dispatch remains the off and fallback path.
5. `ggml/src/ggml-cuda/template-instances/fattn-tile-instance-dkq128-dv128.cu`
   and `fattn-tile-instance-dkq256-dv256.cu`
   - Add only the explicit Q8_0/Q8_0 and Q4_0/Q4_0 instantiations needed by the
     admitted HIP build, guarded by the feature macro. Do not add D64 or any
     ROCmFPX/Turbo instantiation in H01.
6. `ggml/src/ggml-cuda/fattn.cu`
   - Add one named eligibility helper under both HIP and feature macros.
   - Insert its TILE selection after the existing ROCmFP-family rejection and
     after the Turbo fused-VEC decision, but before the current generic HIP
     quantized-decode VEC fallback. This ordering preserves TurboQuant/ROCmFPX
     routing and the allocation-safe VEC fallback for every ineligible case.
7. `tests/test-backend-ops.cpp`
   - Reuse the accepted L14Q-T01 correctness inventory. Add only focused
     selector/fallback cases if kernel-trace qualification exposes a coverage
     gap; do not duplicate the 20-case matrix.

No server, CLI, model, RPC, Vulkan, persistent-store, NOTICE, or WebUI path is
in scope.

## Focused qualification after P3 and implementation

Use nimo-1 as the representative target node. Keep the current deployment
recoverable and build two clean trees from the same parent/toolchain:

- control: `GGML_HIP_QUANT_KV_FATTN_TILE=OFF`;
- candidate: `GGML_HIP_QUANT_KV_FATTN_TILE=ON`.

Required evidence is deliberately focused:

1. OFF build succeeds and its HIP library/help/feature-off behavior is
   equivalent to the parent control.
2. The existing L14Q-T01 20 positive Q8_0/Q4_0 cases pass, the h160 negative
   remains unsupported, and the inherited `FLASH_ATTN_EXT` HIP smoke has no new
   failure.
3. Candidate traces identify TILE for the four admitted decode cells and the
   existing VEC/fallback for prompt batches, mixed types, non-GQA, wrong ratio,
   ROCmFPX, and Turbo controls. No permanent diagnostic write or public CLI is
   introduced.
4. Deterministic greedy output (or the existing stricter backend oracle) agrees
   with control for D128 and D256 Q8_0/Q4_0.
5. Interleaved matched A/B trials report prompt processing and generation
   separately, with raw samples, variance, memory/scratch peaks, and kernel
   traces. The exact 160 GB pinned MiniMax artifact is the primary D128 workload
   when its metadata confirms the admitted shape; use a separately pinned D256
   fixture/model for that cell.
6. Any correctness difference, fallback loss, allocation during graph capture,
   memory-headroom failure, or statistically supported slowdown rejects the
   lane. Ambiguous performance defers admission and leaves OFF as the default.
7. Independent review covers correctness, selector ordering, TurboQuant/ROCmFPX
   preservation, provenance/similarity, rollback, and evidence reconciliation.

Rollback is a single coherent revert or rebuilding with
`GGML_HIP_QUANT_KV_FATTN_TILE=OFF`. H01 cannot be promoted or enabled by default
until its independent P3, implementation review, correctness, and matched
non-regression gates all pass.
