---
section_id: "39"
title: "Coordinator and Rank Architecture Facts"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689", "ROCm HIP 7.2 documentation"]
  software_versions: ["HIP docs 7.2.53211"]
  hardware_revisions: []
related_sections: ["32", "45", "46", "48"]
---

# Facts and constraints

- **[VERIFIED]** `llama.cpp` server documents `server_context` as primary inference state, `server_slot` as a sequence/request abstraction, and thread-safe task/response queues; compatible slots contribute tokens to a shared batch before `llama_decode` [S39-02].
- **[VERIFIED]** The current RPC backend exposes remote ggml devices and is explicitly proof-of-concept and insecure [S39-03]. It does not supply the authenticated, versioned control contract proposed here.
- **[VERIFIED]** HIP graph lifecycle is create/capture, instantiate once, launch repeatedly, then destroy; setup is not part of repeated launch cost [S39-04]. Whether a complete dynamic LLM decode graph can be updated/reused on HaloFPX remains unmeasured.
- **[VERIFIED]** Current `llama.cpp` supports multiple slots, shared batching, tokenizer/sampler logic in the server path, and backend scheduling [S39-02]. These are source patterns, not a mandate to preserve process boundaries.

## State ownership

| Scope | Authoritative state | May be cached elsewhere |
|---|---|---|
| Global/model | protocol version, runtime/build ID, model and tokenizer hashes, quantization/shard manifest, rank topology, mode policy | immutable copy on every rank |
| Rank-local | device/backend context, shard tensors, collective endpoints, staging buffers, graph templates/executables, rank KV/cache files, telemetry | coordinator metadata only |
| Session-local | session ID/epoch, owner mode/ranks, prompt-token history hash, sampler state/RNG counter, KV identities, accepted token position | worker lease/sequence metadata |
| Request-local | request ID, deadline, sampling override, input delta, output budget, cancellation, trace ID | transient command records |

**[RECOMMENDATION]** One authority must exist for every mutable field. Tokenization and stochastic sampling belong on the coordinator so ranks cannot diverge on textual normalization, vocabulary mapping, or RNG consumption. Ranks return logits/partials or verified token results as required by the mode.

## Lifetime constraints

- Process: backend/device discovery through shutdown.
- Model: loaded shard and immutable weight buffers; destroyed only after all sessions drain or abort.
- Graph family: keyed by exact model/shard/backend, phase, batch bucket, context bucket, KV layout, quant kernels, and collective plan.
- Session: KV/cache and sampler state until explicit close, expiry, or checkpoint transfer.
- Iteration: command descriptors, token microbatch, temporary activation/collective buffers.

**[INFERENCE]** Reusing a graph with incompatible pointers, shapes, backend, or collective plan risks incorrect execution; therefore graph-cache identity must be strict and a mismatch must rebuild rather than reuse. This follows from HIP graph lifecycle and LLM shape variability.
