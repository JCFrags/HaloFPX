---
section_id: "28"
title: "Host tuning facts and constraints"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["HaloFPX runtime"]
  software_versions: ["Linux kernel and systemd versions pending machine inventory"]
  hardware_revisions: ["matched Strix Halo hosts", "dual USB4 host links"]
related_sections: ["24", "26", "27"]
---

# Host tuning facts and constraints

| Domain | Verified interface/behavior | HaloFPX constraint |
|---|---|---|
| CPU frequency/boost | **[VERIFIED]** CPUFreq uses driver-specific policy objects and governors; policy sysfs exposes driver/governor/limits. [S28-01] | Inventory policy sharing and `amd-pstate` mode; "performance" does not guarantee a fixed clock or no throttling. |
| CPU affinity/isolation | **[VERIFIED]** affinity is constrained by cpusets; `isolcpus` is a boot parameter and kernel docs recommend housekeeping CPUs for jitter sources. [S28-02][S28-03] | Prefer runtime cpuset experiments before boot-time isolation. Keep OS/IRQ/RCU housekeeping capacity. |
| IRQ affinity | **[VERIFIED]** `/proc/irq/N/smp_affinity_list` controls allowed CPUs where supported. [S28-04] | irqbalance and driver resets may rewrite affinities; capture before/after. |
| RPS/XPS | **[VERIFIED]** RPS is software receive steering and introduces IPIs; XPS selects transmit queues. Both expose per-queue CPU masks and may be redundant with hardware queues. [S28-05] | Tune each USB4-backed interface/link independently after queue/IRQ mapping. |
| Scheduling/RT | **[VERIFIED]** `SCHED_FIFO`/`SCHED_RR` can starve ordinary work; resource limits and privileges constrain real-time priority and memory locking. [S28-06] | RT is a narrow candidate for transport threads, never a blanket service setting; preserve watchdog/SSH/housekeeping. |
| Cgroups v2 | **[VERIFIED]** `cpu.max`, `cpu.weight`, cpuset and `memory.high`/`memory.max` control resource distribution/limits; `memory.max` can invoke cgroup OOM. [S28-07] | Avoid hard memory ceilings until unified GPU/host memory accounting is observed. |
| Swap/mlock | **[INFERENCE]** Swap activity and page faults can add latency, while disabling swap or locking too much memory can turn pressure into OOM. | Measure faults/PSI; reserve safety margin; set `LimitMEMLOCK` only to justified amount. |
| Huge pages | **[VERIFIED]** explicit HugeTLB and transparent huge pages are different mechanisms with distinct controls/accounting. [S28-08] | Benchmark model mappings, KV and transport buffers separately; inspect fallback and fragmentation. |
| Dirty pages | **[VERIFIED]** Linux VM exposes background/writeback timing and byte/ratio thresholds. [S28-09] | SSD cache durability and latency need bounded writeback tests; do not copy server sysctls blindly. |
| Filesystem/NVMe | **[VERIFIED]** ext4/XFS mount choices and block queue attributes are device/filesystem-specific. [S28-10][S28-11] | Preserve crash consistency and cache-corruption-as-miss; queue depth/scheduler changes require device evidence. |
| Service isolation | **[VERIFIED]** systemd maps CPU, memory, I/O, affinity, scheduling and limits into unit properties. [S28-12] | Put accepted settings in versioned unit drop-ins/configuration, not an opaque boot script. |

## Universal safety constraints

- **[RECOMMENDATION]** Do not disable thermal protection, error handling, journaling/barriers, watchdog access or recovery services for a benchmark win.
- **[RECOMMENDATION]** Keep one known-good boot entry and a single-node runtime path when testing boot/cgroup/IRQ changes.
- **[RECOMMENDATION]** A tuning claim needs matched workload, build/model hashes, thermals, initial cache state, repetitions and raw data.
- **[OPEN]** GPU memory charged to cgroup controllers and `mlock` behavior must be observed; do not assume all unified-memory residency is controlled like anonymous CPU memory.

