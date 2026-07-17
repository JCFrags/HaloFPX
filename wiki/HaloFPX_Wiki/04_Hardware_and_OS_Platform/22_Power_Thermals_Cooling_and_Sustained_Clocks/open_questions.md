---
section_id: "22"
title: "Power and thermal open questions"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: []
  software_versions: []
  hardware_revisions: ["two project Strix Halo nodes"]
related_sections: ["18", "21", "23", "73", "74", "79"]
---

# Power and thermal open questions

| ID | Open question | Closure evidence |
|---|---|---|
| 22-OQ01 | **[OPEN]** Are both nodes exactly the stated SKU and matched chassis/cooling/PSU revisions? | Section 18 paired BOM |
| 22-OQ02 | **[OPEN]** Which BIOS power/fan modes exist, and what limits do they actually set? | Screens/config plus sustained telemetry |
| 22-OQ03 | **[OPEN]** Which CPU/GPU/memory/fan/power sensors are exposed and what domains overlap? | Path-label-unit inventory and tool comparison |
| 22-OQ04 | **[OPEN]** What are steady CPU/GPU/memory clocks, temperatures, fan RPM, and wall power in each profile? | 30-60 minute raw traces |
| 22-OQ05 | **[OPEN]** At what ambient/intake conditions does throughput or latency drift? | Repeated controlled-ambient observations |
| 22-OQ06 | **[OPEN]** Does NVMe temperature or writeback become the limiting component? | Mixed-load storage and telemetry trace |
| 22-OQ07 | **[OPEN]** What profile maximizes tokens/J while satisfying latency and acoustic constraints? | Matched A/B matrix and declared SLO |
| 22-OQ08 | **[OPEN]** Is two-node execution more efficient than single-node for each target workload? | Sum-of-wall-energy comparison with equal work |
| 22-OQ09 | **[OPEN]** What thermal-degradation signal and fallback should the runtime use? | fault/policy experiment and distributed-mode decision |

No power mode, fan curve, or sustained-clock target can close before OQ01-OQ08.
