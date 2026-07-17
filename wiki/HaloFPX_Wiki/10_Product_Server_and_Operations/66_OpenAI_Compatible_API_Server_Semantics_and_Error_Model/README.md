---
section_id: "66"
title: "OpenAI-Compatible API, Server Semantics, and Error Model"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["HaloFPX project", "ggml-org/llama.cpp", "openai/openai-openapi"]
  software_versions: ["llama.cpp 788e07d", "OpenAI OpenAPI db3e531"]
  hardware_revisions: ["two-node Strix Halo target"]
related_sections: ["09", "29", "32", "36", "46", "48", "60", "67", "68", "69", "71"]
---

# API contract

**[RECOMMENDATION]** HaloFPX should advertise an explicitly tested OpenAI-compatible subset, not blanket parity. The v1 contractual surface should be:

- `GET /v1/models`
- `POST /v1/chat/completions` with streaming and non-streaming responses
- `POST /v1/completions` only for clients that require it
- `POST /v1/responses` as a documented compatibility adapter
- token counting, health/readiness, metrics, and protected HaloFPX administration under separately documented routes

Embeddings, reranking, multimodal input, and Anthropic-compatible routes are optional profiles, not implied v1 support.

**[VERIFIED]** Pinned `llama-server` documents Chat Completions, Completions, Responses, Embeddings, Models, health, metrics, tool use, structured JSON, and streaming, while qualifying compatibility and implementing Responses by conversion to Chat Completions [S66-01].

See [endpoint facts](facts_and_constraints.md), [extensions and error model](design_implications.md), and [conformance checks](procedures_and_checks.md).

## Research split

- Internet/source research: official OpenAI error/streaming/tool/structured-output docs, OpenAPI head, and pinned llama.cpp behavior.
- Machine work: byte-level conformance, streaming cancellation/faults, tool/schema quality, overload/retry, and distributed error propagation.
- Contingent decisions: final endpoint subset, Responses fidelity, embeddings, extension names, and retry/idempotency policy.

