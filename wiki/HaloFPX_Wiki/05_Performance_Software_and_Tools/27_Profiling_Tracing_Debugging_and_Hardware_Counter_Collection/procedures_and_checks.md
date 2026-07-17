---
section_id: "27"
title: "Profiling procedures and checks"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["charlie12345/ROCmFPX", "ggml-org/llama.cpp"]
  software_versions: ["ROCprofiler-SDK 1.3.2 documentation", "Linux tracing interfaces"]
  hardware_revisions: ["gfx1151 Strix Halo", "dual USB4 host links"]
related_sections: ["24", "25", "26", "28"]
---

# Profiling procedures and checks

Commands are read-only except capture output creation. Some kernel/PMU functions require root or capabilities. Run against a bounded test workload, never an unredacted user session.

## 1. Inventory before capture

```bash
mkdir -p evidence/profile-inventory
uname -a > evidence/profile-inventory/uname.txt
cat /proc/cmdline > evidence/profile-inventory/cmdline.txt
rocprofv3 --version > evidence/profile-inventory/rocprofv3-version.txt
rocprofv3-avail list > evidence/profile-inventory/rocprofv3-avail.txt
perf version > evidence/profile-inventory/perf-version.txt
perf list > evidence/profile-inventory/perf-list.txt
find /sys/kernel/tracing/events -maxdepth 2 -type d | sort > evidence/profile-inventory/trace-events.txt
ip -details link > evidence/profile-inventory/links.txt
lsblk -o NAME,KNAME,TYPE,FSTYPE,MOUNTPOINTS,MODEL,SERIAL > evidence/profile-inventory/block-map.txt
```

Capture section 26 build identity and GPU agents. Record permissions and missing tools as evidence, not as empty success.

## 2. Establish unprofiled baseline

Run warmup plus repeated steady-state trials with fixed model/prompt hashes, rank topology, cache state and thermals. Save per-token/phase timestamps, application logs, `iostat -xz 1`, interface counters, `/proc/interrupts`, PSI, temperatures/clocks and exit status.

## 3. GPU timeline

```bash
rocprofv3 --output-directory evidence/rocprof-runtime \
  --output-format pftrace csv --output-config \
  --hip-trace --kernel-trace --memory-copy-trace -- ./halofpx-repro
```

Use `--sys-trace` or KFD page/queue options only for a targeted question. Validate supported option names against the installed version. Check tool logs and dropped-event reporting.

For counters:

```bash
rocprofv3-avail list --pmc > evidence/available-pmc.txt
rocprofv3-avail pmc-check COUNTER_A COUNTER_B
rocprofv3 --pmc COUNTER_A,COUNTER_B -- ./halofpx-repro
```

Placeholder names are intentional. Select counters from the captured gfx1151 list.

## 4. CPU and scheduler

```bash
perf stat -d -r 5 -o evidence/perf-stat.txt -- ./halofpx-repro
perf record -g --call-graph dwarf -o evidence/perf.data -- ./halofpx-repro
perf sched record -o evidence/perf-sched.data -- ./halofpx-repro
```

Document `perf_event_paranoid`, unwind choice and lost samples. Generate flame graphs as derived views from the retained `perf.data`.

## 5. Network/USB4 and block path

Snapshot `ethtool -S`, `ip -s link`, `/proc/interrupts`, queue/RPS/XPS settings and driver/device topology before and after. Use `trace-cmd list` to select only existing driver, network, IRQ, scheduler and block events. Record both link IDs. For storage, use `iostat -xz`, `nvme smart-log`, `smartctl -x`, and bounded block tracepoints; preserve device mapping.

## 6. Two-host time qualification

Record `chronyc tracking/sources` or `pmc`/`phc_ctl` evidence, timestamping capability (`ethtool -T`), clock IDs and offset/error immediately before and after. Inject matching application markers and a ping-pong sequence. If error bounds exceed the event interval, report only causal order/round trip.

## 7. Closeout

Repeat unprofiled trials; hash raw captures; record profiler overhead, dropped events, redactions and analysis scripts. A tuning win from section 28 requires independent unprofiled reproduction.

