# Category Research Agent Prompt — 05: Performance Software and Tools

The project is a custom local LLM inference stack for two matched AMD Strix Halo systems connected by two tuned, low-latency USB4 host-to-host links. The intended source base is a fork of `charlie12345/ROCmFPX`, selectively incorporating the persistent SSD-backed KV-cache and agent-serving ideas from `fewtarius/CachyLLama` and `fewtarius/llama-ai`, while tracking `ggml-org/llama.cpp`. The product must choose among replication, remote speculative decoding, two-way tensor parallelism, pipeline parallelism, and MoE-aware hybrid execution, with a rank-local persistent cache and a specialized dual-link transport. The wiki will be consumed by local Codex and human engineers before and during implementation.

Build the complete wiki category **05: Performance Software and Tools**. Research every numbered section below as a separate self-contained folder:

- `24_HIP_HSA_RCCL_Memory_Coherence_and_Synchronization/` — 24: HIP, HSA, RCCL, Memory Coherence, and Synchronization
- `25_Vulkan_RADV_Host_Visible_Memory_and_Synchronization/` — 25: Vulkan, RADV, Host-Visible Memory, and Synchronization
- `26_Compiler_CMake_Linker_and_Reproducible_Toolchain/` — 26: Compiler, CMake, Linker, and Reproducible Toolchain
- `27_Profiling_Tracing_Debugging_and_Hardware_Counter_Collection/` — 27: Profiling, Tracing, Debugging, and Hardware-Counter Collection
- `28_Host_System_Tuning_CPU_IRQ_Scheduler_Cgroups_and_Filesystems/` — 28: Host System Tuning: CPU, IRQ, Scheduler, Cgroups, and Filesystems

Apply `../OUTPUT_STANDARD.md` to every section. Use current primary sources, exact commits/versions/dates, and explicit claim labels. Separate Internet research from required on-machine investigation. Do not invent measurements or silently resolve conflicting evidence.

Return one downloadable folder named `05_Performance_Software_and_Tools/` containing the numbered section folders at the exact paths listed above, plus a category `README.md` that summarizes the category, identifies authoritative pages, links all sections, and highlights cross-category dependencies.
