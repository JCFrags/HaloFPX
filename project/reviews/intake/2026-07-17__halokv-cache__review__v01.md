---
title: "HaloKV cache intake review"
date: "2026-07-17"
status: "reviewed"
review_type: "intake"
packages:
  - "HaloKV-LLM-Wiki"
  - "halofpx-kv-cache-wiki"
canonical_sections: ["56", "57", "58", "59", "60", "61", "62", "63", "64", "65", "77", "80"]
---

# HaloKV cache intake review

## 1. Review verdict

**Overall disposition: REVISE, then promote selectively.**

Both packages contain useful, evidence-backed material. Neither should replace canonical Sections 56–65, 77, or 80 as a unit.

- `HaloKV-LLM-Wiki` is strongest as a **distributed checkpoint protocol candidate**: rank-local ownership, all-rank commit, epoch fencing, explicit degraded modes, formal-model seeds, and failure campaigns. It is not a pinned audit of the project engines and its machine-readable checkpoint contract does not close over every continuation-state stream.
- `halofpx-kv-cache-wiki` is strongest as a **local persistence and integrity audit**: pinned CachyLLama behavior, local-file comparators, crash-safe publication recommendations, a complete outer-object validator, corruption fixtures, privacy controls, and an endurance model. Its proposed schema is not a complete distributed checkpoint, uses a different llama.cpp pin from the project baseline, and does not distinguish reusable prefix state from full session continuation strongly enough.

The packages are complementary: the first supplies distributed control semantics; the second supplies local object integrity and donor-code evidence. Their useful parts should be routed through `sources -> review -> canonical section or experiment`, with explicit reconciliation. Their proposed protocols and schemas remain candidates, not approved implementation decisions.

## 2. Authority and evidence reviewed

Review authority:

- project `AGENTS.md`, root `README.md`, canonical Wiki authority statement, and the Section 09 category manifest;
- canonical Sections 56–65, 77, and 80;
- Agent Harness `AGENTS.md`, `guide/architecture.md`, and `reviews/AGENTS.md` through `references/agent-harness.md`;
- both preserved extracted intake packages, including manifests, source locks/catalogs, schemas, validators, model artifacts, and recorded results.

Verification performed during this review:

- all **88** files listed by `HaloKV-LLM-Wiki/MANIFEST.sha256` matched their SHA-256 values;
- all **95** files listed by `halofpx-kv-cache-wiki/MANIFEST.sha256` matched their SHA-256 values;
- the `HaloKV-LLM-Wiki` linter passed its Markdown/JSON/CSV checks; `protoc` was unavailable, so that invocation performed delimiter rather than compilation checks for protobuf;
- the four Python reference-model tests passed when run from the package root;
- the nine `halofpx-kv-cache-wiki` validator tests passed when run from its `validation/` working directory;
- the first cross-workspace invocation of both test suites failed import discovery because each assumes its package-specific working directory. This is a packaging/runner portability issue, not a failed invariant test;
- the exact GitHub commit objects named by the cache-audit source lock were reachable at review time.

The packaged TLC result was inspected, not rerun. It reports 242,384 distinct states and no violation for the included bounded abstraction. That supports only that model/configuration, not a storage implementation or the unexecuted extension matrix.

## 3. Required cache taxonomy

The current intake language often uses “cache,” “KV cache,” “prefix cache,” “checkpoint,” and “state” too interchangeably. Promotion and implementation should require an explicit `object_kind` with different keys, sharing policy, completeness predicates, and restore tests.

