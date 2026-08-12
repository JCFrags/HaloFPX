---
title: "Assumptions and non-goals"
tags: ["scope", "assumptions"]
created: 2026-07-17
updated: 2026-07-17
status: design-proposal
sources: ["KV-01", "KV-05"]
related: ["System-Model", "Partial-Rank-Failure", "Open-Questions"]
---

# Assumptions and non-goals

## Deployment assumptions

- One logical model execution uses exactly two active ranks, numbered `0` and `1` in the baseline deployment.
- The split may be tensor, pipeline, expert, or another deterministic shard mapping. The topology fingerprint records the exact mapping.
- Each rank has local volatile GPU KV state and may have a local durable cache tier. An optional independent blob store can hold rank manifests and pages.
- A small strongly consistent authority can issue epochs and atomically publish commit certificates. This authority is not limited to the two execution nodes.
- The service retains enough compact request state to reconstruct the prompt/input identity and the sequence of already emitted tokens.
- Checkpoints occur only at coherent logical boundaries where both ranks agree on sequence position and no collective is partially applied.

## Definitions

**Rank-local cache state** is the subset of KV tensors owned by one logical rank under one topology. It is not a complete model cache unless that topology has only one rank.

**Logical position** is the protocol’s canonical progress marker. It should include at least processed token count, sequence/beam identity, cache block frontier, and any sliding-window base needed to disambiguate identical token counts.

**Committed checkpoint** is an authoritative commit certificate plus a valid global manifest and all required rank objects at the declared durability class.

**Single-node continuation** means producing additional model outputs while only one execution node is available. Administrative recovery work on one node does not count as continuation.

**Exact resume** means reusing cache bytes without numerical reconstruction. **Rebuild** means recomputing cache bytes from compact semantic state and known tokens.

## Non-goals

- Defining the model framework’s tensor serialization ABI or a universal conversion between arbitrary attention backends.
- Providing Byzantine consensus among compromised control-plane authorities.
- Preserving availability when the complete model cannot execute on the remaining resources.
- Making uncommitted state durable by inference from timestamps or local file presence.
- Hiding all access-pattern or timing side channels from a fully compromised host.
- Treating GPU memory, kernel state, or a partially completed collective as crash-consistent storage.
- Guaranteeing bitwise-identical recomputation across arbitrary hardware, kernels, or numerical modes without an implementation-specific determinism contract.

## Design boundary

This package specifies protocol semantics and testable invariants. Concrete byte limits, checkpoint intervals, cryptographic key management, object-store consistency, GPU serialization format, and SLOs require implementation benchmarking and deployment policy.
