---
title: L14Q HIP tile quantized-KV decode P3 candidate
date: 2026-07-19
status: approved-independent-p3-review
lane: L14Q
capability_id: L14Q-H01
treatment: target-native-clean-reimplementation
runtime_behavior_change: default-off-candidate
donor_code_copied: false
p3_state: approved
reviewer: independent-provenance-review
reviewer_decision: approve
---

# L14Q-H01 target-native quantized-KV tile decode

## Candidate admission boundary

This record proposes one narrowly bounded P3 unit: a HaloFPX-owned clean
reimplementation of HIP tile-kernel dequant-on-load for symmetric standard
Q8_0/Q8_0 and Q4_0/Q4_0 K/V during eligible D128/D256, GQA-ratio-8 decode. It may stage
each K/V tile in SRAM once per block and reuse it across the query heads served
by that block. It must be independently gated and default-off, preserve the
existing vec path, F16 path, ROCmFPX/TurboQuant types and selector authority,
and make unsupported or ineligible shapes take the prior path.

Project authority is the accepted [v03 implementation plan](../plans/2026-07-17__rocmfpx-llama-ai-fork-plan__draft__v03.md), the [L14Q addendum](../plans/2026-07-18__quantized-kv-fa-optimization-plan__v01.md), the preserved [candidate intake](../../sources/repositories/candidate-intake/2026-07-18-strix-halo-quant-kv/README.md), the approved [L14Q-T01 record](2026-07-19__l14q-flash-attn-ext-coverage-p3-candidate__review__v01.md), and the canonical [attention/KV Wiki section](../../wiki/HaloFPX_Wiki/06_Models_Quantization_and_Inference/33_Attention_Variants_KV_Layouts_FlashAttention_and_TurboQuant/README.md).

This record received independent P3 approval on 2026-07-19. Its author inspected
the donor history and is not the independent reviewer. No donor code, comments,
identifiers, tables, loop structure, or patch text may be copied or mechanically
translated. The direct-cherry-pick roster remains empty.

## Exact target anchor and unchanged seams

| Field | Exact value |
|---|---|
| HaloFPX inspected commit / tree | `7e505d202147fb97955b35a15094e3c23029d4bb` / `6b268262270a201a2a914070b6286c8f10c35d7c` |
| ROCmFPX base commit / tree | `61f2f2d7bc4955e9bca821095ef69125837133b5` / `0a35143f33a7b99a81c824fa8ffd8f743f7ae0dd` |
| Runtime target paths | `ggml/src/ggml-cuda/fattn-tile.cu`, `fattn-tile.cuh`, `fattn.cu`, and the 64/128/256 matching `template-instances/fattn-tile-instance-dkq*-dv*.cu` files |
| Target blobs at inspected anchor | `c8281497d14895f70aa6cbd2c1698c31ff89d345`; `3ddeeeae53c09bf3b94fa6ce24ececa5f5cea1e0`; `49768ca7e9fd24673834d0bcc479c10aa547807f`; `5caffac0467d880be9dcf5e7808f319f661f9808`; `1da18105508acc35cf2013640ecf3b319944a139`; `bc65c723eca9df5323feb38c4a5c792ce31dafbc` |
| Qualified target-owned test seam | L14Q-T01 commit `37ff5e4f6ab48ed7d8b0ea2fda05a6304091ae2b`; current test blob `543c26432318b243e3412bb1aff2f8874e67a963` |

The six runtime blobs are unchanged between the exact ROCmFPX base and this
HaloFPX anchor. A later implementation record must refresh its parent/tree and
result blobs; it must not silently advance the source base.

## Preserved source, introducing commit, authorship, and blobs

| Field | Exact value |
|---|---|
| Source | `Nathanw1014/llama.cpp`, `strix-halo-fa-fixes` |
| Locked head / tree | `a18067a85e986f7798f43d98345ed5b86b55cf88` / `130e9cac828f8d8ef877d87ea9c192e24b07c9af` |
| Complete bundle | `bundles/Nathanw1014__llama.cpp--strix-halo-fa-fixes.bundle`, 379353651 bytes, SHA-256 `79C61718BD60ECCE3E5EA3919FB18D8C709B3D26032C5FE9D4F24055ADE3BC3F` |
| Preserved patch | `patches/hip-tile-quant-kv-dequant-on-load-and-tests.patch`, 33907 bytes, SHA-256 `66DDB0E33301AB7231C736BE318483E90CFBA813701FBEB5269F547A65C4F42D` |
| Runtime introducing commit | `2a24abc639763332c0ff32cbc03a78f669ae03a0` |
| Parent / tree | `6b03608e63f48c9371bf5f00423da413ac0288de` / `c6419edc0771043f8c15d359f72901def1b60fb0` |
| Author and date | Nathan Wilson `<nath.flagman151@passmail.com>`, `2026-07-17T08:30:04Z` |
| Subject / signature | `CUDA: dequantize KV on load in the tile FA kernel, use it for quantized decode`; unsigned (`%G? = N`) |
| Trailers | no `Signed-off-by` and no `Assisted-by` trailer |
| Delta | 6 files, 201 insertions, 45 deletions |

