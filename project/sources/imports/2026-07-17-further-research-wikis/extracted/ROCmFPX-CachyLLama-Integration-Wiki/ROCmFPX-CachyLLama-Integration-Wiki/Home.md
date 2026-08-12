---
title: ROCmFPX × CachyLLama Integration Wiki
description: Canonical-fork integration and upstream-synchronization design for selected CachyLLama behavior.
status: Proposed — implementation gated
evidence-date: 2026-07-17
canonical-repository: charlie12345/ROCmFPX
---

# ROCmFPX × CachyLLama Integration Wiki

> [!NOTE]
> Evidence is pinned to **2026-07-17**. Repository heads and source links are locked in [[Source-Register]] and `evidence/commit-lock.json`.


> [!IMPORTANT]
> **Canonical decision:** `charlie12345/ROCmFPX` remains the only merge target. `ggml-org/llama.cpp` is the upstream synchronization authority. `fewtarius/CachyLLama` is a read-only donor used for capability evidence—not a branch to merge. The requested public path `llama-ai/CachyLlama` could not be resolved; the public engine linked by the `fewtarius/llama-ai` project is `fewtarius/CachyLLama`. [S01] [S11] [S24]

## Executive position

ROCmFPX already contains a hardened **per-run** SSD prompt-cache path with target/draft/speculative-state handling, owner-marked run directories, atomic file replacement, conservative failure behavior, bounded LRU storage, and platform tests. CachyLLama adds a materially different requirement: **persistent, cross-restart** checkpoint reuse plus policy features such as global system-prefix reuse, tenant-aware routing, hybrid recurrent-state handling, and expert telemetry. [S06] [S07] [S08] [S09] [S10] [S12]

The correct integration shape is therefore **provider-first**:

1. Preserve ROCmFPX's existing cache implementation and command-line semantics.
2. Add a neutral checkpoint-store interface with ROCmFPX's current cache as the first adapter.
3. Introduce a new, versioned persistent provider behind compile-time and runtime gates.
4. Port selected donor behavior lane by lane; do not import the donor's native-struct cache formats as the canonical format.
5. Keep every commit buildable, feature-off by default, and independently revertible.

## Current decision summary

| Area | Decision | Rationale |
|---|---|---|
| Donor branch integration | **Never merge wholesale** | Donor head mixes upstream and donor work; canonical server/cache code has already diverged. |
| Direct cherry-picks | **None approved yet** | Repository-level MIT compatibility is established, but exact capability commit ranges and upstream overlap are not yet attested. |
| Persistent cache | **Adapt behind interface + manual port** | ROCmFPX's current ephemeral cache must remain intact and rollback-safe. |
| GPL parent scripts | **Clean-room reimplementation only** | `fewtarius/llama-ai` is GPL-3.0 while the two engine repositories are MIT. [S02] [S13] [S23] |
| Cache format | **New ROCmFPX format** | Donor v3/v1 formats use native C++ records and lack the compatibility envelope required for a canonical cross-platform store. [S14] [S15] [S16] |
| AMD small-GPU Vulkan tuning | **No donor action** | The relevant upstream commit is already present in ROCmFPX history. [S26] |
| MLA/DeepSeek support | **Upstream-owned; verify, do not port by default** | ROCmFPX already contains DeepSeek model paths. [S27] |

## Wiki map

### Decisions and architecture

- [[Executive-Decision]] — scope, non-goals, and the approved integration posture.
- [[Repository-and-Provenance]] — repository identities, trust boundaries, and evidence limits.
- [[Capability-Decision-Matrix]] — per-capability integration method.
- [[Target-Architecture]] — provider boundaries and data/control flows.
- [[ADR-001-ROCmFPX-Is-Canonical]], [[ADR-002-Provider-First-Cache-Integration]], [[ADR-003-New-Canonical-Cache-Format]], [[ADR-004-Donor-Parent-GPL-Boundary]].

### Git and delivery

- [[Git-Topology]] — remotes, branches, tags, commit trailers, and quarantine refs.
- [[Patch-Lanes-and-Dependency-Graph]] — buildable lanes and dependency graph.
- [[Conflict-Map]] — hot files and deterministic resolution ownership.
- [[Integration-Order]] — phase gates and rollback points.
- [[Upstream-Synchronization]] — recurring upstream and donor-surveillance procedure.

### Runtime and operations

- [[Feature-Flags]] — compile-time/runtime gates and compatibility aliases.
- [[Cache-Format-Versioning]] — canonical persistent-store format and migration policy.
- [[Rollback-Procedure]] — operational and Git rollback.
- [[Operations-Runbook]] — canary, monitoring, failure containment, and cleanup.
- [[Acceptance-Criteria]] — evidence, build, correctness, security, performance, and rollback gates.

### Governance

- [[Licensing-and-Provenance-Gates]] — when cherry-pick/manual port/clean-room is allowed.
- [[Risk-Register]] — material risks, triggers, and owners.
- [[Source-Register]] — immutable source inventory.
- [[No-Patch-Notice]] — why this package intentionally contains no implementation patch.

## Recommended first merge milestone

The first implementation milestone should contain only:

- provenance records and synchronization metadata;
- cache behavior contract tests that pass against the existing ROCmFPX implementation;
- a provider interface plus a no-op/current-cache adapter;
- metrics and feature-state reporting;
- no persistent writer and no default behavior change.

That milestone is useful by itself, establishes a stable seam for later work, and gives a low-cost rollback point before any on-disk compatibility commitment.


---

**Wiki navigation:** [[Home]] · [[Executive-Decision]] · [[Capability-Decision-Matrix]] · [[Patch-Lanes-and-Dependency-Graph]] · [[Acceptance-Criteria]]
