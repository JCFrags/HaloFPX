---
section_id: "66"
title: "API Conformance and Fault Checks"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["HaloFPX project"]
  software_versions: ["release candidate under test"]
  hardware_revisions: ["actual two-node deployment"]
related_sections: ["09", "48", "68", "69", "71", "77", "78"]
---

# API conformance and fault checks

## Internet/source follow-up

1. Freeze the official OpenAI schema snapshot and pinned llama.cpp server documentation used by the compatibility suite.
2. Diff supported request/response fields, required values, SSE events, tool-call shapes, structured-output rules, usage, finish reasons, and errors.
3. Recheck OpenAI docs and `llama-server` changelog at each release freeze; never silently widen compatibility.

## On-machine conformance suite

Prerequisites: exact binary/model/template/plan manifests, test API identities, isolated endpoints, and raw HTTP capture with secrets redacted. Root is not required.

1. Exercise required endpoints with minimum, maximum, unknown, null, malformed, and conflicting parameters.
2. Compare non-stream and stream assembly for text, tools, structured output, usage, cancellation, and stop reasons.
3. Run official SDK smoke tests against the advertised subset; record SDK versions and any adapter requirements.
4. Validate token counts against actual prompt evaluation for each model/template.
5. Trigger authentication, authorization, missing model, unsupported feature, overload, timeout, model-loading, rank-loss, and internal faults.
6. Drop one link/rank before headers, mid-prompt, and mid-stream; verify terminal state, retry metadata, and no silent partial success.
7. Fuzz extension objects and verify request user IDs cannot override authenticated ownership.
8. Confirm health/metrics/admin exposure matches the Section 71 threat model and never leaks paths, prompts, secrets, or user labels.

## Acceptance evidence

- Machine-readable endpoint/field matrix and negative-test corpus.
- Raw redacted request/response/SSE fixtures.
- SDK/version results and documented deviations.
- Fault-to-error mapping and retry decision for every injected boundary.
- No unsupported endpoint or option advertised in `/v1/models` or capability metadata.

