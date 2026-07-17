---
section_id: "22"
title: "Power and thermal facts and constraints"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: []
  software_versions: ["Linux 7.2 documentation"]
  hardware_revisions: ["AMD Ryzen AI Max+ 395 premise"]
related_sections: ["17", "18", "21", "23"]
---

# Power and thermal facts and constraints

| ID | Claim | Boundary |
|---|---|---|
| 22-F01 | **[VERIFIED]** Ryzen AI Max+ 395: 16 cores/32 threads, 3.0 GHz base, up to 5.1 GHz boost, 55 W default TDP, 45-120 W cTDP, 2.9 GHz graphics frequency, 100 C Tjmax ([S22-01]). | Product specification; does not prove exact node SKU, firmware selection, or sustained behavior. |
| 22-F02 | **[VERIFIED]** amdgpu hwmon may expose GPU temperature, APU-inclusive SoC power, fan, gfx clock, and memory clock; files are capability-dependent ([S22-02]). | Discover labels/units live; absent files are not zero values. |
| 22-F03 | **[VERIFIED]** On APUs, amdgpu `power1_average`/`power1_input` includes CPU power ([S22-02]). | Do not add it to an overlapping CPU/package reading as though domains were disjoint. |
| 22-F04 | **[VERIFIED]** `amd-pstate` uses CPPC controls and exposes active/passive/guided modes plus policy interfaces; actual frequency selection remains workload and platform dependent ([S22-03]). | Record driver, mode, governor, EPP, boost, min/max, and effective frequency. |
| 22-F05 | **[VERIFIED]** turbostat reports processor topology, frequency, idle, temperature, and power telemetry supported by the platform ([S22-04]). | Counter availability and domains vary; it is not a wall meter. |
| 22-F06 | **[VERIFIED]** AMD SMI/ROCm SMI APIs expose supported temperature, power, clock, utilization, and version queries ([S22-05]). | Tool support on gfx1151 must be confirmed; query failure is evidence, not permission to invent. |
| 22-F07 | **[INFERENCE]** Shared APU power and memory bandwidth mean CPU-side transport/cache work can reduce GPU clock or throughput even when reported GPU utilization is high. | Requires simultaneous CPU/GPU/memory/throughput evidence. |

## Required machine state

| Dimension | Node A | Node B |
|---|---|---|
| Exact system/chassis/cooler/PSU | **[OPEN]** | **[OPEN]** |
| BIOS version and named power mode | **[OPEN]** | **[OPEN]** |
| Configured and observed package limit | **[OPEN]** | **[OPEN]** |
| CPU driver/mode/governor/EPP/boost | **[OPEN]** | **[OPEN]** |
| GPU performance profile and clocks | **[OPEN]** | **[OPEN]** |
| Memory controller/data rate telemetry | **[OPEN]** | **[OPEN]** |
| Sensor labels, units, critical limits | **[OPEN]** | **[OPEN]** |
| Fan count/control/curve and RPM | **[OPEN]** | **[OPEN]** |
| Airflow orientation and clearance | **[OPEN]** | **[OPEN]** |
| Idle/steady wall power and ambient | **[OPEN]** | **[OPEN]** |

## Measurement definitions

- **Burst:** first 60 seconds after workload start, reported separately.
- **Warm-up:** until temperature and throughput cease systematic drift; minimum 10 minutes proposed.
- **Steady window:** final 20 minutes of a 30-minute test, or final 30 minutes of a 60-minute test, only if slope criteria pass.
- **Node energy:** wall-meter Wh over the measured interval, minus nothing unless a separately measured subtraction method is declared.
- **Efficiency:** useful output divided by wall energy: prompt/decode tokens per joule and completed request work per Wh. Report latency/SLO at the same time.
- **Two-node efficiency:** total useful work divided by sum of both nodes' wall energy; include fabric and coordinator overhead.

**[RECOMMENDATION]** Record ambient at each intake and map live hwmon labels/units before analysis ([S22-06]). Chassis temperature without ambient and airflow context is not comparable.

[S22-01]: sources.md#s22-01
[S22-02]: sources.md#s22-02
[S22-03]: sources.md#s22-03
[S22-04]: sources.md#s22-04
[S22-05]: sources.md#s22-05
[S22-06]: sources.md#s22-06
