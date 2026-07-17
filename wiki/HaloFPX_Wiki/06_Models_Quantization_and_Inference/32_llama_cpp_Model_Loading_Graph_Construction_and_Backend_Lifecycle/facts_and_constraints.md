---
section_id: "32"
title: "llama.cpp lifecycle facts"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["ggml-org/llama.cpp"]
  software_versions: ["788e07d"]
  hardware_revisions: []
related_sections: ["33", "61", "68"]
---

# Source path

| Stage | Current source authority | Verified behavior at `788e07d` |
|---|---|---|
| model open/load | `src/llama.cpp`, `src/llama-model-loader.*`, `src/llama-model.cpp` | load requires registered backends unless vocab-only; handles path/splits/file/metadata and cleans up on error |
| metadata/architecture | `src/llama-arch.*`, per-architecture `src/models/*` | metadata drives hparams and model-specific graph builder |
| tensor placement | `src/llama-model.cpp` and backend buffer selection | tensors are created/assigned to buffer types; weight location informs scheduler preference |
| context/state | `src/llama-context.cpp`, `src/llama-kv-cache.*`, recurrent state modules | parameters resolve context/RoPE/batch; scheduler and cache/state are allocated |
| graph construction | `src/llama-graph.*`, `src/models/*` | graph inputs and per-architecture forward graph built per ubatch/operation |
| scheduling/execution | `src/llama-context.cpp`, `ggml/src/ggml-backend.cpp` | scheduler reset, callback, graph allocation, async compute, and synchronization as required |
| decode outputs | `llama_context::decode` | batches are split into ubatches; logits/embeddings copied from selected backend buffers |
| serialization | `src/llama-context.cpp` and public APIs in `include/llama.h` | public get/set state and sequence-state APIs delegate to context state read/write logic |
| sampling | `src/llama-sampler.cpp`, `common/sampling.cpp` | sampler chain consumes logits; sampler state affects reproducibility |
| server slots | `tools/server/server-context.cpp`, `server-task.*` | slot lifecycle controls prompt ingestion, context shift, decode, response, cancellation and cache reuse |

**[VERIFIED]** Context construction reserves scheduler graphs for prompt and token-generation shapes, and decode resets/allocates scheduler graphs before async execution [S32-02]. Graph shape/reuse depends on batch, context, outputs, cache cells, architecture and backend capabilities.

**[VERIFIED]** Upstream state APIs expose a serialization mechanism, but a byte blob is not automatically a stable cross-commit, cross-quant, cross-topology persistent-cache format [S32-04].

# Failure and fallback constraints

- Load cancellation/error frees the partially created model.
- Backend scheduler placement may copy or split graphs; distributed ownership cannot be inferred from `n_gpu_layers` alone.
- Pipeline-parallel async work may still be running when the next decode starts, so synchronization/lifetime rules matter.
- Server slot prompt caching and engine state serialization are related but distinct mechanisms.
- Sampling/RNG, grammar, recurrent state, KV cells, token history, RoPE positions and slot policy all affect continuation fidelity.
