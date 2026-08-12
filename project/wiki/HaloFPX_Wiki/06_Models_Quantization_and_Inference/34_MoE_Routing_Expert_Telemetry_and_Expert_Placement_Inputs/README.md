---
section_id: "34"
title: "MoE Routing, Expert Telemetry, and Expert Placement Inputs"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689", "fewtarius/CachyLLama@6be745998f568e379ea197fcf827baec73ff9940"]
  software_versions: []
  hardware_revisions: ["dual Strix Halo; exact BOM unresolved"]
related_sections: ["29", "44", "47", "73"]
---

# 34 - MoE routing and placement inputs

This section defines what HaloFPX must observe before it may place or replicate experts. The source baseline is exact, but no routing trace has yet been captured on the target machines; placement conclusions therefore remain recommendations.

## High-value conclusions

- **[VERIFIED]** Current llama.cpp builds MoE routing as router projection, model-specific score transformation/group filtering, top-k expert selection, selected-weight normalization, expert-indexed gate/up/down multiplication, weighting, and reduction [S34-01].
- **[VERIFIED]** Mixtral selects two of eight feed-forward experts per token; DeepSeek-V2 adds shared experts and finer-grained routed experts. These are different routing contracts, not interchangeable templates [S34-03][S34-04].
- **[INFERENCE]** Total parameter count does not predict per-token compute or traffic. HaloFPX needs layer-specific routed tensor bytes, selected-expert counts, shared-path cost, and batch coalescing.
- **[RECOMMENDATION]** Do not replicate a "hot" expert from global frequency alone. Require stable per-layer, per-workload, per-phase hotness plus a measured transfer-versus-memory tradeoff.
- **[OPEN]** The prompt requests "CachyLLama expert telemetry," but the inspected commit exposes no dedicated expert telemetry facility. Treat this as a feature gap, not an existing capability [S34-02].

See [facts and constraints](facts_and_constraints.md), [design implications](design_implications.md), and [checks](procedures_and_checks.md).

