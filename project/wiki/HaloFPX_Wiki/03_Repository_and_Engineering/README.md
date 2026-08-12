# 03 — Repository and Engineering

## Category manifest

- **Purpose:** Map source lineage, code structure, integration, licensing, and engineering controls.
- **Authoritative files:** This manifest, the six linked section artifact sets, and exact repository source.
- **Current owner:** Implementation owners control source. Documentation workers control this manifest.
- **Status:** Structurally complete. All 86 section metadata files pass the Wiki validator.
- **Last verified date:** 2026-07-29 for routing. Section claims retain their own dates.
- **Source commits:** ROCmFPX `a5605a72768c6562241b248e268e33dc92787394`; llama.cpp `788e07dc91d266ad3162a1ce9037665656269689`; llama-ai `1017f3dfdce3ca2b06aa9007b23295db3bb35722`; CachyLLama `6be745998f568e379ea197fcf827baec73ff9940`.
- **Related decisions:** [Decision map](../decision-map.md) and the linked implementation decision index.
- **Related evidence:** [Evidence map](../evidence-map.md) and [`sources/repositories/`](../../../sources/repositories/README.md).
- **Open work:** Keep source lineage and accepted implementation decisions synchronized.
- **Next safe action:** Inspect exact source and the relevant implementation decision before editing engineering guidance.

Maps the source lineage, code structure, integration approach, and engineering controls.

Artifact state: 6/6 required section artifact sets are present. This is structural completeness, not research acceptance or machine validation.

- [11 — Repository Lineage, Branches, Commits, and Frozen Baselines](11_Repository_Lineage_Branches_Commits_and_Frozen_Baselines/README.md)
- [12 — Codebase Architecture and Module Map](12_Codebase_Architecture_and_Module_Map/README.md)
- [13 — ROCmFPX Feature, Kernel, Format, and Patch Inventory](13_ROCmFPX_Feature_Kernel_Format_and_Patch_Inventory/README.md)
- [14 — llama-ai and CachyLLama Feature and Patch Inventory](14_llama_ai_and_CachyLLama_Feature_and_Patch_Inventory/README.md)
- [15 — Integration Patch Stack and Upstream Synchronization Strategy](15_Integration_Patch_Stack_and_Upstream_Synchronization_Strategy/README.md)
- [16 — Build, Dependencies, Licensing, CI, and AI-Agent Workflow](16_Build_Dependencies_Licensing_CI_and_AI_Agent_Workflow/README.md)
