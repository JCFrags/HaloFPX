---
title: ADR-004 — Donor Parent GPL Boundary
description: Architecture decision record requiring clean-room treatment for parent-project behavior.
status: Proposed
evidence-date: 2026-07-17
canonical-repository: charlie12345/ROCmFPX
---

# ADR-004 — Donor Parent GPL Boundary

> [!NOTE]
> Evidence is pinned to **2026-07-17**. Repository heads and source links are locked in [[Source-Register]] and `evidence/commit-lock.json`.


## Context

The CachyLLama engine is MIT, while `fewtarius/llama-ai` is GPL-3.0. The parent contains runner/build behavior such as hardware/ISA detection referenced by the engine README. [S12] [S13] [S23]

## Decision

Parent-project behavior may inform requirements and black-box tests only. Any equivalent ROCmFPX build/runner feature is clean-room reimplemented from an approved specification.

## Consequences

- No parent source is copied, translated, or diffed into ROCmFPX.
- Clean-room attestations are mandatory for L13.
- If maintainers choose to distribute GPL tooling separately, that is a separate packaging/license decision outside this integration program.


---

**Wiki navigation:** [[Home]] · [[Executive-Decision]] · [[Capability-Decision-Matrix]] · [[Patch-Lanes-and-Dependency-Graph]] · [[Acceptance-Criteria]]
