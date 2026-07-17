---
title: Glossary
description: Terms used throughout the integration design.
status: Reference
evidence-date: 2026-07-17
canonical-repository: charlie12345/ROCmFPX
---

# Glossary

> [!NOTE]
> Evidence is pinned to **2026-07-17**. Repository heads and source links are locked in [[Source-Register]] and `evidence/commit-lock.json`.


| Term | Meaning |
|---|---|
| **Canonical** | The repository whose `main` is the product integration and release authority: ROCmFPX. |
| **Upstream** | `ggml-org/llama.cpp`, the common base whose generic changes are synchronized normally. |
| **Donor** | `fewtarius/CachyLLama`, inspected for selected behavior but never merged wholesale. |
| **Parent** | `fewtarius/llama-ai`, a GPL-3.0 project that consumes/points to the donor engine. |
| **Provider** | A replaceable implementation of a canonical interface, selected by feature/configuration. |
| **State codec** | Capture/validation/restore logic for target, draft, speculative, attention, and recurrent state. |
| **Scope** | An isolation boundary for cache lookup, such as explicit tenant/user or anonymous conversation. |
| **Checkpoint** | A token boundary plus serialized model/context state required to resume without recomputing the prefix. |
| **Entry** | One committed, all-or-nothing persistent checkpoint transaction. |
| **Cold fallback** | Reject cache reuse and evaluate the prompt from canonical model state. |
| **P0–P3** | Provenance confidence levels defined in [[Repository-and-Provenance]]. |
| **CP/MP/IF/CR** | Cherry-pick, manual port, interface adaptation, and clean-room reimplementation. |
| **Lane** | A buildable/bisectable patch series with one owner and rollback marker. |
| **Evidence lock** | Immutable commit/blob IDs observed on the stated date. |


---

**Wiki navigation:** [[Home]] · [[Executive-Decision]] · [[Capability-Decision-Matrix]] · [[Patch-Lanes-and-Dependency-Graph]] · [[Acceptance-Criteria]]
