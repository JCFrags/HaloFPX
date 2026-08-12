---
section_id: "05"
title: "Research Data and Benchmark Open Questions"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["Custom_Inference_Project"]
  software_versions: []
  hardware_revisions: []
related_sections: ["04", "70", "71", "72", "73", "74", "75", "76"]
---

# Open questions

| ID | Question | Needed evidence | Impact |
|---|---|---|---|
| OQ-05-001 | Which content-addressed artifact backend is canonical and backed up? | Storage/recovery trial | Large-artifact durability |
| OQ-05-002 | What Git size threshold routes content externally or to LFS? | Repository-growth and clone tests | Developer usability |
| OQ-05-003 | What retention classes and deletion authority apply? | Capacity, audit, and recovery requirements | Cost and evidence survival |
| OQ-05-004 | Which environment fields are stable comparison keys versus diagnostic metadata? | Pilot runs on both nodes | False match/mismatch risk |
| OQ-05-005 | What prompt sets may be stored, hashed only, or must be redacted? | Privacy/license review | Reproducibility vs sensitivity |
| OQ-05-006 | Which statistical summaries and minimum repetitions apply per benchmark class? | Variance study | Acceptance decisions |
| OQ-05-007 | How are wall power and energy sampled/calibrated? | Instrument inventory/calibration | Efficiency claims |
| OQ-05-008 | How is clock synchronization verified for cross-node traces? | PTP/NTP inspection and skew test | Distributed latency analysis |
| OQ-05-009 | Which caches must be cold, warm, or both for each product claim? | Workload/decision mapping | Benchmark relevance |
| OQ-05-010 | How are runtime-generated outputs stored when they may contain sensitive user data? | Data-handling policy | Safety and retention |

**[OPEN]** No performance values are reported in this section. Repository benchmarks cited as examples remain scoped to their recorded environments.

## Follow-up research

- Pin schemas and build a dry-run validator.
- Inventory native `llama-bench`, server, ROCm, perf, and transport outputs before finalizing metric adapters.
- Define benchmark-class-specific protocols in sections 70-76.
