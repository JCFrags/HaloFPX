# Category Research Agent Prompt — 09: HaloKV Persistent Cache

The project is a custom local LLM inference stack for two matched AMD Strix Halo systems connected by two tuned, low-latency USB4 host-to-host links. The intended source base is a fork of `charlie12345/ROCmFPX`, selectively incorporating the persistent SSD-backed KV-cache and agent-serving ideas from `fewtarius/CachyLLama` and `fewtarius/llama-ai`, while tracking `ggml-org/llama.cpp`. The product must choose among replication, remote speculative decoding, two-way tensor parallelism, pipeline parallelism, and MoE-aware hybrid execution, with a rank-local persistent cache and a specialized dual-link transport. The wiki will be consumed by local Codex and human engineers before and during implementation.

Build the complete wiki category **09: HaloKV Persistent Cache**. Research every numbered section below as a separate self-contained folder:

- `56_CachyLLama_Cache_Semantics_and_Porting_Map/` — 56: CachyLLama Cache Semantics and Porting Map
- `57_Compatibility_Fingerprints_Versioning_and_Topology_Identity/` — 57: Compatibility Fingerprints, Versioning, and Topology Identity
- `58_Rank_Local_Ownership_and_Distributed_Restore_Coordination/` — 58: Rank-Local Ownership and Distributed Restore Coordination
- `59_Immutable_Pages_Segment_Files_Indexes_and_Prefix_DAG/` — 59: Immutable Pages, Segment Files, Indexes, and Prefix DAG
- `60_System_Prompt_Sharing_Deduplication_Copy_on_Write_and_Continuations/` — 60: System-Prompt Sharing, Deduplication, Copy-on-Write, and Continuations
- `61_Attention_KV_Recurrent_MTP_Speculative_Sampling_and_RNG_State/` — 61: Attention KV, Recurrent, MTP, Speculative, Sampling, and RNG State
- `62_Async_I_O_io_uring_Prefetch_DRAM_Tiers_and_GPU_Mapping/` — 62: Async I/O, io_uring, Prefetch, DRAM Tiers, and GPU Mapping
- `63_Durability_Modes_Atomic_Commit_Crash_Recovery_and_Corruption_Handling/` — 63: Durability Modes, Atomic Commit, Crash Recovery, and Corruption Handling
- `64_Eviction_Garbage_Collection_Quotas_User_Isolation_and_Privacy/` — 64: Eviction, Garbage Collection, Quotas, User Isolation, and Privacy
- `65_Cache_Inspection_Migration_Benchmarking_Write_Amplification_and_SSD_Endurance/` — 65: Cache Inspection, Migration, Benchmarking, Write Amplification, and SSD Endurance

Apply `../OUTPUT_STANDARD.md` to every section. Use current primary sources, exact commits/versions/dates, and explicit claim labels. Separate Internet research from required on-machine investigation. Do not invent measurements or silently resolve conflicting evidence.

Return one downloadable folder named `09_HaloKV_Persistent_Cache/` containing the numbered section folders at the exact paths listed above, plus a category `README.md` that summarizes the category, identifies authoritative pages, links all sections, and highlights cross-category dependencies.
