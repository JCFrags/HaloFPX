---
section_id: "66"
title: "API and Error Model Open Questions"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["HaloFPX project"]
  software_versions: []
  hardware_revisions: []
related_sections: ["07", "09", "60", "67", "68", "69", "71"]
---

# Open questions

| ID | Question | Resolution evidence |
|---|---|---|
| OQ-66-01 | **[OPEN]** Which OpenAI request/response fields and SDK versions are contractual? | Client inventory and conformance matrix |
| OQ-66-02 | **[OPEN]** Must `/v1/responses` support stateful continuation or only stateless adapter behavior? | Client requirements and implementation ADR |
| OQ-66-03 | **[OPEN]** Are embeddings, reranking, multimodal, and Anthropic routes in v1? | Section 07 priorities and model tests |
| OQ-66-04 | **[OPEN]** What negotiation field, version, endpoints, and pinned SDKs define the optional `halofpx.error` SSE extension? | Client experiments, fixtures, and schema decision |
| OQ-66-05 | **[OPEN]** Is `halofpx.user_id` needed when authentication already provides identity? | Identity/routing design |
| OQ-66-06 | **[OPEN]** Which extension names and values are stable public API? | API review and version policy |
| OQ-66-07 | **[OPEN]** How long are idempotency outcomes retained and protected? | Storage/privacy decision |
| OQ-66-08 | **[OPEN]** What failures permit transparent retry without semantic duplication? | Request boundary tests |
| OQ-66-09 | **[OPEN]** Which health details are public versus administrator-only? | Threat model |
| OQ-66-10 | **[OPEN]** What deterministic tolerance applies across backends and distributed modes? | Model-specific evaluation |

## New gaps discovered

- Upstream Responses compatibility is an adapter, so event/state parity needs its own contract and tests.
- The official Docs MCP could not be installed in-session because the packaged `codex.exe` was access-denied; official web documentation was used as the bounded fallback.
- No client/SDK inventory or stable HaloFPX extension namespace has been ratified.
