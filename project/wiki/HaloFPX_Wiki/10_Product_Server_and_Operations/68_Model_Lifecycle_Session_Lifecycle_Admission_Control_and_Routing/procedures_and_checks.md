---
section_id: "68"
title: "Lifecycle, Admission, and Routing Checks"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["HaloFPX project"]
  software_versions: ["release candidate under test"]
  hardware_revisions: ["actual node A/node B deployment"]
related_sections: ["09", "45", "46", "48", "61", "67", "69", "72", "78"]
---

# Lifecycle, admission, and routing checks

## Internet/source follow-up

1. Map pinned upstream router/model/slot/cache state transitions to source code and tests, not only README descriptions.
2. Identify all load/unload failure exits and memory-release behavior across Vulkan and HIP/ROCm.
3. Audit CachyLLama user/slot affinity, checkpoint ownership, expiry, and concurrency code before import.
4. Recheck router and server changelogs at release freeze.

## Model lifecycle campaign

Prerequisites: exact manifests, memory telemetry, isolated model files, and rollback build. Root is not required for normal lifecycle tests.

1. Discover valid, missing, duplicated, corrupt, wrong-hash, incomplete-shard, and incompatible models.
2. Measure reservation estimates versus peak/steady memory on both backends and nodes.
3. Repeat cold load, warmup, readiness, idle sleep/wake, drain, unload, replacement, failed load, and restart; retain raw timing/memory logs.
4. Replace a model under active sessions and prove generation pinning, bounded drain, and no mixed artifact/template/cache state.
5. Force allocation failure and verify all reservations and rank resources are released.

## Admission/routing campaign

1. Replay representative interactive, agent, batch, and multi-user workloads with declared caps.
2. Saturate user, model, global, memory, queue, transport, and cache-write limits independently; verify 429 versus 503 semantics.
3. Confirm weighted fairness, interactive reservation, cancellation latency, deadline enforcement, and absence of starvation.
4. Continue sessions on healthy owners, then inject queue imbalance, cache miss, one-link loss, rank loss, coordinator restart, slow rank, and model drain.
5. Verify every active request has exactly one terminal state and all stale epoch results are rejected.
6. Expire/evict idle sessions under concurrent continuation and prove active state is not deleted or crossed between users.

## Required evidence

- State-transition/event log with request/session/model generation IDs.
- Reservation versus observed memory and release accounting.
- Queue/fairness/cap distributions and rejection reasons.
- Fault-to-route/fallback decisions, active-rank ownership, and single-node result.
- No accepted corrupt cache or silently migrated in-flight request.

