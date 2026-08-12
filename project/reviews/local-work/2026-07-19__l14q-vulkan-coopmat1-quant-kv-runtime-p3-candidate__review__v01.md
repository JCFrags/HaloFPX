---
title: L14Q Vulkan coopmat1 quantized-KV prefill P3 candidate
date: 2026-07-19
status: approved-independent-p3-review
lane: L14Q
capability_id: L14Q-VK-01
treatment: target-native-clean-reimplementation
runtime_behavior_change: default-off-candidate
donor_code_copied: false
p3_state: approved
reviewer: independent-provenance-review
reviewer_decision: approve
---

# L14Q-VK-01 target-native Vulkan coopmat1 shared-dequant prefill

## Candidate admission boundary

This record proposes one narrowly bounded P3 unit: a HaloFPX-owned clean
reimplementation that, for eligible Vulkan coopmat1 prompt processing with
symmetric standard Q8_0 K/V, performs one bounded dequantization into F16 scratch
per invocation and reuses that scratch across grouped query work. Scratch or
layout incompatibility must select the exact prior path, never abort. The lane
must remain independent and default-off and must preserve ROCmFPX/TurboQuant FA
routing and its existing Turbo fallback pre-dequant path.

Project authority is the accepted [v03 implementation plan](../plans/2026-07-17__rocmfpx-llama-ai-fork-plan__draft__v03.md), the [L14Q addendum](../plans/2026-07-18__quantized-kv-fa-optimization-plan__v01.md), the preserved [candidate intake](../../sources/repositories/candidate-intake/2026-07-18-strix-halo-quant-kv/README.md), the approved [L14Q-T01 record](2026-07-19__l14q-flash-attn-ext-coverage-p3-candidate__review__v01.md), and the canonical [attention/KV Wiki section](../../wiki/HaloFPX_Wiki/06_Models_Quantization_and_Inference/33_Attention_Variants_KV_Layouts_FlashAttention_and_TurboQuant/README.md).

This record received independent P3 approval on 2026-07-19. Its author inspected
the donor history and is not the independent reviewer. No donor C++, GLSL, comments,
identifiers, addressing expressions, shader-generation entries, or patch text
may be copied or mechanically translated. The direct-cherry-pick roster remains
empty.

## Exact target anchor and unchanged seams

| Field | Exact value |
|---|---|
| HaloFPX inspected commit / tree | `7e505d202147fb97955b35a15094e3c23029d4bb` / `6b268262270a201a2a914070b6286c8f10c35d7c` |
| ROCmFPX base commit / tree | `61f2f2d7bc4955e9bca821095ef69125837133b5` / `0a35143f33a7b99a81c824fa8ffd8f743f7ae0dd` |
| Runtime target paths | `ggml/src/ggml-vulkan/ggml-vulkan.cpp`, `vulkan-shaders/dequant_q8_0.comp`, `vulkan-shaders/vulkan-shaders-gen.cpp` |
| Target blobs at base and inspected anchor | `93fb1331e65767e913aca1ee1d532a5022b426c6`; `10844ddf7813b71f69a90cbaea06fa0df363bd88`; `e5cd458a895676547d7570cfb9aea56f643d9b61` |
| Qualified target-owned test seam | L14Q-T01 commit `37ff5e4f6ab48ed7d8b0ea2fda05a6304091ae2b`; current test blob `543c26432318b243e3412bb1aff2f8874e67a963` |

The runtime blobs are unchanged between the exact ROCmFPX base and this HaloFPX
anchor. The target already has distinct ROCmFPX Turbo fused routing and Turbo
fallback pre-dequant/scratch behavior; this unit must integrate without replacing
or broadening it. A later implementation record must refresh its parent/tree and
result blobs.

## Preserved source, introducing commits, authorship, and blobs

| Field | Exact value |
|---|---|
| Source | `Nathanw1014/llama.cpp`, `strix-halo-fa-fixes` |
| Locked head / tree | `a18067a85e986f7798f43d98345ed5b86b55cf88` / `130e9cac828f8d8ef877d87ea9c192e24b07c9af` |
| Complete bundle | `bundles/Nathanw1014__llama.cpp--strix-halo-fa-fixes.bundle`, 379353651 bytes, SHA-256 `79C61718BD60ECCE3E5EA3919FB18D8C709B3D26032C5FE9D4F24055ADE3BC3F` |
| Preserved patch | `patches/vulkan-coopmat1-dequant-transpose-and-fallback.patch`, 12882 bytes, SHA-256 `AF351194081882C510D38BF86AEB4194BE03A86D389DA756CF2FB73DA1259FDA` |

The runtime unit has two inseparable introducing commits:

