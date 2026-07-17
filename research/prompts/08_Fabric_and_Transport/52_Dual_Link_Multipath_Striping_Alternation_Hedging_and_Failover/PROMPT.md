# Research Agent Prompt — 52: Dual-Link Multipath: Striping, Alternation, Hedging, and Failover

The project is a custom local LLM inference stack for two matched AMD Strix Halo systems connected by two tuned, low-latency USB4 host-to-host links. The intended source base is a fork of `charlie12345/ROCmFPX`, selectively incorporating the persistent SSD-backed KV-cache and agent-serving ideas from `fewtarius/CachyLLama` and `fewtarius/llama-ai`, while tracking `ggml-org/llama.cpp`. The product must choose among replication, remote speculative decoding, two-way tensor parallelism, pipeline parallelism, and MoE-aware hybrid execution, with a rank-local persistent cache and a specialized dual-link transport. The wiki will be consumed by local Codex and human engineers before and during implementation.

Research and build the wiki section **52: Dual-Link Multipath: Striping, Alternation, Hedging, and Failover**.

**Required coverage:** Design and compare application-level policies for two USB4 links: best-link selection, direction separation, alternating collectives, proportional striping, latency hedging, health scoring, packet reordering, sequence handling, reconnect, and one-link degradation. Define workloads and metrics that determine the policy per message size.

Use current primary sources and exact repository commits, releases, document revisions, and dates. Separate verified facts, inferences, assumptions, recommendations, open questions, and actual measurements. Explain the implications for HaloFPX. Include concrete Internet follow-up and on-machine validation tasks; never invent benchmark results.

Return a downloadable folder named:

```text
08_Fabric_and_Transport/52_Dual_Link_Multipath_Striping_Alternation_Hedging_and_Failover/
```

Follow the included `OUTPUT_STANDARD.md`. The folder must contain `README.md`, `facts_and_constraints.md`, `design_implications.md`, `procedures_and_checks.md`, `open_questions.md`, `sources.md`, and `section.yaml`, plus diagrams, scripts, data, or evidence only when useful. Use relative cross-links and stable section IDs, keep pages retrieval-friendly, and identify any conclusions that depend on other wiki sections.
