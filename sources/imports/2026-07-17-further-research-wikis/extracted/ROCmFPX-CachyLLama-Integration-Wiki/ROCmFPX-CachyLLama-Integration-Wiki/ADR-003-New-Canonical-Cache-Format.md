---
title: ADR-003 — New Canonical Cache Format
description: Architecture decision record rejecting donor native records as canonical persistence ABI.
status: Proposed
evidence-date: 2026-07-17
canonical-repository: charlie12345/ROCmFPX
---

# ADR-003 — New Canonical Cache Format

> [!NOTE]
> Evidence is pinned to **2026-07-17**. Repository heads and source links are locked in [[Source-Register]] and `evidence/commit-lock.json`.


## Context

Reviewed donor records use `KVRC` v3, `KVSM` v1, and `KVPG` structures with native fields/fixed arrays. ROCmFPX's current disk cache uses per-run state files and cleans them up. [S08] [S14] [S15] [S16] [S17]

## Decision

Define ROCmFPX Context Store v1 as a versioned directory/manifest/component format with strong model fingerprints, component digests, explicit required-feature negotiation, and transactional directory commit.

## Consequences

- Cross-platform/endian/layout behavior is specified independently of C++ ABI.
- Donor imports require an offline converter.
- Reader can ship and fuzz before writer.
- Rollback uses separate versioned roots and never in-place mutation.

## Rejected alternatives

- Treat donor `KV_SSD_VERSION=3` as canonical.
- Store raw native structs and rely on matching compiler/platform.
- Auto-migrate donor files during server startup.


---

**Wiki navigation:** [[Home]] · [[Executive-Decision]] · [[Capability-Decision-Matrix]] · [[Patch-Lanes-and-Dependency-Graph]] · [[Acceptance-Criteria]]
