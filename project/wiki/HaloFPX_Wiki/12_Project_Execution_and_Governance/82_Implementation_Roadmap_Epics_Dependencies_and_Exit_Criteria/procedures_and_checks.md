---
section_id: "82"
title: "Roadmap Procedures and Checks"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: ["llama.cpp@788e07dc91d266ad3162a1ce9037665656269689", "ROCmFPX@a5605a72768c6562241b248e268e33dc92787394", "CachyLLama@6be745998f568e379ea197fcf827baec73ff9940", "llama-ai@1017f3dfdce3ca2b06aa9007b23295db3bb35722"]
  software_versions: ["roadmap baseline 2026-07-17"]
  hardware_revisions: ["dual Strix Halo premise"]
related_sections: ["05", "09", "11", "16", "18", "20", "23", "29", "38", "48", "50", "51", "52", "53", "55", "57", "58", "59", "60", "70", "71", "72", "73", "74", "75", "76", "77", "78", "79", "80", "81", "84"]
---

# Roadmap procedures and checks

## Gate review procedure

Prerequisites: the candidate commit/tag, phase manifest, linked requirement/ADR/experiment IDs, raw evidence hashes, and a reviewer who did not author every material claim. Root access is not required for review; individual hardware experiments may require it and must say so in their own cards.

1. Freeze candidate identity: source commits, patch manifest, toolchain, package/firmware, model and configuration/plan hashes.
2. Validate evidence structure and hashes. Reproduce summaries from raw data where practical.
3. Map every epic exit criterion to `pass`, `fail`, `not-applicable`, or `waived`. A waiver needs owner, reason, scope, expiry, and rollback effect.
4. Review correctness/security blockers before performance. Any accepted incompatible cache, cross-tenant disclosure, silent partial output, or unexplained oracle divergence fails the gate.
5. Exercise rollback from the exact candidate. Confirm service and data compatibility rather than checking only that an old binary launches.
6. Record `accept`, `revise`, `defer`, or `reject`, with evidence and next owner. An artifact cannot approve its own unsupported claim.
7. Review the gate record itself for freshness, clarity, provenance, and reusable improvements.

## Gate checklist

| Gate | Required checks | Exit record |
|---|---|---|
| G82-00 | source bundles verify; offline checkout; clean build recipe; licenses/notices; patch lanes; evidence IDs | baseline manifest and signed/immutable candidate tag |
| G82-10 | HG82-A..F applicable baselines; node symmetry classification; stable hardware profile | qualified platform profile or explicit single-node-only scope |
| G82-20 | one admitted model; CPU/oracle and backend tests; API/lifecycle/restart; truthful readiness; local auth | MUP-1 manifest, test bundle, runbook |
| G82-30 | independent replicas; session affinity; overload and peer-loss; no state confusion | MUP-2 plan and failover evidence |
| G82-40 | path identity/independence; real-message curves; framing/credit/integrity faults; baseline carrier rollback | fabric envelope and carrier/multipath ADR |
| G82-50 | matched cache-disabled replication/coupled cells; correctness; p99/capacity; rank fault; fallback; operations cost | selected-mode ADR or no-go ADR |
| G82-60 | fingerprint/state inventory; atomic commit; crash/corruption/isolation; restore equivalence; endurance | HaloKV schema and MUP-4 evidence |
| G82-70 | clean deployment/cold boot; security; migration/backup/rollback; stress/fault/soak; reproducibility; cache-integrated cells only if persistent cache is admitted | independently reviewed release evidence bundle with admitted-feature dependency record |

## Required on-machine experiment program

The `M82-*` IDs are immutable section-local roadmap aliases, not canonical experiment IDs. External issues, runs, evidence bundles, and ADRs must use the owning `HLX-EXP-*` ID from [Section 84](../84_On_Machine_Research_Plan_Experiment_Cards_and_Lab_Notebook/README.md) and may retain `M82-*` only as `source_scope` metadata. Because the local aggregates do not have the same boundaries as the Section 84 cards, the crosswalk below states scope relationships rather than inventing one-to-one aliases.

| Local roadmap alias | Canonical Section 84 owner ID(s) | Relationship / external-use rule |
|---|---|---|
| `M82-01` | `HLX-EXP-20260717-841` | narrower inventory slice; schedule under the canonical card |
| `M82-02`, `M82-03`, `M82-04` | `HLX-EXP-20260717-843` | three roadmap slices of one canonical single-node baseline card |
| `M82-05`, `M82-06` | `HLX-EXP-20260717-848` (candidate owner) | **[OPEN]** current canonical dependency order is later than these pre-coupled service slices; Section 84 must split or re-home them before scheduling |
| `M82-07` | `HLX-EXP-20260717-844` | narrower dual-link slice; schedule under the canonical card |
| `M82-08` | `HLX-EXP-20260717-844`, `HLX-EXP-20260717-845` | composite fabric/buffer conformance slice; cite both owner cards |
| `M82-09` | `HLX-EXP-20260717-847` (candidate owner) | **[OPEN]** requires a cache-off canonical card/dependency path; the current owner depends on HaloKV and must not block G82-50 |
| `M82-10`, `M82-11` | `HLX-EXP-20260717-846` | two roadmap slices of the canonical HaloKV card; applicable only when persistent cache is being qualified |
| `M82-12` | `HLX-EXP-20260717-848`, `HLX-EXP-20260717-849`, `HLX-EXP-20260717-850`; add `HLX-EXP-20260717-846` when cache is admitted | composite service/fault/holdout campaign, not an alias to one card |

