---
section_id: "09"
title: "SLO Definitions and Candidate Targets"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["HaloFPX project"]
  software_versions: []
  hardware_revisions: ["actual two-node deployment"]
related_sections: ["07", "38", "46", "48", "60", "69", "78", "80"]
---

# SLO definitions and candidate targets

## Measurement rules

- `TTFT`: client send completion to first response token/stream event, including queue and transport.
- `ITL`: time between consecutive generated token events; report p50/p95/p99 per request and pooled.
- `throughput`: successful generated tokens divided by wall-clock test interval; report prompt throughput separately.
- `startup`: service start to readiness for the declared loaded model/profile.
- `cache restore`: restore start to compatible state ready for suffix evaluation; report restored tokens/bytes and validation cost.
- `availability`: eligible successful requests / eligible requests during the service window; exclude only predeclared maintenance.
- `degradation`: time from injected fault to correct degraded readiness, plus performance relative to the matched pre-fault or best single-node baseline.

All latency percentiles are calculated per workload envelope and cold/warm state; do not mix model or concurrency classes.

## Candidate service objectives

These are **[RECOMMENDATION]** values for initial engineering gates, not measurements. `B` is the matched best single-node baseline and `C` is cold behavior for the same request.

| ID | Objective | Candidate target | Acceptance window |
|---|---|---|---|
| SLO-TTFT-01 | Two-node interactive TTFT | p95 `<= 0.90 * B`, or document capacity-only rationale | >=100 successful requests per ratified W-INT/W-AGENT cell |
| SLO-ITL-01 | Interactive inter-token latency | p95 `<= 1.10 * B`; no p99 stall above ratified timeout | Same run, client-side timestamps |
| SLO-TP-01 | Independent-request throughput | `>= 1.50 * B` for replication under ratified concurrency | >=30 min steady state after warmup |
| SLO-TP-02 | Split-mode value | `>= 1.20 * B` throughput or capacity gate unavailable on one node | Each admitted distributed profile |
| SLO-START-01 | Startup | p95 `<= 1.10 * B` for same model; readiness must be truthful | 10 cold starts per node/profile |
| SLO-CACHE-01 | Compatible persistent restore | p95 TTFT `<= 0.25 * C` and `>=95%` expected prefix restored | >=30 restart restores per workload |
| SLO-CACHE-02 | Invalid-state safety | zero accepted corrupted/incompatible objects | Full mutation/fault matrix |
| SLO-AVL-01 | Request availability | `>=99.0%` eligible success in initial 24-hour soak | Ratified workload mix; maintenance excluded |
| SLO-DEG-01 | Fault detection/degraded readiness | p95 `<=30 s`; zero incorrect successes | >=20 injections per fault class |
| SLO-DEG-02 | Single-node fallback | where model fits, p95 latency `<=1.25 * B` within `<=120 s` | Node/link-loss scenarios |
| SLO-QUAL-01 | Task quality | no ratified task metric regresses beyond its predeclared tolerance | Blind matched corpus |

**[OPEN]** The sponsor must accept, change, or reject every candidate before these become release commitments. Availability needs a longer window for mature releases.

## Error budget and priority

**[RECOMMENDATION]** Correctness, invalid-cache acceptance, unauthorized access, and silent partial distributed output have a zero-tolerance release gate. Performance misses may consume an agreed error budget; safety failures may not be traded for speed.

