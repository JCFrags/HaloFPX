---
section_id: "47"
title: "Planner Design Implications"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["HaloFPX proposed runtime"]
  software_versions: []
  hardware_revisions: ["two matched AMD Strix Halo systems (exact BOM open)"]
related_sections: ["38", "42", "43", "44", "46", "48", "67", "76"]
---

# Design implications

## Search and selection

**[RECOMMENDATION]** Use staged search: capability filter; analytical cost-model prune; microbenchmark; end-to-end benchmark; fault/correctness gate; canary. Always retain full replication and single-node candidates when the full model fits.

**[RECOMMENDATION]** Compare candidates on matched prompts, seeds, cache state, concurrency, power state, and run order. Use repeated randomized trials and retain raw samples. Select only when the candidate clears every hard constraint and its improvement over the incumbent exceeds both a confidence bound and a safety margin.

**[RECOMMENDATION]** Avoid overfitting with held-out prompt-length/model-shape buckets, multiple workload traces, cold/warm cache strata, boot-to-boot repetitions, and a complexity penalty. Prefer the simpler plan when confidence intervals overlap.

## Plan manifest minimum

```yaml
plan_id: content-hash
compatibility: {model_sha256: ..., tokenizer_sha256: ..., runtime_commit: ..., build_hash: ...}
platform: {node_ids: [...], firmware: [...], kernel: ..., backend: hip-or-vulkan, driver: ...}
execution: {mode: ..., rank_owners: ..., split_points: [...], collective_policy: ..., links: [...]}
inference: {context_bucket: ..., batch: ..., ubatch: ..., mtp: ..., deterministic_mode: ...}
cache: {format: ..., fingerprint: ..., rank_layout: ..., restore_policy: ...}
objective: {name: ..., hard_constraints: {...}, score_definition: ...}
evidence: {dataset_hash: ..., raw_run_ids: [...], repetitions: ..., confidence: ..., measured_at: ...}
safety: {memory_headroom: ..., thermal_margin: ..., fallback_plan_id: ..., expires_at: ...}
```

**[RECOMMENDATION]** Rank 0/coordinator owns plan selection and epoch. Workers validate the manifest fingerprint and either accept the exact plan or reject it; they never silently substitute backend, split, model, or cache format.

## Retune and rollback

Retune on model/runtime/build/driver/firmware/hardware/link/cache-format changes; sustained SLO regression; memory or thermal margin breach; new workload bucket; or plan expiry. **[RECOMMENDATION]** Do not retune on a single noisy sample. Canary a candidate, compare paired metrics, and atomically restore the prior manifest on any correctness, health, or hard-SLO failure.

**[RECOMMENDATION]** On rank/link loss, stop using the invalid two-rank plan. Select a prevalidated one-link plan only if its manifest explicitly permits it; otherwise start a new single-node/replicated epoch.
