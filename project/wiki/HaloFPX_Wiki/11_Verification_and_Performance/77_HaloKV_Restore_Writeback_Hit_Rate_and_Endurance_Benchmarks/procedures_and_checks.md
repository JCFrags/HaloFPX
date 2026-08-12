---
section_id: "77"
title: "HaloKV Benchmark Procedures and Checks"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["fewtarius/CachyLLama@6be745998f568e379ea197fcf827baec73ff9940"]
  software_versions: ["fio 3.41 documentation", "nvme-cli version to freeze"]
  hardware_revisions: ["exact NVMe devices pending"]
related_sections: ["21", "63", "65", "73", "76", "78", "80"]
---

# Procedures and Checks

These procedures are non-executed plans. Disk-fill and power-loss tests can destroy data and must use an explicitly approved sacrificial cache/device or VM image; never target a workspace, home directory, model store, or unverified block device.

## 1. Freeze environment

Capture both nodes' build/model/topology hashes, mount/device mapping, filesystem/mount options, free space, kernel, I/O scheduler, block sizes, NVMe model/serial/firmware, temperature, SMART log, power state, cache configuration, and competing I/O. Resolve the exact cache path to its device before tests.

Example read-only inventory, with the device explicitly selected:

```bash
findmnt --target /absolute/sacrificial/cache/path --json
lsblk --json -o NAME,PATH,MODEL,SERIAL,FIRMWARE,SIZE,ROTA,PHY-SEC,LOG-SEC,MOUNTPOINTS
sudo nvme id-ctrl /dev/nvmeX --output-format=json
sudo nvme smart-log /dev/nvmeX --output-format=json
```

## 2. Build correctness oracles

For each supported state type, generate deterministic checkpoints and a no-cache continuation. Restore and compare at Section 78-defined boundaries. Include incompatible model/tokenizer/template/build/KV/topology fingerprints, truncated files, bad lengths, bit flips, stale manifests, missing rank objects, and reordered generations.

## 3. Lookup and restore sweep

For every page/segment and context size, record per-request events for candidate scan, lookup, queue, I/O, validation, installation, rank barrier, residual prefill, TTFT, and outcome class. Create hot, warm, OS-cache-warm, and cold states explicitly; do not infer coldness from elapsed time alone.

For two ranks, launch reads simultaneously from a common coordinator event and retain rank-local timestamps/bytes. Inject a slow rank non-destructively with controlled I/O throttling before physical failure tests.

## 4. Dirty-tail writeback

Sweep dirty tail length, checkpoint interval, batch/coalescing window, durability mode, and concurrent writers. Record logical dirty bytes, serialized bytes, write syscalls, host device write delta, sync calls/duration, manifest publication, foreground pause, and completion status. Reopen after clean restart for every cell.

## 5. Garbage collection and disk pressure

Create only bounded synthetic cache objects under the resolved sacrificial path. Test occupancy bands and fragmentation with foreground decode. Record live/dead bytes, scan/copy/delete duration, freed bytes, write amplification, p95/p99 foreground impact, and quota/user fairness.

Disk-full behavior must first use a bounded filesystem image or quota. Verify the last committed generation remains loadable and new state fails closed without deleting unrelated data.

## 6. Hit-rate replay

Run fixed synthetic traces, then sanitized held-out workload traces. Preserve request order, identity scope, prefix tokens/hash, context, cache state, admission/eviction events, useful tokens restored, residual compute, and latency. Never store prompt text or tenant secrets in the research corpus when hashes/derived features suffice.

## 7. Endurance observation

Take SMART snapshots immediately before/after an isolated run and after controller counters settle. Record other device writes. Compute logical HaloKV bytes, NVMe host-write delta using specification units, and the labeled host-write ratio. Repeat long enough to overcome counter rounding; do not manufacture precision. [S77-006][S77-007]

## 8. Power-loss and crash tests

Route process kill, OS crash, cable/power removal, and forced reset to Section 80 with an approved safety card. After each event, preserve the cache image before repair, run read-only inspection, then verify either the previous committed generation or a miss/recompute. Accepted partial/corrupt state is a blocker.

## 9. Promotion gate

Require raw events, exact commands/config, environment manifest, hashes, correctness results, all failures, latency distributions/intervals, device counters, and recovery evidence. Repository-authored CachyLLama benchmarks do not substitute for this matrix.
