---
section_id: "79"
title: "Stress, Soak, Long-Context, Multi-Session, Power, and Thermal Procedures and Checks"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: ["ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689", "charlie12345/ROCmFPX@a5605a72768c6562241b248e268e33dc92787394"]
  software_versions: ["AMD SMI CLI documentation 26.5.0", "Linux documentation accessed 2026-07-17"]
  hardware_revisions: ["dual-Strix-Halo target; exact revisions open"]
related_sections: ["73", "74", "75", "76", "77", "78", "80", "81"]
---

# Procedures and checks

The durations below are proposed starting points, not evidence-backed release requirements.

## Preflight

1. Pin runtime/model/dataset/workload hashes, topology, device map, link configuration, power profile, firmware, kernel, drivers, cooling state, and ambient sensor.
2. Confirm correctness smoke tests from Section 78, telemetry availability, synchronized clocks, log rotation, free space, and emergency stop.
3. Capture idle telemetry, then a warm-up excluded from scoring. Record monitoring overhead with and without collectors.
4. Preallocate only an approved scratch file for storage contention. Never direct `fio` or another writer at a production block device.

## Proposed phases

| Phase | Initial duration | Checks |
|---|---:|---|
| Preflight/warm-up | 30 minutes | correctness, telemetry, stable request rate, no log loss |
| Bottleneck stress | 6 hours per workload/profile | thermal equilibrium, throttling, queue/cache/link/storage behavior |
| Representative soak | 24 hours per release topology/profile | resource slopes, errors, fairness, drift, evidence completeness |
| Long-context staircase | per context point | retrieval/assertion accuracy, memory, prefill/decode latency, context shifts |
| Multi-session/cancel | fixed request trace | admission fairness, cleanup latency, leaked slots/state |
| Model switching/cache churn | fixed cycles | unload/reload correctness, fragmentation, cache integrity/endurance counters |

Extend duration only when justified by the failure timescale or release policy; preserve all shorter-run evidence.

## Workload execution

- Use `llama-bench`/`batched-bench` for controlled prefill/decode shapes and the server k6 or speed-bench harness for request-level concurrency.
- Replay a versioned request trace for representative traffic. Record randomized run order across candidates and power profiles.
- At each long-context point, embed multiple unique retrieval anchors across early/middle/late positions, validate exact expected facts, and include distractors. Successful generation without retrieval is a failure.
- Increase context geometrically, then refine around the first correctness, allocation, latency, or stability boundary. Do not label the configured context size as the validated maximum.
- Mix interactive short requests with batch long requests. Track every request from admission through completion/cancellation and resource reclamation.
- For NVMe contention, use a bounded preallocated file inside a dedicated scratch filesystem, explicit runtime and rate, and read-only verification afterward. Abort before free-space or endurance safety margins are crossed.

## Evidence and acceptance

Preserve raw request records, telemetry time series, server/kernel/driver logs, workload generator logs, cache/link/storage counters, validation results, and an event timeline. Plot warm-up and steady-state separately.

[RECOMMENDATION] A run is invalid if telemetry gaps, clock discontinuity, uncontrolled workload changes, ambient/cooling changes, or collector overload prevent attribution. A candidate fails on any correctness error or unbounded resource growth. Performance and thermal gates remain [OPEN] until matched baseline distributions and hardware limits are approved.
