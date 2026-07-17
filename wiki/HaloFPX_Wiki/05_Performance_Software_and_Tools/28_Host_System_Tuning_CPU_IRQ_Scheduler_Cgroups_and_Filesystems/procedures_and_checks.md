---
section_id: "28"
title: "Reversible host tuning procedures"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["HaloFPX runtime"]
  software_versions: ["Linux kernel and systemd versions pending machine inventory"]
  hardware_revisions: ["matched Strix Halo hosts", "dual USB4 host links"]
related_sections: ["24", "26", "27"]
---

# Reversible host tuning procedures

## Non-negotiable tuning-script contract

**[RECOMMENDATION]** Any future tuning script must implement `--help`, `capture`, `plan`, `apply`, `verify`, and `restore`; default to `plan`; require an explicit evidence directory; record every path/value before writing; refuse unsupported/missing knobs; use a trap to restore after a bounded trial; and emit machine-readable JSON plus a human log. Boot/mount/unit changes need a separately tested rollback path and must never be silently applied.

## 1. Read-only inventory

Run on both nodes. Most reads are unprivileged; some device/systemd details may require root.

```bash
mkdir -p evidence/host-tuning-baseline
lscpu -e=CPU,CORE,SOCKET,NODE,CACHE,ONLINE > evidence/host-tuning-baseline/lscpu.txt
cat /proc/cmdline > evidence/host-tuning-baseline/cmdline.txt
grep . /sys/devices/system/cpu/cpufreq/policy*/{scaling_driver,scaling_governor,scaling_min_freq,scaling_max_freq} \
  > evidence/host-tuning-baseline/cpufreq.txt 2>&1 || true
cat /proc/interrupts > evidence/host-tuning-baseline/interrupts-before.txt
find /sys/class/net -path '*/queues/*/*ps_cpus' -type f -exec sh -c 'echo "$1 $(cat "$1")"' _ {} \; \
  > evidence/host-tuning-baseline/rps-xps.txt
systemctl show halofpx.service > evidence/host-tuning-baseline/unit-properties.txt
systemd-cgls > evidence/host-tuning-baseline/cgroups.txt
sysctl -a 2>/dev/null | grep -E '^vm\.(dirty|swappiness|overcommit)' > evidence/host-tuning-baseline/vm.txt
grep -H . /sys/kernel/mm/transparent_hugepage/{enabled,defrag} > evidence/host-tuning-baseline/thp.txt
cat /proc/meminfo > evidence/host-tuning-baseline/meminfo.txt
findmnt -o TARGET,SOURCE,FSTYPE,OPTIONS > evidence/host-tuning-baseline/mounts.txt
lsblk -o NAME,KNAME,TYPE,FSTYPE,MOUNTPOINTS,ROTA,SCHED,RQ-SIZE,MODEL,SERIAL > evidence/host-tuning-baseline/block.txt
```

Also save topology for both USB4-backed interfaces, `ethtool -l/-x/-k/-S`, IRQ affinities, irqbalance state/config, `ulimit -a`, cgroup v2 controller files, swap, NUMA, NVMe identify/SMART, thermals and section 26 build identity.

## 2. Plan one experiment

Create a plan containing hypothesis, exact files/unit properties, original/new values, expected effect, correctness invariant, rollback command, maximum duration and abort thresholds. Example hypothesis: "moving link-0 IRQ and its transport worker to the same non-SMT core reduces p99 transfer wakeup delay without increasing token latency or link-1 loss." Placeholder CPU masks are forbidden.

## 3. Apply ephemerally

Prefer transient runtime writes or `systemd-run` properties for the first trial. Capture values immediately after application and fail if the kernel normalizes/rejects them. Keep an independent root shell or out-of-band recovery path when changing affinity, RT, memory or network controls.

## 4. Measure and restore

Use section 27 instrumentation, fixed workload/model/build, thermal steady state, multiple randomized A/B/A/B runs and both hosts. Restore original values in reverse order even after workload failure. Capture interrupts, queues, PSI, faults, thermals, block/network counters and service health after restore.

## 5. Promote declaratively

Only after matched reproduction, encode the smallest accepted subset in systemd or host configuration with comments linking experiment IDs. Test reboot, failure injection, single-node fallback and removal. Preserve the baseline manifest and restore instructions.

## Before/after evidence checklist

- workload outputs and correctness hashes;
- latency distribution, throughput, power/temperature/clocks;
- CPU placement, context switches, IRQ/softirq counts and queue drops;
- page faults, swap, PSI, reclaim/OOM and huge-page fallback;
- filesystem/block latency, NVMe health and durability/failure behavior;
- exact original/applied/restored values and exit status.

