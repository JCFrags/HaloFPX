---
title: Rollback Procedure
description: Operational and Git rollback for each integration milestone.
status: Proposed runbook
evidence-date: 2026-07-17
canonical-repository: charlie12345/ROCmFPX
---

# Rollback Procedure

> [!NOTE]
> Evidence is pinned to **2026-07-17**. Repository heads and source links are locked in [[Source-Register]] and `evidence/commit-lock.json`.


## Rollback objectives

A rollback must restore service without requiring destructive cache migration, preserve forensic evidence, and avoid exposing a newer store to a binary that does not understand it.

## Pre-release requirements

- Signed pre-merge tag and `rollback/<milestone>-premerge` branch exist.
- Previous binary/image is retained and has passed a smoke test.
- Persistent root is versioned and separate from the current per-run cache root.
- The deployment can switch to `off` or `ephemeral` without editing cache files.
- Store snapshot/quarantine procedure has been rehearsed.

## Emergency runtime rollback

```text
1. Stop admitting new requests; drain or terminate active generation safely.
2. Capture provider metrics, logs, binary/build IDs, and store format/version.
3. Remount or reopen the persistent store read-only if corruption is suspected.
4. Snapshot or rename the store root; do not delete it.
5. Restart the current binary with context-store mode = ephemeral or off.
6. Verify cold prompt correctness and standard server health.
7. Deploy the previous release binary if the fault is not isolated to the provider flag.
8. Never point the previous binary at the newer persistent root unless compatibility is explicitly proven.
9. Quarantine affected entries and open an incident record.
```

The existing ROCmFPX per-run cache is the preferred operational fallback because its lifecycle and failure behavior are already tested. [S08] [S09]

## Failure-specific kill switches

| Symptom | Immediate action | Preserve |
|---|---|---|
| Write/fsync/disk-full failures | Open write circuit breaker; continue validated reads or switch read-only. | Existing committed entries and logs. |
| Digest/size/version rejection spike | Disable persistent reads; cold fallback. | Quarantined entries and manifests. |
| Wrong output/state divergence | Disable all persistent restore immediately. | Exact request tokens (under secure incident policy), entry ID, model/build digests. |
| Tenant isolation violation | Disable scoped cache and persistent provider; treat as security incident. | Access logs and scope digests; do not broaden access for debugging. |
| Scheduler starvation/429 regression | Disable slot affinity first, then per-user cap. | Counter snapshots and request IDs. |
| Expert tracking performance regression | Runtime disable; if necessary deploy build without telemetry. | Sampling/performance metrics. |

## Git rollback

Use `git revert`, not reset/force-push, on protected history. Revert in reverse dependency order:

1. L15 aliases/default policy;
2. L11 affinity;
3. L10 concurrency;
4. L09 identity plumbing if no retained provider depends on it;
5. L07 system-prefix and L08 hybrid enablement;
6. L06 matching/importer;
7. L05 persistent provider;
8. L04 writer;
9. retain L03 reader and L02 interface when harmless and used by the existing adapter; revert only if they are causal.

Reverting the writer before the reader is intentional: it stops new state creation while retaining the ability to inspect/quarantine existing entries.

## Rollback points by phase

| Tag | Safe fallback | Store treatment |
|---|---|---|
| `integration-base/*` | Previous canonical release | No persistent store exists. |
| `cache-provider-seam-v0` | Current ROCmFPX adapter | No format commitment. |
| `cache-format-v1-reader` | Feature off / ephemeral | Read-only artifacts may be inspected. |
| `cache-format-v1-writer-off` | Compile/runtime off | Delete only disposable test stores. |
| `persistent-cache-v1-canary` | Ephemeral provider | Preserve v1 store read-only. |
| `persistent-cache-v1-stable` | Prior stable binary + ephemeral | Preserve, snapshot, and migrate only with approved tooling. |

## Roll-forward after rollback

A fix is a new lane commit with a regression test and incident reference. Do not reopen writes to a quarantined store until the fixed reader has completed an offline scan and the rollback acceptance checklist passes.

The state flow diagram is in `diagrams/rollback-flow.mmd`.


---

**Wiki navigation:** [[Home]] · [[Executive-Decision]] · [[Capability-Decision-Matrix]] · [[Patch-Lanes-and-Dependency-Graph]] · [[Acceptance-Criteria]]
