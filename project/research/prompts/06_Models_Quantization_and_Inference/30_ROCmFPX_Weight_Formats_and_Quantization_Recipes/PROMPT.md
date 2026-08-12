# Research Agent Prompt — 30: ROCmFPX Weight Formats and Quantization Recipes

The project is a custom local LLM inference stack for two matched AMD Strix Halo systems connected by two tuned, low-latency USB4 host-to-host links. The intended source base is a fork of `charlie12345/ROCmFPX`, selectively incorporating the persistent SSD-backed KV-cache and agent-serving ideas from `fewtarius/CachyLLama` and `fewtarius/llama-ai`, while tracking `ggml-org/llama.cpp`. The product must choose among replication, remote speculative decoding, two-way tensor parallelism, pipeline parallelism, and MoE-aware hybrid execution, with a rank-local persistent cache and a specialized dual-link transport. The wiki will be consumed by local Codex and human engineers before and during implementation.

Research and build the wiki section **30: ROCmFPX Weight Formats and Quantization Recipes**.

**Required coverage:** Document ROCmFP3/4/6/8 layouts, block sizes, scale encodings, FAST, STRIX_LEAN, COHERENT, AGENT, and other presets, protected tensor policies, expected bits per weight, kernel support, model compatibility, and known quality risks. Distinguish weight quantization from KV-cache quantization and record exact GGUF type identifiers.

Use current primary sources and exact repository commits, releases, document revisions, and dates. Separate verified facts, inferences, assumptions, recommendations, open questions, and actual measurements. Explain the implications for HaloFPX. Include concrete Internet follow-up and on-machine validation tasks; never invent benchmark results.

Return a downloadable folder named:

```text
06_Models_Quantization_and_Inference/30_ROCmFPX_Weight_Formats_and_Quantization_Recipes/
```

Follow the included `OUTPUT_STANDARD.md`. The folder must contain `README.md`, `facts_and_constraints.md`, `design_implications.md`, `procedures_and_checks.md`, `open_questions.md`, `sources.md`, and `section.yaml`, plus diagrams, scripts, data, or evidence only when useful. Use relative cross-links and stable section IDs, keep pages retrieval-friendly, and identify any conclusions that depend on other wiki sections.
