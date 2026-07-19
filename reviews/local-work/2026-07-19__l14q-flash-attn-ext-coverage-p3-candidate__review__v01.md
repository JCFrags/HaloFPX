---
title: L14Q FLASH_ATTN_EXT quantized-KV coverage approved P3 record
date: 2026-07-19
status: approved-p3
lane: L14Q
capability_id: L14Q-T01
treatment: target-native-clean-reimplementation
runtime_behavior_change: false
donor_code_copied: false
p3_state: approved
reviewer: independent-codex-provenance-review
review_date: 2026-07-19
reviewer_decision: approve
---

# L14Q-T01 target-native quantized-KV test coverage

## Admission decision

This record approves exactly one P3 unit: a **target-native, test-only clean
reimplementation** of the coverage requirement evidenced by Nathan Wilson's
commit `6b03608e63f48c9371bf5f00423da413ac0288de`. The unit may add HaloFPX-owned
`FLASH_ATTN_EXT` cases for standard Q8_0 and Q4_0 K/V at head dimensions 128 and
256 and relevant GQA ratios including 8. It must not copy donor source lines,
change a runtime kernel or selector, enable an optimization, or alter feature-off
behavior.

An **independent Codex provenance review** accepted this exact record for P3
approval on 2026-07-19 with no findings. This approval admits the provenance,
license disposition, no-copy treatment, dependency closure, test contract, and
rollback boundary only. It does not assert that the test implementation exists,
passes, or qualifies either runtime optimization.

Project authority is the accepted [v03 implementation plan](../plans/2026-07-17__rocmfpx-llama-ai-fork-plan__draft__v03.md), the [L14Q plan addendum](../plans/2026-07-18__quantized-kv-fa-optimization-plan__v01.md), the preserved [candidate intake](../../sources/repositories/candidate-intake/2026-07-18-strix-halo-quant-kv/README.md), and the canonical [attention/KV Wiki section](../../wiki/HaloFPX_Wiki/06_Models_Quantization_and_Inference/33_Attention_Variants_KV_Layouts_FlashAttention_and_TurboQuant/README.md).

## Exact target anchor

| Field | Exact value |
|---|---|
| HaloFPX target commit | `051084fa3ab724cd290f864c093ff67f16e13a90` |
| HaloFPX target tree | `b2fed43eeb5ccba9286638cf024d39452bf697e0` |
| ROCmFPX base commit | `61f2f2d7bc4955e9bca821095ef69125837133b5` |
| ROCmFPX base tree | `0a35143f33a7b99a81c824fa8ffd8f743f7ae0dd` |
| Target test path | `tests/test-backend-ops.cpp` |
| Target test blob at base and target | `5e712d7271f23e4ebff14b60bf234f8b7e4d394a` |

HaloFPX `051084fa...` descends from both the exact base and the previously
approved `80ab1edc...` target. The intervening L05x milestone did not change
`tests/test-backend-ops.cpp`: its blob remains `5e712d7271f23e4ebff14b60bf234f8b7e4d394a`
at the base, prior target, and refreshed target. This explicit anchor refresh
does not expand the approved treatment or silently advance the source base. A
later implementation commit must record its own parent, tree, and resulting
target blob.

## Preserved source authority and integrity

| Field | Exact value |
|---|---|
| Source repository | `Nathanw1014/llama.cpp` |
| Locked branch | `strix-halo-fa-fixes` |
| Locked head | `a18067a85e986f7798f43d98345ed5b86b55cf88` |
| Locked head tree | `130e9cac828f8d8ef877d87ea9c192e24b07c9af` |
| Complete Git bundle | `bundles/Nathanw1014__llama.cpp--strix-halo-fa-fixes.bundle` |
| Bundle size | `379353651` bytes |
| Bundle SHA-256 | `79C61718BD60ECCE3E5EA3919FB18D8C709B3D26032C5FE9D4F24055ADE3BC3F` |
| Preserved combined HIP/test patch | `patches/hip-tile-quant-kv-dequant-on-load-and-tests.patch` |
| Patch size | `33907` bytes |
| Patch SHA-256 | `66DDB0E33301AB7231C736BE318483E90CFBA813701FBEB5269F547A65C4F42D` |

