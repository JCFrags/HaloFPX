---
section_id: "22"
title: "Power, Thermals, Cooling, and Sustained Clocks"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: []
  software_versions: ["Linux 7.2 documentation"]
  hardware_revisions: ["AMD Ryzen AI Max+ 395 premise; exact systems open"]
related_sections: ["17", "18", "21", "23", "73", "74", "79"]
---

# 22 - Power, Thermals, Cooling, and Sustained Clocks

## High-value conclusion

**[VERIFIED]** AMD specifies the Ryzen AI Max+ 395 at 55 W default TDP, 45-120 W cTDP, up to 5.1 GHz CPU boost, 2.9 GHz graphics frequency, and 100 C Tjmax ([S22-01]). These are product limits/marketing specifications, not sustained clocks or wall-power measurements for either project node.

**[OPEN]** Exact chassis, cooling, firmware power modes, fan controls, sensor exposure, ambient conditions, steady clocks, throttling behavior, and node/wall power have not been measured. No performance-per-watt setting is promoted yet.

## Authoritative pages

- [Facts and constraints](facts_and_constraints.md)
- [Design implications](design_implications.md)
- [Procedures and checks](procedures_and_checks.md)
- [Open questions](open_questions.md)
- [Sources](sources.md)

## Research split

1. **Internet research complete:** silicon envelope and Linux power/thermal telemetry interfaces.
2. **Machine work required:** OEM modes, cooling, sensors, ambient, clocks, temperatures, wall energy, and throttling.
3. **Contingent decisions:** operating power profile, fan curve, placement/airflow, concurrency, and two-node scheduling.

[S22-01]: sources.md#s22-01
