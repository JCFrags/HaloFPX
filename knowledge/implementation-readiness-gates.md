# Implementation readiness gates

## Current verdict

**[VERIFIED]** The v03 fork plan passed independent review for authorized L00A/local read-only Phase 0A only: existing-object verification, local cryptographic inventories/bundles/manifests, provenance/license records, static source archaeology, and non-executing Stage 1 candidate-asset inspection. **[OPEN]** This acceptance does not authorize remote or commit mutation, candidate-tool execution without a separately approved Stage 2 isolation contract, donor import or implementation, persistence work, deployment, or disruptive target-node activity. [Fork-plan v03](../reviews/plans/2026-07-17__rocmfpx-llama-ai-fork-plan__draft__v03.md) and [final review](../reviews/plans/2026-07-17__rocmfpx-llama-ai-fork-plan__review__v03.md)

## Gate map

| Gate | Required evidence | Current state |
|---|---|---|
| G0A local source identity | selected candidate, full commit/tree/gitlinks, clean state, bundles, patch/license/build-input manifest, hashes | **[OPEN]** Clones and research pins exist; candidate selection and complete offline/source-lock receipt remain |
| G0B repository governance | owner/name, visibility, fork form, permissions, branch protections, force-push/tag/signing policy, evidence location, create/push authority | **[OPEN]** `OPEN-GOV-01` |
| G0C candidate test assets | hashes, provenance, license, static review, isolated deterministic validation, explicit promotion | **[OPEN]** Imported scripts remain untrusted candidate code; `OPEN-TEST-01` |
| G1 provenance/license | every selected capability at P3; treatment, dependency closure, notices/SBOM, distribution and clean-reimplementation decisions | **[OPEN]** Direct cherry-pick roster is empty; `OPEN-PROV-01`, `OPEN-LIC-01` |
| G2 target baseline | selected ROCmFPX builds and is characterized on both nodes with exact manifests and approved API/cache/backend fixtures | **[OPEN]** `OPEN-PIN-01`, `OPEN-BASE-01`, `OPEN-API-01` |
| G3 feature-off equivalence | defaults, API, scheduler, current cache, quantization, MTP/speculative, HIP/Vulkan, RPC behavior match approved baseline | **[OPEN]** No integration implementation exists |
| G4–G7 persistent-state safety | complete transactional restore, corruption-as-miss, crash/storage safety, isolation, quotas/reserve, diagnostics, rollback | **[OPEN]** Format/state/scope/storage contracts and experiments unresolved |
| G8 target matrix | both nimo hosts pass exact build/runtime matrix; package and boot skew controlled | **[OPEN]** Live inventory is evidence, not qualification |
| G9 performance/value | human-approved thresholds derived from matched variance; SSD and distributed benefit exceed cost without correctness/quality regression | **[OPEN]** No HaloFPX performance measurement exists |
| G10 release | immutable manifest/tag/artifacts, deployment receipt, rollback proof, independent review | **[OPEN]** Not started |

## Safe execution order

1. **[RECOMMENDATION]** Close G0A locally without remote or live-service mutation.
2. **[RECOMMENDATION]** Perform non-executing Stage 1 test-asset inspection. Execute still-untrusted candidates only after a separately approved Stage 2 isolation contract; promote only an exact human/reviewer-approved subset at Stage 3 (G0C).
3. **[OPEN]** Obtain the human repository-governance decision before remote creation (G0B).
4. **[RECOMMENDATION]** Qualify the ROCmFPX pin, exact target builds, and feature-off/API/cache baseline before donor import (G2–G3).
5. **[RECOMMENDATION]** Complete capability-level provenance and license dispositions before any port/reimplementation lane (G1).
6. **[RECOMMENDATION]** Freeze state, scope, format, and threat contracts before parser/writer/provider work; keep early reader/writer work offline and synthetic-only.
7. **[RECOMMENDATION]** Introduce provider seam, bounded reader, disabled writer, trusted scope, and admitted state codecs in that dependency order. Persistent canaries follow only after the safety gates pass.
8. **[RECOMMENDATION]** Preserve the deployed service and versioned rollback artifacts; use alternate-port disposable canaries, then one-node qualification, then coordinated two-node cutover only if mixed RPC versions are proven safe or both processes switch together.
9. **[RECOMMENDATION]** Begin 200–230 GB dual-node optimization only after the Phase 1 integration release passes correctness, rollback, and single-node recovery gates.

## Stop conditions

- **[OPEN]** Do not import donor code while pin, provenance, license, target baseline, or API surface is unresolved.
- **[OPEN]** Do not enable persistent writes while format, complete state, scope, storage reserve, or acceptance policy is unresolved.
- **[OPEN]** Do not treat imported validators or favorable summaries as release authority without raw-record binding and explicit promotion.
- **[OPEN]** Do not promote USB4STREAM, dual-rail striping, a distributed mode, or large-model fit from nominal rate, equal split, or source presence.
- **[OPEN]** Do not modify both live nodes at once, delete the existing 112 GiB RPC cache, or share a persistent root with the old binary without explicit authorization and rollback evidence.

The canonical roadmap remains [Wiki Section 82](../wiki/HaloFPX_Wiki/12_Project_Execution_and_Governance/82_Implementation_Roadmap_Epics_Dependencies_and_Exit_Criteria/README.md). This module is the compact preflight, not an approved procedure or schedule.
