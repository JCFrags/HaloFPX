---
title: ADR-002 — Provider-First Cache Integration
description: Architecture decision record for preserving the existing cache while adding persistence.
status: Proposed
evidence-date: 2026-07-17
canonical-repository: charlie12345/ROCmFPX
---

# ADR-002 — Provider-First Cache Integration

> [!NOTE]
> Evidence is pinned to **2026-07-17**. Repository heads and source links are locked in [[Source-Register]] and `evidence/commit-lock.json`.


## Context

ROCmFPX has a tested per-run disk prompt cache integrated with current target/draft/spec state. CachyLLama offers persistent per-conversation storage with different modules, matching, and formats. [S07] [S08] [S14] [S18]

## Decision

Introduce a canonical state-codec/store/match provider seam. Adapt the current ROCmFPX cache first. Add persistent storage as a separate provider behind default-off gates.

## Consequences

- Existing flags and lifecycle remain stable.
- Persistent behavior can be disabled without removing the abstraction.
- Matching, retention, tenant routing, and serialization are independently testable.
- Donor code cannot dictate public API or on-disk format.

## Rejected alternatives

- Replace `server_prompt_cache` with donor classes in one patch.
- Add persistence directly inside current disk methods without a lifecycle selector.
- Reuse donor CLI names while changing canonical semantics.


---

**Wiki navigation:** [[Home]] · [[Executive-Decision]] · [[Capability-Decision-Matrix]] · [[Patch-Lanes-and-Dependency-Graph]] · [[Acceptance-Criteria]]