| Object kind | What it stores | Correctness identity | Sharing boundary | Success criterion |
|---|---|---|---|---|
| `MODEL_TENSOR_CACHE` | Static or derived model tensors, converted/quantized layouts, RPC tensor copies, compiled artifacts | Exact source tensor/model digests, tensor name/shape/type, conversion/kernel/runtime ABI, backend/device semantics, format version | Model/license/deployment policy; normally not session scoped | Verified tensor bytes are accepted by the exact consumer; corruption is rebuild/miss |
| `PREFIX_INFERENCE_STATE` | Deterministic engine state after a reusable token/input prefix, including attention KV and any required recurrent/global state | Complete execution fingerprint plus exact prefix/input identity and boundary | Private/tenant/admin-approved public prefix policy | Engine imports a complete prefix into an isolated target and suffix-only execution matches recomputation |
| `SESSION_CHECKPOINT` | Continuation state for one session/generation at a visible logical boundary | Prefix identity plus all mutable continuation streams, sequence topology, output acknowledgement boundary, and session authorization | Private session only unless an explicit handoff authorization says otherwise | Restore consumes only the suffix/new input and preserves declared continuation semantics |
| `SYSTEM_PREFIX` | Administrator-approved reusable system/tool prefix | Same as prefix state plus immutable rendered-template/token range and sharing-policy digest | Explicit private, tenant, or administrator-approved public scope | Full token verification and ordinary prefix-state import checks pass |
| `RUNTIME_TRANSIENT` | RAM slot state, prompt reuse, speculative scratch, uncommitted pages | Runtime-local identity | Process/request scoped | Never advertised as persistent or durable |

This separation matters operationally:

- A valid model-tensor cache cannot prove a session can resume.
- A valid KV prefix does not necessarily contain sampler, RNG, grammar, beam, MTP/speculative, recurrent, sequence-fork, or externally acknowledged output state.
- A session checkpoint may use a prefix object internally, but its global completeness manifest must name every required stream.
- Model-tensor cache corruption can usually cause a tensor rebuild or model-load failure. Session-state corruption must leave the live destination unchanged and force recomputation/reset from a trusted boundary.

Neither intake package fully audits the live `rocmfp4-llama@4860505e` ggml RPC tensor cache identified by canonical Section 63. That cache remains a separate research and remediation track.

## 4. Intake disposition matrix

### 4.1 Package-level matrix

| Package area | Disposition | Reason and condition |
|---|---|---|
| `HaloKV-LLM-Wiki` as a preserved source package | **ACCEPT** | Integrity manifest is complete; license and source catalog are present; keep unchanged under `sources/imports`. |
| Distributed all-rank commit, epoch fencing, stale-worker rejection, and degraded-mode principles | **ACCEPT** | Consistent with Sections 58/63/80 and improves failure vocabulary. Promote as reviewed recommendations, not implemented facts. |
| Three-voter/independent authority as mandatory architecture | **DEFER** | The two-node quorum observation is sound, but the project has not chosen an authority, lease policy, availability target, or added third service. Record as an ADR option and experiment, not a settled requirement. |
| `HaloKV-LLM-Wiki` machine-readable checkpoint schema | **REVISE** | It names two rank manifests and a `recovery_capsule_id`, but defines no recovery-capsule schema and no typed required-state stream inventory. `PageRef` lacks component kind/layout completeness. |
| Package TLC/reference-model results | **ACCEPT** as design evidence | The bounded model and four tests are reproducible evidence for their abstraction. They are not implementation correctness, crash consistency, or liveness proof. |
| Package source catalog for engine behavior | **REJECT** as implementation authority | It relies mainly on public system documentation and contains no exact CachyLLama/llama.cpp/ROCmFPX source audit. Use only for design precedents. |
| `halofpx-kv-cache-wiki` as a preserved source package | **ACCEPT** | Integrity manifest is complete and source lock is exact. Keep unchanged under `sources/imports`. |
| Pinned CachyLLama audit at `6be745...` | **ACCEPT** | Matches the canonical donor pin and independently supports Sections 56, 57, 63, 64, 65, and 77. Promote individual claims with source IDs. |
| llama.cpp audit at `86d86ed...` | **REVISE** | Exact and reachable, but not the canonical project baseline `788e07...`. Mark it as a second applicability snapshot or repeat the audit against the canonical pin before merging claims. |
| LMCache/SGLang/vLLM comparator audits | **ACCEPT** as comparative source evidence | Exact commits are supplied. Promote bounded implementation observations, not their behavior as a HaloFPX guarantee. |
| Proposed local object/manifest format and integrity invariants | **REVISE** | Strong candidate for Sections 57/59/63, but requires object-kind/state-closure fields, distributed composition, an access-metadata write policy, and license clarification. |
| Offline validators and corruption fixtures | **ACCEPT** as candidate experiment tooling | Nine tests passed. They prove structural/HMAC/digest behavior for synthetic fixtures only; no engine import, concurrent hostile reader, crash, ENOSPC, power loss, or GPU restore is exercised. |
| Endurance formulas and illustrative scenarios | **ACCEPT** as method; **DEFER** values | Correctly labeled modeled inputs. No project SSD TBW, WAF, firmware, workload, or physical-write measurement is supplied. |
| Direct promotion of either proposed storage format as implementation-ready | **REJECT** | No implementation, exact engine adapter, crash matrix, distributed composition, or machine endurance evidence exists. |
| Direct import of legacy CachyLLama checkpoint bytes as trusted hits | **REJECT** | Outer payload authenticity/integrity is absent. Legacy parsing may inventory or diagnose; reuse requires a trusted migration/recompute path. |

