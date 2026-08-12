# 10. Bounded auxiliary-role evaluation

## Decision matrix

| Role | Evidence | Linux status | Integration decision |
|---|---|---|---|
| Prompt classification | exact BF16 DistilBERT example | `[VENDOR-ONLY]` plausible | conditional isolated experiment only |
| Moderation | classifier-shaped, but no policy model proof | `[INFERENCE]` plausible | conditional only with task-specific validation |
| Embeddings | public BGE NPU example | `[UNKNOWN]` Linux qualification | keep excluded |
| Reranking | no exact captured Linux model/example | `[MISSING]` | keep excluded |
| Small draft model | 135M OGA models listed | `[VENDOR-ONLY]` model availability | keep excluded |
| `llama.cpp` transformer offload | iGPU path only | `[UNSUPPORTED]` on NPU | keep excluded |
| Whisper NPU | Windows-only, Linux planned | `[WINDOWS-ONLY]` | keep excluded |

## Prompt classification

[VENDOR-ONLY] The DistilBERT example is the strongest bounded evidence because it has a small fixed-shape encoder, explicit ONNX/BF16 path, simple outputs, and a public execution-provider configuration.

[UNKNOWN] The example does not establish target Linux packaging, graph placement, latency, energy, or reliability.

[DECISION] It is the only role eligible for a pre-authorized experiment.

## Moderation

[INFERENCE] Moderation can use the same technical shape as prompt classification.

[MISSING] A sentiment classifier does not supply moderation labels, policy coverage, multilingual behavior, calibration, safety thresholds, or production accuracy.

[DECISION] A moderation test must use a task-specific model and held-out policy dataset; it cannot inherit the sentiment example's correctness claim.

## Embeddings

[UNKNOWN] AMD's public BGE code uses the NPU execution provider and a static maximum length of 512.

[UNKNOWN] The captured RAG instructions use a Ryzen AI 1.7.0 hybrid environment and do not establish the exact Linux 1.7.1 target path.

[INFERENCE] Embedding economics also depend strongly on batching, document indexing cadence, tokenizer cost, vector-store placement, transfer overhead, and whether queries are latency sensitive.

[DECISION] Keep excluded until the smaller classifier gate passes and a Linux-qualified embedding manifest exists.

## Reranking

[MISSING] No exact supported Linux reranker model, compile recipe, execution-provider configuration, or operator assignment report was captured.

[DECISION] Keep excluded.

## Small draft model

[VENDOR-ONLY] AMD lists 135M SmolLM/SmolLM2 models in its versioned OGA support.

[MISSING] There is no captured interface for speculative decoding with the primary runtime, shared KV state, token acceptance, or low-overhead handoff.

[INFERENCE] Running a small model independently is not equivalent to a useful draft-model architecture.

[DECISION] Keep excluded.

## Product-path consequence

[DECISION] None of these roles justifies changing the main HIP/Vulkan architecture. The NPU remains a separately gated optional device.
