# Category Research Agent Prompt — 01: Wiki Governance

The project is a custom local LLM inference stack for two matched AMD Strix Halo systems connected by two tuned, low-latency USB4 host-to-host links. The intended source base is a fork of `charlie12345/ROCmFPX`, selectively incorporating the persistent SSD-backed KV-cache and agent-serving ideas from `fewtarius/CachyLLama` and `fewtarius/llama-ai`, while tracking `ggml-org/llama.cpp`. The product must choose among replication, remote speculative decoding, two-way tensor parallelism, pipeline parallelism, and MoE-aware hybrid execution, with a rank-local persistent cache and a specialized dual-link transport. The wiki will be consumed by local Codex and human engineers before and during implementation.

Build the complete wiki category **01: Wiki Governance**. Research every numbered section below as a separate self-contained folder:

- `01_Wiki_Architecture_Navigation_and_Root_Manifest/` — 01: Wiki Architecture, Navigation, and Root Manifest
- `02_Evidence_Citation_and_Source_Policy/` — 02: Evidence, Citation, and Source Policy
- `03_Glossary_Naming_and_Stable_Identifiers/` — 03: Glossary, Naming, and Stable Identifiers
- `04_Assumption_Open_Question_and_Decision_Ledgers/` — 04: Assumption, Open-Question, and Decision Ledgers
- `05_Research_Data_and_Benchmark_Artifact_Conventions/` — 05: Research Data and Benchmark Artifact Conventions

Apply `../OUTPUT_STANDARD.md` to every section. Use current primary sources, exact commits/versions/dates, and explicit claim labels. Separate Internet research from required on-machine investigation. Do not invent measurements or silently resolve conflicting evidence.

Return one downloadable folder named `01_Wiki_Governance/` containing the numbered section folders at the exact paths listed above, plus a category `README.md` that summarizes the category, identifies authoritative pages, links all sections, and highlights cross-category dependencies.
