---
section_id: "68"
title: "Lifecycle State Machines and Routing Policy"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["HaloFPX project"]
  software_versions: []
  hardware_revisions: ["two-node Strix Halo target"]
related_sections: ["38", "39", "40", "45", "46", "47", "48", "60", "61", "67"]
---

# State machines and routing policy

## Model lifecycle

```text
discovered -> validating -> admitted -> reserving -> loading -> warming -> ready
                    \-> rejected       \-> failed <- any active state
ready -> draining -> unloading -> unloaded -> discovered
ready -> sleeping -> loading
```

- `admitted` means hashes, shards, licenses, compatibility, and allowed plans pass—not that memory is allocated.
- `ready` is per model/plan generation and requires every required rank.
- Replacement creates a new immutable generation, warms it, shifts new traffic, drains the old generation, then unloads. In-place mutation is prohibited.

## Session lifecycle

```text
new -> authenticating -> queued -> admitted -> active -> idle -> active
          \-> rejected       \-> cancelled/failed/completed -> retained -> expired
```

Each session records authenticated owner, session ID, epoch, model generation, plan, rank owners, compatibility hash, last committed token boundary, cache policy, deadlines, and terminal result. A continuation increments a request epoch but retains session ownership.

## Admission algorithm

1. Authenticate and bind requested user identity.
2. Validate API/body/extensions, deadlines, size, model, template, and requested objective.
3. Enforce global, per-user, per-model, and administrative-drain caps.
4. Select only compatible, unexpired plans whose required ranks/links/storage are healthy.
5. Reserve queue, slots, memory/KV, transport credits, and cache-write budget atomically or reject/defer.
6. Assign a session/request epoch and dispatch; release reservations on every terminal path.

**[RECOMMENDATION]** Queue by authenticated tenant using weighted fair scheduling, with a reserved interactive pool and bounded batch capacity. Return 429 with retry guidance when policy limits are hit; return 503 when no compatible plan is ready.

## Routing order

For a new request: compatible plan objective score, ready capacity, queue cost, then deterministic tie-break. For continuation: current healthy owner/plan, exact cache locality, another compatible replica with validated state, then clean recomputation. User caps and fairness outrank cache locality.

## Degraded behavior

| Failure | Allowed response |
|---|---|
| One link lost | Continue on a validated one-link plan or fail/re-admit from a clean boundary |
| Worker lost | Fail active split request; route later work to admitted single-node/replica only if model fits |
| Coordinator lost | New coordinator may recover committed metadata; in-flight output is not assumed committed |
| Cache unavailable/corrupt | Miss/recompute; never accept invalid state |
| Model generation draining | Existing sessions finish within deadline; new requests use replacement or explicit unavailable error |

