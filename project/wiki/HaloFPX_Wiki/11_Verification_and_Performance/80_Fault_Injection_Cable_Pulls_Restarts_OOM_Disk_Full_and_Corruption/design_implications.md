---
section_id: "80"
title: "Fault Injection Design Implications"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: ["ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689", "charlie12345/ROCmFPX@a5605a72768c6562241b248e268e33dc92787394"]
  software_versions: ["Linux fault-injection documentation accessed 2026-07-17"]
  hardware_revisions: ["dual-Strix-Halo target; exact revisions open"]
related_sections: ["54", "56", "62", "65", "68", "76", "77", "78", "79", "81"]
---

# Design implications

## Fault-to-invariant matrix

| Injection | Required observable behavior | Forbidden outcome |
|---|---|---|
| Pull/retrain rail A or B | new link epoch; traffic either drains/retries on the surviving approved path or fails explicitly | stale in-flight bytes accepted under old epoch |
| Lose both rails | distributed request fails; single-node fallback only with complete local model/request state | fabricated continuation or indefinite hang |
| Worker crash/kill | coordinator expires ownership, fails or safely replays request, cleans leases | duplicate tokens/tools or reuse of worker-owned stale state |
| Coordinator crash/kill | workers reject old epoch after restart; orphan state is reclaimed | two active coordinators or split brain |
| GPU reset/OOM/allocation failure | affected request fails explicitly; backend/context rebuilt before reuse | partial logits/output accepted |
| Stale binary/model/protocol | compatibility handshake rejects before work/state exchange | mixed-version silent operation |
| Malformed, truncated, duplicate, reordered, checksum-bad message | reject, count, and log without mutating committed state | parser crash or state advancement |
| Torn/corrupt cache entry | checksum/version/commit validation causes miss, quarantine, or recomputation | corrupt entry treated as a hit |
| ENOSPC/read-only filesystem | atomic write fails without replacing last valid generation; service degrades explicitly | partial file promoted or evidence/model overwritten |
| NVMe EIO/timeout | bounded error propagation, cache bypass/quarantine where safe | infinite retry or silent data substitution |

## Epoch and commit model

[RECOMMENDATION] Coordinator incarnation, topology, rail session, model/runtime identity, and cache generation participate in compatibility. Restart or topology change advances the relevant epoch. Messages and state from an older epoch are rejected.

[RECOMMENDATION] Persistent cache/state writes use write-to-new, checksum/fingerprint, flush as required by the durability contract, and atomic publish. Readers never discover an uncommitted generation. Corruption produces a miss or recomputation, never an accepted hit.

## Recovery accounting

Each injection must record detection time, request outcome, retries, duplicate suppression, recovery start/end, degraded topology, lost work, cleanup completion, and post-recovery correctness. Process liveness alone is insufficient.

## Safety boundary

The harness must resolve and print the exact cgroup, PID, interface/rail, loop device, mount, and disposable paths before arming. A second explicit confirmation is appropriate for physical cable, GPU reset, kernel fault, and block-device scenarios. Automatic cleanup may remove only resources created and identified by that test run.