### 4.2 Dimension matrix

| Audit dimension | `HaloKV-LLM-Wiki` | `halofpx-kv-cache-wiki` | Review outcome |
|---|---|---|---|
| Cache taxonomy | Mostly session/distributed KV checkpoint; model-tensor cache absent | Prefix/full-context state mechanisms are compared, but proposed manifest does not encode a definitive object kind | **REVISE** |
| Exact source commits | No pinned engine audit; dated docs/catalog | CachyLLama, llama.cpp, LMCache, SGLang, vLLM pinned | **ACCEPT second; REVISE applicability** |
| Crash consistency | Strong proposed rank/global commit sequence; no implementation | Strong proposed local object/manifest sequence; donor crash gaps precisely identified | **ACCEPT invariants; DEFER proof** |
| Corruption-as-miss | Explicit invariant and fuzz plan | Executable synthetic digest/HMAC rejection tests | **ACCEPT** |
| Fingerprint completeness | Broad topology and prefix fields | Strong full execution manifest fields | **MERGE/REVISE** to one schema plus state-kind closure |
| Write amplification/endurance | Byte credits and checkpoint sizes, but no full endurance treatment | Full-state write model, quotas, dedup, coalescing, SMART/WAF method | **PROMOTE method from second; DEFER numbers** |
| Privacy/isolation | Strong threat model but schema permits a plain `tenant_namespace` | HMAC namespace, authorization-before-lookup, quotas, AEAD/key guidance | **REVISE first schema; ACCEPT second policy candidates** |
| Distributed ownership | Core strength: rank-local objects plus global certificate | Explicitly local; no all-rank global commit | **PROMOTE first; do not stretch second** |
| Testability | TLA/reference model/fuzz campaigns | Parsers, validators, fixtures, fault cases | **ACCEPT as complementary candidates** |

## 5. Exact source lineage

Canonical project pins remain:

- CachyLLama: `6be745998f568e379ea197fcf827baec73ff9940`;
- llama.cpp: `788e07dc91d266ad3162a1ce9037665656269689`;
- ROCmFPX: `a5605a72768c6562241b248e268e33dc92787394`.

`halofpx-kv-cache-wiki/research/source-lock.json` pins:

- CachyLLama: `6be745998f568e379ea197fcf827baec73ff9940` — aligned;
- llama.cpp: `86d86ed4396b4130922f7b9af26e3d9fc11a591b` — **different applicability snapshot**;
- LMCache: `c9439c6535503c9e17fe236da9bc88807b58c2bc`;
- SGLang: `fec613184480bd6fc5bfc9967bfb24a6125f684c`;
- vLLM: `bf578e1abdffc2d25232783ff59a3132279e6bdd`.

The package does not pin or audit ROCmFPX. Its compatibility and storage proposals therefore cannot claim ROCmFPX state-ABI, MTP, speculative, or distributed behavior. The second llama.cpp pin must not silently replace the frozen baseline. A promotion note must state whether a claim applies to `788e07...`, `86d86ed...`, both after matched inspection, or neither.

