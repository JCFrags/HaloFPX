---
section_id: "40"
title: "Replication Routing Design"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["HaloFPX design candidate"]
  software_versions: []
  hardware_revisions: ["dual Strix Halo premise"]
related_sections: ["38", "39", "46", "48", "55"]
---

# Design implications

## Routing policy

**[RECOMMENDATION]** Filter candidates by readiness, exact model/capabilities, deadline feasibility, and memory safety. Then score:

`score(r) = predicted_finish_p99(r) + miss_recompute_p99(r) + reload_penalty(r) + risk_penalty(r)`.

For a valid warm owner, subtract its measured prefix-reuse benefit, but cap affinity: if predicted owner completion exceeds the best cold replica by more than an empirically chosen guard, reroute and recompute. Use hysteresis to avoid session ping-pong.

New session tie-break order: warm exact prefix, lowest predicted completion, lowest memory pressure, stable hash of session ID. A stable hash is a tie-breaker, not a substitute for load awareness.

## Ownership and commits

1. Coordinator leases `(session, epoch)` to one primary replica.
2. Request carries last committed token position/hash.
3. Replica rejects stale epochs, executes, and streams provisional output tagged with request/position.
4. Coordinator is sole commit authority and advances the session only once.
5. Retry uses a new request attempt under the same semantic request ID; duplicate completions are discarded.

**[RECOMMENDATION]** Never retry stochastic generation from an unknown sampler/RNG state and claim seamless continuity. Either restore compatible state, replay deterministically from the last checkpoint under an explicit contract, or terminate the interrupted response and start a new one.

## Failover

| Failure | Action |
|---|---|
| owner unhealthy before request dispatch | route to compatible replica; load/recompute cache |
| failure during prefill before output | retry from authoritative prompt if deadline permits |
| failure after streamed tokens | preserve committed prefix; follow section 48 policy for resume vs terminate |
| cache corruption/incompatibility | quarantine cache record; recompute |
| coordinator restart | rebuild directory from durable commit log; fence old epochs |

**[RECOMMENDATION]** Replicas never automatically merge or overwrite each other's persistent cache. Transfer is copy-then-validate-then-register, with source retained until receipt.

## When replication wins

Prefer replication when each model fits with required KV/concurrency; independent sessions dominate; p99 collective/activation traffic would harm ITL; failure isolation matters; or nodes host different models. Prefer a coupled mode only for single-request capacity or a measured p99 advantage under section 38 gates.
