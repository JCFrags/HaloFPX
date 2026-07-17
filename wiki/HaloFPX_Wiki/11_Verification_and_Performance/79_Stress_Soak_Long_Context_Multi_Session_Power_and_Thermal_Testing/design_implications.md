---
section_id: "79"
title: "Stress, Soak, Long-Context, Multi-Session, Power, and Thermal Design Implications"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: ["ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689", "charlie12345/ROCmFPX@a5605a72768c6562241b248e268e33dc92787394"]
  software_versions: ["AMD SMI CLI documentation 26.5.0", "Linux documentation accessed 2026-07-17"]
  hardware_revisions: ["dual-Strix-Halo target; exact revisions open"]
related_sections: ["73", "74", "75", "76", "77", "78", "80", "81"]
---

# Design implications

## Workload families

| Family | Purpose | Required variation |
|---|---|---|
| Thermal stress | Reach stable high utilization and reveal throttling | prefill-heavy, decode-heavy, mixed; each approved power profile |
| Representative soak | Detect drift and leaks | recorded prompt/output mix, arrivals, session lengths, idle gaps |
| Long context | Find allocation, retrieval, and context-shift boundaries | geometric context staircase through the declared maximum |
| Multi-session | Measure fairness and isolation | short/long prompts, interactive/batch, heterogeneous output lengths |
| Cancellation storm | Exercise queue/slot/state cleanup | cancel before admission, during prefill, during decode, after completion race |
| Cache churn | Exercise admission, eviction, restore, and writeback | working set below, near, and above cache capacity |
| Model switching | Expose unload/reload and fragmentation defects | pinned model sequence and dwell time; cold and warm paths |
| Storage/link disturbance | Measure bounded degradation | scratch-file NVMe load and approved link jitter/retrain scenarios |

## Telemetry contract

[RECOMMENDATION] Collect per-request timestamps, prompt/generated tokens, status, cancellation phase, slot/session identity, TTFT, inter-token latency, throughput, and validation result. At approximately one-second cadence, collect process RSS, open descriptors, threads, accelerator memory, cache occupancy and writeback, queue depth, CPU/GPU utilization, clocks, power, temperatures, throttling flags, NVMe latency/temperature/error counters, and transport state/errors/retransmits where available. The exact cadence must be benchmarked so monitoring overhead is known.

## Drift and equilibrium

[RECOMMENDATION] Declare warm-up before the run. Define thermal equilibrium using a predeclared rolling-window temperature/power slope plus stable clocks, not visual inspection. Evaluate resource leakage only after equilibrium and request-rate stabilization; use a slope with uncertainty and retained time series. “Final minus initial” alone confounds warm-up and cache population.

[RECOMMENDATION] Release acceptance requires zero crashes, hangs, correctness mismatches, silent request loss, invalid cache acceptance, or unexplained resets. Latency, throughput, fairness, temperature, power, memory, and storage drift limits must be calibrated from Section 73 baselines and exact hardware limits before becoming gates.

## Fairness

Report per-session service share, wait time, TTFT and token rate distributions, starvation count, and the slowest/median ratio. An aggregate throughput increase is not a pass if interactive sessions starve. Cancellation must reclaim queue, slot, KV/cache, and transport resources within a measured bounded time.
