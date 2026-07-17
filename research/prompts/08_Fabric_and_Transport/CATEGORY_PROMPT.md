# Category Research Agent Prompt — 08: Fabric and Transport

The project is a custom local LLM inference stack for two matched AMD Strix Halo systems connected by two tuned, low-latency USB4 host-to-host links. The intended source base is a fork of `charlie12345/ROCmFPX`, selectively incorporating the persistent SSD-backed KV-cache and agent-serving ideas from `fewtarius/CachyLLama` and `fewtarius/llama-ai`, while tracking `ggml-org/llama.cpp`. The product must choose among replication, remote speculative decoding, two-way tensor parallelism, pipeline parallelism, and MoE-aware hybrid execution, with a rank-local persistent cache and a specialized dual-link transport. The wiki will be consumed by local Codex and human engineers before and during implementation.

Build the complete wiki category **08: Fabric and Transport**. Research every numbered section below as a separate self-contained folder:

- `49_Fabric_Requirements_and_Transport_Abstraction/` — 49: Fabric Requirements and Transport Abstraction
- `50_USB4STREAM_and_thunderbolt_net_Implementation_Options/` — 50: USB4STREAM and thunderbolt-net Implementation Options
- `51_Existing_ggml_RPC_and_ROCmFPX_RDMA_Transport_Audit/` — 51: Existing ggml RPC and ROCmFPX RDMA Transport Audit
- `52_Dual_Link_Multipath_Striping_Alternation_Hedging_and_Failover/` — 52: Dual-Link Multipath: Striping, Alternation, Hedging, and Failover
- `53_Message_Framing_Credits_Flow_Control_Integrity_and_Security/` — 53: Message Framing, Credits, Flow Control, Integrity, and Security
- `54_GPU_Visible_Buffers_Coherence_Copies_and_Zero_Copy_Options/` — 54: GPU-Visible Buffers, Coherence, Copies, and Zero-Copy Options
- `55_Fabric_Microbenchmark_Plan_and_USB4_Kernel_Patch_Decision/` — 55: Fabric Microbenchmark Plan and USB4 Kernel-Patch Decision

Apply `../OUTPUT_STANDARD.md` to every section. Use current primary sources, exact commits/versions/dates, and explicit claim labels. Separate Internet research from required on-machine investigation. Do not invent measurements or silently resolve conflicting evidence.

Return one downloadable folder named `08_Fabric_and_Transport/` containing the numbered section folders at the exact paths listed above, plus a category `README.md` that summarizes the category, identifies authoritative pages, links all sections, and highlights cross-category dependencies.
