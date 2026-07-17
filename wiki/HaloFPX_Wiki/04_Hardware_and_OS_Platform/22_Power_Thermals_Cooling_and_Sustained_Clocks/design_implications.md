---
section_id: "22"
title: "Power and thermal design implications"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["HaloFPX"]
  software_versions: []
  hardware_revisions: ["two project Strix Halo nodes"]
related_sections: ["21", "23", "49", "73", "74", "79"]
---

# Power and thermal design implications

## Operating policy

- **[RECOMMENDATION]** Define named, reproducible profiles (`quiet`, `balanced`, `sustained`, `qualification`) as complete tuples: BIOS mode, OS CPU policy, GPU profile, fan policy, ambient range, and software versions.
- **[RECOMMENDATION]** Select the production profile from steady-state throughput, tail latency, errors, acoustic/thermal limits, and tokens/J - not peak clock or first-minute throughput.
- **[RECOMMENDATION]** Keep defaults until a reversible, measured profile wins. Never apply overdrive/voltage/fan writes as an inventory step.
- **[INFERENCE]** The maximum cTDP may be inferior if cooling saturates, clocks oscillate, NVMe throttles, or extra CPU/fabric power reduces GPU efficiency.

## Distributed runtime

- **[RECOMMENDATION]** Report rank-local clocks, temperature, power, and throughput. Aggregate throughput can conceal one throttled rank and synchronization stalls.
- **[INFERENCE]** Tensor/pipeline execution is limited by the slower steady rank; unequal cooling or power configuration can create bubbles even on nominally matched hardware.
- **[RECOMMENDATION]** Define fallback behavior: on thermal limit, reduce concurrency/power or fall back to a supported single-node mode rather than silently accepting unbounded latency.
- **[OPEN]** Whether two nodes at moderate power outperform one node at high power per joule and per latency SLO requires matched model/context tests.

## Cooling and storage interaction

- **[RECOMMENDATION]** Log NVMe temperature and storage latency with SoC telemetry. Shared chassis airflow makes storage throttling a possible late-test bottleneck.
- **[RECOMMENDATION]** Preserve chassis placement, orientation, clearance, intake/exhaust temperature, dust state, and fan firmware in the experiment record.

## Promotion gate

A profile can become a production recommendation only when both nodes pass:

1. 60-minute representative inference at thermal equilibrium;
2. mixed decode + fabric + cache writeback;
3. no thermal/clock oscillation that violates latency SLO;
4. repeatability across at least three runs and two ambient observations;
5. sensor-vs-wall-energy accounting with overlap stated;
6. clean fallback and restart behavior.
