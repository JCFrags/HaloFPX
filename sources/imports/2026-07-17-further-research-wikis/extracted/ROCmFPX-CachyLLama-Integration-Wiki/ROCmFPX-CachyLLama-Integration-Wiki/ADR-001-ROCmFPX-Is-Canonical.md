---
title: ADR-001 — ROCmFPX Is Canonical
description: Architecture decision record fixing repository authority and merge direction.
status: Proposed / Accepted when maintainer signs
evidence-date: 2026-07-17
canonical-repository: charlie12345/ROCmFPX
---

# ADR-001 — ROCmFPX Is Canonical

> [!NOTE]
> Evidence is pinned to **2026-07-17**. Repository heads and source links are locked in [[Source-Register]] and `evidence/commit-lock.json`.


## Context

ROCmFPX carries quantization/backend/MTP/server changes and its own tested disk-cache behavior. CachyLLama tracks llama.cpp closely while adding persistent-cache and APU-oriented behavior. A naive three-way fork merge would make both forks peers and repeatedly reintroduce conflicts.

## Decision

`charlie12345/ROCmFPX` is the sole canonical integration target. `ggml-org/llama.cpp` is upstream. `fewtarius/CachyLLama` is a donor. `fewtarius/llama-ai` is a GPL parent metadata source.

## Consequences

- All implementation lanes start from a frozen ROCmFPX base.
- No donor merge commit is admitted to canonical history.
- Generic upstream-equivalent behavior is obtained via upstream synchronization.
- Donor behavior is integrated by CP/MP/IF/CR decisions.
- Conflict resolution defaults to preserving canonical owned behavior.

## Rejected alternatives

- **Use CachyLLama as base and reapply ROCmFPX:** would invert product ownership and risk ROCmFPX quant/backend regressions.
- **Maintain permanent bidirectional merges:** produces non-bisectable conflict accumulation and ambiguous source authority.
- **Vendor donor cache wholesale:** overwrites canonical server/cache safety and fixes the donor format as an accidental ABI.


---

**Wiki navigation:** [[Home]] · [[Executive-Decision]] · [[Capability-Decision-Matrix]] · [[Patch-Lanes-and-Dependency-Graph]] · [[Acceptance-Criteria]]
