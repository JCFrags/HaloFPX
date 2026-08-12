---
section_id: "05"
title: "Research Data and Benchmark Artifact Conventions"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["Custom_Inference_Project", "ROCmFPX", "CachyLlama", "llama-ai", "llama.cpp"]
  software_versions: ["DataCite Metadata Schema 4.7", "MLPerf Inference Rules 5.1"]
  hardware_revisions: []
related_sections: ["02", "03", "04", "70", "71", "72", "73", "74", "75", "76"]
---

# Research Data and Benchmark Artifact Conventions

**[RECOMMENDATION]** A benchmark result is a derived view over an immutable run bundle. Preserve raw output, exact command/configuration, environment, model/tokenizer identities, prompt-set identity, and integrity hashes before creating summaries or plots.

## Non-negotiable comparison rule

Only call results directly comparable when their declared comparison keys match or the difference is the intentional independent variable. Hardware, firmware, power/thermal policy, OS/kernel, ROCm/driver, source/build, backend, model bytes, tokenizer/template, context/batch/KV types, distributed topology, transport state, prompts, sampling, warm-up, repetitions, and metric boundary are material keys.

**[VERIFIED]** MLPerf Inference rules require replicability, consistent system/framework configuration, restricted nondeterminism, and tagged approved benchmark revisions for formal submissions [S05-01]. HaloFPX is not claiming MLPerf compliance; those controls inform local practice.

## Storage boundary

- Commit small text manifests, schemas, checksums, summaries, and plots.
- Store large raw logs/traces/cache images/model weights in the artifact area or an approved content-addressed store.
- Commit a pointer manifest containing URI/path, size, media type, SHA-256, retention class, and recovery instructions.
- Never commit secrets, private prompts, full model weights, or persistent cache images to the wiki.

## Research split

- Source research completed: metadata/provenance, units, timestamps, checksums/pointers, and replicability principles.
- Two-node work required: validate collectors, run matched/no-op trials, verify hashes and artifact restore, and quantify run-to-run variability.
- Contingent decisions: artifact backend, size threshold, retention periods, prompt redaction, and accepted statistical summaries.

See [facts](facts_and_constraints.md), [schemas](design_implications.md), [run procedure](procedures_and_checks.md), [questions](open_questions.md), and [sources](sources.md).
