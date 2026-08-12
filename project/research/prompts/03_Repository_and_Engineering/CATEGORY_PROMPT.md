# Category Research Agent Prompt — 03: Repository and Engineering

The project is a custom local LLM inference stack for two matched AMD Strix Halo systems connected by two tuned, low-latency USB4 host-to-host links. The intended source base is a fork of `charlie12345/ROCmFPX`, selectively incorporating the persistent SSD-backed KV-cache and agent-serving ideas from `fewtarius/CachyLLama` and `fewtarius/llama-ai`, while tracking `ggml-org/llama.cpp`. The product must choose among replication, remote speculative decoding, two-way tensor parallelism, pipeline parallelism, and MoE-aware hybrid execution, with a rank-local persistent cache and a specialized dual-link transport. The wiki will be consumed by local Codex and human engineers before and during implementation.

Build the complete wiki category **03: Repository and Engineering**. Research every numbered section below as a separate self-contained folder:

- `11_Repository_Lineage_Branches_Commits_and_Frozen_Baselines/` — 11: Repository Lineage, Branches, Commits, and Frozen Baselines
- `12_Codebase_Architecture_and_Module_Map/` — 12: Codebase Architecture and Module Map
- `13_ROCmFPX_Feature_Kernel_Format_and_Patch_Inventory/` — 13: ROCmFPX Feature, Kernel, Format, and Patch Inventory
- `14_llama_ai_and_CachyLLama_Feature_and_Patch_Inventory/` — 14: llama-ai and CachyLLama Feature and Patch Inventory
- `15_Integration_Patch_Stack_and_Upstream_Synchronization_Strategy/` — 15: Integration Patch Stack and Upstream Synchronization Strategy
- `16_Build_Dependencies_Licensing_CI_and_AI_Agent_Workflow/` — 16: Build, Dependencies, Licensing, CI, and AI-Agent Workflow

Apply `../OUTPUT_STANDARD.md` to every section. Use current primary sources, exact commits/versions/dates, and explicit claim labels. Separate Internet research from required on-machine investigation. Do not invent measurements or silently resolve conflicting evidence.

Return one downloadable folder named `03_Repository_and_Engineering/` containing the numbered section folders at the exact paths listed above, plus a category `README.md` that summarizes the category, identifies authoritative pages, links all sections, and highlights cross-category dependencies.
