---
section_id: "05"
title: "Research Data and Benchmark Design Implications"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["Custom_Inference_Project"]
  software_versions: ["YAML 1.2.2"]
  hardware_revisions: []
related_sections: ["02", "03", "04", "70", "71", "72", "73", "74", "75", "76"]
---

# Design implications

## Run bundle layout

```text
experiments/HLX-EXP-YYYYMMDD-NNN_slug/
  README.md
  experiment.yaml
  runs/
    HLX-RUN-YYYYMMDDTHHMMSSZ-xxxx/
      run.yaml
      command.txt
      environment.json
      topology.json
      prompts.manifest.json
      stdout.log
      stderr.log
      metrics.jsonl
      checksums.sha256
      artifacts.yaml
  derived/
    results.csv
    analysis.yaml
    plots/
  conclusions.md
```

**[RECOMMENDATION]** Raw files become immutable once `checksums.sha256` is finalized. Large-file placeholders use the same logical layout but point outside Git.

## Minimum `run.yaml`

```yaml
schema_version: "1.0.0"
experiment_id: "HLX-EXP-20260716-001"
run_id: "HLX-RUN-20260716T000000Z-ab12"
status: "complete"
started_at: "2026-07-16T00:00:00Z"
ended_at: "2026-07-16T00:01:00Z"
operator: "codex-or-human-id"
node_ids: ["node-01"]
ranks: [{rank_id: 0, node_id: "node-01", role: "target"}]
source:
  repository: "charlie12345/ROCmFPX"
  commit: "<40-hex>"
  dirty: false
build:
  build_id: "HLX-BUILD-..."
  compiler: "<exact>"
  cmake_cache_sha256: "sha256:<64hex>"
model:
  sha256: "sha256:<64hex>"
  gguf_metadata_sha256: "sha256:<64hex>"
  tokenizer_sha256: "sha256:<64hex>"
runtime:
  backend: "<enum>"
  arguments: ["<exact argv entries>"]
  environment_allowlist: {}
workload:
  prompt_set_sha256: "sha256:<64hex>"
  seed: 0
  warmup_runs: 1
  measured_runs: 5
metrics_schema: "metrics-v1"
artifacts_manifest: "artifacts.yaml"
```

## Comparison keys

**[RECOMMENDATION]** `analysis.yaml` must list `fixed_keys`, `independent_variables`, `dependent_metrics`, exclusions, aggregation, and uncertainty method. For dual-node comparisons also include rank ownership, world size, link/interface mapping, MTU, routing/bonding policy, transport commit/config, failure policy, and single-node fallback state.

## Units and names

- Store numeric value separately from unit.
- Use `byte`, `bit/s`, `byte/s`, `GiB`, `ms`, `s`, `token`, `token/s`, `J`, `W`, and `degC` as schema enums with documented conversions.
- Use long-form metric keys such as `decode_tokens_per_second`, never overloaded `speed`.

## External artifact pointer

```yaml
artifact_id: "sha256:<64hex>"
logical_path: "runs/.../trace.pftrace"
uri: "file:///approved/artifact/root/..."
size_bytes: 123456
media_type: "application/octet-stream"
sha256: "<64hex>"
retention_class: "project-critical"
created_by_run: "HLX-RUN-..."
license_or_sensitivity: "internal"
restore_check: "sha256"
```

**[OPEN]** `file:` URIs are illustrative metadata, not portable download routes; the artifact backend decision must define relocatable resolution.