| Commit | Parent / tree | Author/date | Subject | Signature/trailers |
|---|---|---|---|---|
| `4edaca09fa36acc16e7b95a6544a80ccd0dff657` | `635cdd5fcc5bdeb8ec2e108bb2a40acf62d9039b` / `3f6781d154574ea82d3f8037fb4b2bba069945cb` | Nathan Wilson `<nath.flagman151@passmail.com>`, `2026-07-09T15:40:08Z` | `vulkan : dequant q8_0 KV once in coopmat1` | unsigned; `Assisted-by: Claude (Opus 4.8)`; no sign-off |
| `4355d03e86083004bd9a084eed039987806ece8a` | `4edaca09fa36acc16e7b95a6544a80ccd0dff657` / `05a8fa612fe7fb587edeee8f7b66e5f096ac713c` | Nathan Wilson `<nath.flagman151@passmail.com>`, `2026-07-10T18:41:26Z` | `vulkan : fall back instead of aborting when FA scratch exceeds maxStorageBufferRange` | unsigned; no assisted/sign-off trailer |

The first commit changes 3 files (94 insertions, 5 deletions); the required
fallback follow-up changes 1 file (3 insertions, 4 deletions). Exact donor blob
lineage:

| Donor path | Pre-unit blob | After mechanism | After safe fallback |
|---|---|---|---|
| `ggml/src/ggml-vulkan/ggml-vulkan.cpp` | `c3fd436a101193f2faad5272f25ca28db32a7c79` | `3bd7d5062583f9aaae80e1dbd116f8c2f4749486` | `f20dbb02472f87f286ed41bd7914642e4d6e0213` |
| `.../vulkan-shaders/dequant_q8_0.comp` | `10844ddf7813b71f69a90cbaea06fa0df363bd88` | `8ca44cf40ce4e583fbbffbca3541c307783f1675` | unchanged |
| `.../vulkan-shaders/vulkan-shaders-gen.cpp` | `58d347bc547d75fa891bd34003067b7655f0c36c` | `85a789244ff230cf62e49fda87dcdeb6d987d605` | unchanged |

The assisted-by trailer is retained as provenance. It does not change Nathan
Wilson's Git authorship or authorize importing generated expression. The open
upstream PR snapshot is supplementary review evidence; the locked branch and
complete bundle above are the exact source authority for this record.

## License, attribution, dependencies, and distribution

| Field | Disposition |
|---|---|
| Source license | MIT |
| Preserved license | `Nathanw1014__llama.cpp--strix-halo-fa-fixes/LICENSE`; blob `e7dca554bcb802f98408383a864404e3aa4eacca`; 1099 bytes; SHA-256 `BCD8EC749126D45CB06737D0690295D73DF4B6E7E194205BCF91190368F27285` |
| Copyright notice | `Copyright (c) 2023-2026 The ggml authors` |
| Target license | HaloFPX MIT core unchanged |
| Donor code/assets distributed | none under this clean-reimplementation treatment |
| NOTICE/SBOM effect | no new binary/package/library/dependency; generated target-owned shader remains part of HaloFPX. Retain repository, author, commits, assisted-by fact, and this record. Reclassify and re-review if similarity shows donor expression. |

Dependency closure is target-owned: Vulkan device limits and buffer allocation,
pipeline/shader generation, Q8_0 type/block definitions, FA tuning/coopmat1
selection, layout/stride authority, synchronization, existing Turbo fused and
fallback pre-dequant paths, L14Q-T01, and the CPU correctness oracle. No external
library, submodule, downloaded shader, model, service, persistence state, or
writer is admitted. Q4_0 shared-dequant is not admitted by this record; it must
continue through the prior path unless separately proven and reviewed.

## Implementation, tests, rollback, and review gate

The clean implementer may use this behavioral contract and target source only:
for explicitly admitted dense Q8_0 layout and prefill/coopmat1 shapes, compute
bounded F16 scratch sizes with overflow-safe arithmetic, reject any per-buffer or
aggregate allocation beyond device/project limits, populate target-owned scratch,
synchronize before FA consumption, and select the existing prior path on every
ineligible or failed condition. No abort, partial scratch use, persistent write,
decode change, Turbo/ROCmFPX-type broadening, or automatic enablement is allowed.

Promotion requires clean CPU and Vulkan builds, feature-off/help/output and
inherited regressions, L14Q-T01 plus deterministic output/logit comparison,
layout/stride and eligible-hit traces, Q4_0/oversize/non-coopmat/unsupported
fallback evidence, allocation-overflow and device-limit tests, representative
nimo qualification, and matched repeated prompt/decode/scratch/resident-memory
trials with no accepted-cell regression. Rollback is the default-off control plus
immediate revert of the single Vulkan lane.

Independent provenance review verified and approved the exact source, commits,
blobs, license, authorship and assistance disclosure, dependency, distribution,
treatment, test, and rollback fields in this record. Implementation promotion
must still cover correctness and no-copy similarity against all three
final donor blobs (and the intermediate C++ blob), provenance/license, the
assisted-by disclosure, dependency closure, scratch bounds/synchronization,
TurboQuant/ROCmFPX noninterference, default-off authority, tests, performance,
and rollback. Until those implementation gates pass, shader/runtime behavior
must not be promoted or enabled by default. P3 approval admits only the bounded
clean-reimplementation work described here.
