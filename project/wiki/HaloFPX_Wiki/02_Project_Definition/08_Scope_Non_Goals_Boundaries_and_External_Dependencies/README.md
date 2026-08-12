---
section_id: "08"
title: "Scope, Non-Goals, Boundaries, and External Dependencies"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["HaloFPX project", "charlie12345/ROCmFPX", "fewtarius/CachyLLama", "fewtarius/llama-ai", "ggml-org/llama.cpp"]
  software_versions: ["commits in sources.md"]
  hardware_revisions: ["two matched AMD Strix Halo systems; exact BOM open"]
related_sections: ["06", "07", "09", "10", "11", "15", "18", "23", "38", "49", "60", "69"]
---

# Scope and boundaries

**[RECOMMENDATION]** HaloFPX v1 should be a source-built, Linux-first, two-node local inference service with a correct single-node fallback. It should integrate selected upstream capabilities behind explicit interfaces rather than becoming an unbounded merger of four repositories.

## In scope

- Reproducible ROCmFPX-derived inference on the two declared Strix Halo nodes.
- OpenAI-compatible chat/completion serving for target agent clients.
- Measured mode selection among single-node, replication, remote speculation, two-way tensor parallel, pipeline parallel, and MoE-aware hybrid candidates.
- Rank-local persistent cache with compatibility validation and safe miss/recompute.
- Specialized transport over two verified host-to-host USB4 links, with one-link and one-node failure behavior.
- Installation, authentication, observability, administration, upgrades, rollback, and evidence-backed evaluation.

## Explicit non-goals for v1

- General heterogeneous cluster orchestration, cloud SaaS, training/fine-tuning, arbitrary accelerators, Windows/macOS production support, or compatibility with every model/client.
- Guaranteed bitwise identity across all backends/batch shapes.
- Accepting a cache entry after failed integrity or compatibility checks.
- Automatically upstreaming every HaloFPX experiment or reproducing all CachyLLama/llama-ai behavior.

See [boundary details](facts_and_constraints.md) and [dependency checks](procedures_and_checks.md).

