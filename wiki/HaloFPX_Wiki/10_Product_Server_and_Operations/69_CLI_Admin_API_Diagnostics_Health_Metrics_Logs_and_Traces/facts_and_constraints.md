---
section_id: "69"
title: "Operator Interface and Observability Facts and Constraints"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["Custom_Inference_Project", "llama.cpp"]
  software_versions: ["llama.cpp 788e07dc91d266ad3162a1ce9037665656269689", "OpenMetrics da13205984909cd67d853c7d2bddcbe25b6697c9"]
  hardware_revisions: ["dual Strix Halo target; exact nodes open"]
related_sections: ["66", "68", "71"]
---

# Facts and constraints

## Source-backed facts

- **[VERIFIED]** The pinned llama.cpp server documents `/health`, optional `/metrics`, slot inspection, property inspection, API-key files, TLS key/certificate options, logging controls, and request timeouts [S69-01].
- **[VERIFIED]** Prometheus recommends a domain prefix, base units, `_total` for counters, and labels whose cardinality cannot grow without bound [S69-02].
- **[VERIFIED]** OpenMetrics defines a text exposition model for metric families, including counters, gauges, histograms, and metadata [S69-03].
- **[VERIFIED]** OpenTelemetry defines common models for traces, metrics, logs, resources, and context propagation; semantic conventions are versioned independently [S69-04, S69-05].
- **[VERIFIED]** W3C Trace Context defines `traceparent` and `tracestate`, warns against sensitive information in those fields, and requires validation at trust boundaries [S69-06].

## Project constraints

- **[RECOMMENDATION]** Liveness means the process can answer; readiness means the intended API can safely accept work. Model/rank/link failures must be represented as explicit dependency and degraded states, not collapsed into HTTP reachability.
- **[RECOMMENDATION]** The canonical health routes are `/health/live` for process liveness, `/health/ready` for admission readiness, and `/health/startup` for startup completion/terminal startup failure. Minimal health responses may be public only on the configured client interface; detailed dependency state, metrics, slots, plans, ranks, sessions, cache state, and diagnostics require operator authorization.
- **[RECOMMENDATION]** Metric labels must never include prompt text, generated text, API keys, user/session/request IDs, raw paths, or full arbitrary model names.
- **[RECOMMENDATION]** Logs and support bundles must omit prompt/token contents by default. Identifiers should be bounded, pseudonymous, and useful only for correlation.
- **[OPEN]** Candidate SLOs in the requirements are not yet ratified; histogram buckets and alert thresholds therefore remain machine- and decision-dependent.
