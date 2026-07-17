# HaloFPX Project Goal

This file records the user-directed top-level goal for the HaloFPX project as of 2026-07-17. It defines the destination and phase order. The Wiki owns supporting evidence and evolving design analysis; decision records and experiment results own approved implementation choices.

## Goal

Build a stable, provenance-preserving HaloFPX inference stack for two AMD Strix Halo nodes by:

1. Forking [`charlie12345/ROCmFPX`](https://github.com/charlie12345/ROCmFPX) as the canonical integration repository.
2. Incorporating the useful SSD-backed KV-cache management and other selected Strix Halo features represented by [`fewtarius/llama-ai`](https://github.com/fewtarius/llama-ai) and its pinned CachyLLama component.
3. Once the combined runtime is stable and working, optimizing inference for an approximately 200–230 GB model partitioned across two Strix Halo nodes connected by dual USB4 links.

The intended result is one maintainable llama.cpp-derived runtime that combines ROCmFPX's AMD/ROCm work with selected cache, session, lifecycle, and operational capabilities from the llama-ai/CachyLLama lineage, then extends that foundation into an evidence-backed dual-node inference system.

## Phase 1 — Stable integration fork

Create a writable project fork from ROCmFPX and keep ROCmFPX, llama.cpp, llama-ai, and CachyLLama as traceable donor/upstream remotes. Integrate capabilities as audited, buildable patch lanes rather than assuming the repositories have a safe Git merge base.

Initial feature priorities are:

- SSD-backed prompt, session, prefix, and KV-cache reuse where the semantics and performance benefit are validated.
- Restart and continuation behavior with explicit compatibility and invalidation rules.
- Cache lifecycle controls, user/session isolation, corruption detection, diagnostics, and observability.
- Useful model/server lifecycle, administration, packaging, and Strix Halo runtime features discovered through file-level review.
- Preservation of ROCmFPX capabilities needed for AMD Strix Halo inference, including the project's selected ROCmFPX, RPC, quantization, and speculative/MTP paths.

The llama-ai repository is GPL-3.0-or-later and its documentation has a separate CC-BY-NC-SA-4.0 license, while the pinned CachyLLama code lineage is MIT. Therefore, “merge useful features” means inspect provenance at file and commit level, then either port compatible code, redesign the behavior from documented requirements, or explicitly reject the feature. It does not authorize an unreviewed wholesale code merge or silent relicensing.

Phase 1 is stable only when:

- Both target nodes build from documented, pinned source and toolchain inputs.
- Every imported or reimplemented feature has source lineage, license disposition, and a retain/redesign/reject decision.
- Baseline inference, supported quantization paths, server behavior, and applicable RPC/MTP behavior pass regression tests.
- Cache hits are demonstrably correct; incompatible, partial, or corrupt cache state causes a miss or recomputation rather than accepted invalid state.
- Cold start, warm start, checkpoint/restore, eviction, isolation, and crash-recovery behavior are tested with retained raw evidence.
- Single-node operation remains usable for development and recovery.
- Deployed binaries can be mapped back to exact commits, build flags, dependencies, and configuration.

## Phase 2 — Dual-node 200–230 GB inference

After the integration fork passes the Phase 1 stability gates, optimize a large-model workload whose stored model footprint is approximately 200–230 GB across the two Strix Halo nodes over dual USB4.

This phase will evaluate and measure the viable execution modes—such as tensor splitting, pipeline or stage splitting, expert/MoE placement, remote speculation, and hybrids—rather than preselecting one without evidence. The system must define rank ownership, sampling ownership, KV/cache ownership, transport and retry semantics, failure behavior, and the single-node fallback or recovery path.

Optimization priorities are:

- Fit the selected model and its runtime state safely within the combined memory budget.
- Establish a correct single-node or reduced-model baseline before attributing gains to distribution.
- Use both USB4 links only when the operating system exposes independent, measurable paths and the transport can exploit them correctly.
- Minimize synchronization, activation, and KV movement across the fabric; prefer locality-aware placement and rank-local persistent cache state.
- Improve time to first token, steady-state generation rate, long-context behavior, reliability, and energy/thermal stability without accepting quality regressions or silent corruption.

Phase 2 succeeds when a fully specified and reproducible configuration can load and run the target model class across both nodes, produces correct outputs under controlled tests, and shows a measured operational benefit over the applicable baseline. If the evidence shows that a proposed distributed mode cannot overcome communication or software constraints, that mode receives a documented no-go result rather than an unsupported performance claim.

## Current research pins

These are research pins, not permanent release selections:

- ROCmFPX: `a5605a72768c6562241b248e268e33dc92787394`
- llama-ai: `1017f3dfdce3ca2b06aa9007b23295db3bb35722`
- CachyLLama component recorded by the donor gitlink and reviewed Wiki evidence: `6be745998f568e379ea197fcf827baec73ff9940`

Pins must be refreshed deliberately, with upstream changes reviewed before promotion.

## Evidence and decision routing

- Preserve fetched archives, commits, licenses, manifests, and raw experiment data under `sources/`.
- Keep research prompts under `research/prompts/` and promote reviewed findings into `wiki/HaloFPX_Wiki/`.
- Record executable trials under `experiments/` and material architectural decisions as linked decision records.
- Use [`references/agent-harness.md`](references/agent-harness.md) to route Agent Harness concepts; the reference authority remains `C:\Users\britt\Documents\Agent_Harness`.
- Do not present unverified compatibility, benchmark, capacity, or performance claims as project facts.

## Related project material

- [`README.md`](README.md)
- [`wiki/HaloFPX_Wiki/03_Repository_and_Engineering/13_ROCmFPX_Feature_Kernel_Format_and_Patch_Inventory/README.md`](wiki/HaloFPX_Wiki/03_Repository_and_Engineering/13_ROCmFPX_Feature_Kernel_Format_and_Patch_Inventory/README.md)
- [`wiki/HaloFPX_Wiki/03_Repository_and_Engineering/14_llama_ai_and_CachyLLama_Feature_and_Patch_Inventory/README.md`](wiki/HaloFPX_Wiki/03_Repository_and_Engineering/14_llama_ai_and_CachyLLama_Feature_and_Patch_Inventory/README.md)
- [`wiki/HaloFPX_Wiki/03_Repository_and_Engineering/15_Integration_Patch_Stack_and_Upstream_Synchronization_Strategy/README.md`](wiki/HaloFPX_Wiki/03_Repository_and_Engineering/15_Integration_Patch_Stack_and_Upstream_Synchronization_Strategy/README.md)
- [`wiki/HaloFPX_Wiki/07_Distributed_Runtime/38_Distributed_Runtime_Goals_Cost_Model_and_Mode_Selection/README.md`](wiki/HaloFPX_Wiki/07_Distributed_Runtime/38_Distributed_Runtime_Goals_Cost_Model_and_Mode_Selection/README.md)
- [`wiki/HaloFPX_Wiki/07_Distributed_Runtime/43_Contiguous_Layer_Pipeline_Parallelism_and_Microbatching/README.md`](wiki/HaloFPX_Wiki/07_Distributed_Runtime/43_Contiguous_Layer_Pipeline_Parallelism_and_Microbatching/README.md)
- [`wiki/HaloFPX_Wiki/09_HaloKV_Persistent_Cache/56_CachyLLama_Cache_Semantics_and_Porting_Map/README.md`](wiki/HaloFPX_Wiki/09_HaloKV_Persistent_Cache/56_CachyLLama_Cache_Semantics_and_Porting_Map/README.md)
- [`wiki/HaloFPX_Wiki/12_Project_Execution_and_Governance/82_Implementation_Roadmap_Epics_Dependencies_and_Exit_Criteria/README.md`](wiki/HaloFPX_Wiki/12_Project_Execution_and_Governance/82_Implementation_Roadmap_Epics_Dependencies_and_Exit_Criteria/README.md)
