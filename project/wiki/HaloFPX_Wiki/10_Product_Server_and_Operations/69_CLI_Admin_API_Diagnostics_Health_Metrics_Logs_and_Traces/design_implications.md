---
section_id: "69"
title: "Operator Interface and Observability Design Implications"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["Custom_Inference_Project"]
  software_versions: ["design candidate 2026-07-16"]
  hardware_revisions: ["dual Strix Halo target; exact nodes open"]
related_sections: ["66", "67", "68", "71", "72"]
---

# Design implications

## Proposed command and API inventory

| Area | Candidate CLI | Required behavior |
|---|---|---|
| Build/inventory | `version`, `build`, `inventory`, `status` | Exact commit, artifact digest, runtime/kernel, device and node identity |
| Models/plans | `models list|verify|load|unload`; `plans list|validate|activate` | Hash-addressed input, validation before mutation, activation audit |
| Distributed state | `links`, `ranks`, `health` | Ownership, peer version, negotiated capabilities, degradation reason |
| Work | `sessions list|cancel` | Metadata only by default; authorization before cancellation |
| Cache | `cache stats|verify|evict` | Namespace/scope explicit; corruption becomes rejection or miss |
| Performance | `bench run|status|export`; `tune inspect|apply` | Workload/config provenance and nonproduction warning |
| Recovery | `recover`; `support-bundle` | Preview, bounded scope, preserved evidence, verification result |

**[RECOMMENDATION]** Use `GET` for safe inspection and authenticated `POST` for state changes. Every response carries `schema_version`, `build_id`, `request_id`, machine-readable status/error codes, and a redacted human message.

## Signal contract

- **[RECOMMENDATION]** Core metrics use `halofpx_`: `build_info`, `up`, `ready`, `degraded`, `requests_total`, request/queue/TTFT/inter-token histograms in seconds, token counters, active/queued requests, rank/link/collective health, cache hit/miss/reject/evict/bytes, fallback count, errors, and startup duration.
- **[RECOMMENDATION]** Bounded labels include route class, status class, execution mode, node/rank, cache outcome, and stable short manifest identifiers. Full hashes belong in logs/build manifests, not every time series.
- **[RECOMMENDATION]** Structured events include UTC timestamp, severity, event name/schema, service/build/node/rank, trace/span/request correlation, mode and manifest hashes, duration, result, and error code. Sensitive fields are excluded at the producer.
- **[RECOMMENDATION]** Trace spans cover admission, queueing, coordinator work, rank work, transport/collectives, cache lookup/materialization, and streaming. Reject malformed external trace context and allow a new internal trace at a trust boundary.
- **[RECOMMENDATION]** A support bundle contains a signed/hashed manifest, redacted effective configuration, build/topology summaries, bounded journal excerpts, metric snapshot, diagnostic results, and file hashes. Show the manifest before capture; encrypt output; include sensitive artifacts only by explicit opt-in.

**[INFERENCE]** A single schema shared by CLI, API, logs, and bundles reduces contradictory operator views, but it requires compatibility tests across releases.
