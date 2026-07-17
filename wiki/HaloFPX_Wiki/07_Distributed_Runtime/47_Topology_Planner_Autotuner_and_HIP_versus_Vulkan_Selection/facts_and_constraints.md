---
section_id: "47"
title: "Planner Facts and Constraints"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689"]
  software_versions: []
  hardware_revisions: ["two matched AMD Strix Halo systems (exact BOM open)"]
related_sections: ["23", "24", "25", "37", "38", "42", "43", "74"]
---

# Facts and constraints

| Claim | Planner consequence |
|---|---|
| **[VERIFIED]** `llama-server` provides split mode, tensor-split proportions, main device, automatic fit, and per-device fit margin controls. | Candidate generation can map to concrete runtime knobs [S47-LLAMA-SERVER]. |
| **[VERIFIED]** Upstream labels tensor split experimental and says non-NVIDIA performance is not guaranteed; layer split minimizes transfers but needs enough tokens for throughput. | HIP/Vulkan and USB4 behavior must be measured locally [S47-LLAMA-MGPU]. |
| **[VERIFIED]** The pinned build declares HIP, HIP graphs, optional RCCL, Vulkan, Vulkan validation, and Vulkan result-check options separately. | A backend is part of the compatibility identity, not a transparent substitution [S47-LLAMA-CMAKE]. |
| **[VERIFIED]** `llama-bench` accepts model, prompt, generation, batch, ubatch, repetitions, and output-format controls. | It is one low-level measurement tool; service and fault workloads remain necessary [S47-LLAMA-BENCH]. |
| **[VERIFIED]** ROCmFPX and CachyLLama heads are distinct forks at the pinned revisions. | The planner must fingerprint the actual integrated commit and patches, not repository name [S47-ROCMFPX, S47-CACHY]. |

**[INFERENCE]** A globally fastest average plan can violate latency, quality, memory, power, or recovery requirements. Planning is multi-objective with hard constraints and an explicitly chosen objective.

**[INFERENCE]** Cache layout and scheduler settings change both available memory and workload shape, so they cannot be tuned independently of topology.

**[ASSUMPTION]** Both systems are matched and connected by two independent usable links. Exact identity and independence remain machine-validation dependencies.

## Required planner inputs

- model bytes hash, tokenizer/chat template, quantization, architecture, layer/expert/state sizes, context and output buckets;
- runtime commit/dirty patch hash, compiler, HIP/ROCm/Mesa/Vulkan/RCCL versions and build flags;
- machine/BIOS/firmware/GPU identity, memory limits, NUMA/CPU layout, thermal and power profile;
- each link's latency/bandwidth/error distribution and failover state; collective curves by size/concurrency;
- backend operator support/correctness, kernel latency distributions, graph-capture compatibility;
- cache format/fingerprint, ownership/layout, hit/restore/writeback curves;
- objective weights and hard limits for TTFT, ITL, throughput, quality, memory, power, availability.
