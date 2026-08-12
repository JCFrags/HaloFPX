---
section_id: "75"
title: "Fabric Benchmark Open Questions"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: []
  software_versions: []
  hardware_revisions: ["planned dual Strix Halo nodes; exact revisions open"]
related_sections: ["20", "49", "50", "51", "52", "54", "55", "73", "76", "80"]
---

# Open Questions

| ID | Question | Required evidence | Decision unblocked |
|---|---|---|---|
| O75-01 | Are the two USB4 ports backed by independent controller, tunnel, memory, and IRQ resources? | topology, counters, simultaneous saturation | credible dual-link scaling |
| O75-02 | Which exact interface/transport represents each cable? | route/bind/counter proof | per-link attribution |
| O75-03 | Does either path support reliable hardware TX/RX timestamps? | `ethtool -T`, PTP inventory, calibration | admissible one-way delay |
| O75-04 | What are MTU, offload, coalescing, queue, socket-buffer, and congestion-control baselines? | immutable host manifests | matched comparisons |
| O75-05 | Can the transport DMA to/from GPU-visible memory, or is host staging mandatory? | buffer trace and validation | copy-path architecture |
| O75-06 | Which synchronization operation makes producer bytes visible and consumer results final? | source audit plus HIP event/visibility tests | correct E2E timing |
| O75-07 | Where are latency and bandwidth knee points by payload and queue depth? | raw sweeps with intervals | chunk and queue policy |
| O75-08 | Does A+B improve application goodput without worsening p99 or CPU cost? | matched A/B/A+B runs | striping versus affinity |
| O75-09 | How much do decode and NVMe traffic degrade the fabric, and vice versa? | counterbalanced contention matrix | resource isolation |
| O75-10 | What happens on loss, corruption, timeout, reorder, and single-link failure? | Section 80 fault evidence | recovery and fallback |
| O75-11 | What workload-shaped tensor/message sizes must Section 76 supply? | distributed plan traces | representative E2E matrix |
| O75-12 | Is a kernel patch necessary and maintainable? | bottleneck attribution and matched patch/no-patch tests | Section 55 decision |

## Internet follow-up

**[OPEN]** Freeze the actual kernel and ROCm versions, then replace moving documentation links with their matching versioned manuals and inspect the exact transport driver source/commit.

**[OPEN]** Audit whether the selected transport exposes per-queue counters, timestamps, completion semantics, retransmission, integrity checks, and path binding. Record absence explicitly.

## Machine follow-up

**[OPEN]** Execute the procedures on both directions and both cable/port permutations. Cable labels alone do not establish controller independence.

**[OPEN]** Validate the consumer result rather than accepting sender completion. Any stale or corrupt payload is a correctness failure, not an outlier to discard.

## Contingent choices

Transport selection, multipath policy, direct versus staged buffers, chunk size, queue depth, affinity, patching, and release thresholds remain contingent. No winner is selected in this planning section.
