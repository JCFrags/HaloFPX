# Category Research Agent Prompt — 12: Project Execution and Governance

The project is a custom local LLM inference stack for two matched AMD Strix Halo systems connected by two tuned, low-latency USB4 host-to-host links. The intended source base is a fork of `charlie12345/ROCmFPX`, selectively incorporating the persistent SSD-backed KV-cache and agent-serving ideas from `fewtarius/CachyLLama` and `fewtarius/llama-ai`, while tracking `ggml-org/llama.cpp`. The product must choose among replication, remote speculative decoding, two-way tensor parallelism, pipeline parallelism, and MoE-aware hybrid execution, with a rank-local persistent cache and a specialized dual-link transport. The wiki will be consumed by local Codex and human engineers before and during implementation.

Build the complete wiki category **12: Project Execution and Governance**. Research every numbered section below as a separate self-contained folder:

- `82_Implementation_Roadmap_Epics_Dependencies_and_Exit_Criteria/` — 82: Implementation Roadmap, Epics, Dependencies, and Exit Criteria
- `83_Risk_Register_Failure_Modes_Mitigations_and_Contingencies/` — 83: Risk Register, Failure Modes, Mitigations, and Contingencies
- `84_On_Machine_Research_Plan_Experiment_Cards_and_Lab_Notebook/` — 84: On-Machine Research Plan, Experiment Cards, and Lab Notebook
- `85_Internet_Research_Backlog_Upstream_Watch_and_Knowledge_Freshness/` — 85: Internet Research Backlog, Upstream Watch, and Knowledge Freshness
- `86_Issues_Labels_Milestones_ADRs_Code_Review_and_Contribution_Process/` — 86: Issues, Labels, Milestones, ADRs, Code Review, and Contribution Process

Apply `../OUTPUT_STANDARD.md` to every section. Use current primary sources, exact commits/versions/dates, and explicit claim labels. Separate Internet research from required on-machine investigation. Do not invent measurements or silently resolve conflicting evidence.

Return one downloadable folder named `12_Project_Execution_and_Governance/` containing the numbered section folders at the exact paths listed above, plus a category `README.md` that summarizes the category, identifies authoritative pages, links all sections, and highlights cross-category dependencies.
