---
section_id: "05"
title: "Research Data and Benchmark Facts and Constraints"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["Custom_Inference_Project"]
  software_versions: ["DataCite Metadata Schema 4.7", "MLPerf Inference Rules 5.1"]
  hardware_revisions: []
related_sections: ["02", "03", "04"]
---

# Facts and constraints

- **[VERIFIED]** DataCite Metadata Schema 4.7 was released 2026-03-03 and defines core metadata for consistent resource identification, citation, and retrieval [S05-02].
- **[VERIFIED]** W3C PROV represents entities, activities, agents, derivation, and primary sources, supporting a raw -> processed -> plot lineage [S05-03].
- **[VERIFIED]** Git LFS stores a small pointer in Git while content is stored separately; its v1 pointer includes content identity and size [S05-04].
- **[VERIFIED]** NIST distinguishes decimal SI prefixes from binary prefixes; `k`, `M`, and `G` must not mean powers of 1024 [S05-05].
- **[VERIFIED]** RFC 3339 defines unambiguous Internet timestamps and recommends UTC for interoperability [S05-06].
- **[VERIFIED]** ROCmFPX README benchmarks at commit `a5605a7...` explicitly scope results by model/backend/settings and warn that results are hardware-, driver-, model-, prompt-, and recipe-dependent [S05-07]. Those are upstream-repository measurements, not HaloFPX measurements.

## Artifact classes

| Class | Examples | Default handling |
|---|---|---|
| definition | hypothesis, protocol, comparison keys, acceptance criteria | Git-tracked text |
| raw | stdout/stderr, JSONL events, counters, traces, environment dumps | immutable; artifact store if large |
| derived | normalized tables, aggregates, confidence intervals | regenerable, Git-tracked if small |
| presentation | plots, Markdown summaries | Git-tracked with generator/version |
| external immutable | model, dataset, source bundle | pointer plus hash/license; do not duplicate blindly |
| sensitive | private prompts, secrets, identifiers | redact/exclude; store only under explicit policy |

## Measurement constraints

- **[RECOMMENDATION]** Never edit raw artifacts in place. Corrections create a new derived artifact and preserve lineage.
- **[RECOMMENDATION]** Record clock source, start/end UTC, warm-up policy, repetitions, exclusions, failure count, and whether caching was cold/warm.
- **[RECOMMENDATION]** Record prompt and generated-token counts from the runtime's actual tokenizer; do not infer counts from characters.
- **[RECOMMENDATION]** A failed or partial run remains recorded with status and reason; it is excluded only by a predeclared or documented rule.
- **[RECOMMENDATION]** Plots are never sole evidence. Include the underlying table, code/config, and units.
- **[RECOMMENDATION]** Cache corruption or hash mismatch produces an invalid artifact/cache miss and recomputation, never accepted input.
