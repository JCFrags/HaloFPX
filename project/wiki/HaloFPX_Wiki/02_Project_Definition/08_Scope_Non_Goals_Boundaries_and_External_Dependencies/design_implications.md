---
section_id: "08"
title: "Scope Design Implications"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["HaloFPX project"]
  software_versions: []
  hardware_revisions: ["two-node Strix Halo target"]
related_sections: ["09", "10", "15", "23", "38", "49", "60", "69", "71"]
---

# Design implications

## Dependency strategy

1. **[RECOMMENDATION]** Freeze an initial llama.cpp ancestor and maintain a small, ordered patch stack: ROCmFPX essentials, then independently reviewed cache/telemetry changes, then HaloFPX distributed interfaces.
2. **[RECOMMENDATION]** Put project-specific behavior behind stable internal boundaries: `model_runtime`, `rank_protocol`, `transport`, `cache_store`, `planner`, and `api_adapter`.
3. **[RECOMMENDATION]** Version every wire message and persistent object. Compatibility keys should include code/schema version, model hash, quantization, tokenizer/template, backend-sensitive state description, rank/shard plan, and relevant runtime parameters.
4. **[RECOMMENDATION]** Treat optional features as feature-gated and observable. Unsupported combinations must fail at admission, not mid-generation.

## Optional versus required

| Capability | v1 classification | Reason |
|---|---|---|
| Correct single-node serve/fallback | Required | Usable baseline and recovery path |
| Replication | Required candidate | Simple two-node baseline |
| Persistent cache | Required candidate; may ship disabled | Core workload hypothesis needs safety proof |
| Dual-link transport | Required experiment; fallback to one link | Target topology but independence unverified |
| Remote speculation | Optional | Acceptance/coordination benefit is model-specific |
| Tensor/pipeline parallel | Optional until capacity requires | Complexity and communication cost |
| MoE-aware placement | Optional/experimental | Needs representative expert telemetry |
| Multi-model router/multimodal/embeddings | Deferred unless Section 07 prioritizes | Expands API and validation surface |

## Prohibited shortcuts

- Do not infer hardware compatibility from `gfx1151` alone.
- Do not compare unmatched models, quantizations, contexts, batch sizes, or thermal states.
- Do not share or restore rank state without explicit owner/epoch/compatibility validation.
- Do not expose unauthenticated administrative, cache, metrics, or slot-control routes to an untrusted network.
- Do not silently fall back to a slower or lower-quality mode; report the active mode and reason.

