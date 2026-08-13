---
section_id: "80"
title: "Fault Injection Procedures and Checks"
status: "needs-machine-validation"
last_verified: "2026-08-12"
applies_to:
  repositories: ["ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689", "charlie12345/ROCmFPX@a5605a72768c6562241b248e268e33dc92787394"]
  software_versions: ["Linux fault-injection documentation accessed 2026-07-17"]
  hardware_revisions: ["dual-Strix-Halo target; exact revisions open"]
related_sections: ["54", "56", "62", "65", "68", "76", "77", "78", "79", "81"]
---

# Procedures and checks

These are guarded procedures, not commands to run blindly.

## Mandatory preflight

1. Use an isolated test deployment and copies of all mutable inputs. Hash model, runtime, configuration, and golden requests. Never use an always-on production deployment as the fault target.
2. Enforce [issue #41](https://github.com/JCFrags/HaloFPX/issues/41): require an authorized maintenance window, exact before-state identities, a clean kernel-OOM baseline, and no protected production or unaccounted KFD/render/HMM owner. `MemAvailable`, free RAM, swap, and RSS cannot override the owner gate.
3. Confirm out-of-band console/SSH, watchdog behavior, emergency stop, rollback, and a second operator for physical cable tests.
4. Resolve exact PIDs/services, cgroups, PCI/GPU identities, USB4 rails, interfaces, loop devices, mounts, and test paths. Verify no target contains workspace, model, source, boot, home, or retained evidence data.
5. Establish a clean Section 78 correctness baseline and Section 79 telemetry capture. Synchronize clocks and begin an event log.
6. Execute one deterministic fault at a time, then the approved combinations. Restore and verify the clean state between cases. After any worker identity change, require both-rank authority reconciliation and a real minimal inference; health alone is insufficient.

## Staged matrix

| Stage | Injection mechanism | Assertions |
|---|---|---|
| Protocol | proxy/test shim injects truncate, bad length/checksum, wrong epoch/version, duplicate/reorder/delay | reject before state mutation; bounded error; diagnostic counter |
| Process | supervised `SIGTERM`, then `SIGKILL`, worker and coordinator separately | ownership cleanup, no split brain/duplicate output, correct restart epoch |
| Memory | dedicated cgroup v2 with calibrated `memory.max`; allocation fault in isolated test binary | host remains healthy; affected request explicit; no partial-state reuse |
| Cache | mutate copies; interrupt before atomic publish; replay stale generation | miss/quarantine/recompute; last valid generation remains usable |
| Filesystem | quota/loopback reaches ENOSPC, remount test FS read-only | no partial promotion; clear degraded mode; cleanup succeeds |
| Block I/O | `dm-flakey` or kernel fail-I/O/NVMe facilities on loopback/sacrificial device | bounded retries/errors; no corrupt read accepted |
| Transport | rail A, rail B, then both; controlled retrain/jitter where supported | epoch transition, declared degradation/failure, correct recovery |
| Accelerator | supported reset in isolated run; scoped OOM | request fails; device/context health proven before readmission |

## Per-case sequence

1. Start a request trace containing short, long-context, structured-output, state-restore, and cache-hit cases.
2. Arm a single fault at a predeclared event or ordinal; record the exact activation and removal times.
3. Validate every request outcome against its golden invariant. Detect duplicates and missing terminal statuses.
4. Remove the fault, await supervised recovery, then rerun the clean correctness subset.
5. Check leaked processes, ranks, leases, slots, mounts, loop devices, temporary files, cache generations, GPU contexts, and network sessions.
6. Preserve raw logs, kernel journal, fault configuration, telemetry, request trace, and before/after hashes.

## Acceptance

[RECOMMENDATION] Any silent wrong result, accepted bad checksum/version/epoch, partial cache promotion, duplicate externally visible action, split brain, unbounded retry/hang, host-wide OOM, or loss outside the disposable target is an immediate failure and safety review. Recovery-time limits remain [OPEN] until requirements are approved; every observed recovery time must still be reported.
