# 09 — HaloKV Persistent Cache

## Category manifest

- **Purpose:** Define persistent cache identity, ownership, durability, recovery, isolation, and operation.
- **Authoritative files:** This manifest, the 10 linked section artifact sets, and accepted cache decisions.
- **Current owner:** Cache implementation workers own runtime evidence. Documentation workers own routing.
- **Status:** Bounded CPU exact-key restart qualification, the optional run-local EVP provider, a default-off world-1 prefix product shell, and its inclusive native cache-maintenance telemetry are complete. Positive model-backed prefix reuse, target performance, and distributed product work remain open.
- **Last verified date:** 2026-08-13 for current routing. Category technical review remains dated 2026-07-17.
- **Source commits:** CachyLLama `6be745998f568e379ea197fcf827baec73ff9940`; the [decision map](../decision-map.md) routes exact HaloFPX commits.
- **Related decisions:** [Decision map](../decision-map.md) routes the implementation decision index and accepted records.
- **Related evidence:** [Evidence map](../evidence-map.md) and the [implementation evidence index](../../../../docs/halofpx/README.md).
- **Open work:** The world-1 shell remains cold until trusted live-loader authority exists. No positive prefix hit or end-user two-node cache product is accepted.
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
**[VERIFIED]** [PR #23](https://github.com/JCFrags/HaloFPX/pull/23) merged as
`aee627bd46de21327c9082f7915818430d38f453` and closed
[issue #14](https://github.com/JCFrags/HaloFPX/issues/14). A default-off
Linux CPU fixture proved fresh-process exact-key reuse, deterministic
continuation, compatibility miss/recomputation, and same-size corruption
miss/recomputation for world size 1, rank 0, ordinary transformer memory, and
greedy memoryless sampling. It does not establish prefix reuse, recurrent or
hybrid state, multiple slots, target performance, or two-rank coordination.

**[VERIFIED]** [PR #27](https://github.com/JCFrags/HaloFPX/pull/27) merged as
`bf420e9f1db4ea4ba1d7c87771b6a4d662b5be67` and adds an optional OpenSSL
EVP SHA-256 provider to the separate run-local SSD prompt cache. It preserves
full-file integrity checks and does not change the PR #23 context-store
semantics. No end-to-end cache-speed claim exists.

**[VERIFIED]** The default-off L10f product shell composes authenticated
manifest-only boundary discovery, exact longest-prefix selection, one-shot
state installation, residual accounting, and native cache telemetry for the
narrow world-1 ordinary-transformer profile. No trusted live-loader authority
provider exists, so its reachable server behavior remains cold and opens no
store. The hosted test uses a preseeded canonical prefix; automatic system/chat
boundary capture and model-backed suffix replay are not established.

**[VERIFIED]** ADR-0059/L10g adds request-attempt-safe native telemetry for
automatic slot transition, full lookup preparation/lookup, state install
through rollback, synchronous idle-slot saves, checked aggregate time, and
semantic successful state-apply input bytes. Compile-time and runtime OFF
remain absent. The byte field is not physical/read/total I/O, the existing A/B
v1 schema is unchanged, and client TTFT remains the benefit arbiter. Local
WSL2 model execution proves wiring only, not target speed.

**[OPEN]** [Issue #26](https://github.com/JCFrags/HaloFPX/issues/26) owns
restart-safe cache-state coordination across two RPC ranks.
[Issue #32](https://github.com/JCFrags/HaloFPX/issues/32) owns verified longest
exact-token prefix selection and model-backed suffix replay. [Issue #33](https://github.com/JCFrags/HaloFPX/issues/33) owns
deriving the compatibility authority from the live model and inference plan.
**[VERIFIED]** The default-off standalone
[live-authority contract](../../../../docs/halofpx/cache-live-authority-v1.md)
now derives exact model/shard bytes, typed model/tokenizer/template facts,
immutable runtime/state ABI, and a fixed two-rank topology into the closed
compatibility root. It remains outside the product path: loader-time typed
capture, resolved context/request semantics, reload invalidation, and the live
server adapter remain **[OPEN]**.
[Issue #18](https://github.com/JCFrags/HaloFPX/issues/18) owns cache-source,
restored-work, and phase-attribution metrics. HaloKV v1 product composition,
prefix reuse, and target performance remain separate claims.

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
