---
section_id: "28"
title: "Host tuning sources"
status: "draft"
last_verified: "2026-07-16"
applies_to:
  repositories: ["HaloFPX runtime"]
  software_versions: ["Linux kernel and systemd versions pending machine inventory"]
  hardware_revisions: ["matched Strix Halo hosts", "dual USB4 host links"]
related_sections: ["24", "26", "27"]
---

# Host tuning sources

Access date: 2026-07-16. Kernel `latest` pages must be pinned to the selected kernel source before production promotion.

| ID | Primary source | Supports | Limitations |
|---|---|---|---|
| S28-01 | Linux kernel, [CPU performance scaling](https://docs.kernel.org/admin-guide/pm/cpufreq.html) | Policies, drivers, governors and sysfs | Exact amd-pstate mode/firmware is host-specific. |
| S28-02 | Linux kernel, [cgroup v2](https://docs.kernel.org/admin-guide/cgroup-v2.html) | CPU, cpuset and memory controller semantics | Distribution/systemd delegation varies. |
| S28-03 | Linux kernel, [Reducing OS jitter from per-CPU kthreads](https://docs.kernel.org/admin-guide/kernel-per-CPU-kthreads.html) | Housekeeping and jitter considerations | Guidance, not proof isolation helps HaloFPX. |
| S28-04 | Linux kernel, [SMP IRQ affinity](https://docs.kernel.org/core-api/irq/irq-affinity.html) | IRQ mask/list interface | Drivers/managed IRQs may restrict writes. |
| S28-05 | Linux kernel, [Scaling in the Linux networking stack](https://docs.kernel.org/networking/scaling.html) | RSS/RPS/RFS/XPS behavior and tradeoffs | USB4 transport topology/driver may differ. |
| S28-06 | Linux man-pages, [sched(7)](https://man7.org/linux/man-pages/man7/sched.7.html) and [getrlimit(2)](https://man7.org/linux/man-pages/man2/getrlimit.2.html) | Scheduling policies, RT and resource limits | Privilege/service policy still required. |
| S28-07 | Linux kernel, [cgroup v2 memory controller](https://docs.kernel.org/admin-guide/cgroup-v2.html#memory) | `memory.high`, `memory.max`, OOM behavior | GPU/unified-memory accounting needs experiment. |
| S28-08 | Linux kernel, [HugeTLB pages](https://docs.kernel.org/admin-guide/mm/hugetlbpage.html) and [THP](https://docs.kernel.org/admin-guide/mm/transhuge.html) | Explicit vs transparent huge-page mechanisms | Benefit and fragmentation are workload-specific. |
| S28-09 | Linux kernel, [`/proc/sys/vm`](https://docs.kernel.org/admin-guide/sysctl/vm.html) | Dirty/writeback/swap VM controls | Defaults are distro/memory-size dependent. |
| S28-10 | Linux kernel, [ext4](https://docs.kernel.org/admin-guide/ext4.html) and [XFS](https://docs.kernel.org/admin-guide/xfs.html) | Filesystem behavior/options | Actual filesystem unknown. |
| S28-11 | Linux kernel, [block queue sysfs](https://docs.kernel.org/block/queue-sysfs.html) | Scheduler and queue attributes | Attributes are device/driver-specific; writes can harm performance. |
| S28-12 | systemd, [systemd.resource-control](https://www.freedesktop.org/software/systemd/man/latest/systemd.resource-control.html) and [systemd.exec](https://www.freedesktop.org/software/systemd/man/latest/systemd.exec.html) | Unit cgroup, affinity, scheduling, limits and isolation | `latest` manual must match installed systemd. |

## Conflicts and caution

**[OPEN]** Generic tuning guides often prescribe `performance`, swapoff, isolated CPUs, RT, huge pages, large dirty ratios or `noatime` without workload evidence. Those are hypotheses here, not sources of verified HaloFPX settings.

