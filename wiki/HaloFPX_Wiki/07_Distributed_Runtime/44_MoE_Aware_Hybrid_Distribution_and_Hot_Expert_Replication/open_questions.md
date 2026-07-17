---
section_id: "44"
title: "MoE Hybrid Open Questions"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["charlie12345/ROCmFPX@a5605a72768c6562241b248e268e33dc92787394", "ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689"]
  software_versions: ["vLLM@9354f222042986addf20709e5274fc26e0d09745", "Megatron-LM@740c16e6b80a753bea26232148d9bb2d7f0c827a", "DeepEP@dd758caf451848bd150e1046af3d0a73e5fff38d"]
  hardware_revisions: ["dual AMD Strix Halo; exact BOM and USB4 fabric measurements unresolved"]
related_sections: ["34", "38", "42", "43", "45", "47", "52", "76", "80"]
---

# Open questions

| ID | Question | Evidence required | Owner/dependency |
|---|---|---|---|
| O44-01 | Does a target MoE model and its full runtime state fit on one node with required safety headroom? | E44-01 exact allocation ledger | sections 19, 29, 34 |
| O44-02 | Are hot experts stable by layer, workload class, and prefill/decode phase? | E44-02 repeated traces and churn statistics | section 34 |
| O44-03 | Does token-wise dispatch/combine beat coarse hidden-state handoff at observed shapes? | E44-03 and E44-05 matched p50/p95/p99 results | sections 43, 52, 76 |
| O44-04 | Should attention be replicated, tensor-sharded, or kept with a pipeline stage around MoE layers? | full graph/memory map plus matched topologies | sections 42, 43, 47 |
| O44-05 | Which shared-expert placement minimizes latency without displacing required KV/state headroom? | exact shared bytes and overlap experiments | E44-01, E44-04 |
| O44-06 | What per-layer replica set maximizes critical-path benefit under an exact byte budget? | trace replay plus cost model validated against E44-05 | sections 34, 38 |
| O44-07 | Can the custom transport coalesce top-k assignments per destination without changing combine semantics? | wire-format prototype and output-equivalence test | sections 49, 53, 78 |
| O44-08 | Which boundary is safe and affordable for map changes: model reload, session drain, or batch/step barrier? | E44-06 latency and stale-epoch tests | sections 45, 46 |
| O44-09 | What is the correct degraded mode when peer-only cold experts disappear? | full-model fit/reload timing, state replay, fault injection | sections 48, 58, 80 |

## Current assumptions awaiting evidence

- **[ASSUMPTION]** The two Strix Halo nodes can expose enough transport bandwidth and sufficiently low decode latency for some fine-grained expert plan.
- **[ASSUMPTION]** Representative workloads have reusable short-horizon expert skew. Training-system observations from FlexMoE/FasterMoE do not establish this for HaloFPX inference [S44-05][S44-08].
- **[ASSUMPTION]** Exact expert tensors can be copied into inactive backend allocations without forcing unsafe graph-wide rebuilds.
- **[ASSUMPTION]** A correct baseline can retain enough memory headroom for at least one useful replica on each rank.

## Decision gates

- **[RECOMMENDATION]** Choose coarse pipeline unless E44-05 shows a repeatable fine-grained win under matched correctness and thermals.
- **[RECOMMENDATION]** Keep placement static unless E44-02 shows hot-set evolution that is both predictable enough and valuable enough to amortize reconfiguration.
- **[RECOMMENDATION]** Do not promise degraded single-node service unless E44-01 and E44-06 prove a complete compatible plan can load and resume.

## Internet follow-up

- Track pinned changes to vLLM EPLB, Megatron token dispatch, DeepEP backend prerequisites, and ROCmFPX tensor/expert placement. Record changed commits before importing conclusions.
- Review new inference-focused replication papers only as candidates until code, model revisions, hardware, and evaluation artifacts are available.
- Watch ROCm/RCCL and Vulkan upstreams for unequal all-to-all, graph-capture, and GPU-visible inter-node transport capabilities relevant to section 24/25/49.

## Three-way split

1. **Source questions answerable now:** upstream algorithms and APIs are documented, but their CUDA/NCCL results are not portable proof.
2. **Machine questions:** O44-01 through O44-09 require the target models and dual-Halo fabric.
3. **Decisions:** base topology, replica set, cadence, and fallback remain deliberately unselected.
