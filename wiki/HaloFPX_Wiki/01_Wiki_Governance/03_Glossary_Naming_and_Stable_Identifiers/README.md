---
section_id: "03"
title: "Glossary, Naming, and Stable Identifiers"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["Custom_Inference_Project", "ROCmFPX", "CachyLlama", "llama-ai", "llama.cpp"]
  software_versions: []
  hardware_revisions: []
related_sections: ["01", "02", "04", "05", "43", "49", "57"]
---

# Glossary, Naming, and Stable Identifiers

This section is the authoritative controlled vocabulary for HaloFPX records. It distinguishes upstream terms from proposed project terms so that code, logs, experiments, and wiki pages do not use the same word for different mechanisms.

**[RECOMMENDATION]** Human labels may change; stable IDs must not. Use `HLX-<TYPE>-<zero-padded number or digest>` for project records and retain native upstream identifiers alongside them.

## Highest-risk ambiguities

- `node` is a physical host; `rank` is a participating process role. Never use them interchangeably.
- `replication`, `remote speculative decoding`, `tensor parallelism`, and `pipeline parallelism` are distinct execution modes.
- `model-weight quantization`, runtime K/V-cache quantization, and persistent cache storage are distinct.
- llama.cpp `--split-mode row` is named `llamacpp-row-split`; do not call it generic tensor parallelism without documenting its actual collective and ownership behavior.
- GB/Gb and GiB/Gib differ; throughput requires an explicit unit and measurement boundary.

## Research split

- Source research completed: exact upstream project descriptions, current llama.cpp split-mode names, GGUF purpose/naming, URN persistence intent, RFC timestamps, and SI/binary units.
- Machine inspection required: actual host names, device enumeration, network interfaces, filesystem mount IDs, build IDs, and runtime log vocabulary.
- Contingent decisions: final product name, rank numbering, execution-mode enum, and compatibility-ID canonicalization.

See [facts and glossary](facts_and_constraints.md), [design implications](design_implications.md), [checks](procedures_and_checks.md), [questions](open_questions.md), and [sources](sources.md).
