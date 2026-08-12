---
title: "Open questions"
tags: ["open-questions", "experiments"]
created: 2026-07-17
updated: 2026-07-17
status: design-proposal
sources: []
related: ["Decision-Log", "Formal-Modeling", "Fuzzing-and-Fault-Injection"]
---

# Open questions

## Implementation contracts

- Which parallelism modes are in scope first, and what exact shard-completeness predicate validates each global manifest?
- What is the canonical logical position for paged attention, sliding windows, beams, speculative branches, and sequence forks?
- Is deterministic force-replay guaranteed across the production hardware/kernel matrix, or only semantic continuity?
- Which cache-page size minimizes write amplification and recovery request count for actual model shapes?
- Which fields affect exact bytes versus only transport compatibility?
- Can the engine atomically attach restored pages at a barrier without exposing partially materialized state?

## Authority and storage

- Which authority provides linearizable compare-and-swap, leases/fencing, durable revision, backup/restore anti-rollback, and audit?
- Does the selected blob store provide conditional create, version IDs, read-after-write behavior, checksum exposure, and deletion consistency required by the protocol?
- What off-host replication proof is sufficient before claiming `HOST_FAILURE_DURABLE`?
- How are authority and encryption/HMAC keys rotated without orphaning retained checkpoints?

## Product policy

- Under persistence lag, should inference pause, continue with a declared replay window, or shed new sessions?
- How many older certificates should be retained for latent-corruption fallback?
- What client contract defines externally acknowledged tokens and restart/reset behavior?
- Is one-node reconfiguration a product requirement? If yes, which models fit, what is the startup latency, and is cache conversion worthwhile versus replay?

## Verification experiments

- Measure delta-recovery bytes and time across realistic prefix reuse and rank failure points.
- Inject crash at every write/flush/rename/manifest/certificate boundary on each supported filesystem and store.
- Compare cache and logits after cross-host replay for every supported dtype/backend.
- Quantify Bloom-filter false positives, inventory exchange cost, and final verification misses.
- Find credit/window settings that avoid both memory spikes and control-plane starvation.
- Model cancellation and epoch bump during every checkpoint phase and reconcile implementation traces against TLA+/P models.
