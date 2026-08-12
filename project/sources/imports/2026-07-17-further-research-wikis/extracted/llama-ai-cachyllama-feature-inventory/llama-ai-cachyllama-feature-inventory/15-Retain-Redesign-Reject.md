# Retain / redesign / reject matrix

This is the complete feature-by-feature disposition table. The machine-readable form is [`data/retain-redesign-reject.csv`](data/retain-redesign-reject.csv).

- **RETAIN:** preserve the capability or target-native implementation with limited reconciliation.
- **REDESIGN:** preserve the outcome but change architecture, durability, security, licensing, or integration.
- **REJECT:** do not import the pinned implementation or behavior.

## Persistent SSD-backed KV cache

| ID | Feature | Decision | Portability | Maturity | ROCmFPX disposition | Evidence |
|---|---|---|---|---|---|---|
| [F-001](03-Persistent-SSD-KV-Cache.md#f-001) | Process-restart-persistent sequence cache | **REDESIGN** | High | M2 | Add an explicit persistent mode to the existing ROCmFPX cache engine rather than replacing its tested run-scoped mode. | [E-030](20-Evidence-Index.md#e-030), [E-031](20-Evidence-Index.md#e-031), [E-068](20-Evidence-Index.md#e-068) |
| [F-002](03-Persistent-SSD-KV-Cache.md#f-002) | Hot/warm/cold checkpoint tiers | **REDESIGN** | Medium | M2 | Preserve the policy objective but implement one budgeted cache with explicit residency state, pressure feedback, and testable promotion/demotion. | [E-030](20-Evidence-Index.md#e-030), [E-037](20-Evidence-Index.md#e-037), [E-039](20-Evidence-Index.md#e-039) |
| [F-003](03-Persistent-SSD-KV-Cache.md#f-003) | Target + draft + speculative state record | **RETAIN** | High | M2 | Unify record metadata and validation with ROCmFPX's existing target/draft/state_spec representation. | [E-030](20-Evidence-Index.md#e-030), [E-063](20-Evidence-Index.md#e-063), [E-067](20-Evidence-Index.md#e-067) |
| [F-004](03-Persistent-SSD-KV-Cache.md#f-004) | Bounded chunk I/O | **RETAIN** | High | M2 | Port the chunk-loop concept behind the target's current I/O abstraction and preserve its fault injection. | [E-032](20-Evidence-Index.md#e-032), [E-033](20-Evidence-Index.md#e-033) |
| [F-005](03-Persistent-SSD-KV-Cache.md#f-005) | Optional fsync durability | **REDESIGN** | High | M2 | Represent durability as an explicit strict/relaxed policy and retain ROCmFPX's atomic pair commit. | [E-023](20-Evidence-Index.md#e-023), [E-034](20-Evidence-Index.md#e-034) |
| [F-006](03-Persistent-SSD-KV-Cache.md#f-006) | Checkpoint compatibility fingerprint | **RETAIN** | High | M2 | Define a versioned fingerprint covering model identity, context layout, KV types, backend-relevant serialization version, and draft/MTP shape. | [E-038](20-Evidence-Index.md#e-038), [E-040](20-Evidence-Index.md#e-040), [E-060](20-Evidence-Index.md#e-060) |
| [F-007](03-Persistent-SSD-KV-Cache.md#f-007) | Checkpoint token-prefix metadata | **RETAIN** | High | M2 | Persist a compact, versioned token index or manifest in persistent mode; verify candidate tokens against the loaded file before accepting. | [E-030](20-Evidence-Index.md#e-030), [E-040](20-Evidence-Index.md#e-040) |
| [F-008](03-Persistent-SSD-KV-Cache.md#f-008) | Cold checkpoint count limit | **RETAIN** | High | M2 | Retain count as a secondary guard only; use byte quotas as the primary policy. | [E-037](20-Evidence-Index.md#e-037), [E-037](20-Evidence-Index.md#e-037) |
| [F-009](03-Persistent-SSD-KV-Cache.md#f-009) | Automatic hot/warm RAM sizing | **REDESIGN** | Medium | M1 | Keep explicit limits authoritative; optional auto mode must use MemAvailable/cgroup limits and reserve headroom for model/runtime pressure. | [E-039](20-Evidence-Index.md#e-039) |
| [F-010](03-Persistent-SSD-KV-Cache.md#f-010) | Platform read-ahead/prefetch | **RETAIN** | Medium | M2 | Add as an optional optimization after correctness and persistence land; measure hit rate and I/O amplification. | [E-033](20-Evidence-Index.md#e-033), [E-042](20-Evidence-Index.md#e-042) |
| [F-011](03-Persistent-SSD-KV-Cache.md#f-011) | Per-user cache namespace | **REDESIGN** | High | M1 | Use an authenticated tenant ID and opaque keyed hash in the path; never trust a request field as the authorization boundary. | [E-061](20-Evidence-Index.md#e-061), [E-080](20-Evidence-Index.md#e-080), [E-081](20-Evidence-Index.md#e-081) |
| [F-012](03-Persistent-SSD-KV-Cache.md#f-012) | On-disk permission model | **REJECT** | High | M1 | Do not import CachyLlama's permission choices; keep ROCmFPX's 0700/0600 posture and add optional encryption for persistent multi-tenant use. | [E-041](20-Evidence-Index.md#e-041) |
| [F-013](03-Persistent-SSD-KV-Cache.md#f-013) | Direct final-name checkpoint writes | **REJECT** | High | M1 | Keep the target's commit protocol; persistent mode should add a manifest transaction rather than regress to direct publication. | [E-034](20-Evidence-Index.md#e-034) |
| [F-014](03-Persistent-SSD-KV-Cache.md#f-014) | POSIX I/O compatibility shim | **RETAIN** | Medium | M2 | Port only useful abstractions, preserving Windows UTF-8 and fault-injection behavior in the target. | [E-033](20-Evidence-Index.md#e-033) |
| [F-015](03-Persistent-SSD-KV-Cache.md#f-015) | Persistent index reconstruction | **REDESIGN** | High | M2 | Use a checksummed manifest plus orphan reconciliation; bound startup cost and quarantine malformed entries. | [E-031](20-Evidence-Index.md#e-031), [E-040](20-Evidence-Index.md#e-040) |

## System-prompt caching

| ID | Feature | Decision | Portability | Maturity | ROCmFPX disposition | Evidence |
|---|---|---|---|---|---|---|
| [F-016](04-System-Prompt-Cache.md#f-016) | Cross-conversation system-state reuse | **REDESIGN** | High | M2 | Implement as a scoped prefix-cache class in the target engine, keyed by model/template/tenant/system token sequence. | [E-050](20-Evidence-Index.md#e-050), [E-052](20-Evidence-Index.md#e-052), [E-055](20-Evidence-Index.md#e-055) |
| [F-017](04-System-Prompt-Cache.md#f-017) | Restart persistence for system prefixes | **REDESIGN** | High | M2 | Use the same versioned persistent manifest and atomic commit layer as conversation checkpoints. | [E-051](20-Evidence-Index.md#e-051) |
| [F-018](04-System-Prompt-Cache.md#f-018) | Entry-count and age retention | **RETAIN** | High | M2 | Combine age, byte, and count limits; expose tenant and global quotas. | [E-050](20-Evidence-Index.md#e-050), [E-053](20-Evidence-Index.md#e-053) |
| [F-019](04-System-Prompt-Cache.md#f-019) | Full-prompt hash plus bounded token prefix | **RETAIN** | High | M2 | Use a cryptographic or collision-resistant content key over canonical tokens and include template/model/tenant scope. | [E-050](20-Evidence-Index.md#e-050), [E-052](20-Evidence-Index.md#e-052) |
| [F-020](04-System-Prompt-Cache.md#f-020) | Heuristic system-boundary detection | **REJECT** | High | M1 | Derive the boundary from the parsed message/template AST before tokenization, then verify the exact token range. | [E-054](20-Evidence-Index.md#e-054) |

## Prefix matching and session continuation

| ID | Feature | Decision | Portability | Maturity | ROCmFPX disposition | Evidence |
|---|---|---|---|---|---|---|
| [F-021](05-Prefix-Matching-and-Session-Continuation.md#f-021) | Longest-common-prefix candidate scoring | **RETAIN** | High | M2 | Consolidate around the target implementation and add restart-persistent index candidates. | [E-035](20-Evidence-Index.md#e-035) |
| [F-022](05-Prefix-Matching-and-Session-Continuation.md#f-022) | High-threshold cross-conversation continuation | **REDESIGN** | Medium | M2 | Make threshold configurable by scope; require authenticated tenant equality and a minimum absolute token count. | [E-062](20-Evidence-Index.md#e-062) |
| [F-023](05-Prefix-Matching-and-Session-Continuation.md#f-023) | Global anonymous continuation scan | **REDESIGN** | High | M1 | Replace global scanning with a manifest/index partitioned by model and authenticated tenant; anonymous reuse should default to process-local only. | [E-036](20-Evidence-Index.md#e-036), [E-062](20-Evidence-Index.md#e-062) |
| [F-024](05-Prefix-Matching-and-Session-Continuation.md#f-024) | Minimum compared-token guard | **RETAIN** | Medium | M2 | Use both minimum absolute tokens and ratio thresholds, with higher requirements for cross-conversation or cross-restart matches. | [E-036](20-Evidence-Index.md#e-036) |
| [F-025](05-Prefix-Matching-and-Session-Continuation.md#f-025) | Slot prompt similarity | **RETAIN** | High | M3 | Keep target behavior; document interaction with persistent cache and user affinity. | [E-005](20-Evidence-Index.md#e-005), [E-023](20-Evidence-Index.md#e-023), [E-082](20-Evidence-Index.md#e-082) |
| [F-026](05-Prefix-Matching-and-Session-Continuation.md#f-026) | Read-ahead of likely continuation | **RETAIN** | Medium | M2 | Add only after persistence and candidate correctness are tested; instrument useful-prefetch ratio. | [E-042](20-Evidence-Index.md#e-042), [E-062](20-Evidence-Index.md#e-062) |

## Checkpoint restore and hybrid state

| ID | Feature | Decision | Portability | Maturity | ROCmFPX disposition | Evidence |
|---|---|---|---|---|---|---|
| [F-027](06-Checkpoint-Restore-and-Hybrid-State.md#f-027) | Periodic prefill checkpoints | **RETAIN** | High | M2 | Keep target checkpoint scheduling and integrate persistent snapshots as a separate policy. | [E-022](20-Evidence-Index.md#e-022), [E-023](20-Evidence-Index.md#e-023), [E-071](20-Evidence-Index.md#e-071) |
| [F-028](06-Checkpoint-Restore-and-Hybrid-State.md#f-028) | Deferred final checkpoint | **RETAIN** | Medium | M2 | Preserve the semantic boundary, but express it as an explicit checkpoint reason/state in the target. | [E-071](20-Evidence-Index.md#e-071) |
| [F-029](06-Checkpoint-Restore-and-Hybrid-State.md#f-029) | Target/draft/spec checkpoint restore | **RETAIN** | High | M2 | Merge semantics into the target's paired-state validation and exact-boundary rules. | [E-063](20-Evidence-Index.md#e-063), [E-067](20-Evidence-Index.md#e-067), [E-068](20-Evidence-Index.md#e-068) |
| [F-030](06-Checkpoint-Restore-and-Hybrid-State.md#f-030) | Destination sequence-ID remapping | **RETAIN** | High | M2 | Keep explicit destination sequence IDs in every restore API and add a regression test based on the historical cold-restore fix. | [E-063](20-Evidence-Index.md#e-063), [E-068](20-Evidence-Index.md#e-068), [E-070](20-Evidence-Index.md#e-070) |
| [F-031](06-Checkpoint-Restore-and-Hybrid-State.md#f-031) | Hybrid attention-only cleanup | **RETAIN** | High | M2 | Retain the primitive only if the target base does not already contain an equivalent; validate per architecture. | [E-068](20-Evidence-Index.md#e-068), [E-069](20-Evidence-Index.md#e-069), [E-071](20-Evidence-Index.md#e-071) |
| [F-032](06-Checkpoint-Restore-and-Hybrid-State.md#f-032) | Manual slot save/restore/erase | **RETAIN** | High | M3 | Keep the endpoint but enforce owner-only directories, API authorization, quotas, and audit logs. | [E-090](20-Evidence-Index.md#e-090), [E-091](20-Evidence-Index.md#e-091) |
| [F-033](06-Checkpoint-Restore-and-Hybrid-State.md#f-033) | Cold-restore failure fallback | **RETAIN** | High | M2 | Use the target's stricter file-size/pair validation and circuit-breaker semantics. | [E-067](20-Evidence-Index.md#e-067), [E-071](20-Evidence-Index.md#e-071) |
| [F-034](06-Checkpoint-Restore-and-Hybrid-State.md#f-034) | In-memory prompt state cache | **RETAIN** | High | M3 | Keep the target implementation as the first tier; persistent disk should be a lower tier. | [E-022](20-Evidence-Index.md#e-022), [E-071](20-Evidence-Index.md#e-071) |
| [F-035](06-Checkpoint-Restore-and-Hybrid-State.md#f-035) | KV-shift cache reuse | **RETAIN** | Medium | M3 | Retain target-native behavior; test interactions with loaded persistent state and hybrid memory. | [E-023](20-Evidence-Index.md#e-023), [E-024](20-Evidence-Index.md#e-024) |

## Slot affinity and user isolation

| ID | Feature | Decision | Portability | Maturity | ROCmFPX disposition | Evidence |
|---|---|---|---|---|---|---|
| [F-036](07-Slot-Affinity-and-User-Isolation.md#f-036) | OpenAI request user hint | **REDESIGN** | High | M2 | Accept a user hint only behind a trusted gateway; bind an internal tenant principal from API-key/JWT context. | [E-081](20-Evidence-Index.md#e-081), [E-082](20-Evidence-Index.md#e-082), [E-087](20-Evidence-Index.md#e-087) |
| [F-037](07-Slot-Affinity-and-User-Isolation.md#f-037) | Anthropic metadata user hint | **RETAIN** | Medium | M2 | Keep protocol parsing as a hint, then map through the same authenticated principal layer as OpenAI requests. | [E-081](20-Evidence-Index.md#e-081), [E-087](20-Evidence-Index.md#e-087) |
| [F-038](07-Slot-Affinity-and-User-Isolation.md#f-038) | User-ID validation | **RETAIN** | High | M2 | Retain strict syntax validation but store a keyed hash/opaque internal ID instead of the raw value. | [E-080](20-Evidence-Index.md#e-080), [E-081](20-Evidence-Index.md#e-081) |
| [F-039](07-Slot-Affinity-and-User-Isolation.md#f-039) | Per-user concurrent request cap | **REDESIGN** | High | M2 | Implement one authoritative scheduler quota keyed by authenticated tenant, with atomic admission and release. | [E-022](20-Evidence-Index.md#e-022), [E-023](20-Evidence-Index.md#e-023), [E-082](20-Evidence-Index.md#e-082), [E-083](20-Evidence-Index.md#e-083), [E-085](20-Evidence-Index.md#e-085) |
| [F-040](07-Slot-Affinity-and-User-Isolation.md#f-040) | Anonymous-bucket concurrency policy | **REJECT** | High | M1 | Define an explicit unauthenticated/global policy; production multi-tenant deployments should require authentication. | [E-080](20-Evidence-Index.md#e-080), [E-084](20-Evidence-Index.md#e-084), [E-085](20-Evidence-Index.md#e-085) |
| [F-041](07-Slot-Affinity-and-User-Isolation.md#f-041) | Same-user slot affinity | **REDESIGN** | High | M2 | Combine authorization, user affinity, and prefix score in a single eligibility/ranking pass; never select cross-tenant state. | [E-082](20-Evidence-Index.md#e-082), [E-085](20-Evidence-Index.md#e-085) |
| [F-042](07-Slot-Affinity-and-User-Isolation.md#f-042) | User-scoped persistent cache directories | **REDESIGN** | High | M1 | Namespace by authenticated tenant hash and model fingerprint, with per-tenant owner/key policy. | [E-061](20-Evidence-Index.md#e-061), [E-080](20-Evidence-Index.md#e-080) |
| [F-043](07-Slot-Affinity-and-User-Isolation.md#f-043) | Per-user cache statistics | **REJECT** | High | M1 | Do not port the omission; define aggregate and tenant-scoped metrics with cardinality controls. | [E-060](20-Evidence-Index.md#e-060), [E-064](20-Evidence-Index.md#e-064) |
| [F-044](07-Slot-Affinity-and-User-Isolation.md#f-044) | Per-user tier aging on turn completion | **REJECT** | High | M1 | Centralize all cache instances in one registry and iterate one lifecycle interface. | [E-064](20-Evidence-Index.md#e-064) |
| [F-045](07-Slot-Affinity-and-User-Isolation.md#f-045) | Raw user identifiers in logs | **REJECT** | High | M1 | Log a stable keyed pseudonym, not raw identity; keep raw values out of metrics labels. | [E-086](20-Evidence-Index.md#e-086) |
| [F-046](07-Slot-Affinity-and-User-Isolation.md#f-046) | Identity as routing metadata, not authentication | **REDESIGN** | High | M1 | Resolve tenant identity in authenticated middleware and ignore/validate client hints according to gateway policy. | [E-081](20-Evidence-Index.md#e-081), [E-087](20-Evidence-Index.md#e-087), [E-103](20-Evidence-Index.md#e-103) |
| [F-047](07-Slot-Affinity-and-User-Isolation.md#f-047) | Unified cache and slot ownership lifecycle | **REDESIGN** | Medium | M1 | Introduce one TenantContext/CacheScope object passed through admission, selection, restore, save, metrics, and release. | [E-060](20-Evidence-Index.md#e-060), [E-081](20-Evidence-Index.md#e-081), [E-082](20-Evidence-Index.md#e-082), [E-085](20-Evidence-Index.md#e-085) |

## Cache administration

| ID | Feature | Decision | Portability | Maturity | ROCmFPX disposition | Evidence |
|---|---|---|---|---|---|---|
| [F-048](08-Cache-Administration.md#f-048) | SSD cache CLI and environment controls | **REDESIGN** | High | M2 | Extend target-native naming with explicit persistent-mode, durability, retention, and namespace controls; preserve backward compatibility. | [E-022](20-Evidence-Index.md#e-022), [E-023](20-Evidence-Index.md#e-023), [E-024](20-Evidence-Index.md#e-024) |
| [F-049](08-Cache-Administration.md#f-049) | Parent profile overrides | **REDESIGN** | High | M2 | Re-express useful profile rules in an MIT-compatible configuration layer or external GPL deployment project. | [E-005](20-Evidence-Index.md#e-005), [E-006](20-Evidence-Index.md#e-006), [E-007](20-Evidence-Index.md#e-007), [E-118](20-Evidence-Index.md#e-118) |
| [F-050](08-Cache-Administration.md#f-050) | Retention by unused days | **RETAIN** | High | M2 | Add optional age eviction to persistent mode with monotonic/validated timestamps and dry-run reporting. | [E-037](20-Evidence-Index.md#e-037), [E-053](20-Evidence-Index.md#e-053) |
| [F-051](08-Cache-Administration.md#f-051) | Conversation-count eviction | **REDESIGN** | High | M1 | Use manifest-backed LRU/bytes per tenant; do not choose victims solely by directory mtime. | [E-065](20-Evidence-Index.md#e-065) |
| [F-052](08-Cache-Administration.md#f-052) | Startup discovery and cleanup | **REDESIGN** | High | M2 | Combine target ownership/lock safety with a persistent manifest, quarantine, bounded reconciliation, and schema migration. | [E-031](20-Evidence-Index.md#e-031), [E-051](20-Evidence-Index.md#e-051), [E-053](20-Evidence-Index.md#e-053) |
| [F-053](08-Cache-Administration.md#f-053) | Administrative cache API | **REDESIGN** | Medium | M1 | Add authenticated inspect/prune/invalidate endpoints or an offline admin CLI, with tenant scoping and dry-run. | [E-090](20-Evidence-Index.md#e-090), [E-091](20-Evidence-Index.md#e-091) |
| [F-054](08-Cache-Administration.md#f-054) | Cache statistics API surface | **REDESIGN** | High | M1 | Expose low-cardinality cache counters and gauges from one target cache engine. | [E-030](20-Evidence-Index.md#e-030), [E-064](20-Evidence-Index.md#e-064), [E-092](20-Evidence-Index.md#e-092), [E-093](20-Evidence-Index.md#e-093) |
| [F-055](08-Cache-Administration.md#f-055) | Cache invalidation on compatibility change | **RETAIN** | High | M2 | Retain fail-closed rejection and add explicit invalidate/migrate tooling keyed by exact model fingerprint. | [E-038](20-Evidence-Index.md#e-038), [E-052](20-Evidence-Index.md#e-052) |
| [F-056](08-Cache-Administration.md#f-056) | Standalone asynchronous KV page-manager prototype | **REJECT** | High | M0 | Do not port this duplicate path. Extract only independently valuable test ideas after verifying semantics. | [E-072](20-Evidence-Index.md#e-072), [E-073](20-Evidence-Index.md#e-073), [E-074](20-Evidence-Index.md#e-074), [E-075](20-Evidence-Index.md#e-075) |

## Model lifecycle

| ID | Feature | Decision | Portability | Maturity | ROCmFPX disposition | Evidence |
|---|---|---|---|---|---|---|
| [F-057](09-Model-Lifecycle.md#f-057) | Router-managed child model processes | **RETAIN** | Medium | M2 | Retain upstream/target router implementation rather than transplanting an older fork copy. | [E-094](20-Evidence-Index.md#e-094), [E-105](20-Evidence-Index.md#e-105) |
| [F-058](09-Model-Lifecycle.md#f-058) | Model load and unload API | **RETAIN** | High | M2 | Use target-native routes and apply authentication, authorization, resource quotas, and audit logging. | [E-095](20-Evidence-Index.md#e-095), [E-096](20-Evidence-Index.md#e-096) |
| [F-059](09-Model-Lifecycle.md#f-059) | Model autoload on first routed request | **RETAIN** | Medium | M2 | Retain behind an explicit policy and admission controller. | [E-094](20-Evidence-Index.md#e-094), [E-103](20-Evidence-Index.md#e-103) |
| [F-060](09-Model-Lifecycle.md#f-060) | Maximum loaded-model count with LRU unload | **RETAIN** | High | M2 | Retain target-native LRU but make memory pressure and pinned models first-class inputs. | [E-094](20-Evidence-Index.md#e-094) |
| [F-061](09-Model-Lifecycle.md#f-061) | Model preset reload | **RETAIN** | Medium | M2 | Retain with schema validation and atomic catalog swap. | [E-106](20-Evidence-Index.md#e-106) |
| [F-062](09-Model-Lifecycle.md#f-062) | Local and cached model discovery | **RETAIN** | Medium | M2 | Keep target-native discovery with explicit trust boundaries and allowlisted roots. | [E-094](20-Evidence-Index.md#e-094), [E-105](20-Evidence-Index.md#e-105) |
| [F-063](09-Model-Lifecycle.md#f-063) | Hugging Face GGUF discovery and split download | **REDESIGN** | High | M2 | Implement or document an MIT-compatible downloader outside the inference core; verify checksums and free space. | [E-008](20-Evidence-Index.md#e-008) |
| [F-064](09-Model-Lifecycle.md#f-064) | Model-class profile selection | **REDESIGN** | High | M2 | Reimplement as a declarative, tested preset resolver in the target or an external deployment package. | [E-007](20-Evidence-Index.md#e-007), [E-011](20-Evidence-Index.md#e-011), [E-118](20-Evidence-Index.md#e-118) |
| [F-065](09-Model-Lifecycle.md#f-065) | Cache scope tied to model compatibility | **RETAIN** | High | M2 | Namespace cache roots by exact model fingerprint and runtime state ABI, not just human model alias. | [E-038](20-Evidence-Index.md#e-038), [E-060](20-Evidence-Index.md#e-060) |
| [F-118](09-Model-Lifecycle.md#f-118) | Automatic device-memory fit | **RETAIN** | Medium | M2 | Retain target-native estimator and teach it persistent cache/RAM reservation costs. | [E-129](20-Evidence-Index.md#e-129) |

## Server APIs

| ID | Feature | Decision | Portability | Maturity | ROCmFPX disposition | Evidence |
|---|---|---|---|---|---|---|
| [F-066](10-Server-APIs.md#f-066) | OpenAI chat and completions compatibility | **RETAIN** | High | M3 | Keep target-native API behavior and add cache/user extensions through namespaced fields or headers. | [E-090](20-Evidence-Index.md#e-090), [E-102](20-Evidence-Index.md#e-102), [E-093](20-Evidence-Index.md#e-093) |
| [F-067](10-Server-APIs.md#f-067) | OpenAI Responses compatibility | **RETAIN** | High | M2 | Reconcile with target rather than cherry-picking fork code. | [E-090](20-Evidence-Index.md#e-090), [E-102](20-Evidence-Index.md#e-102), [E-093](20-Evidence-Index.md#e-093) |
| [F-068](10-Server-APIs.md#f-068) | Anthropic Messages compatibility | **RETAIN** | Medium | M2 | Retain target-native implementation; map metadata.user_id only through trusted identity policy. | [E-090](20-Evidence-Index.md#e-090), [E-102](20-Evidence-Index.md#e-102), [E-081](20-Evidence-Index.md#e-081) |
| [F-069](10-Server-APIs.md#f-069) | Legacy completion and infill endpoints | **RETAIN** | High | M3 | Keep target-native support and capability checks. | [E-090](20-Evidence-Index.md#e-090), [E-102](20-Evidence-Index.md#e-102) |
| [F-070](10-Server-APIs.md#f-070) | Embeddings and reranking endpoints | **RETAIN** | High | M3 | Retain from target base. | [E-090](20-Evidence-Index.md#e-090), [E-102](20-Evidence-Index.md#e-102) |
| [F-071](10-Server-APIs.md#f-071) | Audio transcription compatibility | **RETAIN** | Medium | M2 | Keep target-native endpoint; exclude multimodal prompt states from persistent cache until deterministic state identity is defined. | [E-090](20-Evidence-Index.md#e-090), [E-102](20-Evidence-Index.md#e-102) |
| [F-072](10-Server-APIs.md#f-072) | Slot state administration API | **RETAIN** | High | M3 | Keep with authentication and distinguish manual slot files from automatic persistent cache entries. | [E-090](20-Evidence-Index.md#e-090), [E-091](20-Evidence-Index.md#e-091) |
| [F-073](10-Server-APIs.md#f-073) | Resumable stream sessions | **RETAIN** | Medium | M2 | Port only if absent from the target base; bind stream lookup/deletion to authenticated ownership. | [E-097](20-Evidence-Index.md#e-097), [E-098](20-Evidence-Index.md#e-098), [E-099](20-Evidence-Index.md#e-099) |
| [F-074](10-Server-APIs.md#f-074) | Model router lifecycle API | **RETAIN** | Medium | M2 | Use target-native router API with authorization and resource controls. | [E-095](20-Evidence-Index.md#e-095), [E-106](20-Evidence-Index.md#e-106) |
| [F-075](10-Server-APIs.md#f-075) | LoRA hot-swap API | **RETAIN** | High | M3 | Keep target-native implementation; include adapter set in cache compatibility fingerprint. | [E-100](20-Evidence-Index.md#e-100), [E-101](20-Evidence-Index.md#e-101) |
| [F-076](10-Server-APIs.md#f-076) | MoE expert observation/control API | **REDESIGN** | Medium | M1 | Treat as an optional diagnostic extension; reconcile against target graph/kernel architecture and protect control endpoints. | [E-100](20-Evidence-Index.md#e-100) |
| [F-077](10-Server-APIs.md#f-077) | Tokenize, detokenize, and apply-template APIs | **RETAIN** | High | M3 | Keep target-native endpoints and include template identity in persistent cache keys. | [E-090](20-Evidence-Index.md#e-090), [E-104](20-Evidence-Index.md#e-104) |
| [F-078](10-Server-APIs.md#f-078) | Health and properties APIs | **RETAIN** | High | M3 | Keep target-native behavior and add a cache readiness/degraded status field. | [E-090](20-Evidence-Index.md#e-090), [E-104](20-Evidence-Index.md#e-104) |
| [F-111](10-Server-APIs.md#f-111) | Experimental built-in agent tools | **REJECT** | High | M1 | Keep disabled; place tools in a separately sandboxed, authenticated agent runtime if needed. | [E-120](20-Evidence-Index.md#e-120), [E-121](20-Evidence-Index.md#e-121) |
| [F-112](10-Server-APIs.md#f-112) | Experimental MCP CORS proxy | **REJECT** | High | M1 | Do not enable in ROCmFPX's inference server; use a dedicated, allowlisted reverse proxy. | [E-122](20-Evidence-Index.md#e-122), [E-123](20-Evidence-Index.md#e-123) |
| [F-113](10-Server-APIs.md#f-113) | Google Cloud / Vertex AI compatibility | **RETAIN** | Medium | M2 | Retain target-native implementation if required by deployments. | [E-124](20-Evidence-Index.md#e-124) |
| [F-114](10-Server-APIs.md#f-114) | Idle sleep mode | **RETAIN** | Medium | M2 | Keep target-native behavior; define how persistent caches and router children behave across sleep/wake. | [E-104](20-Evidence-Index.md#e-104), [E-125](20-Evidence-Index.md#e-125) |
| [F-115](10-Server-APIs.md#f-115) | Reasoning parsing, budget, and history controls | **REDESIGN** | Medium | M2 | Use target-native reasoning controls and keep cache keys sensitive to template kwargs. | [E-126](20-Evidence-Index.md#e-126), [E-006](20-Evidence-Index.md#e-006) |
| [F-116](10-Server-APIs.md#f-116) | Grammar and JSON-Schema constrained generation | **RETAIN** | High | M3 | Keep target-native implementation. | [E-127](20-Evidence-Index.md#e-127) |
| [F-117](10-Server-APIs.md#f-117) | Embedded Web UI | **RETAIN** | Medium | M2 | Keep target-native UI; do not expose administrative/cache controls without authorization. | [E-128](20-Evidence-Index.md#e-128), [E-090](20-Evidence-Index.md#e-090) |
| [F-119](10-Server-APIs.md#f-119) | Multimodal capability gating | **RETAIN** | High | M2 | Keep target-native capability detection; exclude multimodal state from persistence until identity/serialization is proven. | [E-102](20-Evidence-Index.md#e-102), [E-130](20-Evidence-Index.md#e-130) |
| [F-120](10-Server-APIs.md#f-120) | Speculative decoding configuration | **RETAIN** | High | M3 | Retain target-native MTP/speculation and preserve exact-boundary cache rules. | [E-131](20-Evidence-Index.md#e-131), [E-063](20-Evidence-Index.md#e-063), [E-093](20-Evidence-Index.md#e-093) |

## Observability

| ID | Feature | Decision | Portability | Maturity | ROCmFPX disposition | Evidence |
|---|---|---|---|---|---|---|
| [F-079](11-Observability.md#f-079) | Prometheus metrics endpoint | **RETAIN** | High | M3 | Extend target metrics with persistent cache hits/misses/bytes/restore latency/circuit-breaker state. | [E-092](20-Evidence-Index.md#e-092) |
| [F-080](11-Observability.md#f-080) | Per-response cache and timing data | **RETAIN** | High | M3 | Keep target response timings and add an optional cache source field such as ram, run-disk, persistent-disk, or system-prefix. | [E-093](20-Evidence-Index.md#e-093) |
| [F-081](11-Observability.md#f-081) | SSD cache event logging | **REDESIGN** | High | M2 | Normalize target logs with stable event names, byte/token counts, duration, scope pseudonym, and reason codes. | [E-031](20-Evidence-Index.md#e-031), [E-037](20-Evidence-Index.md#e-037), [E-067](20-Evidence-Index.md#e-067) |
| [F-082](11-Observability.md#f-082) | Model lifecycle SSE | **RETAIN** | Medium | M2 | Keep target-native stream with authorization and reconnect semantics. | [E-095](20-Evidence-Index.md#e-095), [E-106](20-Evidence-Index.md#e-106) |
| [F-083](11-Observability.md#f-083) | Expert/MoE statistics | **REDESIGN** | Medium | M1 | Validate overhead and output semantics on target models before enabling. | [E-100](20-Evidence-Index.md#e-100) |
| [F-084](11-Observability.md#f-084) | Cache-stat aggregation | **REJECT** | High | M1 | Do not copy the split-map aggregation; centralize metrics in the target cache object. | [E-030](20-Evidence-Index.md#e-030), [E-064](20-Evidence-Index.md#e-064) |

## Deployment

| ID | Feature | Decision | Portability | Maturity | ROCmFPX disposition | Evidence |
|---|---|---|---|---|---|---|
| [F-085](12-Deployment.md#f-085) | Multi-backend build helper | **REDESIGN** | High | M2 | Keep ROCmFPX build scripts; transplant only configuration knowledge through clean-room MIT-compatible changes. | [E-010](20-Evidence-Index.md#e-010) |
| [F-086](12-Deployment.md#f-086) | systemd service unit | **REDESIGN** | High | M1 | Generate a parameterized unit or packaging template from target-supported flags; validate with systemd-analyze. | [E-013](20-Evidence-Index.md#e-013) |
| [F-087](12-Deployment.md#f-087) | Aggressive stale-process cleanup | **REJECT** | High | M1 | Use PID files, systemd/container ownership, graceful shutdown, and a bounded escalation policy. | [E-009](20-Evidence-Index.md#e-009) |
| [F-088](12-Deployment.md#f-088) | API key and TLS configuration | **RETAIN** | High | M3 | Keep target-native implementation and use authenticated principal context for cache tenant binding. | [E-090](20-Evidence-Index.md#e-090), [E-103](20-Evidence-Index.md#e-103) |
| [F-089](12-Deployment.md#f-089) | Model download orchestration | **REDESIGN** | High | M2 | Provide a separate, checksummed provisioning command or documented workflow under compatible licensing. | [E-008](20-Evidence-Index.md#e-008) |
| [F-090](12-Deployment.md#f-090) | GPU/APU hardware detection | **REDESIGN** | High | M2 | Move stable detection into an MIT-compatible capability probe with explicit overrides and tests. | [E-011](20-Evidence-Index.md#e-011) |
| [F-091](12-Deployment.md#f-091) | APU GTT/TTM configuration helper | **REJECT** | High | M1 | Document validated host settings and provide a separate opt-in administrator tool, not part of the inference package. | [E-012](20-Evidence-Index.md#e-012) |
| [F-092](12-Deployment.md#f-092) | Layered component/runner configuration | **REDESIGN** | High | M1 | Publish one generated schema and precedence table; make profile overrides explicit and machine-readable. | [E-006](20-Evidence-Index.md#e-006), [E-022](20-Evidence-Index.md#e-022), [E-024](20-Evidence-Index.md#e-024) |
| [F-093](12-Deployment.md#f-093) | License-separated orchestration layer | **REDESIGN** | High | M3 | Port MIT component code with notices; clean-room reimplement parent behavior or keep it as a separate GPL work. | [E-003](20-Evidence-Index.md#e-003), [E-004](20-Evidence-Index.md#e-004), [E-021](20-Evidence-Index.md#e-021) |

## Strix Halo optimizations

| ID | Feature | Decision | Portability | Maturity | ROCmFPX disposition | Evidence |
|---|---|---|---|---|---|---|
| [F-094](13-Strix-Halo-Optimizations.md#f-094) | gfx1151/RDNA3.5 detection | **RETAIN** | High | M3 | Use target-native detection; reconcile only missing device IDs or capability probes. | [E-011](20-Evidence-Index.md#e-011), [E-111](20-Evidence-Index.md#e-111) |
| [F-095](13-Strix-Halo-Optimizations.md#f-095) | HSA and unified-memory launch environment | **RETAIN** | High | M2 | Keep target-documented environment and add version-gated validation rather than copying shell code. | [E-117](20-Evidence-Index.md#e-117), [E-119](20-Evidence-Index.md#e-119) |
| [F-096](13-Strix-Halo-Optimizations.md#f-096) | Large-context in-memory Halo profile | **REDESIGN** | Medium | M2 | Convert to a declarative memory estimator based on model/KV/context/parallelism, retaining SSD as an explicit resilience option. | [E-007](20-Evidence-Index.md#e-007), [E-118](20-Evidence-Index.md#e-118) |
| [F-097](13-Strix-Halo-Optimizations.md#f-097) | RDNA3.5 MMVQ launch tuning | **REDESIGN** | Medium | M2 | Benchmark-port individual launch heuristics per ROCmFPX type; do not wholesale replace target kernels. | [E-110](20-Evidence-Index.md#e-110), [E-112](20-Evidence-Index.md#e-112) |
| [F-098](13-Strix-Halo-Optimizations.md#f-098) | RDNA3.5 gated-delta-net tuning | **REDESIGN** | Medium | M2 | Cherry-pick concept only after target-side correctness and end-to-end model benchmarks. | [E-110](20-Evidence-Index.md#e-110), [E-113](20-Evidence-Index.md#e-113) |
| [F-099](13-Strix-Halo-Optimizations.md#f-099) | Vulkan AMD architecture routing | **RETAIN** | High | M3 | Keep target-native implementation; reconcile any missing capability logic narrowly. | [E-114](20-Evidence-Index.md#e-114) |
| [F-100](13-Strix-Halo-Optimizations.md#f-100) | Integrated Vulkan memory accounting | **RETAIN** | High | M3 | Retain target implementation and add pressure-aware reservations rather than treating all UMA as available. | [E-115](20-Evidence-Index.md#e-115) |
| [F-101](13-Strix-Halo-Optimizations.md#f-101) | Vulkan graph reordering and fusion preservation | **REDESIGN** | Medium | M2 | Diff at symbol level and port only missing optimizer cases with backend-op correctness tests. | [E-114](20-Evidence-Index.md#e-114), [E-116](20-Evidence-Index.md#e-116) |
| [F-102](13-Strix-Halo-Optimizations.md#f-102) | Experiment ledger with accepted/rejected variants | **RETAIN** | High | M2 | Retain the engineering practice: every kernel change needs correctness gates, matched baselines, and a rejection record. | [E-110](20-Evidence-Index.md#e-110) |
| [F-103](13-Strix-Halo-Optimizations.md#f-103) | Vulkan-first decode recommendation | **RETAIN** | High | M2 | Keep backend choice workload-driven and benchmark both for each format/model. | [E-110](20-Evidence-Index.md#e-110), [E-202](20-Evidence-Index.md#e-202), [E-212](20-Evidence-Index.md#e-212) |
| [F-104](13-Strix-Halo-Optimizations.md#f-104) | MoE-focused tuning priority | **REDESIGN** | Medium | M2 | Separate kernel/profile presets by workload rather than one global Strix default. | [E-110](20-Evidence-Index.md#e-110), [E-118](20-Evidence-Index.md#e-118) |

## ROCmFPX target baseline

| ID | Feature | Decision | Portability | Maturity | ROCmFPX disposition | Evidence |
|---|---|---|---|---|---|---|
| [F-105](14-ROCmFPX-Target-Baseline.md#f-105) | Run-scoped disk prompt cache | **RETAIN** | High | M3 | Keep as the default non-persistent mode and extend rather than replace it. | [E-203](20-Evidence-Index.md#e-203), [E-204](20-Evidence-Index.md#e-204), [E-205](20-Evidence-Index.md#e-205) |
| [F-106](14-ROCmFPX-Target-Baseline.md#f-106) | Atomic target/draft disk commit | **RETAIN** | High | M3 | Use it as the foundation for persistent CachyLlama-style checkpoints. | [E-206](20-Evidence-Index.md#e-206) |
| [F-107](14-ROCmFPX-Target-Baseline.md#f-107) | Disk-cache failure circuit breaker | **RETAIN** | High | M3 | Preserve for persistent mode and expose breaker state in metrics/health. | [E-208](20-Evidence-Index.md#e-208), [E-210](20-Evidence-Index.md#e-210), [E-211](20-Evidence-Index.md#e-211) |
| [F-108](14-ROCmFPX-Target-Baseline.md#f-108) | Focused disk-cache test suite | **RETAIN** | High | M3 | Extend it with restart persistence, manifest corruption, tenant isolation, concurrent instances, and upgrade tests. | [E-210](20-Evidence-Index.md#e-210), [E-211](20-Evidence-Index.md#e-211) |
| [F-109](14-ROCmFPX-Target-Baseline.md#f-109) | Stateful MTP exact-boundary cache semantics | **RETAIN** | High | M3 | Preserve when adding persistence and system-prefix caching. | [E-207](20-Evidence-Index.md#e-207) |
| [F-110](14-ROCmFPX-Target-Baseline.md#f-110) | Strix Halo ROCmFPX and MTP baseline | **RETAIN** | High | M3 | Port server/cache capabilities without overwriting newer quant/backend/MTP work. | [E-202](20-Evidence-Index.md#e-202), [E-212](20-Evidence-Index.md#e-212) |

