# 09 — HaloKV Persistent Cache

Defines the distributed SSD-backed prefix and inference-state cache.

Research status: source-backed design complete; machine validation and policy decisions remain open.

Last category review: 2026-07-17. Sections 56-65 use pinned predecessor/source evidence where available, keep HaloKV structures as recommendations, and route disruptive validation through disposable targets and the Section 80 fault-authorization boundary.

- 56 — CachyLLama Cache Semantics and Porting Map
- 57 — Compatibility Fingerprints, Versioning, and Topology Identity
- 58 — Rank-Local Ownership and Distributed Restore Coordination
- 59 — Immutable Pages, Segment Files, Indexes, and Prefix DAG
- 60 — System-Prompt Sharing, Deduplication, Copy-on-Write, and Continuations
- 61 — Attention KV, Recurrent, MTP, Speculative, Sampling, and RNG State
- 62 — Async I/O, io_uring, Prefetch, DRAM Tiers, and GPU Mapping
- 63 — Durability Modes, Atomic Commit, Crash Recovery, and Corruption Handling
- 64 — Eviction, Garbage Collection, Quotas, User Isolation, and Privacy
- 65 — Cache Inspection, Migration, Benchmarking, Write Amplification, and SSD Endurance
