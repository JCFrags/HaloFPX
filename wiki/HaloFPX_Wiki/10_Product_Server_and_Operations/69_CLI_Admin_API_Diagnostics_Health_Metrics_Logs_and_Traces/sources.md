---
section_id: "69"
title: "Operator Interface and Observability Sources"
status: "verified"
last_verified: "2026-07-16"
applies_to:
  repositories: ["Custom_Inference_Project", "llama.cpp"]
  software_versions: ["source snapshot 2026-07-16"]
  hardware_revisions: []
related_sections: ["66", "71"]
---

# Sources

| ID | Source | Revision/access | Scope and use | Limitations |
|---|---|---|---|---|
| S69-01 | [llama.cpp server README](https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/tools/server/README.md) | Commit `788e07dc91d266ad3162a1ce9037665656269689`; accessed 2026-07-16 | Upstream endpoints, authentication/TLS flags, logging, timeouts, metrics | Upstream documentation; not HaloFPX runtime evidence |
| S69-02 | [Prometheus metric and label naming](https://prometheus.io/docs/practices/naming/) | Maintained documentation; accessed 2026-07-16 | Units, counter suffix, bounded label guidance | Mutable guidance; does not choose HaloFPX metric names or SLOs |
| S69-03 | [OpenMetrics specification](https://github.com/prometheus/OpenMetrics/blob/da13205984909cd67d853c7d2bddcbe25b6697c9/specification/OpenMetrics.md) | Commit `da13205984909cd67d853c7d2bddcbe25b6697c9`; accessed 2026-07-16 | Metric exposition model | Format specification; not an observability policy |
| S69-04 | [OpenTelemetry specification](https://github.com/open-telemetry/opentelemetry-specification/tree/b59c1f71e6419483e243ce386325d411f1ca9a75) | Commit `b59c1f71e6419483e243ce386325d411f1ca9a75`; accessed 2026-07-16 | Signal and context model | Does not establish HaloFPX instrumentation coverage |
| S69-05 | [OpenTelemetry semantic conventions](https://github.com/open-telemetry/semantic-conventions/tree/235496cf6e2b1bc52921f883205c3cd4d78f17f0) | Commit `235496cf6e2b1bc52921f883205c3cd4d78f17f0`; accessed 2026-07-16 | Versioned naming conventions | Conventions evolve independently and require an explicit adopted version |
| S69-06 | [W3C Trace Context Recommendation](https://www.w3.org/TR/trace-context/) | W3C Recommendation page; accessed 2026-07-16 | Propagation, privacy, validation | Does not define HaloFPX authorization or retention |
| S69-07 | [Agent Harness architecture](../../../../references/agent-harness.md) | Local canonical pointer; accessed 2026-07-16 | Evidence-promotion and layer boundaries | Conceptual authority, not machine behavior |

**[VERIFIED]** Source-code claims are pinned to commits. Standards and documentation URLs identify the reviewed normative or maintained pages. No source establishes HaloFPX machine behavior; experiments do.