`HaloKV-LLM-Wiki` supplies dated primary/official design references, not exact engine source commits. Its protocol synthesis is valuable, but should not be used to assert current llama.cpp/CachyLLama/ROCmFPX behavior.

## 6. Contradictions and blocking gaps

### C-01 — Session generation is inside the proposed immutable page identity

`HaloKV-LLM-Wiki/wiki/Rank-Local-Cache-Keys.md` places `session_generation` in the canonical page header while also presenting pages as reusable content-addressed objects and advocating system/prefix reuse. That makes byte-identical pages differ across session generations and prevents the intended deduplication domain.

**Required revision:** separate immutable semantic page identity from ownership/reachability metadata. Put tenant sharing scope, session generation, and authorization in the manifest/catalog unless a threat decision deliberately chooses tenant-scoped ciphertext/object IDs. Document whether deduplication is intra-checkpoint, intra-session, intra-tenant, or cross-tenant.

### C-02 — Recovery capsule is required but undefined

The global checkpoint schema requires `recovery_capsule_id`; no machine-readable capsule schema defines token history, input references, sampler/RNG/grammar/beam state, sequence layout, emitted-token acknowledgement, or authorization.

**Required revision:** define a versioned recovery-capsule object or remove it as a required placeholder. Its manifest must distinguish stored, reconstructible, and forbidden streams and bind their digests atomically.

### C-03 — Rank pages do not prove complete inference state

The first package's `PageRef` names position/layer ranges and lengths but not state kind, dtype, layout, tensor coordinates, required/optional status, or complete shard coverage. The prose is broader than the schema.

**Required revision:** add typed component descriptors and a topology-specific completeness predicate. A global commit must prove exact coverage of all required target, recurrent, draft/speculative, sampler/RNG/grammar/beam, sequence, and output-boundary streams for the declared checkpoint kind.

### C-04 — Plain tenant namespace conflicts with privacy-preserving identity

The first package recommends privacy-preserving HMAC lookup keys, yet its protocol/JSON schemas accept a general `tenant_namespace` string. This can invite raw tenant identifiers into manifests, traces, and paths.

**Required revision:** make the wire/storage namespace an opaque fixed-length derived identifier and keep human tenant identity only in the authorization system. Define rotation and deletion semantics.

### C-05 — Prefix hit and session continuation are not separated

The second package permits `required_segments: ["target"]` and describes `HIT_VERIFIED(state_handle, verified_prefix_tokens)`. That can be correct for prefix prefill reuse, but not by itself for exact session continuation. Canonical Section 61 specifically leaves sampler/grammar/RNG continuity unresolved.

**Required revision:** add `object_kind` and a required-stream profile. `PREFIX_INFERENCE_STATE` may require only the target engine state plus architecture-specific global streams; `SESSION_CHECKPOINT` must additionally close over every mutable continuation stream and client-visible boundary.

### C-06 — Access time in the authenticated manifest conflicts with write-amplification guidance

The second schema requires `last_access_unix_ns` in the authenticated, generationed manifest, while its endurance chapter warns against rewriting metadata on every hit. No normative update/coalescing rule reconciles the two.

**Required revision:** keep correctness manifests immutable or change only on semantic generation. Store recency in a lossy/coalesced rebuildable access journal/database that is outside the hit-authenticity root, or specify a bounded coalescing transaction and quantify its writes.

### C-07 — Local and distributed commit points are not composed

The second package's authenticated local manifest makes one object reachable. The first package's global certificate makes all ranks reachable at one boundary. Neither package defines the composed rule.

**Required revision:** rank-local authenticated manifests/objects become `PREPARED`; only the global all-rank certificate is a distributed `COMMITTED` checkpoint. A local valid object is never sufficient to resume a distributed session.

### C-08 — Proposed external authority is not a project decision

