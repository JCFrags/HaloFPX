# Release Gates

> **Wiki status:** Proposed · **Evidence state:** D0 — design only · **Last reviewed:** 2026-07-17  
> **Machine-validation status:** Not run. Missing measurements are `INSUFFICIENT_EVIDENCE`, never an implicit pass.


The machine-readable source of truth is [config/release-gates.yaml](../config/release-gates.yaml). This page explains the staged decision.

## Gate stages

| Stage | Required evidence | Authorized outcome |
|---|---|---|
| G0 Design ready | Reviewed wiki, cards, schemas, SUT template, owners | `DESIGN_COMPLETE` only |
| G1 Lab ready | Instrumentation overhead checked; SUT frozen; safety and provenance preflight | May begin baseline measurement |
| G2 Integration exit | A-only, B-only, USB4 reference, dual functional/correctness, latency decomposition | `INTEGRATION_EXIT_PASS` |
| G3 Release candidate | Full matrix, regression comparison, long context, faults, 24-hour soak | `RC_PASS` |
| G4 Stable | Independent reproduction, 72-hour soak, upstream freshness, signed decision | `STABLE` for named profile only |

## Mandatory stable gates

- Evidence class is machine (`M2`/`R1`), not synthetic.
- All required experiment IDs are present, with no unresolved invalid/incomplete records.
- Provenance completeness is 100%; raw hashes verify.
- Unexpected crashes, hangs, kernel oopses, GPU resets, silent corruption, thermal throttles, and normal-run link renegotiations are zero.
- Request success ≥99.99%, and any observed unexpected failure is adjudicated.
- Deterministic/protocol critical correctness is 100%; task quality remains inside the declared drift threshold.
- Absolute SLOs are approved under [SLO Definition and Approval](SLO-Definition-and-Approval.md), populated, and met.
- Relative regressions stay within [config/regression-thresholds.yaml](../config/regression-thresholds.yaml).
- Stable soak reaches 72 hours with no raw-evidence gap >60 seconds.
- Every mandatory scenario ID in [config/fault-matrix.yaml](../config/fault-matrix.yaml) passes three of three and recovers inside bounds; a category-level summary is not sufficient.
- Upstream critical/security sources are fresh; no untriaged blocker remains.

## Profile-specific gates

### Scale-out

Default stable target: SLO-qualified output goodput ≥1.15× `best_single`, p95 TTFT/ITL no worse than 1.10×, and p99 no worse than 1.15×. A project may set stricter SLOs. Failing the scaling target may be reclassified as capacity extension only through a new release decision; results cannot be relabeled after publication.

### Capacity extension

The large workload must exceed a documented single-node safe-capacity boundary, meet absolute SLOs, retain required memory headroom, and pass a smaller matched control across A/B/dual. No minimum dual/single speedup applies to the large workload because no valid single-node denominator exists.

## Decision states

- `PASS`: all mandatory gates satisfied.
- `PASS_WITH_WAIVER`: only a waivable noncritical gate, with owner, expiry, rollback, and bounded claim.
- `FAIL`: measured violation of a mandatory gate.
- `INSUFFICIENT_EVIDENCE`: missing, synthetic, stale, unverifiable, or unmatched evidence.

Correctness corruption, security-boundary violation, missing provenance, crash/reset/oops, and absent baselines are not waivable for stable release.
