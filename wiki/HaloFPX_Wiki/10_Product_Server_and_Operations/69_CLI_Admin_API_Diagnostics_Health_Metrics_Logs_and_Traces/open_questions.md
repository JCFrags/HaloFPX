---
section_id: "69"
title: "Operator Interface and Observability Open Questions"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["Custom_Inference_Project"]
  software_versions: ["design candidate 2026-07-16"]
  hardware_revisions: ["dual Strix Halo target; exact nodes open"]
related_sections: ["66", "71", "72"]
---

# Open questions

1. **[OPEN]** Which admin authentication and roles are required for a single owner, household users, and automation?
2. **[OPEN]** Are metrics scraped over loopback, a protected LAN listener, or a local collector/Unix socket?
3. **[OPEN]** Which SLOs determine latency histogram buckets and alert thresholds?
4. **[OPEN]** What log, trace, metric, audit, and support-bundle retention is acceptable?
5. **[OPEN]** Which session fields are operationally necessary without exposing content or user identity?
6. **[OPEN]** What clock synchronization error is tolerable for cross-node traces?
7. **[OPEN]** Which recovery actions may be exposed remotely, and which require local console confirmation?
8. **[OPEN]** Is a standard OpenTelemetry Collector part of the supported deployment, or only an export target?
