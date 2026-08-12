# Persistent SSD-backed KV cache

CachyLlama implements a process-restart-persistent sequence-state cache with checkpoint metadata and hot/warm/cold aging. ROCmFPX already has a safer but run-scoped disk prompt cache. The dominant porting pattern is therefore **retain the data semantics, redesign the persistence layer**.

| Decision | Count |
|---|---:|
| RETAIN | 7 |
| REDESIGN | 6 |
| REJECT | 2 |

<a id="f-001"></a>
### F-001 — Process-restart-persistent sequence cache

**Decision:** `REDESIGN` · **Portability:** `High` · **Maturity:** `M2` · **Confidence:** `High`

**Observed behavior.** Reopens a configured cache root, scans stored checkpoints, and rebuilds the index so target sequence state can be reused after server restart.

**Implementation.** `common/kv-ssd-cache.h`, `common/kv-ssd-cache.cpp`, `tools/server/server-context-page-manager.cpp`

**Dependencies.** C++17 filesystem; llama per-sequence state API; POSIX-style file I/O

**License.** MIT

**ROCmFPX overlap.** ROCmFPX has disk prompt-cache spill, but its owned namespace is deliberately removed on shutdown.

**Porting rationale.** Add an explicit persistent mode to the existing ROCmFPX cache engine rather than replacing its tested run-scoped mode.

**Risks / caveats.** Crash consistency, stale format detection, tenant confidentiality, and restart index integrity.

