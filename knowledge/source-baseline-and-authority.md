# Source baseline and authority

## Decision rule

When sources conflict, use the user-approved project goal for destination and phase order; exact Git objects and preserved manifests for code identity; live captures for machine state; the canonical Wiki for reviewed research context; and review artifacts for recommendations. Imported Wikis and draft plans never approve themselves.

## Baseline ledger

| Role | Exact identity | Status and use |
|---|---|---|
| Canonical product lineage | `charlie12345/ROCmFPX` | **[VERIFIED]** The project goal selects ROCmFPX as the future writable-fork base. This resolves product lineage, not the exact starting commit or synchronization mechanics. [Goal](../PROJECT_GOAL.md) |
| Frozen ROCmFPX research control | `a5605a72768c6562241b248e268e33dc92787394` | **[VERIFIED]** Preserved and present in the reference clone. Retain for comparison and research applicability. [Repository manifest](../sources/repositories/manifest.yaml) |
| Selected ROCmFPX implementation base | `61f2f2d7bc4955e9bca821095ef69125837133b5` / tree `0a35143f33a7b99a81c824fa8ffd8f743f7ae0dd` | **[VERIFIED]** Selected after matched build, correctness, runtime, and RPC qualification on both gfx1151 nodes. `OPEN-PIN-01` is closed for local implementation; release qualification remains separate. [Pin decision](../reviews/readiness/2026-07-18__implementation-pin__decision__v01.md) |
| Operational requirements donor | `fewtarius/llama-ai@1017f3dfdce3ca2b06aa9007b23295db3bb35722` | **[VERIFIED]** Exact preserved GPL-licensed wrapper snapshot. Requirements reference unless a separate distribution decision says otherwise. [Repository manifest](../sources/repositories/manifest.yaml) |
| MIT engine donor | `fewtarius/CachyLLama@6be745998f568e379ea197fcf827baec73ff9940` | **[VERIFIED]** Exact preserved donor and matching llama-ai gitlink. Capability-level provenance is still required before import. [Repository manifest](../sources/repositories/manifest.yaml) |
| Donor comparison parent | `ggml-org/llama.cpp@92366df30d4eaa4b85139b5fd694360237731b19` | **[VERIFIED]** Comparison anchor for the preserved CachyLLama merge, not the HaloFPX base. [Accepted Phase 0A plan](../reviews/plans/2026-07-17__rocmfpx-llama-ai-fork-plan__draft__v03.md) |
| Canonical Wiki upstream control | `ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689` | **[VERIFIED]** Research applicability control. Imported packages using other upstream pins require an explicit delta, not silent substitution. [Wiki Section 11](../wiki/HaloFPX_Wiki/03_Repository_and_Engineering/11_Repository_Lineage_Branches_Commits_and_Frozen_Baselines/README.md) |
| Deployed operational baseline | `charlie12345/rocmfp4-llama@4860505ee322091f0f61eba77d6ad49be88cf4ea` | **[MEASURED]** Running on both nodes during the 2026-07-17 capture. It is rollback and matched-performance evidence, not the HaloFPX source base. [Live comparison](../sources/measurements/2026-07-17-strix-halo-live-inventory/comparison.md) |

## Research control versus implementation candidate

- **[VERIFIED]** A research control fixes the applicability of existing findings. It is not automatically the commit from which implementation should begin.
- **[VERIFIED]** Keep `a5605a...` immutable as the research control and begin local implementation from selected pin `61f2f2d...`. If a later mandatory case fails, repair or disable the affected lane against the retained control; do not silently advance the base.
- **[OPEN]** The writable repository owner/name, visibility, fork-network form, protections, signing authority, evidence location, and create/push authority remain `OPEN-GOV-01`.
- **[OPEN]** The canonical Wiki recommends real llama.cpp ancestry for long-term synchronization, while the user-directed goal requires a ROCmFPX-based product fork. The revised plan uses the ROCmFPX product lineage and prohibits ordinary unrelated-history merges, but the final synchronization/reconstruction ADR remains unapproved. [Wiki Section 15](../wiki/HaloFPX_Wiki/03_Repository_and_Engineering/15_Integration_Patch_Stack_and_Upstream_Synchronization_Strategy/README.md)

## Evidence handling

- **[VERIFIED]** Reference clones under `sources/repositories/` are clean, read-only evidence captures; no code was built or executed during capture. [Repository manifest](../sources/repositories/manifest.yaml)
- **[VERIFIED]** The twelve imported Wiki archives passed preservation/integrity review, but structural integrity does not establish claim correctness, licensing permission, executable safety, or machine applicability. [Integrity review](../reviews/intake/2026-07-17__twelve-wiki-integrity__review__v01.md)
- **[RECOMMENDATION]** Record every build and result with full source/tree/patch identity, dirty state, toolchain, dependency lock, kernel/firmware, CMake cache, environment allowlist, device identity, fixture/model hashes, executable hash, and command line.
