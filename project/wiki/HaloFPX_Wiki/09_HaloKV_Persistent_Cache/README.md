# 09 — HaloKV Persistent Cache

## Category manifest

- **Purpose:** Define persistent cache identity, ownership, durability, recovery, isolation, and operation.
- **Authoritative files:** This manifest, the 10 linked section artifact sets, and accepted cache decisions.
- **Current owner:** Cache implementation workers own runtime evidence. Documentation workers own routing.
- **Status:** Source-backed design complete. Machine validation and policy choices remain open.
- **Last verified date:** 2026-08-12 for current routing. Category technical review remains dated 2026-07-17.
- **Source commits:** CachyLLama `6be745998f568e379ea197fcf827baec73ff9940`; the [decision map](../decision-map.md) routes exact HaloFPX commits.
- **Related decisions:** [Decision map](../decision-map.md) routes the implementation decision index and accepted records.
- **Related evidence:** [Evidence map](../evidence-map.md) and the [implementation evidence index](../../../../docs/halofpx/README.md).
- **Open work:** Product composition remains paused. No end-user two-node cache product is accepted.
- **Next safe action:** Treat corrupt, incomplete, incompatible, or unauthorized state as a miss and recompute.

## 2026-08-12 current priority authority

HaloFPX is model-general. Persistent prompt and KV-state work is ordered by
integrity first, then correct reuse across process restart and verified prefix
reuse. A cache hit must preserve the applicable continuation semantics;
corrupt, incomplete, incompatible, stale, or unauthorized state is a miss and
must follow a correct cold-recomputation path.

**[VERIFIED]** PR #20 completed
[GitHub issue #5](https://github.com/JCFrags/HaloFPX/issues/5): the reachable
run-local SSD prompt-cache path now rejects same-size content corruption. That
directory is process-scoped and this does not establish restart reuse.
**[VERIFIED current work authority]**
[GitHub issue #14](https://github.com/JCFrags/HaloFPX/issues/14) is the active
P0 execution tracker for fresh-process exact-key reuse, deterministic
continuation, corruption-miss, and compatibility-mismatch behavior. Accepted
Project Lead/source decisions remain authoritative. HaloKV v1, distributed
cache composition, and target performance remain separate claims.

Cache lookup/validation/restore time and avoided prompt work must be measured
separately from the cold/cache-off prompt-processing engine and from token
generation. Target performance claims require the real dual-Strix-Halo Linux
machines; local Windows checks cannot establish cache, prompt, or generation
speed.

Defines the distributed SSD-backed prefix and inference-state cache.

Research status: source-backed design complete; machine validation and policy decisions remain open.

Last category review: 2026-07-17.
Sections 56–65 use pinned predecessor or source evidence where available.
The sections identify HaloKV structures as recommendations.
The sections route disruptive validation through disposable targets.
Section 80 defines the fault-authorization boundary.

The canonical titles use these terms:
directed acyclic graph (DAG), dynamic random-access memory (DRAM),
multi-token prediction (MTP), and random number generator (RNG).

- [56 — CachyLLama Cache Semantics and Porting Map](56_CachyLLama_Cache_Semantics_and_Porting_Map/README.md)
- [57 — Compatibility Fingerprints, Versioning, and Topology Identity](57_Compatibility_Fingerprints_Versioning_and_Topology_Identity/README.md)
- [58 — Rank-Local Ownership and Distributed Restore Coordination](58_Rank_Local_Ownership_and_Distributed_Restore_Coordination/README.md)
- [59 — Immutable Pages, Segment Files, Indexes, and Prefix DAG](59_Immutable_Pages_Segment_Files_Indexes_and_Prefix_DAG/README.md)
- [60 — System-Prompt Sharing, Deduplication, Copy-on-Write, and Continuations](60_System_Prompt_Sharing_Deduplication_Copy_on_Write_and_Continuations/README.md)
- [61 — Attention KV, Recurrent, MTP, Speculative, Sampling, and RNG State](61_Attention_KV_Recurrent_MTP_Speculative_Sampling_and_RNG_State/README.md)
- [62 — Async I/O, io_uring, Prefetch, DRAM Tiers, and GPU Mapping](62_Async_I_O_io_uring_Prefetch_DRAM_Tiers_and_GPU_Mapping/README.md)
- [63 — Durability Modes, Atomic Commit, Crash Recovery, and Corruption Handling](63_Durability_Modes_Atomic_Commit_Crash_Recovery_and_Corruption_Handling/README.md)
- [64 — Eviction, Garbage Collection, Quotas, User Isolation, and Privacy](64_Eviction_Garbage_Collection_Quotas_User_Isolation_and_Privacy/README.md)
- [65 — Cache Inspection, Migration, Benchmarking, Write Amplification, and SSD Endurance](65_Cache_Inspection_Migration_Benchmarking_Write_Amplification_and_SSD_Endurance/README.md)
