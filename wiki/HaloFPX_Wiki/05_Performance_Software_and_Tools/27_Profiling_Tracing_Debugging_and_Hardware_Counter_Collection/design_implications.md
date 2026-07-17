---
section_id: "27"
title: "Profiling design implications"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["charlie12345/ROCmFPX", "ggml-org/llama.cpp"]
  software_versions: ["ROCprofiler-SDK 1.3.2 documentation", "Linux tracing interfaces"]
  hardware_revisions: ["gfx1151 Strix Halo", "dual USB4 host links"]
related_sections: ["24", "25", "26", "28"]
---

# Profiling design implications

## Correlation contract

**[RECOMMENDATION]** Every request, token step, rank transfer, cache lookup, device dispatch and storage operation should carry or map to:

```text
experiment_id, host_id, process/thread, rank, request_id,
token_step, link_id, monotonic timestamp, clock_id, build_id
```

Instrument user-space phases with ROCTx and an equivalent CPU trace marker. **[INFERENCE]** Stable IDs are more reliable than kernel/function-name matching and allow rank-local traces to remain useful when full clock alignment is unavailable.

## Capture ladder

1. **[RECOMMENDATION]** Start with low-overhead application timestamps, system telemetry and request outcomes.
2. Add `perf stat`, scheduler/block/network counters and rocprof runtime traces around a bounded reproduction.
3. Add sampling, KFD/page traces or eBPF to answer one hypothesis.
4. Use GPU hardware counters in validated groups and repeatable passes.
5. Use function tracing, crash dumps or full packet capture only when justified by a narrower fault.

**[RECOMMENDATION]** Run the same workload without profiling before and after each capture and report overhead. Never publish profiled latency as baseline latency.

## Distributed attribution

```mermaid
flowchart LR
  A["Rank 0 request marker"] --> B["Rank 0 GPU work"]
  B --> C["Link 0/1 send"]
  C --> D["Rank 1 receive"]
  D --> E["Rank 1 GPU work"]
  E --> F["Reply and token marker"]
  G["Clock offset/error model"] -. qualifies .-> C
  G -. qualifies .-> D
```

**[RECOMMENDATION]** Record both links independently. Aggregate bandwidth hides imbalance, retransmission, IRQ placement and head-of-line blocking. State rank ownership and preserve single-node traces as the fallback comparator.

## Evidence retention

- Preserve raw profiler configuration, tool output, environment/build manifest and dropped-event counters.
- Convert to Perfetto or summary tables as derived artifacts, linking back to raw inputs.
- Restrict cores and packet captures because prompts, tokens, credentials or model data can appear in them.
- Version metric formulas; do not silently reinterpret old counter names after a ROCm upgrade.

