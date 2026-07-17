---
section_id: "69"
title: "CLI, Admin API, Diagnostics, Health, Metrics, Logs, and Traces"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["Custom_Inference_Project", "llama.cpp"]
  software_versions: ["llama.cpp 788e07dc91d266ad3162a1ce9037665656269689", "OpenTelemetry Specification b59c1f71e6419483e243ce386325d411f1ca9a75"]
  hardware_revisions: ["dual Strix Halo target; exact nodes open"]
related_sections: ["66", "67", "68", "70", "71", "72"]
---

# CLI, Admin API, Diagnostics, Health, Metrics, Logs, and Traces

**[RECOMMENDATION]** Provide one versioned operator surface: `halofpxctl` for humans and automation, backed by a protected `/admin/v1` API. Read-only inspection and mutating operations must be distinguishable; mutations require authorization, an audit event, idempotency where practical, and `--dry-run` when a safe preview exists.

**[VERIFIED]** The pinned llama.cpp server exposes `/health`; conditionally exposes `/metrics` with `--metrics`; and exposes `/slots` and `/props` with controls described in its server documentation [S69-01]. These are upstream behaviors, not proof of the HaloFPX contract.

## Research split

- Online/source research completed: upstream endpoints, Prometheus/OpenMetrics naming, OpenTelemetry signal conventions, and W3C trace propagation.
- Machine validation required: endpoint access control, metric cardinality, redaction, trace continuity, health transitions, support-bundle contents, and fault diagnostics on both nodes.
- Contingent decisions: authentication mechanism, histogram buckets, retention, bundle encryption/transport, and which diagnostic mutations are supported.

See [facts and constraints](facts_and_constraints.md), [design implications](design_implications.md), [procedures and checks](procedures_and_checks.md), [open questions](open_questions.md), and [sources](sources.md).
