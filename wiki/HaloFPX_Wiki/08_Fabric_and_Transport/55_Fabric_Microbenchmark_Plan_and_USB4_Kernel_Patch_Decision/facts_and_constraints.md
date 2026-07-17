---
section_id: "55"
title: "Fabric benchmark facts and constraints"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: ["torvalds/linux@8cdeaa50eae8dad34885515f62559ee83e7e8dda"]
  software_versions: ["Linux 7.1.3 historical baseline", "Linux 7.2-rc2 candidate"]
  hardware_revisions: ["two Nimo MME3L Strix Halo nodes; exact revisions open"]
related_sections: ["20", "50", "52", "53", "54", "73", "75"]
---

# Facts and constraints

- **[VERIFIED]** USB4STREAM v7.2-rc2 exposes ordinary character-device `read_iter`, `write_iter`, and `poll`, fragments DATA into at most 4 KiB, copies through kernel pages, and has no mmap/registered-buffer ABI [S55-01].
- **[VERIFIED]** Its ConfigFS interface, ring size, throttling, HopIDs, service identity, and device indices are testing ABI and must be captured, not assumed [S55-02].
- **[MEASURED]** The canonical 2026-07-12 no-model report observed both nodes on Linux `7.1.3-1-cachyos`, USB4NET working over both rails, and about 20.8 Gb/s aggregate means in several four-stream cells [S55-L03]. USB4STREAM was not tested. A new-kernel crossover is therefore required before attributing any later difference to transport rather than kernel version.
- **[RECOMMENDATION]** Direct comparison must use the same kernel, record codec, credits, checksums, payload bytes, queue depth, CPU/IRQ policy, and rail mapping. iperf and older native TCP RTTs are descriptive only.

## Required axes

| Axis | Primary values |
|---|---|
| Payload | 64 B, 256 B, 1 KiB, 4 KiB, 16 KiB, 64 KiB, 256 KiB, 1 MiB, 4 MiB; plus exact real trace |
| Queue depth | 1, 4, 16, 64 outstanding, credit bounded |
| Direction | nimo-1→nimo-2, reverse; bidirectional separately |
| Rail | A only, B only, A+B striped; control+bulk topology |
| Transport | same-kernel framed TCP per rail; upstream USB4STREAM |
| Write cap/batch | 4 KiB, 64 KiB, 1 MiB; declared flush timeout |
| Stream settings | ring 256 and throttling 8192 ns baseline; separate tuning sweep |
| Contention | idle, GPU compute, NVMe read/write, CPU memory load, combined safe cell |
| Failure | process restart, each rail loss/reconnect, cleanup/recreate |

## Metrics

Application goodput; p50/p95/p99/max and jitter; per-rail balance; queue/credit stalls; short I/O/`EAGAIN`; syscall counts; combined two-host cycles/instructions/context switches; interrupts/softirq; interface errors/drops/retransmits; MPTCP fallback; kernel copy functions; memory bandwidth/PSI; GPU idle and end-to-end GPU-produced-to-peer-GPU time; NVMe throughput; clocks/power/temperature; kernel warnings/resets; cleanup state hashes.

**[RECOMMENDATION]** Logical payload, record bytes, control bytes, submitted bytes, and any source-modelled 4 KiB frame estimate must be reported separately.

## Invalid comparisons

Different kernels without crossover, tuned versus untuned code on the same holdout, pooling individual messages as independent samples, using throughput-only success, comparing iperf bytes to framed payload without accounting, or claiming zero-copy/copy count without trace evidence.