**Evidence.** [E-030](20-Evidence-Index.md#e-030), [E-031](20-Evidence-Index.md#e-031), [E-068](20-Evidence-Index.md#e-068)

<a id="f-002"></a>
### F-002 — Hot/warm/cold checkpoint tiers

**Decision:** `REDESIGN` · **Portability:** `Medium` · **Maturity:** `M2` · **Confidence:** `High`

**Observed behavior.** Classifies checkpoints by recency and moves them through hot, warm, and cold tiers after turns.

**Implementation.** `common/kv-ssd-cache.h`, `common/kv-ssd-cache.cpp`

**Dependencies.** turn counter; tier RAM budgets; checkpoint metadata

**License.** MIT

**ROCmFPX overlap.** ROCmFPX currently has RAM plus bounded disk entries, not the same three-tier policy.

**Porting rationale.** Preserve the policy objective but implement one budgeted cache with explicit residency state, pressure feedback, and testable promotion/demotion.

**Risks / caveats.** Source defaults and parent overrides disagree; auto budgeting is not cgroup-aware.

**Evidence.** [E-030](20-Evidence-Index.md#e-030), [E-037](20-Evidence-Index.md#e-037), [E-039](20-Evidence-Index.md#e-039)

<a id="f-003"></a>
### F-003 — Target + draft + speculative state record

**Decision:** `RETAIN` · **Portability:** `High` · **Maturity:** `M2` · **Confidence:** `High`

**Observed behavior.** Stores target-model state, optional draft-model state, and speculative-decoder metadata as one logical checkpoint.

**Implementation.** `common/kv-ssd-cache.h`, `tools/server/server-context-ssd-cache.cpp`, `tools/server/server-context-page-manager.cpp`

**Dependencies.** target llama_context; optional draft context; speculative decoder state

**License.** MIT

**ROCmFPX overlap.** ROCmFPX already persists target/draft pairs and stateful MTP metadata.

**Porting rationale.** Unify record metadata and validation with ROCmFPX's existing target/draft/state_spec representation.

**Risks / caveats.** Partial component restores must never be treated as a complete hit.

**Evidence.** [E-030](20-Evidence-Index.md#e-030), [E-063](20-Evidence-Index.md#e-063), [E-067](20-Evidence-Index.md#e-067)

<a id="f-004"></a>
### F-004 — Bounded chunk I/O

**Decision:** `RETAIN` · **Portability:** `High` · **Maturity:** `M2` · **Confidence:** `High`

**Observed behavior.** Reads and writes large state blobs in 64 MiB chunks instead of requiring a single giant host operation.

**Implementation.** `common/kv-ssd-cache.cpp`, `common/kv-ssd-posix.h`

**Dependencies.** pread/pwrite or platform equivalents; bounded temporary buffers

**License.** MIT

**ROCmFPX overlap.** ROCmFPX has robust file commit handling but can reuse a bounded transfer helper where its serialization path benefits.

**Porting rationale.** Port the chunk-loop concept behind the target's current I/O abstraction and preserve its fault injection.

**Risks / caveats.** Short reads/writes and cancellation paths require tests.

**Evidence.** [E-032](20-Evidence-Index.md#e-032), [E-033](20-Evidence-Index.md#e-033)

<a id="f-005"></a>
### F-005 — Optional fsync durability

**Decision:** `REDESIGN` · **Portability:** `High` · **Maturity:** `M2` · **Confidence:** `High`

**Observed behavior.** Can force checkpoint data to stable storage unless no-fsync is selected.

**Implementation.** `common/kv-ssd-cache.h`, `common/kv-ssd-cache.cpp`, `common/arg.cpp`

**Dependencies.** fsync; cache configuration

**License.** MIT

**ROCmFPX overlap.** ROCmFPX already flushes files and directories in its tested commit path.

**Porting rationale.** Represent durability as an explicit strict/relaxed policy and retain ROCmFPX's atomic pair commit.

**Risks / caveats.** CachyLlama writes directly to final names, so fsync alone does not make metadata publication atomic.

**Evidence.** [E-023](20-Evidence-Index.md#e-023), [E-034](20-Evidence-Index.md#e-034)

<a id="f-006"></a>
### F-006 — Checkpoint compatibility fingerprint

**Decision:** `RETAIN` · **Portability:** `High` · **Maturity:** `M2` · **Confidence:** `High`

**Observed behavior.** Rejects state whose model/cache compatibility hash differs from the active context.

**Implementation.** `common/kv-ssd-cache.h`, `common/kv-ssd-cache.cpp`, `tools/server/server-context-page-manager.cpp`

**Dependencies.** model metadata fingerprint; state-format version

**License.** MIT

**ROCmFPX overlap.** ROCmFPX already validates state shape and file integrity but needs a stable restart-persistence namespace key.

**Porting rationale.** Define a versioned fingerprint covering model identity, context layout, KV types, backend-relevant serialization version, and draft/MTP shape.

**Risks / caveats.** Under-specified fingerprints can load semantically incompatible state.

**Evidence.** [E-038](20-Evidence-Index.md#e-038), [E-040](20-Evidence-Index.md#e-040), [E-060](20-Evidence-Index.md#e-060)

<a id="f-007"></a>
### F-007 — Checkpoint token-prefix metadata

**Decision:** `RETAIN` · **Portability:** `High` · **Maturity:** `M2` · **Confidence:** `High`

**Observed behavior.** Persists a bounded token prefix with checkpoint metadata for restart-time prefix matching.

**Implementation.** `common/kv-ssd-cache.h`, `common/kv-ssd-cache.cpp`

**Dependencies.** llama_token representation; metadata serialization

**License.** MIT

**ROCmFPX overlap.** ROCmFPX's in-memory disk index already tracks complete prompt token vectors for the current run.

**Porting rationale.** Persist a compact, versioned token index or manifest in persistent mode; verify candidate tokens against the loaded file before accepting.

**Risks / caveats.** A 4096-token metadata prefix cannot prove equality for longer prompts by itself.

**Evidence.** [E-030](20-Evidence-Index.md#e-030), [E-040](20-Evidence-Index.md#e-040)

<a id="f-008"></a>
### F-008 — Cold checkpoint count limit

**Decision:** `RETAIN` · **Portability:** `High` · **Maturity:** `M2` · **Confidence:** `High`

**Observed behavior.** Bounds the number of cold checkpoints and evicts old entries from a cold ring.

**Implementation.** `common/kv-ssd-cache.h`, `common/kv-ssd-cache.cpp`

**Dependencies.** LRU/ring metadata; filesystem delete

**License.** MIT

**ROCmFPX overlap.** ROCmFPX already enforces a byte limit and LRU eviction.

**Porting rationale.** Retain count as a secondary guard only; use byte quotas as the primary policy.

**Risks / caveats.** Count-only limits behave poorly across models with very different state sizes.

**Evidence.** [E-037](20-Evidence-Index.md#e-037), [E-037](20-Evidence-Index.md#e-037)

<a id="f-009"></a>
### F-009 — Automatic hot/warm RAM sizing

**Decision:** `REDESIGN` · **Portability:** `Medium` · **Maturity:** `M1` · **Confidence:** `High`

**Observed behavior.** Derives tier capacities from reported free RAM and allocates most of the calculated budget to the hot tier.

**Implementation.** `common/kv-ssd-cache.cpp`

**Dependencies.** Linux sysinfo or platform memory query; host RAM

**License.** MIT

**ROCmFPX overlap.** ROCmFPX already has an explicit RAM cache limit.

**Porting rationale.** Keep explicit limits authoritative; optional auto mode must use MemAvailable/cgroup limits and reserve headroom for model/runtime pressure.

**Risks / caveats.** Current free-RAM calculation can overcommit and is not container-aware.

**Evidence.** [E-039](20-Evidence-Index.md#e-039)

<a id="f-010"></a>
### F-010 — Platform read-ahead/prefetch

**Decision:** `RETAIN` · **Portability:** `Medium` · **Maturity:** `M2` · **Confidence:** `High`

**Observed behavior.** Issues read-ahead hints for a likely checkpoint before the restore is required.

**Implementation.** `common/kv-ssd-cache.cpp`, `common/kv-ssd-posix.h`

**Dependencies.** posix_fadvise or platform equivalent; continuation prediction

**License.** MIT

**ROCmFPX overlap.** ROCmFPX has no documented persistent prefetch layer.

**Porting rationale.** Add as an optional optimization after correctness and persistence land; measure hit rate and I/O amplification.

**Risks / caveats.** Prefetch may evict useful page cache or waste I/O on low-confidence candidates.

**Evidence.** [E-033](20-Evidence-Index.md#e-033), [E-042](20-Evidence-Index.md#e-042)

<a id="f-011"></a>
### F-011 — Per-user cache namespace

**Decision:** `REDESIGN` · **Portability:** `High` · **Maturity:** `M1` · **Confidence:** `High`

**Observed behavior.** Places explicit-user caches under a user-oriented namespace distinct from anonymous conversation roots.

**Implementation.** `tools/server/server-context-page-manager.cpp`, `common/kv-ssd-cache.h`

**Dependencies.** validated user ID; filesystem namespace

**License.** MIT

**ROCmFPX overlap.** ROCmFPX's current run directory is process-private, not tenant-addressed.

**Porting rationale.** Use an authenticated tenant ID and opaque keyed hash in the path; never trust a request field as the authorization boundary.

**Risks / caveats.** Path disclosure, user-supplied identity spoofing, and owner-wide file permissions.

**Evidence.** [E-061](20-Evidence-Index.md#e-061), [E-080](20-Evidence-Index.md#e-080), [E-081](20-Evidence-Index.md#e-081)

<a id="f-012"></a>
### F-012 — On-disk permission model

**Decision:** `REJECT` · **Portability:** `High` · **Maturity:** `M1` · **Confidence:** `High`

**Observed behavior.** Creates cache directories and state files using conventional shared-readable modes in the component path.

**Implementation.** `common/kv-ssd-cache.cpp`

**Dependencies.** filesystem permissions; deployment umask

**License.** MIT

**ROCmFPX overlap.** ROCmFPX's existing disk cache uses owner-only namespaces and files.

**Porting rationale.** Do not import CachyLlama's permission choices; keep ROCmFPX's 0700/0600 posture and add optional encryption for persistent multi-tenant use.

**Risks / caveats.** Prompts and hidden reasoning can be secrets.

**Evidence.** [E-041](20-Evidence-Index.md#e-041)

<a id="f-013"></a>
### F-013 — Direct final-name checkpoint writes

**Decision:** `REJECT` · **Portability:** `High` · **Maturity:** `M1` · **Confidence:** `High`

**Observed behavior.** Writes checkpoint payloads directly to their final paths and removes them on detected failure.

**Implementation.** `common/kv-ssd-cache.cpp`

**Dependencies.** filesystem write; optional fsync

**License.** MIT

**ROCmFPX overlap.** ROCmFPX already uses temporary files, atomic rename, and directory synchronization.

**Porting rationale.** Keep the target's commit protocol; persistent mode should add a manifest transaction rather than regress to direct publication.

**Risks / caveats.** Power loss can leave a final-name partial file.

**Evidence.** [E-034](20-Evidence-Index.md#e-034)

<a id="f-014"></a>
### F-014 — POSIX I/O compatibility shim

**Decision:** `RETAIN` · **Portability:** `Medium` · **Maturity:** `M2` · **Confidence:** `High`

**Observed behavior.** Centralizes open, pread, pwrite, fsync, unlink, and advisory behavior.

**Implementation.** `common/kv-ssd-posix.h`

**Dependencies.** POSIX; platform conditional compilation

**License.** MIT

**ROCmFPX overlap.** ROCmFPX supports cross-platform server builds and has its own filesystem code.

**Porting rationale.** Port only useful abstractions, preserving Windows UTF-8 and fault-injection behavior in the target.

**Risks / caveats.** A POSIX-named layer is not sufficient for Windows portability.

**Evidence.** [E-033](20-Evidence-Index.md#e-033)

<a id="f-015"></a>
### F-015 — Persistent index reconstruction

**Decision:** `REDESIGN` · **Portability:** `High` · **Maturity:** `M2` · **Confidence:** `High`

**Observed behavior.** Scans checkpoint files and restores cache metadata when no live process index exists.

**Implementation.** `common/kv-ssd-cache.cpp`

**Dependencies.** directory scan; metadata decoder; format version

**License.** MIT

**ROCmFPX overlap.** ROCmFPX current entries exist only in process memory and are deleted on exit.

**Porting rationale.** Use a checksummed manifest plus orphan reconciliation; bound startup cost and quarantine malformed entries.

**Risks / caveats.** Unbounded directory scans and corrupted metadata can delay startup.

**Evidence.** [E-031](20-Evidence-Index.md#e-031), [E-040](20-Evidence-Index.md#e-040)


