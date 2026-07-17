# Research Agent Prompt — 65: Cache Inspection, Migration, Benchmarking, Write Amplification, and SSD Endurance

The project is a custom local LLM inference stack for two matched AMD Strix Halo systems connected by two tuned, low-latency USB4 host-to-host links. The intended source base is a fork of `charlie12345/ROCmFPX`, selectively incorporating the persistent SSD-backed KV-cache and agent-serving ideas from `fewtarius/CachyLLama` and `fewtarius/llama-ai`, while tracking `ggml-org/llama.cpp`. The product must choose among replication, remote speculative decoding, two-way tensor parallelism, pipeline parallelism, and MoE-aware hybrid execution, with a rank-local persistent cache and a specialized dual-link transport. The wiki will be consumed by local Codex and human engineers before and during implementation.

Research and build the wiki section **65: Cache Inspection, Migration, Benchmarking, Write Amplification, and SSD Endurance**.

**Required coverage:** Define administration and analysis tools for listing, validating, exporting, importing, migrating, compacting, deleting, and measuring cache entries. Specify benchmark matrices for restore, store, hit rate, lookup, concurrent I/O, long contexts, power loss, and endurance, including write-amplification accounting and SMART monitoring.

Use current primary sources and exact repository commits, releases, document revisions, and dates. Separate verified facts, inferences, assumptions, recommendations, open questions, and actual measurements. Explain the implications for HaloFPX. Include concrete Internet follow-up and on-machine validation tasks; never invent benchmark results.

Return a downloadable folder named:

```text
09_HaloKV_Persistent_Cache/65_Cache_Inspection_Migration_Benchmarking_Write_Amplification_and_SSD_Endurance/
```

Follow the included `OUTPUT_STANDARD.md`. The folder must contain `README.md`, `facts_and_constraints.md`, `design_implications.md`, `procedures_and_checks.md`, `open_questions.md`, `sources.md`, and `section.yaml`, plus diagrams, scripts, data, or evidence only when useful. Use relative cross-links and stable section IDs, keep pages retrieval-friendly, and identify any conclusions that depend on other wiki sections.
