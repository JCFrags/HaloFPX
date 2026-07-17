---
section_id: "44"
title: "MoE-Aware Hybrid Distribution and Hot-Expert Replication"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["charlie12345/ROCmFPX@a5605a72768c6562241b248e268e33dc92787394", "ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689"]
  software_versions: ["vLLM@9354f222042986addf20709e5274fc26e0d09745", "Megatron-LM@740c16e6b80a753bea26232148d9bb2d7f0c827a", "DeepEP@dd758caf451848bd150e1046af3d0a73e5fff38d"]
  hardware_revisions: ["dual AMD Strix Halo; exact BOM and USB4 fabric measurements unresolved"]
related_sections: ["34", "38", "42", "43", "45", "47", "52", "73", "76", "78", "80"]
---

# 44 - MoE-aware hybrid distribution and hot-expert replication

## Decision summary

- **[VERIFIED]** Current expert-parallel systems dispatch routed token activations to expert owners and combine outputs with a second collective; Megatron Core implements this as an all-to-all token dispatcher, and DeepEP exposes distinct MoE dispatch/combine primitives [S44-03][S44-04].
- **[VERIFIED]** vLLM's EPLB distinguishes logical, physical, and redundant experts, records load over a window, and can periodically rearrange physical expert placement. Its documented replica memory cost scales with every MoE layer [S44-02].
- **[VERIFIED]** DeepSeekMoE defines shared experts separately from top-k routed experts. Shared experts are model semantics, not optional hot replicas [S44-06].
- **[VERIFIED]** The pinned ROCmFPX/llama.cpp line supports layer, row, and tensor split modes and MoE weight offload controls, but the inspected code exposes no expert-owner map, token all-to-all dispatcher, or hot-replica controller [S44-01].
- **[RECOMMENDATION]** Implement the first HaloFPX MoE plan as a versioned static map: replicate routers and semantically shared experts when memory permits, partition cold routed experts, and add byte-identical hot replicas only after representative traces show persistent skew.
- **[RECOMMENDATION]** Prepare a new map off-path and activate it only at a drained batch/step boundary. Never migrate an expert during an in-flight MoE layer.
- **[OPEN]** No source proves that token-wise expert parallelism beats coarse hidden-state handoff on two Strix Halo nodes over the target dual-USB4 fabric. Sections 52, 73, and 76 own the required transport and end-to-end measurements.

## Candidate execution shapes

| Shape | Placement | Inter-node traffic | Intended use |
|---|---|---|---|
| Coarse pipeline | Whole layers and their routers/experts remain on one rank | fixed hidden-state handoff at stage boundaries | predictable fallback when stage memory fits |
| Attention/EP hybrid | attention is sharded or replicated; routed experts have owners | variable token dispatch plus output combine at each MoE layer | only if fine-grained traffic wins on the measured fabric |
| Hot-replica hybrid | shared and selected hot experts replicated; cold experts partitioned | local hot work; remote cold assignments; combine results | target optimization after stable telemetry |

The authoritative model-routing inputs are in [section 34](../../06_Models_Quantization_and_Inference/34_MoE_Routing_Expert_Telemetry_and_Expert_Placement_Inputs/README.md). General mode selection belongs to [section 38](../38_Distributed_Runtime_Goals_Cost_Model_and_Mode_Selection/README.md); tensor and pipeline baselines belong to [sections 42](../42_Two_Way_Tensor_Parallelism_and_Collective_Placement/README.md) and [43](../43_Contiguous_Layer_Pipeline_Parallelism_and_Microbatching/README.md).

## Research split

1. **Internet/source-code research completed now:** pinned implementation patterns, model semantics, collective shape, load-balancing controls, and current ROCmFPX gaps are recorded in [facts and constraints](facts_and_constraints.md) and [sources](sources.md).
2. **Target-machine work required:** exact expert bytes, routing traces, link curves, kernel timings, memory headroom, output equivalence, reconfiguration, and failure tests are specified in [procedures and checks](procedures_and_checks.md).
3. **Contingent decisions:** attention-versus-pipeline base, replica count, placement epoch, dual-link policy, and degraded-mode behavior remain in [open questions](open_questions.md).

No HaloFPX performance result in this section is labeled **[MEASURED]** because the required two-machine experiments have not run.
