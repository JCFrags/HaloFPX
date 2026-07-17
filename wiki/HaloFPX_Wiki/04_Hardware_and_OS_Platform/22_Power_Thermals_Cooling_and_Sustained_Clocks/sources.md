---
section_id: "22"
title: "Power and thermal sources"
status: "draft"
last_verified: "2026-07-16"
applies_to:
  repositories: []
  software_versions: []
  hardware_revisions: []
related_sections: ["17", "18", "21", "23"]
---

# Power and thermal sources

## S22-01

- **Title/publisher:** AMD Ryzen AI Max+ 395 product specifications / AMD
- **URL:** https://www.amd.com/en/products/processors/laptop/ryzen/ai-300-series/amd-ryzen-ai-max-plus-395.html
- **Revision/date/access:** page current at 2026-07-16
- **Supports:** core count, clocks, default/cTDP range, graphics frequency, Tjmax.
- **Limitations:** Product envelope, not proof of node identity, OEM limits, or sustained results.

## S22-02

- **Title/publisher:** GPU Power/Thermal Controls and Monitoring / Linux kernel documentation
- **URL:** https://docs.kernel.org/gpu/amdgpu/thermal.html
- **Revision/date/access:** Linux 7.2 documentation; accessed 2026-07-16
- **Supports:** amdgpu hwmon temperatures, APU-inclusive power, clocks, fan and power interfaces.
- **Limitations:** Actual attributes depend on ASIC/platform/driver; write interfaces are not recommendations.

## S22-03

- **Title/publisher:** amd-pstate CPU Performance Scaling Driver / Linux kernel documentation
- **URL:** https://docs.kernel.org/admin-guide/pm/amd-pstate.html
- **Revision/date/access:** Linux 7.2 documentation; accessed 2026-07-16
- **Supports:** CPPC-based driver modes, policy and EPP behavior.
- **Limitations:** Does not prove active mode or optimal project setting.

## S22-04

- **Title/repository:** `turbostat(8)` / Linux stable tree
- **URL:** https://kernel.googlesource.com/pub/scm/linux/kernel/git/stable/linux-stable/+/8d09617b076fd03ee9ae124abce94dda17bf3723/tools/power/x86/turbostat/turbostat.8
- **Revision/date/access:** commit `8d09617b076fd03ee9ae124abce94dda17bf3723`; accessed 2026-07-16
- **Supports:** supported processor frequency, idle, temperature, and power telemetry.
- **Limitations:** Capability and domain dependent; not a wall-power meter.

## S22-05

- **Title/publisher:** AMD SMI documentation / AMD ROCm
- **URL:** https://rocm.docs.amd.com/projects/amdsmi/en/latest/
- **Revision/date/access:** latest docs accessed 2026-07-16
- **Supports:** supported metric, power, temperature, clock, utilization and version queries.
- **Limitations:** Live gfx1151/tool support must be tested; tool version must be recorded.

## S22-06

- **Title/publisher:** Hardware monitoring sysfs interface / Linux kernel documentation
- **URL:** https://docs.kernel.org/hwmon/sysfs-interface.html
- **Revision/date/access:** Linux 7.2 documentation; accessed 2026-07-16
- **Supports:** hwmon naming, units, labels, and optional attributes.
- **Limitations:** Does not identify project sensors.
