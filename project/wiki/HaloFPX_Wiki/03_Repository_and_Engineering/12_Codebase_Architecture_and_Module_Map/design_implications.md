---
section_id: "12"
title: "Codebase Architecture Design Implications"
status: "draft"
last_verified: "2026-07-16"
applies_to:
  repositories:
    - "ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689"
    - "charlie12345/ROCmFPX@a5605a72768c6562241b248e268e33dc92787394"
    - "fewtarius/CachyLLama@6be745998f568e379ea197fcf827baec73ff9940"
    - "fewtarius/llama-ai@1017f3dfdce3ca2b06aa9007b23295db3bb35722"
  software_versions: []
  hardware_revisions: []
related_sections: ["13", "14", "15", "32", "36", "39", "51", "56", "57"]
---

# Design implications

## Recommended layer boundaries

**[RECOMMENDATION]** Keep HaloFPX as a reviewed patch stack over one frozen ROCmFPX baseline, with upstream-tracking commits separated from Halo-specific commits. Do not create a blended source snapshot without ancestry and patch provenance. Section 11 selects the baseline; section 15 owns synchronization policy.

**[RECOMMENDATION]** Use five explicit ownership layers:

1. `ggml/libllama core` - model semantics, graph construction, state APIs.
2. `ROCmFPX formats/backends` - type ABI, reference math, HIP/Vulkan kernels.
3. `Halo distributed adapters` - topology planner, rank protocol, transport/backend integration.
4. `HaloKV` - compatibility-fingerprinted, rank-local persistence behind a narrow state interface.
5. `product/operations` - server API, admission, profiles, packaging, services.

**[INFERENCE]** This decomposition follows existing seams but does not claim they are clean plugins. ROCmFPX crosses layers 1-2 and CachyLLama crosses layers 1, 4, and 5, so integration commits must state which owner accepts each shared-file modification. [S12-009][S12-010]

## Extension points to prefer

| Need | Preferred seam | Avoid |
|---|---|---|
| New weight format | GGML type/trait/reference path plus backend-specific kernels and quantization tests | Server-specific recognition of tensor types |
| Backend/device discovery | Backend registry and device/buffer interfaces | Hard-coded global device lists in product code |
| Dual-node transport | A versioned backend/transport adapter adjacent to RPC, with explicit capabilities | Encoding link policy inside model graph builders |
| Placement policy | A plan manifest translated to scheduler/backend assignments | Inferring topology solely from CLI device order |
| Persistent state | Versioned HaloKV adapter around sequence-state capture/restore | Treating raw state blobs as portable files |
| Request/session identity | Server task/session layer | Content hashes as the only privacy boundary |
| Hardware profiles | Outer runner/config package | Compile-time constants spread across kernels |

## ROCmFPX integration

**[RECOMMENDATION]** Treat each ROCmFPX type as a vertical feature slice with a manifest listing its numeric type ID, block layout, GGUF behavior, quantizer, CPU reference, HIP ops, Vulkan ops, unsupported operations/fallbacks, tests, and known model restrictions.

**[RECOMMENDATION]** Require a type-support matrix generated from both source inspection and runtime backend-op checks. A green build is not an acceptance gate.

**[OPEN]** Whether HIP, Vulkan, or a per-model mixture is the default on `gfx1151` depends on sections 24-27, 32, 37, and 74; this map contains no performance measurement.

## Persistent cache and state

**[RECOMMENDATION]** HaloKV records should include, at minimum, exact model-content hash, tokenizer/GGUF identity, runtime commit, state-format version, architecture, context parameters, K/V types, backend, rank/topology identity, speculative target/draft identities, and an integrity checksum. Restore must reject or recompute on any incompatible or corrupt record.

**[RECOMMENDATION]** Make rank ownership explicit: each rank persists only state it owns; a coordinator record references rank-local commits and is usable only when all required records validate. A missing rank record causes a miss/recompute or documented single-node fallback, never partial acceptance.

**[INFERENCE]** CachyLLama's use of the libllama sequence-state API is the least invasive reusable seam, but its server-resident identity/tiering logic should be ported behind a HaloKV interface rather than transplanted wholesale into a new distributed `server-context.cpp`. [S12-005][S12-010]

## Scheduler, RPC, and distributed execution

**[VERIFIED]** Existing RPC remote devices participate through the backend registry and scheduler. [S12-003][S12-004]

**[INFERENCE]** That mechanism is a useful execution substrate but not the full Halo distributed runtime: it does not establish product-level rank lifecycle, dual-link path selection, collective semantics, failure recovery, or degraded-mode behavior.

**[RECOMMENDATION]** Define a capability contract before altering RPC:

- protocol/version and peer authentication;
- device/buffer/op capability enumeration;
- ordering, completion, timeout, retry, and idempotency;
- link selection/striping and integrity;
- rank loss behavior and single-node fallback;
- observability for bytes, copies, stalls, and per-link errors.

## Speculative decoding

**[VERIFIED]** Common speculative code owns target/draft compatibility and draft initialization, while the server owns lifecycle and slot integration. [S12-007]

**[RECOMMENDATION]** Keep remote speculative execution above the core algorithm: the target/draft adapter should expose tokens/probabilities, state checkpoints, cancellation, and timing without making the sampler transport-aware.

**[RECOMMENDATION]** Persist target, draft, sampler/RNG, recurrent, and MTP state as one compatibility unit when required by the selected method. A target-only cache hit must not be reported as a complete speculative checkpoint.

## Tests and packaging

**[RECOMMENDATION]** Preserve upstream unit tests, add fork-delta tests near the owning module, and keep two-node/fault/benchmark tests in project `experiments/` with raw environment metadata.

**[RECOMMENDATION]** Build HIP and Vulkan as separately identifiable artifacts from the same commit and emit a manifest containing compiler, ROCm/Mesa, CMake options, enabled backends, linked libraries, and source hashes. Packaging detail is authoritative in section 16.

## Decisions contingent on machine evidence

- **[OPEN]** Default backend and format per target model.
- **[OPEN]** Whether upstream RPC is retained, wrapped, or replaced for dual-link transport.
- **[OPEN]** Granularity and timing of rank-local persistence.
- **[OPEN]** Scheduler placement strategy for replication, speculation, tensor parallel, pipeline, and MoE hybrid modes.
- **[OPEN]** Which state formats can survive restart, build change, backend change, or topology change.

