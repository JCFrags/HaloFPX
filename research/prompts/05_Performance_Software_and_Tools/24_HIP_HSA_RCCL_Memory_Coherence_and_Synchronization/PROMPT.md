# Research Agent Prompt — 24: HIP, HSA, RCCL, Memory Coherence, and Synchronization

The project is a custom local LLM inference stack for two matched AMD Strix Halo systems connected by two tuned, low-latency USB4 host-to-host links. The intended source base is a fork of `charlie12345/ROCmFPX`, selectively incorporating the persistent SSD-backed KV-cache and agent-serving ideas from `fewtarius/CachyLLama` and `fewtarius/llama-ai`, while tracking `ggml-org/llama.cpp`. The product must choose among replication, remote speculative decoding, two-way tensor parallelism, pipeline parallelism, and MoE-aware hybrid execution, with a rank-local persistent cache and a specialized dual-link transport. The wiki will be consumed by local Codex and human engineers before and during implementation.

Research and build the wiki section **24: HIP, HSA, RCCL, Memory Coherence, and Synchronization**.

**Required coverage:** Research HIP and HSA behavior on gfx1151: device memory, mapped pinned host memory, fine- and coarse-grained allocations, system-scope atomics and fences, streams, events, graph capture, queue behavior, peer limitations, profiling, and RCCL or network-plugin options. Identify APIs suitable for GPU-produced transport buffers and two-rank synchronization.

Use current primary sources and exact repository commits, releases, document revisions, and dates. Separate verified facts, inferences, assumptions, recommendations, open questions, and actual measurements. Explain the implications for HaloFPX. Include concrete Internet follow-up and on-machine validation tasks; never invent benchmark results.

Return a downloadable folder named:

```text
05_Performance_Software_and_Tools/24_HIP_HSA_RCCL_Memory_Coherence_and_Synchronization/
```

Follow the included `OUTPUT_STANDARD.md`. The folder must contain `README.md`, `facts_and_constraints.md`, `design_implications.md`, `procedures_and_checks.md`, `open_questions.md`, `sources.md`, and `section.yaml`, plus diagrams, scripts, data, or evidence only when useful. Use relative cross-links and stable section IDs, keep pages retrieval-friendly, and identify any conclusions that depend on other wiki sections.
