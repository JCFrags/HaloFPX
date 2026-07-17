---
section_id: "55"
title: "Fabric benchmark decision implications"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["torvalds/linux@8cdeaa50eae8dad34885515f62559ee83e7e8dda"]
  software_versions: ["Linux 7.1.3 historical baseline", "Linux 7.2-rc2 candidate"]
  hardware_revisions: ["two Nimo MME3L Strix Halo nodes; exact revisions open"]
related_sections: ["49", "50", "52", "53", "54", "75", "76", "80"]
---

# Design implications

- **[RECOMMENDATION]** Preserve TCP/USB4NET as compiled, tested control and recovery path regardless of USB4STREAM outcome.
- **[RECOMMENDATION]** Separate tuning from confirmation. Use a declared tuning subset for ring, throttling, batching, affinity, and queue depth; freeze one configuration before paired holdout.
- **[RECOMMENDATION]** Require real layer-boundary or scheduler message traces for production relevance. Synthetic large-write wins cannot authorize integration alone.
- **[RECOMMENDATION]** Measure GPU-to-peer-GPU time as GPU completion → transport → peer visibility → peer GPU verified completion. CPU payload arrival is not the final metric.
- **[RECOMMENDATION]** A transport loss invalidates the session epoch; surviving rails cannot accept incomplete old messages. Failure tests are correctness gates, not optional resilience polish.
- **[INFERENCE]** If USB4STREAM reduces syscall/protocol cost but page copies dominate, it may improve small control latency yet not bulk GPU-to-GPU time. Per-size and real-trace results must drive policy.

## Kernel-extension decision tree

```text
upstream prerequisites/correctness/rollback pass?
  no -> retain USB4NET
  yes -> upstream USB4STREAM meets accepted benefit gate?
    yes -> optional upstream backend; no custom kernel ABI
    no -> traced current page-copy path is material and removable?
      no -> retain upstream interfaces
      yes -> separately reviewed registered-buffer prototype
        prototype meets stricter benefit+safety gate?
          no -> reject/archive patch
          yes -> propose upstreamable extension; keep fallback
```

**[RECOMMENDATION]** A local patch must not leak into model format, scheduler ownership, rank state, or cache format. It remains an optional transport adapter behind the Section 49 abstraction.

## Research split

1. Completed now: source semantics, accepted local decision, experiment and evidence thresholds.
2. On-machine: S55-E01–E06.
3. Contingent: candidate kernel, USB4STREAM backend, tuning, registered-buffer prototype, and production adoption.
