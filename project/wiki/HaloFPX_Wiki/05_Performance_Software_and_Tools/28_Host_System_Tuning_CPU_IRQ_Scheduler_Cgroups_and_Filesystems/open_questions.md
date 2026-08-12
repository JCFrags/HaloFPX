---
section_id: "28"
title: "Host tuning open questions"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["HaloFPX runtime"]
  software_versions: ["Linux kernel and systemd versions pending machine inventory"]
  hardware_revisions: ["matched Strix Halo hosts", "dual USB4 host links"]
related_sections: ["24", "26", "27"]
---

# Host tuning open questions

| ID | Question | Evidence needed | Blocks |
|---|---|---|---|
| O28-01 | **[OPEN]** What CPU/core/SMT/cache/NUMA and cpufreq-policy topology does each host expose? | Full two-host inventory | CPU masks/governor |
| O28-02 | **[OPEN]** Which `amd-pstate` mode and boost controls are active and stable under load? | sysfs/kernel plus thermal A/B tests | Frequency policy |
| O28-03 | **[OPEN]** Which IRQs/queues serve each USB4 link, GPU and NVMe, and does irqbalance rewrite them? | Topology and traces | IRQ partition |
| O28-04 | **[OPEN]** Do affinity or RPS/XPS changes reduce tail latency without excess IPIs/drops? | Per-link A/B experiment | Queue policy |
| O28-05 | **[OPEN]** Is scheduler jitter material, and can normal affinity/nice solve it before RT? | ftrace/perf sched experiments | RT decision |
| O28-06 | **[OPEN]** Which buffers benefit from mlock, THP or HugeTLB, and what are fallback/fragmentation costs? | Fault, residency, latency and pressure tests | Memory policy |
| O28-07 | **[OPEN]** How are unified GPU allocations charged/limited under the selected cgroup/KMD? | Bounded allocation tests and cgroup counters | Unit memory limits |
| O28-08 | **[OPEN]** Does swap or writeback affect steady-state/p99 behavior, and what safety margin prevents OOM? | PSI/fault/writeback A/B tests | VM policy |
| O28-09 | **[OPEN]** Which filesystem, mount options and I/O modes serve models and persistent KV cache? | `findmnt`, source, recovery and fio/application tests | Storage policy |
| O28-10 | **[OPEN]** Which NVMe scheduler/queue settings are exposed, and do changes help the real cache pattern? | Device inventory and matched trace | NVMe tuning |
| O28-11 | **[OPEN]** What service isolation still permits GPU, link, storage, observability and recovery access? | systemd sandbox/cgroup fault tests | Deployment unit |
| O28-12 | **[OPEN]** Can every accepted change survive reboot and be fully removed without losing remote access? | Reboot/rollback drill on each node | Production promotion |

## Internet follow-up

Pin the exact kernel, systemd, network/USB4 driver and filesystem documentation after host inventory. Review distro-specific `amd-pstate`, irqbalance and systemd defaults; generic kernel interfaces do not define the current configuration.

## Machine experiments

Execute O28-01 through O28-12 using the reversible procedure. No tuning recommendation is currently a measured result.