The first package declares a normally three-voter strongly consistent service. It correctly explains that two members tolerate zero failures, but adds an operational dependency not chosen in canonical architecture.

**Required revision:** route this to an ADR comparing strict two-node stop-on-uncertainty, an independent local authority, and a three-voter service. Include backup/rollback, failure availability, resource cost, and home-lab operability.

### C-09 — Licensing is incomplete for the second package

The first package declares CC BY 4.0 for original documentation and Apache-2.0 for code/schema examples. The second package has a `NOTICE.md` and four Kaitai schemas marked CC0-1.0, but no repository-level license for its documentation, Python validators, other schemas, tables, or diagrams.

**Required revision:** clarify the original artifact license before copying or adapting those materials into canonical Wiki or experiments. Links and independently restated factual observations may still be used with provenance.

### C-10 — Validation is outer-format only

The second package's tests correctly stop at `IMPORT_CANDIDATE_VALID`; no real llama/Cachy/ROCm engine state is imported. The first package's finite model abstracts storage bytes, cryptography, quota, and GPU state.

**Required revision:** do not label either package as an implementation proof. Add integrated suffix-only restore, destination-unchanged-on-failure, concurrent writer/reader, ENOSPC/EIO, process kill, reboot/power-cut, distributed rank loss, and exact topology mismatch tests.

## 7. Implementation-ready invariants

The following invariants are suitable for promotion as normative requirements after terminology/license review. They do not assert an implementation exists.

| ID | Invariant |
|---|---|
| `HK-TAX-01` | Every persisted object declares exactly one `object_kind`; readers never infer model tensor, prefix, or session semantics from a filename or payload shape. |
| `HK-KEY-01` | Direct reuse requires a versioned canonical fingerprint of exact model/shard bytes, tokenizer/effective template, adapters, runtime/state ABI, attention/RoPE/window semantics, K/V representation, feature modes, and topology fields that affect values/layout. |
| `HK-KEY-02` | Prefix identity commits to exact token/input boundaries plus multimodal, embedding, adapter, position/mask, and policy inputs that affect state. |
| `HK-TENSOR-01` | Model-tensor cache keys and acceptance are separate from session/prefix state; a model-tensor hit grants no continuation authority. |
| `HK-STATE-01` | Each checkpoint kind has a versioned required-stream profile. Unknown, absent, corrupt, or mismatched required streams make the checkpoint a miss/reset candidate. |
| `HK-STATE-02` | Optional draft/speculative state may fail independently only when the target state is complete and the exact engine contract proves safe reconstruction/catch-up. |
| `HK-INTEGRITY-01` | Length, schema, structure, content digest/AEAD, manifest authenticity, compatibility, ownership, and required-stream checks complete before engine import or GPU materialization. |
| `HK-MISS-01` | Any uncertain cache state produces a typed miss/recompute/reset outcome; invalid bytes are never partially accepted. |
| `HK-IMPORT-01` | Restore imports into an isolated/transactional destination and leaves the live context unchanged on any failure. |
| `HK-DUR-01` | Data is written to a unique temporary object, bounded and verified, synced, published immutably/no-replace, and directory-synced before an authenticated manifest may reference it. |
| `HK-DUR-02` | A manifest/catalog update is atomic and monotonic; failure leaves the prior committed generation valid. Orphans are unreachable and garbage-collected after a grace period. |
| `HK-DIST-01` | In distributed mode, rank-local valid objects/manifests are only prepared state. One authoritative global certificate commits exactly the expected rank set at one logical boundary and topology. |
| `HK-DIST-02` | Missing/corrupt/timed-out ranks cause miss/recompute or an explicitly validated complete single-node fallback; partial rank state is never published. |
| `HK-AUTH-01` | Principal authorization and opaque namespace derivation occur before existence lookup. Content similarity never grants continuation access. |
| `HK-SHARE-01` | Deduplication/sharing scope is explicit. Public/system sharing requires administrator-approved immutable rendered tokens and the complete compatibility fingerprint. |
| `HK-GC-01` | Eviction is reachability-aware, quota-fair, active-reader-safe, and cannot remove objects referenced by retained committed manifests. |
| `HK-END-01` | Admission and retention are byte-based. Host writes, physical-device writes where available, WAF assumptions, compaction/migration bytes, SMART health, and saved-prefill benefit are measured separately. |
| `HK-TEST-01` | Restore correctness is tested by restoring a saved prefix/checkpoint and supplying only the suffix/new input; full-prompt replay alone is not restore proof. |
| `HK-TEST-02` | Every durability claim has a crash matrix for write, sync, rename, directory-sync, local/rank manifest, and global-certificate boundaries on disposable storage. |

