# Master Research Orchestrator Prompt

The project is a custom local LLM inference stack for two matched AMD Strix Halo systems connected by two tuned, low-latency USB4 host-to-host links. The intended source base is a fork of `charlie12345/ROCmFPX`, selectively incorporating the persistent SSD-backed KV-cache and agent-serving ideas from `fewtarius/CachyLLama` and `fewtarius/llama-ai`, while tracking `ggml-org/llama.cpp`. The product must choose among replication, remote speculative decoding, two-way tensor parallelism, pipeline parallelism, and MoE-aware hybrid execution, with a rank-local persistent cache and a specialized dual-link transport. The wiki will be consumed by local Codex and human engineers before and during implementation.

Build the complete HaloFPX LLM Wiki using the section registry in `section_index.json`. Produce all 12 category folders and all 86 numbered section folders at their exact registered paths.

Work category by category. Apply `OUTPUT_STANDARD.md` to every section. Use current primary sources and exact commits, versions, document revisions, and dates. Maintain stable source IDs, claim labels, cross-links, applicability, conflicts, Internet research tasks, on-machine validation tasks, and contingent decisions. Do not invent measurements or generalize project-local benchmarks.

Return one downloadable `HaloFPX_Wiki/` folder containing:
- a root `README.md` and root manifest;
- all category and section folders;
- a consolidated glossary;
- assumption, open-question, decision, and source ledgers;
- an unresolved-conflict report;
- a machine-research checklist;
- no prompt-package files.

If output limits prevent a complete high-quality wiki, return completed categories as separate downloadable folders without omitting the required structure.
