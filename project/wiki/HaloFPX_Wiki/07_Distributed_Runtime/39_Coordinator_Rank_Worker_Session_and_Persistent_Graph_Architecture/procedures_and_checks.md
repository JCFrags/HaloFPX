---
section_id: "39"
title: "Coordinator and Rank Procedures"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["HaloFPX design candidate"]
  software_versions: []
  hardware_revisions: []
related_sections: ["45", "48", "66"]
---

# Procedures and checks

## Source research completed

Pinned and inspected current `llama.cpp` server/RPC patterns, ROCmFPX head, HIP graph lifecycle, and Orca scheduling. No source establishes the proposed HaloFPX protocol as implemented.

## `DR-39-E1`: lifecycle conformance

Prerequisites: debug build with structured trace IDs; exact model manifest; two nodes. Root not normally required.

1. Start workers in reverse order and with coordinator absent; verify bounded wait and no model execution.
2. Exercise valid startup and record each state transition/allocation.
3. Mutate one copied manifest field at a time (protocol, model hash, tokenizer hash, tensor shape/type, quantization, cache ABI, graph plan); verify readiness is refused and allocations are released.
4. Replay and reorder command IDs; verify idempotence/epoch rejection.
5. Cancel during prefill, decode, collective, and graph launch; verify no late token is committed.
6. Drain with idle and active sessions, then kill coordinator/worker abruptly; verify bounded cleanup and restart behavior.

## `DR-39-E2`: graph reuse matrix

For eager and graph paths, sweep prefill/decode, batch/context buckets, cache hit/miss, and mode. Compare token IDs/logits under section 48 tolerances; record instantiate, launch, update, hit, rebuild, fallback, memory high-water, and p99 latency. Inject incompatible graph keys and failed update/launch. A failure must produce eager recomputation or a request error, never stale output.

## Required logs

Retain monotonic timestamps, process/boot IDs, all version/hash identities, state transition, command identity/epoch, buffer/graph key, bytes and collective, return status, and cleanup result. Redact request text; token IDs require project privacy policy.

## Contingent decisions

Heartbeat/collective deadlines, graph bucket sizes, graph-cache capacity, warmup count, session TTL, checkpoint boundary, and whether sampler/logit processing can remain coordinator-local require machine evidence.