No row above creates a new canonical experiment definition. The two mappings marked **[OPEN]** are not executable until Section 84 resolves card ownership and dependency order.

| ID | Experiment | Prerequisites | Captured result | Decision unlocked |
|---|---|---|---|---|
| M82-01 | same-window BOM, firmware, port/cable and software inventory | physical access; Section 18 card | raw commands/photos, hashes, discrepancy table | HG82-A/B |
| M82-02 | clean/offline HIP+Vulkan builds on both nodes | frozen bundles/toolchain | logs, binary hashes, dependency/provenance records | G82-00 |
| M82-03 | model/quant/backend correctness and quality oracle | hashed source/GGUF/corpus | backend-op results, logits/tokens, PPL/task metrics, fallback inventory | G82-20 |
| M82-04 | memory/context/concurrency allocation staircase | M82-03 | peak allocations, failure boundary, safety headroom | MUP-1/fallback envelope |
| M82-05 | single-node service conformance and restart | E82-21 | request/stream/cancel/error/readiness traces | G82-20 |
| M82-06 | replicated routing, affinity, overload and peer-loss | M82-05 on both nodes | client-visible outcomes, recovery timing, state disposition | G82-30 |
| M82-07 | single/dual-link independence and service envelope | stable node profiles | RTT/bandwidth/tails, CPU/IRQ/errors/power, path identity | HG82-D |
| M82-08 | GPU-produce to peer-GPU-consume transport conformance | E82-41; M82-07 | copy/sync path, integrity, completion semantics, faults | G82-40 |
| M82-09 | matched cache-disabled replication/remote-draft/TP/pipeline/MoE cells that pass feasibility | M82-03,M82-08; canonical owner split still OPEN | correctness, capacity, TTFT/ITL/throughput/tails/power | G82-50 |
| M82-10 | cache restore/recompute, crash, mutation, incompatibility and isolation matrix | E82-60 | raw state identities, rejection reason, equivalence, recovery | G82-60 safety |
| M82-11 | cache I/O, hit-rate, write amplification and endurance under mixed load | M82-10; storage baseline | latency, bytes, SMART deltas, thermal/contention | cache value/durability tier |
| M82-12 | integrated long-context, multi-session, fault, 24-hour initial soak, upgrade/rollback | cache-off admitted features; add M82-10/11 only when persistent cache is admitted | availability/errors, tails, power/thermal, migration and rollback evidence | G82-70 |

**[OPEN]** Sample counts, warmup, statistical intervals, and acceptance thresholds must be ratified under Sections 09, 73, 74, 76-81 before release use. “24-hour initial soak” is inherited from Section 09 and is not a mature availability claim.

## Safe source freeze example

Prerequisites: Git 2.x, network for creation, enough non-secret storage. No root required. Replace paths with a dedicated artifact directory; commands are non-destructive to the checkout.

```powershell
$repoPath = 'C:\path\to\repo'
$artifactPath = 'C:\path\to\artifacts\source-baseline'
$commit = '788e07dc91d266ad3162a1ce9037665656269689'
git -C $repoPath rev-parse --verify "$commit^{commit}"
git -C $repoPath status --porcelain=v2
git -C $repoPath bundle create "$artifactPath\llama-cpp.bundle" $commit
git bundle verify "$artifactPath\llama-cpp.bundle"
Get-FileHash -Algorithm SHA256 "$artifactPath\llama-cpp.bundle"
```

Record output rather than assuming success. A bundle excludes working-tree changes, model files, external packages, and LFS payloads; inventory those separately [S82-06].

## Prototype review template

Every E82-52 spike answers:

- hypothesis and admitted model/workload;
- exact source/build/hardware/plan identities;
- ownership of sequence, rank-local state, sampling/RNG, output visibility and recovery;
- comparator and controlled variables;
- correctness before performance;
- link/rank/cancel/timeout behavior and single-node fallback scope;
- measured value with raw evidence, or a capacity-only justification;
- maintenance/security/operability cost;
- terminal `admit`, `defer`, `reject`, or `retire` decision and rollback.

## Internet follow-up

| ID | Task | Trigger | Output |
|---|---|---|---|
| I82-01 | refresh all four repository heads, releases, ancestry and license metadata | each baseline proposal | superseding source snapshot; old pin retained |
| I82-02 | monitor llama.cpp server/backend/state changes affecting patch lanes | weekly or upstream candidate | lane impact report and tests |
| I82-03 | monitor ROCm/kernel/Mesa/USB4 changes only against exact target profile | monthly and before platform change | compatibility proposal; no automatic upgrade |
| I82-04 | refresh upstream facts used by Sections 50-53,57-60,70-72,74,76-77,79-81 and execute their unresolved experiment/decision backlogs | before dependent ADR/gate and on expiry trigger | reviewed evidence updates, ADRs, experiment receipts, and retained superseded baseline |
| I82-05 | refresh security advisories and dependency/SBOM tooling | each release candidate | vulnerability disposition |

## Closeout validation for this section

Validate front matter/YAML, exactly seven required files, relative links, source/open-question/experiment counts, unique IDs, claim labels, word counts below 2,500 per page, and `git diff --check`. No benchmark is implied by a documentation validation pass.
