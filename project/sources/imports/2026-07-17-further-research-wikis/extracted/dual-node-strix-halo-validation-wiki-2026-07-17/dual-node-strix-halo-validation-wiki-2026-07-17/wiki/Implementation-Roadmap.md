# Evidence-Driven Implementation Roadmap

> **Wiki status:** Proposed · **Evidence state:** D0 — design only · **Last reviewed:** 2026-07-17  
> **Machine-validation status:** Not run. Missing measurements are `INSUFFICIENT_EVIDENCE`, never an implicit pass.


## Phase 0 — Freeze and ownership

**Deliverables:** populated SUT file, owners, model/data licenses, release profile, absolute SLOs, risk boundary, current source snapshot.  
**Exit evidence:** reviewed manifest template; no required `null`; G0 only.  
**Do not claim:** any machine behavior.

## Phase 1 — Instrumentation and matched single-node controls

Implement request/token timestamps, node telemetry, block/link counters, model hashes, and collector-overhead controls. Run A-only and B-only C0–C3, prefill/decode, correctness, long-context subset, power/thermal, and 4-hour soak.

**Exit:** M1 baselines reproducible, collector overhead within limits, node asymmetry understood.

## Phase 2 — USB4 and dual functional integration

Characterize USB4 in both directions and bidirectionally; freeze MTU/offloads; bring up worker/coordinator; prove model/device placement, API correctness, and explicit startup failure behavior.

**Exit:** EXP-002 and dual correctness pass; no performance claim yet.

## Phase 3 — Performance matrix and partition tuning

Run cache states, prefill/decode, TTFT/ITL, concurrency saturation, long-context, disk/link utilization, resource balance, and energy. Tune tensor/layer split, batch/ubatch, context/KV, threads/affinity, and transport only one factor at a time.

**Exit:** candidate configuration meets absolute SLOs and the selected scale-out or capacity-extension gate.

## Phase 4 — Resilience and recovery

Execute software network faults, process failures, model/storage failures, overload/cancel behavior, then physical cable and node reboot scenarios. Automate health state and restart where required.

**Exit:** mandatory fault matrix passes three of three with no silent corruption and bounded recovery.

## Phase 5 — Release candidate and stable proof

Run 24-hour RC soak, remediate, then an independent 72-hour stable soak with mixed workload and upstream freshness checks. Reproduce key performance/correctness blocks on a different day.

**Exit:** signed G4 decision and immutable evidence packet.

## Phase 6 — Continuous maintenance

Deploy upstream watcher, weekly digest, monthly baseline refresh, change-triggered canaries, quarterly fault rehearsal, and immediate security triage. Baseline promotion follows stable acceptance; old baselines remain immutable.

## Dependency-critical path

`SUT/SLO freeze → collectors → single-node baselines → USB4 reference → dual correctness → performance tuning → faults → 72h reproduction → stable decision`.

Skipping a step produces `INSUFFICIENT_EVIDENCE`, not schedule acceleration.
