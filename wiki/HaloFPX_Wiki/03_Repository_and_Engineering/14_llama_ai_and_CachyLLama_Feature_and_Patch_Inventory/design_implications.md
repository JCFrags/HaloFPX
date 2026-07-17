---
section_id: "14"
title: "Design Implications for HaloFPX"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories:
    - "fewtarius/llama-ai@1017f3dfdce3ca2b06aa9007b23295db3bb35722"
    - "fewtarius/CachyLLama@6be745998f568e379ea197fcf827baec73ff9940"
  software_versions: []
  hardware_revisions:
    - "planned dual matched AMD Strix Halo nodes"
related_sections: ["13", "15", "16"]
---

# Design Implications for HaloFPX

## Recommended port boundary

**[RECOMMENDATION]** Treat CachyLLama as a design/reference donor and `llama-ai` as an operational-policy donor. Do not base HaloFPX directly on either moving branch. Port capabilities as reviewable commits onto the frozen ROCmFPX/llama.cpp base selected by sections 11, 13, and 15.

| Disposition | Capability | Reason |
|---|---|---|
| port first, redesigned | rank-local exact-prefix checkpoint store | high user value; separable; must add atomic writes, checksums, and a stronger compatibility manifest |
| port with tests | target state serialization; attention-only hybrid cleanup | necessary for recurrent correctness; runtime-specific |
| defer | MTP/draft/speculative state blobs | useful but expands ABI and equivalence matrix substantially |
| defer | fuzzy cross-conversation continuation | incorrect acceptance is worse than a miss; 4,096-token evidence is too weak for general reuse |
| scope before port | system-prompt cache | valuable static-prefix reuse, but current global scope may cross tenant boundaries |
| redesign before use | `llama_user_id`, slot affinity, concurrency cap | identity must come from trusted authentication; pinned release ordering loses ownership before decrement and may expose residual slot state |
| optional | expert activation telemetry | instrumentation only; do not couple to placement until overhead and accuracy are measured |
| rederive | `llama-run.sh` profiles | values are device/model observations, not universal configuration |
| reuse as fixture idea | benchmark cold/warm phases and raw logs | useful structure, but tighten cache-state assertions and provenance |

## Required cache identity

**[RECOMMENDATION]** A HaloFPX checkpoint key should cover at least:

`model content hash + tokenizer hash + chat-template hash + runtime commit + cache format version + K/V types + backend/device layout + context parameters + target/draft/MTP identities + rank and world-size + tensor/pipeline partition + user/security scope`.

**[INFERENCE]** The CachyLLama compatibility hash is too narrow for transfer between HaloFPX ranks or builds. A matching architecture shape does not establish that opaque serialized bytes have the same semantics.

**[RECOMMENDATION]** Invalid, short, checksum-failing, version-mismatched, or topology-mismatched records must be quarantined or ignored as a cache miss. They must never be accepted as state. This is a hard design invariant.

## Distributed ownership and fallback

**[ASSUMPTION]** HaloFPX may execute replicated, speculative, tensor-parallel, pipeline-parallel, or MoE-aware modes. Each produces different state ownership.

**[RECOMMENDATION]** Keep checkpoint files rank-local and encode topology in the manifest. A coordinator may publish a small logical checkpoint descriptor, but each rank should restore only its own shard. Restoration succeeds only if every required rank validates its shard; otherwise all ranks recompute the prefix.

**[RECOMMENDATION]** Define single-node fallback explicitly:

- replicated mode: a compatible surviving node may use its local checkpoint;
- tensor/pipeline parallel: sharded state is not assumed usable on one node unless an explicit conversion path exists;
- remote speculative mode: target and draft/MTP state have separate identities, and loss of draft state falls back to non-speculative target decoding;
- MoE-aware placement: expert telemetry is advisory and checkpoint validity must not depend on mutable placement unless placement is in the key.

## Matching policy

**[RECOMMENDATION]** Phase 1 should allow only exact token-prefix matches inside the same authenticated user/session namespace. Store a full token digest and enough tokens to prove the restored prefix, not merely the first 4,096.

**[RECOMMENDATION]** Add fuzzy/cross-conversation continuation only after differential tests show logit equivalence for every supported architecture. For recurrent models, partial LCP does not imply valid later recurrent state; recompute from the last proven boundary.

**[RECOMMENDATION]** System-prompt entries should be scoped as one of: public deployment prompt, tenant prompt, or user prompt. Never infer shareability from token equality alone when the prompt can contain secrets.

## Durability and concurrency

**[RECOMMENDATION]** Write payload and manifest to unique temporary files, flush payload, atomically rename, flush the directory, then publish the index. Use a cryptographic digest per blob and bounded lengths before allocation. Recovery should rebuild the index from validated records.

**[RECOMMENDATION]** `no_fsync` may exist only as an explicitly unsafe performance mode with a visible metric and fault-injection test. It must not be the HaloFPX default.

**[RECOMMENDATION]** User identity must be injected by a trusted gateway; reject client attempts to override it. Concurrency accounting needs a single lifecycle (reserve before enqueue, release exactly once on every completion/cancel/error path) and a separately specified anonymous/global policy.

## Profile and telemetry policy

**[RECOMMENDATION]** Convert `llama-run.sh` heuristics into versioned data profiles after measurements. Record model SHA-256, GGUF architecture metadata, backend, context, cache sizes, slots, quantization, topology, and rationale. Do not match only filenames.

**[RECOMMENDATION]** Expert telemetry should be disabled by default. Before use, measure counter accuracy, synchronization overhead, memory growth, endpoint exposure, reset semantics, and behavior under batched/multi-rank execution.

## Dependencies on other wiki sections

- Section 11 chooses frozen repository pins.
- Section 13 determines which ROCmFPX cache/state/kernel behavior survives integration.
- Section 15 owns patch ordering and upstream synchronization.
- Section 16 owns license, CI, build, and agent workflow controls.
- Distributed execution and transport sections must define rank ownership before persistent state can graduate from local experiment to product design.
