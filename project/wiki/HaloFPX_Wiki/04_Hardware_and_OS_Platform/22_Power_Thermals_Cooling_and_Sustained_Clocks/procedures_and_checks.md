---
section_id: "22"
title: "Power and thermal procedures and checks"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: []
  software_versions: ["Linux", "lm-sensors", "turbostat", "AMD SMI or ROCm SMI"]
  hardware_revisions: ["two project Strix Halo nodes"]
related_sections: ["18", "21", "23", "73", "74", "79"]
---

# Power and thermal procedures and checks

## 1. Preserve configuration before load

Read-only unless noted; root may be required for `dmidecode`, `turbostat`, and debug/sysfs files.

```bash
date --iso-8601=seconds
uname -a
cat /etc/os-release
sudo dmidecode --type bios --type system --type baseboard --type chassis
cat /sys/devices/system/cpu/amd_pstate/status 2>/dev/null
grep . /sys/devices/system/cpu/cpufreq/policy0/{scaling_driver,scaling_governor,energy_performance_preference,scaling_min_freq,scaling_max_freq,cpuinfo_max_freq} 2>/dev/null
find /sys/class/drm/card*/device/hwmon -type f -o -type l 2>/dev/null
sensors -j
amd-smi static --json 2>/dev/null || true
amd-smi metric --json 2>/dev/null || rocm-smi --showallinfo --json 2>/dev/null || true
sudo turbostat --quiet --num_iterations 1 --interval 1
```

Also photograph/record physical orientation, cooler/fan/vent geometry, clearance, PSU label, room and intake ambient, and BIOS power/fan screens. Link the exact OEM manual/BIOS release after the system model is known.

## 2. Telemetry mapping

Map every sensor path to label and units before sampling. Capture at 1 s cadence where supported:

- monotonic and wall-clock timestamps;
- CPU effective/busy frequency, package temperature/power;
- GPU gfx clock, utilization, temperature, APU-inclusive SoC power;
- memory clock if exposed;
- fan RPM/PWM and NVMe temperature;
- wall watts/Wh from a logged meter;
- workload phase, prompt/decode throughput and latency;
- kernel thermal, amdgpu, reset, and hardware-error messages.

**[RECOMMENDATION]** Prefer raw sysfs/JSON alongside any dashboard. Record missing/unsupported fields explicitly.

## 3. 30-60 minute matrix

Use one pinned model artifact, context, batch/concurrency, seed/input corpus, and build commit.

| Run | Duration | Workload |
|---|---:|---|
| idle | 10 min | service stopped or quiescent |
| CPU control | 30 min | repeatable CPU load, no GPU inference |
| GPU inference | 60 min | single-node representative prompt + decode |
| mixed node | 60 min | inference + cache read/writeback + link traffic |
| dual node | 60 min | chosen distributed mode, both wall meters |
| recovery | 15 min | return to idle after load |

Run at each approved firmware power profile. Randomize profile order where practical and let the node return to a declared starting temperature.

## 4. Steady-state analysis

Report first-minute and steady-window results separately. For each 60 s bin compute throughput, p95/p99 latency, median/effective clocks, temperatures, fan RPM, wall watts, and errors. **[RECOMMENDATION]** Treat a window as steady only if both temperature and throughput linear trends are within predeclared tolerances; proposed initial tolerances are less than 1 C/10 min and less than 2% throughput/10 min, subject to Section 73 review.

Flag throttling only with evidence: clock/power/thermal limit telemetry, correlated throughput change, or kernel/firmware signal. A clock below advertised maximum is not by itself throttling.

## 5. Safe changes

Before changing a governor, EPP, fan curve, or power limit: save the current value, confirm the control is supported, make one change, define rollback, and obtain authorization for firmware/root writes. Do not combine fan target and PWM controls; kernel documentation warns they can override one another.

No result is `[MEASURED]` until raw telemetry, exact workload/build/model, configuration snapshot, ambient, timestamps, and analysis code are linked.
