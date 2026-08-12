# Implementation readiness gates

## Current verdict

**[VERIFIED] READY TO BEGIN LOCAL IMPLEMENTATION.** ROCmFPX `61f2f2d7bc4955e9bca821095ef69125837133b5` is the selected base. The source lock, complete research intake, safe implementation-start license boundary, feature-off contract, two-node candidate qualification, and static fixture subset are ready. Begin with a local writable fork, contracts, and target-native seams. Remote creation, donor capability import, persistent writes, deployment, and release remain separately gated. [Preparation status](../PREPARATION_STATUS.md)

**[MEASURED]** A later explicit user authorization permits controlled local experiments on both target nodes and unloading the current model. It does not authorize donor import, remote-fork creation, production cutover, model/cache deletion, or release promotion. The controlled unload and current experiment scopes are preserved under `sources/measurements/2026-07-17-local-preparation/` and `experiments/`.

## Gate map

| Gate | Required evidence | Current state |
|---|---|---|
| G0A local source identity | selected candidate, full commit/tree/gitlinks, clean state, bundles, patch/license/build-input manifest, hashes | **[VERIFIED] Complete.** Selected pin `61f2f2d...`; source lock and offline bundles verified |
| G0B repository governance | owner/name, visibility, fork form, permissions, branch protections, force-push/tag/signing policy, evidence location, create/push authority | **[OPEN]** Nonblocking for local work; `OPEN-GOV-01` blocks remote creation/push only |
| G0C candidate test assets | hashes, provenance, license, static review, isolated deterministic validation, explicit promotion | **[VERIFIED] Partial promotion.** 52 static CC0 references accepted by exact hash; seven candidate-execution assets remain `OPEN-TEST-01` |
| G1 provenance/license | every selected capability at P3; treatment, dependency closure, notices/SBOM, distribution and clean-reimplementation decisions | **[RECOMMENDATION] Ready for the first capability record.** MIT-core/GPL-separation policy fixed; direct cherry-pick roster empty; each donor capability still requires P3 approval |
| G2 target baseline | selected ROCmFPX builds and is characterized on both nodes with exact manifests and approved API/cache/backend fixtures | **[VERIFIED] Complete for implementation start.** Selected revision built and passed available CPU, ROCm0 attention, F16/Turbo4 request, and private RPC smokes on both nodes; quality/performance/release expansion remains open |
| G3 feature-off equivalence | defaults, API, scheduler, current cache, quantization, MTP/speculative, HIP/Vulkan, RPC behavior match approved baseline | **[OPEN]** No integration implementation exists |
| G4–G7 persistent-state safety | complete transactional restore, corruption-as-miss, crash/storage safety, isolation, quotas/reserve, diagnostics, rollback | **[OPEN]** Format/state/scope/storage contracts and experiments unresolved |
| G8 target matrix | both nimo hosts pass exact build/runtime matrix; package and boot skew controlled | **[OPEN]** Live inventory is evidence, not qualification |
| G9 performance/value | human-approved thresholds derived from matched variance; SSD and distributed benefit exceed cost without correctness/quality regression | **[OPEN]** No HaloFPX performance measurement exists |
| G10 release | immutable manifest/tag/artifacts, deployment receipt, rollback proof, independent review | **[OPEN]** Not started |

## Safe execution order

1. **[RECOMMENDATION]** Restore the locked selected base into a new writable local repository and record a clean baseline manifest.
2. **[RECOMMENDATION]** Reproduce the feature-off contract, then add target-native interfaces and seams before donor code.
3. **[OPEN]** Obtain the human repository-governance decision before remote creation (G0B).
4. **[RECOMMENDATION]** Qualify the seven deferred assets only in isolation against the new candidate.
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
