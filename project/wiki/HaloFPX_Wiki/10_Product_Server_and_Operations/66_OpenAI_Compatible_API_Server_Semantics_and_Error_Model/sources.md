---
section_id: "66"
title: "API Sources"
status: "verified"
last_verified: "2026-07-16"
applies_to:
  repositories: ["ggml-org/llama.cpp", "openai/openai-openapi", "fewtarius/CachyLLama"]
  software_versions: ["llama.cpp 788e07d", "OpenAI OpenAPI db3e531", "CachyLLama 6be7459"]
  hardware_revisions: []
related_sections: ["09", "60", "69", "71"]
---

# Sources

## S66-01 — llama.cpp server documentation

- Publisher/URL: `ggml-org/llama.cpp`, https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/tools/server/README.md
- Revision/access: commit `788e07dc91d266ad3162a1ce9037665656269689`, 2026-07-17 UTC; accessed 2026-07-16 America/Los_Angeles.
- Supports: endpoint, streaming, health, metrics, auth, server error, router, and Responses-adapter behavior.
- Limitations: self-documentation; compatibility is qualified and must be tested.

## S66-02 — OpenAI OpenAPI repository

- Publisher/URL: OpenAI, https://github.com/openai/openai-openapi/tree/db3e53198a66732cfe161339ea63bf36fc0137ad
- Revision/access: commit `db3e53198a66732cfe161339ea63bf36fc0137ad`; accessed 2026-07-16.
- Supports: official machine-readable API schema snapshot.
- Limitations: current head is volatile; HaloFPX implements only an advertised subset.

## S66-03 — OpenAI streaming guide

- Publisher/URL: OpenAI, https://developers.openai.com/api/docs/guides/streaming-responses
- Revision/access: live documentation, accessed 2026-07-16.
- Supports: official Responses streaming/event guidance.
- Limitations: live page has no immutable revision and targets OpenAI service behavior.

## S66-04 — OpenAI function calling and structured output guides

- Publisher/URLs: OpenAI, https://developers.openai.com/api/docs/guides/function-calling and https://developers.openai.com/api/docs/guides/structured-outputs
- Revision/access: live documentation, accessed 2026-07-16.
- Supports: official tool and schema-output semantics.
- Limitations: model-dependent OpenAI behavior; local-model quality is not implied.

## S66-05 — OpenAI error code guide

- Publisher/URL: OpenAI, https://developers.openai.com/api/docs/guides/error-codes
- Revision/access: live documentation, accessed 2026-07-16.
- Supports: official HTTP/error categories used as compatibility input.
- Limitations: does not define HaloFPX distributed failures.

## S66-06 — CachyLLama server extensions

- Publisher/URL: `fewtarius/CachyLLama`, https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/README.md
- Revision/access: commit `6be745998f568e379ea197fcf827baec73ff9940`, 2026-07-09; accessed 2026-07-16.
- Supports: `llama_user_id`, per-user 429, slot affinity, and SSD-cache controls.
- Limitations: fork-specific README claims; not OpenAI standard behavior.