## 8. Promotion mapping

| Canonical destination | Promote now as reviewed candidate | Revise or defer before promotion |
|---|---|---|
| Section 56 | Second package's exact CachyLLama v3/index/system format audit, FNV compatibility gap, final-path writes, index-loss collision inference, and optional draft behavior | Preserve bounded source wording; no legacy file becomes trusted state |
| Section 57 | Combined complete fingerprint field inventory and exact-vs-transport compatibility distinction | Reconcile llama pin; add object kind and required-stream profile; publish canonical test vectors |
| Section 58 | First package's rank-local ownership, all-ready/global certificate, missing-rank recovery, stale epoch, and single-node feasibility predicates | Authority implementation/lease choice; exact ROCmFPX ownership map; composed local/global schema |
| Section 59 | Immutable objects, rebuildable indexes, no sequential ID dependency, orphan-safe publication | Select canonical encoding/page size/metadata engine only after prototypes; remove session generation from content ID unless deliberately scoped |
| Section 60 | HMAC-derived namespace, authorization-before-lookup, explicit sharing modes, no fuzzy cross-user continuation | Resolve dedup domain, timing policy, rendered-template boundaries, deletion and key rotation |
| Section 61 | The identified need for target plus optional draft/spec streams and transactional import | Do **not** promote either machine schema as complete; define sampler/RNG/grammar/beam/recurrent/MTP/sequence/output streams and exact-continuation contract |
| Section 62 | Bounded streaming validation and I/O cancellation as requirements | Both packages lack target-machine async-I/O/GPU-mapping proof; keep current section and run experiments |
| Section 63 | Temp-write/verify/sync/no-replace/directory-sync, authenticated manifests, corruption-as-miss, quarantine, old-or-new commit invariant | Physical power-loss, filesystem, firmware, multi-process, and distributed certificate tests remain required |
| Section 64 | HMAC namespaces, pre-lookup authorization, quotas, AEAD/key separation, no secure-delete claim from unlink/overwrite | Privacy retention, encryption default, backup/export, key rotation, timing and media-sanitization policy remain open |
| Section 65 | Read-only validator first, stable reason codes, byte accounting, WAF/TBW formulas, access-write coalescing, migration copy-on-read | License clarification; exact SSD/firmware/TBW/SMART inventory and physical-write experiments |
| Section 77 | Second package validators/fault fixtures and first package outcome/byte metrics as experiment seeds | No hit-rate, latency, engine restore, writeback, or endurance value is a HaloFPX measurement |
| Section 80 | First package crash/corruption/network campaign and second package deterministic mutation corpus | Run only under Section 80's isolated/disposable authorization boundary; add real process/reboot/rank fault execution |

Promotion should use small claim-level edits, source records, or experiment cards. It should not copy either Wiki wholesale or silently replace canonical pages.

## 9. Research and implementation gaps

### P0 — block schema implementation

1. Define `object_kind` and required-stream profiles for model tensors, prefix state, session checkpoints, and system prefixes.
2. Define the recovery-capsule schema, ownership, sensitivity, authentication, and relation to emitted-token acknowledgement.
3. Reconcile the canonical llama.cpp pin with `86d86ed...`; audit ROCmFPX `a5605a...` state, MTP/speculative, and distributed ownership surfaces.
4. Compose rank-local authenticated manifests with the global all-rank certificate.
5. Decide whether object IDs deduplicate within session, tenant, or globally; remove accidental session-generation coupling if broader reuse is intended.
6. Clarify `halofpx-kv-cache-wiki` licensing before copying its prose/code/schema artifacts.

