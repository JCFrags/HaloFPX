# Category Research Agent Prompt — 02: Project Definition

The project is a custom local LLM inference stack for two matched AMD Strix Halo systems connected by two tuned, low-latency USB4 host-to-host links. The intended source base is a fork of `charlie12345/ROCmFPX`, selectively incorporating the persistent SSD-backed KV-cache and agent-serving ideas from `fewtarius/CachyLLama` and `fewtarius/llama-ai`, while tracking `ggml-org/llama.cpp`. The product must choose among replication, remote speculative decoding, two-way tensor parallelism, pipeline parallelism, and MoE-aware hybrid execution, with a rank-local persistent cache and a specialized dual-link transport. The wiki will be consumed by local Codex and human engineers before and during implementation.

Build the complete wiki category **02: Project Definition**. Research every numbered section below as a separate self-contained folder:

- `06_Project_Charter_Vision_and_Intended_Outcomes/` — 06: Project Charter, Vision, and Intended Outcomes
- `07_Users_Workloads_Personas_and_Use_Cases/` — 07: Users, Workloads, Personas, and Use Cases
- `08_Scope_Non_Goals_Boundaries_and_External_Dependencies/` — 08: Scope, Non-Goals, Boundaries, and External Dependencies
- `09_Functional_Requirements_SLOs_and_Acceptance_Criteria/` — 09: Functional Requirements, SLOs, and Acceptance Criteria
- `10_Architecture_Principles_and_Tradeoff_Framework/` — 10: Architecture Principles and Tradeoff Framework

Apply `../OUTPUT_STANDARD.md` to every section. Use current primary sources, exact commits/versions/dates, and explicit claim labels. Separate Internet research from required on-machine investigation. Do not invent measurements or silently resolve conflicting evidence.

Return one downloadable folder named `02_Project_Definition/` containing the numbered section folders at the exact paths listed above, plus a category `README.md` that summarizes the category, identifies authoritative pages, links all sections, and highlights cross-category dependencies.
