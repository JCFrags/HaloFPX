---
section_id: "66"
title: "API Facts and Endpoint Matrix"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["ggml-org/llama.cpp", "openai/openai-openapi", "fewtarius/CachyLLama"]
  software_versions: ["llama.cpp 788e07d", "OpenAI OpenAPI db3e531", "CachyLLama 6be7459"]
  hardware_revisions: []
related_sections: ["09", "32", "60", "68", "69", "71"]
---

# API facts and endpoint matrix

## Verified inputs

- **[VERIFIED]** OpenAI publishes distinct Responses and Chat Completions surfaces, streaming guidance, function calling, structured-output guidance, and an HTTP error taxonomy [S66-02, S66-03, S66-04, S66-05].
- **[VERIFIED]** Pinned `llama-server` says its OpenAI-style endpoints do not make a strong claim of full compatibility; `/v1/responses` is converted internally to Chat Completions [S66-01].
- **[VERIFIED]** `llama-server` documents `/health` returning 503 while loading and 200 when ready; health is public in that upstream implementation [S66-01].
- **[VERIFIED]** It exposes metrics only when enabled, supports API-key and TLS inputs, and returns an OpenAI-shaped `error` object plus llama.cpp-specific error types [S66-01].
- **[VERIFIED]** CachyLLama adds a request `llama_user_id`, per-user concurrency limits, HTTP 429, and slot affinity. These are fork extensions, not OpenAI API fields [S66-06].

## Proposed endpoint contract

| Endpoint | v1 status | Core semantics | Notes |
|---|---|---|---|
| `GET /v1/models` | Required | List only admitted/visible model IDs and capabilities | No filesystem paths |
| `POST /v1/chat/completions` | Required | Stream/non-stream chat, tools, supported structured output | Exact option matrix versioned |
| `POST /v1/completions` | Compatibility | Legacy text completion for tested clients | No feature parity promise |
| `POST /v1/responses` | Required adapter | Accept tested text/tool subset; return Responses-shaped object/events | **[OPEN]** native stateful semantics deferred |
| `POST /v1/embeddings` | Optional | Dedicated admitted embedding models only | Must not run on arbitrary generation model |
| `POST /v1/*/input_tokens` | Recommended | Count under the exact admitted tokenizer/template | Must identify model/template |
| `GET /health/live` | Required | Process liveness only | Minimal response; no model warmup side effect |
| `GET /health/ready` | Required | Coordinator plus selected model/plan readiness | Protected detail remains in admin status |
| `GET /health/startup` | Required | Startup has completed or reached a terminal startup failure | Allows slow initialization without conflating liveness and readiness |
| `GET /metrics` | Required, protected | Prometheus-compatible product metrics | No prompt/user labels |
| `/admin/v1/*` | Required, protected | Models, plans, sessions, cache, drain, diagnostics | Never OpenAI-compatible namespace |

## Streaming constraints

- **[RECOMMENDATION]** Use SSE for compatible streaming routes, preserve event ordering, send a documented terminal event, and close after terminal success/error.
- **[RECOMMENDATION]** A failure before headers returns the normal JSON error. Strict OpenAI-compatible streaming clients receive only the tested endpoint-specific compatible event sequence and connection close behavior; they are never sent an unsolicited custom event.
- **[RECOMMENDATION]** A client may negotiate the namespaced HaloFPX streaming-error extension before streaming (for example through an explicitly versioned request capability). Only then may a post-header failure emit a documented `halofpx.error` terminal SSE event containing `schema_version`, `request_id`, the stable error envelope, `partial_output: true`, and no secret detail, followed by connection close.
- **[OPEN]** The exact negotiation field, extension version, endpoint coverage, and pinned SDK fixtures are not ratified; until conformance evidence exists, the extension must not be advertised as generic OpenAI compatibility.
- **[OPEN]** Exact Responses event parity must be established by the conformance suite; Chat chunk translation is not assumed equivalent.