Exact donor parent -> result blobs:

| Donor path | Parent blob | Result blob |
|---|---|---|
| `ggml/src/ggml-cuda/fattn-tile.cu` | `c8281497d14895f70aa6cbd2c1698c31ff89d345` | `339e38dbc968e019a087b9fd0cd9ff388c875711` |
| `ggml/src/ggml-cuda/fattn-tile.cuh` | `3e07a9f7e04faa763d6850cd17faff3cbbc8fd33` | `78ff3d01644862eb856aaf9829516ef8e0ed2eb0` |
| `ggml/src/ggml-cuda/fattn.cu` | `00ffacf2992104e94af25e6b4091b5c55fc94dff` | `fcceb18a21012093912556d204d7f72612b97c00` |
| `.../fattn-tile-instance-dkq64-dv64.cu` | `5caffac0467d880be9dcf5e7808f319f661f9808` | `da5e2a46ec39509837cd88f9edaec6b35fc95840` |
| `.../fattn-tile-instance-dkq128-dv128.cu` | `1da18105508acc35cf2013640ecf3b319944a139` | `b229c4d6e9ac45ca0000245fead303b523eda875` |
| `.../fattn-tile-instance-dkq256-dv256.cu` | `bc65c723eca9df5323feb38c4a5c792ce31dafbc` | `c25ddd8064ee9c3334e39f6133bd0a913d2b17c2` |

The preceding donor test commit `6b03608e...` is already represented only by
the independently approved, no-copy L14Q-T01 test requirement; it does not
authorize this runtime unit. Donor benchmark claims remain secondary evidence,
not HaloFPX measurements.

## License, attribution, dependencies, and distribution

| Field | Disposition |
|---|---|
| Source license | MIT |
| Preserved license | `Nathanw1014__llama.cpp--strix-halo-fa-fixes/LICENSE`; blob `e7dca554bcb802f98408383a864404e3aa4eacca`; 1099 bytes; SHA-256 `BCD8EC749126D45CB06737D0690295D73DF4B6E7E194205BCF91190368F27285` |
| Copyright notice | `Copyright (c) 2023-2026 The ggml authors` |
| Target license | HaloFPX MIT core unchanged |
| Donor code/assets distributed | none under this clean-reimplementation treatment |
| NOTICE/SBOM effect | no new binary/package/shader/dependency; retain repository, author, commit, and this provenance record. Reclassify and re-review if similarity shows donor expression. |

Dependency closure is target-owned: the existing HIP/CUDA FA dispatcher and
tile kernel, Q8_0/Q4_0 type and dequant primitives, ROCm compile workarounds,
the 64/128/256 template-instance build registration, the prior vec/F16 paths,
ROCmFPX/TurboQuant selector logic, L14Q-T01, and existing CPU correctness
oracle. No new external library, submodule, generated asset, model, runtime
service, state format, or persistent write is admitted.

## Implementation, tests, rollback, and review gate

The clean implementer may use this behavioral contract and target source only:
add compile-time `GGML_HIP_QUANT_KV_FATTN_TILE=OFF` and a selector that, only
when enabled, admits symmetric standard Q8_0/Q4_0 at D128/D256, GQA ratio 8,
and single-token decode; dequantize bounded K/V tiles into shared memory,
and fall back to the exact prior selector otherwise. ROCmFPX-only and Turbo types,
asymmetric K/V, Vulkan, prefill, persistence, and automatic enablement are out of
scope. The D64 donor paths/blobs are retained above for complete provenance but
are not an admitted H01 implementation shape.

Promotion requires clean CPU and HIP builds, feature-off/help/output and inherited
regressions, L14Q-T01 plus deterministic output/logit comparison, eligible-hit and
forced-fallback traces, malformed/unsupported-shape safety, representative nimo
qualification, and matched repeated prompt/decode/memory trials with no accepted
cell regression. Record build/binary/environment/model hashes and preserve the
prior path as immediate disable/revert rollback.

Independent provenance review verified and approved the exact source, commit,
blob, license, authorship, dependency, distribution, treatment, test, and
rollback fields in this record. Implementation promotion must still cover
correctness and no-copy similarity against all six
donor result blobs, provenance/license, dependency closure, TurboQuant/ROCmFPX
noninterference, default-off authority, tests, performance, and rollback. Until
those implementation gates pass, provider/kernel behavior must not be promoted
or enabled by default. P3 approval admits only the bounded clean-reimplementation
work described here.
