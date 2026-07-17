---
title: Capability Decision Matrix
description: Per-capability integration treatment and gating rationale.
status: Proposed
evidence-date: 2026-07-17
canonical-repository: charlie12345/ROCmFPX
---

# Capability Decision Matrix

> [!NOTE]
> Evidence is pinned to **2026-07-17**. Repository heads and source links are locked in [[Source-Register]] and `evidence/commit-lock.json`.


## Decision key

- **CP** — cherry-pick exact commit.
- **MP** — manual port with attribution.
- **IF** — adapt behind a canonical interface; implementation may be MP or clean-room.
- **CR** — clean-room reimplementation from approved requirements/tests.
- **NA** — no donor integration; consume through normal upstream synchronization.
- **DEFER** — not selected for the first program.

> [!IMPORTANT]
> No capability currently has unconditional **CP** approval. A row that mentions a possible cherry-pick is still blocked by the P2/P3 provenance gate.

## Matrix

| ID | Capability | Canonical state | Treatment | Lane | Default flag state | Rationale and evidence |
|---|---|---|---|---|---|---|
| C01 | Cross-restart conversation checkpoint persistence | ROCmFPX disk cache is deliberately per-run and cleaned on shutdown. | **IF + MP** | L05 | Compiled off, runtime off | Preserve the current cache as an adapter; add persistence as a separate provider. [S07] [S08] [S09] [S12] |
| C02 | Hot/warm/cold retention and bounded checkpoint LRU | Canonical has disk LRU but not donor-style persistent tiering. | **IF + MP** | L05 | Runtime off | Port policy semantics, not donor containers or server integration. Donor structures couple policy, storage, and native records. [S14] [S15] |
| C03 | Kernel readahead/prefetch | Canonical current path focuses on durable save/drop behavior; donor exposes prefetch helpers. | **MP**, with **CP candidate** only after exact isolated commit is P3 | L05 | Runtime automatic only for persistent provider | Small platform abstraction can remain isolated; must not pull the surrounding donor cache. [S15] |
| C04 | Global system-prefix cache | Not present as a persistent global pool. | **IF + MP** | L07 | Off | Separate namespace/provider; explicit or template-derived boundary only. Donor heuristic boundary detection is not adopted blindly. [S12] [S16] |
| C05 | Hybrid attention + recurrent-state checkpoint restore | Canonical already serializes target/draft/speculative state for MTP, but lacks the donor's explicit attention-only removal API. | **IF + MP**, **CR fallback** | L08 | Off | High-risk state semantics; implement capability contract first and test exact boundaries. [S10] [S20] [S21] |
| C06 | Public `llama_memory_seq_rm_attn_only` API | No equivalent in observed canonical public header. | **MP to internal interface first** | L08 | No public ABI | Avoid premature C ABI. Promote only after upstream discussion and ABI acceptance. [S20] [S21] |
| C07 | Cross-conversation checkpoint matching and overflow safety | Canonical has prompt-cache matching; donor adds conversation-aware recurrent constraints. | **CR from safety contract** | L06/L08 | Off | Correctness/privacy-critical matching should be specified independently and verified with adversarial tests, not transplanted from coupled donor code. [S12] [S18] |
| C08 | Tenant/user cache namespace | Not first-class in observed canonical server flow. | **IF + MP** | L09 | Off | Route an opaque scope key through request → scheduler → store; never mix explicit and anonymous scopes. [S19] |
| C09 | Per-user concurrency cap and HTTP 429 | Global server scheduling exists; donor design adds a per-user limit. | **MP** | L10 | `0` / disabled | Server-local behavior; port after identity plumbing, with race-safe authoritative check. [S19] |
| C10 | User-aware slot affinity | Canonical allocator has its own LCP/LRU behavior. | **MP** | L11 | Off / `prefer` opt-in | Add as a scoring hint, not a hard partition; keep separate from concurrency for bisectability. [S19] |
| C11 | Expert activation telemetry (C API + HTTP) | No selected canonical provider seam established. | **IF + CR/MP after provenance** | L12 | Compile/runtime off | Hot-path instrumentation and ABI changes require an internal telemetry provider first. [S12] [S22] |
| C12 | Small-AMD-GPU Vulkan submission tuning | Relevant upstream change is already in ROCmFPX history. | **NA** | L14 verification | Existing behavior | Track through upstream sync; do not import an older donor variant. [S26] |
| C13 | CPU ISA auto-detection build wrapper | Behavior is associated with the GPL parent project. | **CR** | L13 | Opt-in preset | Reimplement requirements in ROCmFPX-owned scripts/CMake without copying GPL source. [S12] [S23] |
| C14 | MLA/DeepSeek model support | DeepSeek model paths already exist in canonical tree. | **NA** | L14 verification | Existing behavior | Prove a specific missing differential before opening a donor lane. [S27] |
| C15 | Donor v3/v1 cache-format migration | Canonical has a different ephemeral layout. | **CR, offline read-only importer only** | L06 optional | Compile/runtime off | Never make server startup parse donor native records by default. Importer must write the new canonical format. [S14] [S15] [S16] |
| C16 | Page-level SSD paging for very large context | Donor exposes a page manager API; production call-site/test maturity was not established in this review. | **DEFER** | Research | Not built | Persistent whole-checkpoint storage can ship without page-level eviction; avoid making an unproven subsystem foundational. [S17] [S18] |
| C17 | Donor `--cache-ssd*` CLI compatibility | Canonical already uses `--cache-disk*` with different lifecycle semantics. | **DEFER / explicit alias late** | L15 | Off | Silent aliasing would confuse persistent and per-run semantics. [S08] [S12] |
| C18 | Donor benchmark claims | Canonical acceptance data not yet collected. | **CR benchmark reproduction** | Validation | N/A | Reproduce on controlled hardware; donor figures are context, not pass/fail evidence. [S12] |

## Direct cherry-pick decision

The present roster is:

```text
approved donor cherry-picks: none
upstream-owned/no-op item: 0bbc87b163ff7826656b1024dac5703e3f2bd6b6 (already present)
```

A later provenance review may promote C03 or another isolated helper to CP. Such promotion changes the matrix and requires an ADR or recorded maintainer decision.


---

**Wiki navigation:** [[Home]] · [[Executive-Decision]] · [[Capability-Decision-Matrix]] · [[Patch-Lanes-and-Dependency-Graph]] · [[Acceptance-Criteria]]