The combined patch also contains an out-of-scope runtime commit. Its presence in
the preserved artifact does not admit that runtime code. This unit derives only
the coverage requirement from the exact test commit below.

## Introducing commit, authorship, and blob lineage

| Field | Exact value |
|---|---|
| Introducing commit | `6b03608e63f48c9371bf5f00423da413ac0288de` |
| Parent | `4355d03e86083004bd9a084eed039987806ece8a` |
| Tree | `ef7e7ca94a7d2146d8649cdcf1256d66fdca5da3` |
| Author | `Nathan Wilson <nath.flagman151@passmail.com>` |
| Author date | `2026-07-17T08:30:04Z` |
| Subject | `tests: cover quantized KV at head sizes 128 and 256, and gqa_ratio 8` |
| Git signature state | unsigned (`%G? = N`) |
| Signed-off-by trailer | absent |
| Assisted-by trailer | absent |
| Donor path | `tests/test-backend-ops.cpp` |
| Donor parent blob | `5b5f196184ba8eb098e2bd20f0b0c74d706e73ab` |
| Donor result blob | `1cbd27cb4d163fbc9970b57e0886a38adf62542f` |
| Donor delta | 15 insertions, 2 deletions |

The author states that the donor enumeration previously skipped quantized K/V
for head dimensions other than 64 or 72, did not cover head dimensions 128 or
256, and did not exercise GQA ratio 8 at head dimension 128. The commit message's
reported increase from 2,880 to 6,000 upstream cases and its “all pass on
master” result are donor claims, not HaloFPX measurements or acceptance results.

### Authorship and AI-assistance disposition

- This record attributes the coverage requirement to Nathan Wilson and retains
  the exact commit identity even though no donor implementation text will be
  copied.
- The introducing test commit has no `Assisted-by` trailer. A separate,
  out-of-scope Vulkan commit in the preserved branch records `Assisted-by:
  Claude (Opus 4.8)`; that trailer does not establish authorship of this unit and
  is not imported into the target. It remains visible in the source intake.
- The absent Git signature and sign-off mean Git history alone is not a legal
  approval. The independent Codex provenance review accepted the recorded MIT
  source identity and no-copy treatment for this project-scoped P3 disposition;
  that review is not legal advice and does not authorize a broader import.

## License, attribution, notice, SBOM, and distribution

| Field | Disposition |
|---|---|
| Source license | MIT |
| Preserved license path | `Nathanw1014__llama.cpp--strix-halo-fa-fixes/LICENSE` |
| License Git blob at locked head | `e7dca554bcb802f98408383a864404e3aa4eacca` |
| License file size | `1099` bytes |
| License SHA-256 | `BCD8EC749126D45CB06737D0690295D73DF4B6E7E194205BCF91190368F27285` |
| Copyright notice | `Copyright (c) 2023-2026 The ggml authors` |
| Target code license | HaloFPX MIT core remains unchanged |
| Donor code in target | none permitted for this unit |
| Release NOTICE change | none required for the no-copy target-native treatment; retain this provenance record |
| SBOM change | none; no package, library, generated binary, shader, or runtime dependency is added |
| Distribution effect | HaloFPX-owned test code remains MIT; no donor substantial portion is distributed |
| Requirement attribution | retain source repository, commit, author, and this record in project provenance |

If review or similarity inspection finds copied expressive text or code, this
disposition is invalid. The unit must be removed or reclassified as an
attributed manual port with a new notice/distribution review.

## Treatment and implementation boundary

Treatment code: **CR/NA** — clean target-native reimplementation of a behavioral
test requirement.

Allowed:

- write new HaloFPX-owned test-case construction using the target's existing
  `test_flash_attn_ext` interface and local conventions;
- cover symmetric Q8_0/Q8_0 and Q4_0/Q4_0 K/V at head dimensions 128 and 256;
- cover GQA ratio 8 where the target contract supports it;
- add target-specific boundary, unsupported-shape, and fallback assertions;
- record a separately source-pinned MiniMax shape before adding a model-specific
  case; the current planning statement alone is not an exact shape authority.

