# EXP-017 — Process and Node Fault Injection

> **Wiki status:** Proposed · **Evidence state:** D0 — design only · **Last reviewed:** 2026-07-17  
> **Machine-validation status:** Not run. Missing measurements are `INSUFFICIENT_EVIDENCE`, never an implicit pass.


| Field | Value |
|---|---|
| Objective | Validate coordinator/worker behavior when runtime processes terminate abruptly or a worker node reboots. |
| Release profiles | Availability recovery; mandatory stable |
| Required evidence | M2 fault evidence, three repeats/scenario |
| Estimated measured duration | 4–8 hours |
| Risk class | High—process termination/reboot |

## Decision question

Are process/node failures detected and surfaced without stale state, corruption, or unbounded client impact?

## Hypotheses

- **H0:** A process/node failure hangs the pair, silently retries, corrupts output, or requires undocumented manual repair.
- **H1:** Failures are explicit and recovery/restart follows the declared policy within bounds.

## Preconditions and provenance

- Fault-free controls pass; service manager/restart policy, request acknowledgement semantics, client retry policy, and health checks are documented.
- Out-of-band access and log persistence survive worker reboot.

## Factors, controls, and run order

- Worker SIGTERM, worker SIGKILL, coordinator SIGKILL, controlled worker reboot; idle/prefill/decode phases per matrix.
- Three repeats per mandatory scenario; deterministic request IDs prevent double counting.

## Procedure

1. Run control and verify process IDs, cgroups, worker registration, and critical canary.
2. Inject the exact signal/reboot at a scheduled monotonic time during the declared phase.
3. Observe request outcomes, coordinator state, worker disappearance, restart attempts, memory/ports/temp files, and client retry behavior.
4. Restore according to the declared policy; confirm fresh process identity and worker capability handshake.
5. Run post-recovery correctness, latency, cache-state, and resource-leak checks.

## Required measurements

- Detection/restart/rejoin time, failed/lost/duplicated requests, retry count, stale process/socket/cache artifacts, output correctness, post-recovery performance, and log completeness.

All run-level data validates against [the raw-data schemas](../schemas/README.md). Use the canonical definitions in [Metric Definitions](../wiki/Metric-Definitions.md), the matrix in [`config/benchmark-matrix.yaml`](../config/benchmark-matrix.yaml), and immutable run manifests.

## Acceptance and regression rules

- All mandatory scenarios pass 3/3; silent corruption, duplicate acknowledged completion, and lost acknowledged requests =0.
- No indefinite coordinator wait or stale worker marked healthy.
- Detection/recovery meet approved SLO; post-recovery critical correctness =100%.
- Ports, memory, processes, and temp/cache artifacts return to the declared steady state without manual cleanup unless manual recovery is explicitly the supported policy.
- Any auto-retry is observable and preserves idempotency semantics.

A missing measurement, unmatched control, invalid cache state, unverifiable hash, or synthetic record yields `INSUFFICIENT_EVIDENCE`; it is not an implicit pass.

## Invalidation and safety-abort rules

- Signal targets the wrong PID/cgroup.
- Service manager policy differs from production.
- Reboot logs are lost or boot identity is not captured.

Safety aborts remain measured events and retain all evidence. Do not exclude an unfavorable but valid run.

## Outputs

- `manifest.json`, `requests.jsonl`, `tokens.jsonl`, telemetry/log channels, and applicable correctness/fault records.
- Derived per-cell statistics with calculation code commit and paired baseline IDs.
- One experiment outcome: `PASS`, `WARN_RETEST`, `FAIL`, or `INSUFFICIENT_EVIDENCE`.

## Interpretation limits

Passing a controlled reboot does not prove survival of hardware data corruption or power-quality events.

## Research basis

[[SRC-007]](../references/Sources.md#src-007)
