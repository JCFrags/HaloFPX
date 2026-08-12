---
section_id: "06"
title: "Charter Outcomes and Success Definitions"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["HaloFPX project"]
  software_versions: []
  hardware_revisions: ["two matched AMD Strix Halo systems; exact BOM open"]
related_sections: ["07", "08", "09", "10", "38", "68", "80"]
---

# Outcomes and success definitions

## Strategic rationale

**[INFERENCE]** The combination of AMD-oriented formats, a capable local server, persistent prefix reuse, and two APUs creates an engineering opportunity, but the value depends on workload-specific measurements and maintainable integration—not simply combining every fork feature.

## Proposed success gates

| Gate | Definition | Evidence required |
|---|---|---|
| Correctness | **[RECOMMENDATION]** Outputs, tool-call structure, cache restores, and distributed modes pass reference and fault tests at the declared tolerance. | Golden tests, differential results, raw fault logs |
| Practical speed | **[RECOMMENDATION]** At least one target workload materially improves over its matched best single-node baseline without violating quality or reliability gates. | Matched benchmark matrix; threshold ratified in Section 09 |
| Capacity | **[RECOMMENDATION]** The pair serves at least one ratified model/context/concurrency envelope unavailable or impractical on one node. | Memory accounting and successful acceptance run |
| Resilience | **[RECOMMENDATION]** Loss of a node or link fails explicitly or degrades to a documented mode without accepting invalid state. | Fault injection and recovery timing |
| Operability | **[RECOMMENDATION]** Reproducible install, pinned builds, health/readiness, metrics, logs, backup/restore, and upgrade/rollback exist. | Clean-host rehearsal and operations checklist |
| Privacy | **[RECOMMENDATION]** Normal inference is usable without cloud dependency, with authenticated network exposure and tenant-isolated cache data. | Egress check, access tests, cache namespace audit |
| Maintainability | **[RECOMMENDATION]** Changes are a reviewable patch stack against exact upstream commits, with licenses and provenance retained. | Repository lineage and update rehearsal |

**[OPEN]** Numeric pass thresholds, supported model set, availability window, and acceptable recovery time require sponsor decisions plus baseline data.

## Outcome hierarchy

1. Minimum viable outcome: reproducible single-node ROCmFPX service with compatibility and measurement records.
2. Two-node outcome: one distributed mode passes correctness and improves a ratified workload.
3. Product outcome: automatic mode selection, persistent rank-local cache, dual-link resilience, and documented operations pass acceptance.
4. Stretch outcome: model-specific autotuning and MoE-aware placement deliver repeatable gains without unacceptable maintenance burden.

**[RECOMMENDATION]** Stop or narrow the project if dual-node modes cannot beat replication/single-node baselines after transport and scheduling are tuned within an agreed effort budget.