### P1 — correctness prototypes

1. Implement a minimal adapter that imports into a disposable sequence/context and proves the live destination is unchanged on failure.
2. Produce suffix-only restore tests for deterministic target state and separate exact/stochastic continuation tests.
3. Mutate each required state stream independently, including recurrent, draft, speculative/MTP, sampler, RNG, grammar, beam, sequence, and output-boundary metadata.
4. Build canonical fingerprint vectors across at least two implementations/languages and test one-field-at-a-time invalidation.
5. Audit the existing RPC model-tensor cache separately, including read-time rehash, atomic publication, disk pressure, and rebuild semantics.

### P2 — durability and distributed recovery

1. Execute kill/restart/ENOSPC/EIO tests at every local publication boundary.
2. Execute two-rank prepare/commit/cancel/epoch/rank-loss tests against the exact execution plan and storage topology.
3. Compare strict stop-on-uncertainty with candidate external authority designs through an ADR and failure experiment.
4. Verify older-checkpoint-plus-forced-token replay and complete single-node fallback predicates without resampling emitted tokens.

### P3 — performance, endurance, and privacy

1. Inventory exact SSDs, firmware, filesystem, mount, SMART tool revision, rated TBW, and available physical-write counters.
2. Measure full-state versus page/delta writes, metadata access-journal writes, compaction/migration/rotation writes, WAF assumptions, and saved prefill.
3. Test tenant quotas, anonymous/public sharing, timing leakage, key rotation, deletion/backup/export, and quarantine retention.
4. Benchmark digest/HMAC/AEAD validation and isolated import separately from lookup, I/O, rank coordination, and resumed prefill.

## 10. Primary-source register

Project/fork commits:

- CachyLLama `6be745998f568e379ea197fcf827baec73ff9940`: <https://github.com/fewtarius/CachyLLama/commit/6be745998f568e379ea197fcf827baec73ff9940>
- canonical llama.cpp `788e07dc91d266ad3162a1ce9037665656269689`: <https://github.com/ggml-org/llama.cpp/commit/788e07dc91d266ad3162a1ce9037665656269689>
- intake llama.cpp `86d86ed4396b4130922f7b9af26e3d9fc11a591b`: <https://github.com/ggml-org/llama.cpp/commit/86d86ed4396b4130922f7b9af26e3d9fc11a591b>
- ROCmFPX `a5605a72768c6562241b248e268e33dc92787394`: <https://github.com/charlie12345/ROCmFPX/commit/a5605a72768c6562241b248e268e33dc92787394>
- LMCache `c9439c6535503c9e17fe236da9bc88807b58c2bc`: <https://github.com/LMCache/LMCache/commit/c9439c6535503c9e17fe236da9bc88807b58c2bc>
- SGLang `fec613184480bd6fc5bfc9967bfb24a6125f684c`: <https://github.com/sgl-project/sglang/commit/fec613184480bd6fc5bfc9967bfb24a6125f684c>
- vLLM `bf578e1abdffc2d25232783ff59a3132279e6bdd`: <https://github.com/vllm-project/vllm/commit/bf578e1abdffc2d25232783ff59a3132279e6bdd>

Preserved intake roots:

- `sources/imports/2026-07-17-further-research-wikis/extracted/HaloKV-LLM-Wiki/HaloKV-LLM-Wiki/`
- `sources/imports/2026-07-17-further-research-wikis/extracted/halofpx-kv-cache-wiki/halofpx-kv-cache-wiki/`

## 11. Closeout assessment

The highest-value reusable improvement is the merged safety boundary:

> A local object can be structurally valid and still not be compatible, authorized, complete, imported, or globally committed. A distributed session hit exists only after all of those gates pass for the declared cache kind and every required rank/state stream.

That principle is ready for promotion. The concrete schemas, authority choice, runtime adapters, durability modes, privacy defaults, and performance values are not.