Forbidden:

- copying or mechanically translating donor test lines, loop structure,
  comments, names, ordering, or literal case tables;
- applying either preserved patch, cherry-picking the donor commit, or importing
  the donor test blob;
- modifying `ggml/src/ggml-cuda`, `ggml/src/ggml-vulkan`, kernel routing,
  TurboQuant selection, model code, CLI/help, or server runtime behavior;
- treating passing tests as admission of either the HIP or Vulkan optimization;
- claiming ROCmFPX weight-format coverage. Large MiniMax UD-Q6_K_XL weights are
  only a workload; standard runtime `-ctk/-ctv q8_0/q4_0` types are independent.

The direct-cherry-pick roster remains empty.

## Dependency closure

This unit depends only on target-owned test infrastructure already present at
the target anchor:

1. `tests/test-backend-ops.cpp` and its existing `test_flash_attn_ext`
   constructor, backend enumeration, correctness oracle, skip logic, and failure
   reporting;
2. existing standard `GGML_TYPE_Q8_0` and `GGML_TYPE_Q4_0` definitions and CPU
   reference behavior;
3. existing HIP and Vulkan `FLASH_ATTN_EXT` backend availability and fallback
   behavior;
4. existing CMake/test registration and inherited backend-op runner.

There are no new external dependencies, submodules, downloaded fixtures,
generated source assets, runtime flags, environment variables, model files, or
service changes. HIP and Vulkan results must be reported separately even when a
shared test generator constructs their cases.

## Qualification contract

Before promotion, the implementation owner must retain:

1. a source diff proving only target-owned test/build-registration paths changed
   and no runtime path changed;
2. a similarity review against donor blob `1cbd27cb...` confirming no copied or
   mechanically translated expressive code;
3. clean feature-off build/help/output contract results;
4. clean CPU build plus applicable HIP and Vulkan builds on the pinned source;
5. inherited backend-op results and exact new case inventory;
6. deterministic oracle results for Q8_0/Q8_0 and Q4_0/Q4_0 at head dimensions
   128 and 256, GQA ratios including 8, boundary context lengths, and all
   applicable fallback/unsupported cells;
7. separate nimo-1 and nimo-2 raw results with executable/source/environment
   hashes and zero unexpected skips;
8. proof that the same tests fail or skip for a deliberately unsupported
   configuration for the documented reason rather than silently passing;
9. independent correctness, provenance, license, clarity, rollback, and
   reusable-improvement review.

This test-only unit does not require a performance claim. It must not be used to
claim a HIP/Vulkan speedup or zero-regression result. Those gates belong to the
later runtime lanes.

## Rollback and contamination response

Rollback is deletion/revert of the single target-native test milestone. Because
the unit changes no runtime source, runtime behavior and deployments remain the
selected ROCmFPX/HaloFPX control throughout. If copied donor text, an undeclared
dependency, a runtime change, or misleading pass coverage is found, quarantine
the test commit, restore the prior target test blob, and repeat independent
provenance and correctness review before reconsideration.

## Independent reviewer decision

| Field | Value |
|---|---|
| Reviewer | **Independent Codex provenance review** — independent of the implementation author |
| Review date | `2026-07-19` |
| Decision | **APPROVE for P3**, no findings |
| Approved target parent | confirmed `051084fa3ab724cd290f864c093ff67f16e13a90`; descendant of prior approved target `80ab1edc8a66f3a1922f1af620141c24d5881da0`, with the target test blob unchanged |
| Similarity/no-copy finding | no donor source is admitted; the no-copy boundary is approved and post-change similarity evidence remains an implementation promotion gate |
| License/notice/SBOM finding | approved exactly as recorded; no notice or SBOM change for this no-copy test-only treatment |
| Test ownership and qualification acceptance | qualification contract approved; execution evidence remains required before implementation promotion |

`L14Q-T01` satisfies P3 only for the exact target-native, test-only treatment in
this record. Any donor code import, runtime source change, different target
anchor, or broader test capability requires a new or revised independently
approved provenance record.
