---
title: "Adversarial review of HaloFPX Wiki sections 49-72"
date: "2026-07-17"
status: "proposed"
scope: "wiki/HaloFPX_Wiki sections 49-72"
review_type: "transport, cache, operations, security, versioning, provenance, and machine-evidence audit"
---

# Adversarial review: sections 49-72

## Verdict

All 24 section directories contain the seven required artifacts. The repository validator reports 86 of 86 total wiki sections complete, the audited YAML parses, and no broken relative Markdown link was found. Exact live HEAD checks on 2026-07-17 confirmed the principal source pins: ROCmFPX `a5605a72768c6562241b248e268e33dc92787394`, llama.cpp `788e07dc91d266ad3162a1ce9037665656269689`, CachyLlama `6be745998f568e379ea197fcf827baec73ff9940`, llama-ai `1017f3dfdce3ca2b06aa9007b23295db3bb35722`, and OpenAI OpenAPI `db3e53198a66732cfe161339ea63bf36fc0137ad`.

The 2026-07-17 remediation resolves all seven original P1 findings at the design and documentation layer and applies the safe mechanical P2/P3 corrections. Sections 49-72 are accepted as an evidence-backed research baseline with explicit implementation and machine-validation gates; they are not promoted to verified runtime authority. No HaloFPX TLA+/TLC model has been executed, no new machine benchmark is claimed, and deployed RPC binary/library provenance plus current listener exposure remain open. RPC therefore remains disabled by default.

Two new security/correctness gates were incorporated during remediation. GHSA-j8rj-fmpv-wcxw and source fix `ba38f3becce7d1283585c73d796eb47d72bbbd30` are now mapped into Sections 51 and 71, but source containment is not treated as deployed remediation. Sections 53 and 63 now require proposed TLA+/TLC v1.7.4 model-checking gates and state explicitly that no HaloFPX model or run exists yet.

| Category | Status | Basis |
|---|---|---|
| 08 Fabric and Transport (49-55) | **ACCEPT WITH OPEN MACHINE GATES** | Global epoch reset, requirement traceability, authenticated bulk records, RPC source/deployment gate, fault isolation, and measurement provenance are documented. Wire conformance, model checking, and two-node tests remain open. |
| 09 HaloKV Persistent Cache (56-65) | **ACCEPT WITH OPEN MACHINE GATES** | Durability is mode-aware, object framing is deterministic, research splits/provenance are repaired, and destructive tests are isolated. Golden vectors, crash matrices, model checking, and target-filesystem tests remain open. |
| 10 Product, Server, and Operations (66-72) | **ACCEPT WITH OPEN MACHINE GATES** | Source authority, per-field configuration ownership, health/error contracts, security gates, and complete runtime/data cutover phases are documented. Deployment, compatibility, and rollback rehearsals remain open. |

## Applied remediation summary

| Original finding | Disposition on 2026-07-17 |
|---|---|
| Rail/epoch contradiction | **RESOLVED.** Section 52 now uses the Section 53 global-epoch barrier; old-epoch partial records do not migrate rails and retry occurs only at an idempotent whole-operation boundary. |
| Durability-mode contradiction | **RESOLVED.** Sections 59 and 63 make publication/recovery guarantees mode-aware and require validation, rejection/quarantine, or recomputation of incomplete generations. |
| Unauthenticated bulk DATA | **RESOLVED.** `AUTH_INTEGRITY` authenticates every post-handshake control and DATA record; CRC32C is diagnostic only. Omitting confidentiality requires explicit Section 71 threat acceptance. |
| Section 49 wire traceability | **RESOLVED.** Required identity/order fields are mapped to connection-bound, negotiated, wire-header, or authenticated upper-layer properties. |
| Broken CachyLlama authority | **RESOLVED.** Sections 71 and 72 use the pinned `fewtarius/CachyLLama` authority and scoped claims. |
| Global configuration precedence | **RESOLVED.** Section 67 defines field ownership first and permits precedence only among authorities allowed to set that field. |
| Pointer-only cutover | **RESOLVED.** Sections 70 and 72 distinguish install, activation pointer, process, identity/readiness, traffic, durable state, and rollback phases. |
| Disruptive procedure safety | **RESOLVED.** Affected procedures require exact disposable/sacrificial targets, privileges and ceilings, stop conditions, preserved recovery access, cleanup, and Section 80 authorization where applicable. |
| Front-matter/category status drift | **RESOLVED.** Invalid statuses and stale category summaries were normalized. `validate_wiki.py` now enforces the required permissive-core `section.yaml` shape, registry identity/category, enums, dates, types, and content-matched source/open-question counts while allowing deliberate extensions; all 86 sections pass. |
| Hash framing | **RESOLVED.** Sections 57 and 60 specify domain-separated, length-delimited, fixed-endian encoding and retain conformance-vector gates. |
| Historical measurement provenance | **RESOLVED.** Section 55 cites the direct report/raw-tree provenance and keeps the result historical and configuration-scoped; Section 51 uses `[VERIFIED]` for source identity. |
| API/health drift | **RESOLVED.** Operations uses `/health/live`, `/health/ready`, and `/health/startup`; `halofpx.error` is a negotiated namespaced extension, not generic compatibility. |
| Research splits/source records | **RESOLVED.** Sections 61-65 and 69-72 separate completed research, machine work, and contingent decisions and improve pins, access dates, claims, and limitations. |
| Source-audit versus deployment kernel | **RESOLVED.** Inspected source snapshots are explicitly distinct from a future deployment candidate, which requires re-diff and machine capability evidence. |

## Severity-ranked findings

The evidence and impact text below records the pre-remediation state found by this review. Heading dispositions and the applied-remediation table above record the current outcome; retained wording is an audit trail, not a claim that a resolved defect remains present.

### P1 — Rail failure has contradictory epoch and retransmission semantics — RESOLVED

**Evidence.** Section 52 says missing chunks can be retransmitted on the surviving rail while retaining the original generation (`52_Dual_Link_Multipath_Striping_Alternation_Hedging_and_Failover/design_implications.md:35-37`). The proposed authoritative v1 protocol says any rail failure terminates the global epoch, outstanding partial records do not move rails, and all rails renegotiate (`53_Message_Framing_Credits_Flow_Control_Integrity_and_Security/README.md:20`; `design_implications.md:93-101`). Section 55 also says transport loss invalidates the session epoch (`55_Fabric_Microbenchmark_Plan_and_USB4_Kernel_Patch_Decision/design_implications.md:19`).

**Impact.** Two conforming-looking implementations can disagree about whether old-epoch chunks, credits, ACKs, nonces, and rank state remain valid after one rail fails. That creates stale-state acceptance and duplicate/lost work risk.

**Required revision.** Adopt one global rule. The safer v1 baseline is: barrier all rails, fail or drain outstanding work at an identified upper-layer boundary, create a new epoch/keys/credits, and retry only an idempotent whole operation. If same-epoch failover remains a candidate, give it a distinct version and a full correctness/security/fault proof.

### P1 — Cache crash guarantees conflict with the performance durability mode — RESOLVED

**Evidence.** Section 59 says a crash after metadata commit leaves a fully referenced generation and requires committed DAG nodes never to reference nondurable pages (`59_Immutable_Pages_Segment_Files_Indexes_and_Prefix_DAG/design_implications.md:59-66`; `procedures_and_checks.md:41-43`). Section 63's performance mode explicitly permits checkpoint loss after failure (`63_Durability_Modes_Atomic_Commit_Crash_Recovery_and_Corruption_Handling/facts_and_constraints.md:23-29`).

**Impact.** The format cannot promise post-commit durability if the selected mode permits data or metadata reordering/loss. Recovery could encounter a committed reference to a missing/torn object.

**Required revision.** Make the invariant mode-aware. In every mode, recovery validates every object/reference and rejects, quarantines, or recomputes an incomplete generation. Reserve the stronger “no dangling committed reference after the declared failure model” guarantee for turn-durable/strict modes whose flush and filesystem contract has been tested.

### P1 — The plaintext transport profile does not authenticate bulk inference state — RESOLVED

**Evidence.** `AUTH_PLAINTEXT` authenticates the handshake and control records but gives DATA only CRC32C; bulk DATA has no cryptographic trailer (`53_.../design_implications.md:70-79`). Section 71 requires mutual authentication and integrity for peer commands and transferred inference state and says any plaintext exception must still prevent impersonation, tampering, and replay (`71_Security_Trust_Boundaries_Permissions_Local_Network_and_Secrets/design_implications.md:29-31`).

**Impact.** CRC detects accidents but does not prevent an active party from modifying payload and recomputing CRC. The profile name and “authenticated” language can mislead implementers into accepting unauthenticated tensors/cache/state.

**Required revision.** At minimum authenticate every DATA record with a bulk MAC, and rename the current proposal if retained for a tightly scoped lab probe. Bind profile selection to Section 71 threat acceptance. Encryption may be optional in a trusted lab; peer identity, record integrity, freshness, and replay protection may not be silently optional.

### P1 — Section 49 requirements are not traced into the authoritative wire contract — RESOLVED

**Evidence.** Section 49 requires peer/session/rank IDs, lane, correlation ID, and step in the transport header (`49_Fabric_Requirements_and_Transport_Abstraction/facts_and_constraints.md:45`). Section 53's 64-byte header contains rail/channel, epoch, message ID, offsets, lengths, and CRCs, but no explicit peer, rank, step, or correlation fields (`53_.../design_implications.md:19-38`).

**Impact.** Required identity may be silently dropped, inconsistently inferred from a connection, or duplicated in an undocumented upper layer. Diagnostics, replay rejection, and rank ownership then disagree across components.

**Required revision.** Publish a requirement-to-field map. State which values are connection-bound, negotiated, or upper-layer descriptors; define how each is integrity-bound and logged. Add fields or a versioned upper-layer descriptor where the mapping cannot preserve the requirement.

### P1 — Security and migration claims cite a nonexistent repository — RESOLVED

**Evidence.** Sections 71 and 72 cite `github.com/anthony-maio/CachyLlama` (`71_.../sources.md:24`; `72_.../sources.md:21`) for verified cache-format/integrity claims (`71_.../facts_and_constraints.md:23`; `72_.../facts_and_constraints.md:21`). Live Git access returned “Repository not found.” The valid pinned authority is `fewtarius/CachyLLama@6be745998f568e379ea197fcf827baec73ff9940`.

**Impact.** A reader cannot retrieve the claimed primary evidence from the wiki, and automated provenance checking will fail.

**Required revision.** Replace both URLs with the exact valid source paths/commit and add access date, claims, and limitations per the output standard. Recheck the exact files before retaining `[VERIFIED]`.

### P1 — Configuration precedence crosses authority and security boundaries — RESOLVED

**Evidence.** Section 67 orders model and plan manifests, environment variables, and CLI arguments above base service configuration (`67_Configuration_Hardware_Profiles_Model_Manifests_and_Plan_Manifests/facts_and_constraints.md:32-44`). Its own README instead describes separate immutable inputs and says request hints cannot bypass safety, identity, memory, or compatibility gates (`README.md:15-22`).

**Impact.** A tuned plan or launch flag could be read as authorized to override listeners, authentication, secret policy, durability, model admission, or tenant isolation merely because it has higher global precedence.

**Required revision.** Define field ownership and authority first. Apply precedence only among sources allowed to set a field. Service security/policy constrains hardware/model/plan inputs; plans select execution within admitted bounds and never override security or artifact identity.

### P1 — Atomic release-pointer switching is not a complete runtime cutover — RESOLVED

**Evidence.** Sections 70 and 72 recommend immutable release directories and an atomic `current`/activation pointer (`70_Packaging_systemd_Containers_Deployment_and_Cold_Boot_Procedure/design_implications.md:17-20`; `72_Upgrades_Rollbacks_Protocol_and_Cache_Migration_Backup_and_Runbooks/design_implications.md:23-26`). The rehearsal tests failure around pointer switching but does not define the distinct process restart/re-exec, readiness, traffic commit, and state compatibility transitions (`72_.../procedures_and_checks.md:19-21`).

**Impact.** Updating a symlink does not alter an already-running listener. Reversing it does not roll back migrated state, process memory, or client-visible work.

**Required revision.** Define phases: install immutable candidate; validate offline; switch candidate pointer; start/restart candidate; prove build/model/plan/dependency identity; obtain rank/readiness gates; canary; commit traffic; and separately preserve/restore old pointer plus compatible state preimage. Report deployed runtime identity, not only source qualification.

### P2 — Fault procedures do not consistently protect the machines and retained data — RESOLVED

Potentially disruptive instructions lack the safety boundary already modeled in Section 80:

- cable removal/interface down/worker restart in `49_.../procedures_and_checks.md:42-44` and `52_.../procedures_and_checks.md:36-38`;
- rank-component delete/corrupt/truncate in `58_.../procedures_and_checks.md:30-35`;
- cache truncate, ENOSPC, and EIO in `62_.../procedures_and_checks.md:27-29`;
- cache mutation/deletion/endurance in `65_.../procedures_and_checks.md:29-35`;
- disk-full/power-loss/path/parser tests in Sections 69-71.

Every such procedure must require a disposable deployment/store/loopback or sacrificial target, exact resolved targets, free-space/resource ceilings, root/privilege declaration, preserved recovery access, stop conditions, cleanup, and Section 80 authorization for physical/kernel/device faults. No production cache, model store, workspace, boot disk, or sole evidence copy may be a target.

### P2 — Section metadata does not conform to the declared machine-readable standard — RESOLVED

The output standard permits only `draft`, `verified`, `needs-machine-validation`, or `superseded` and requires `section.yaml` to expose category and applicability (`OUTPUT_STANDARD.md:24-42`). Sections 69-72 use invalid Markdown statuses such as `open`, `verified-online`, and `verified-online-and-source-audited`. Across Sections 49-72, `section.yaml` generally uses `category_id`/`category_title` and `applicability` rather than the standard's `category` and applicability shape.

This is a schema/indexing defect even though the current completeness validator does not reject it. Define one JSON/YAML schema, migrate all manifests, and make the validator enforce it before claiming machine-readable completeness.

### P2 — Hash framing is ambiguous in the compatibility design — RESOLVED

Section 57 defines `object_id = SHA-256(domain || object_type || length || object_bytes)` without specifying byte order, fixed widths, length prefixes, or the object-type encoding (`57_Compatibility_Fingerprints_Versioning_and_Topology_Identity/design_implications.md:19-23`). Section 60's content/policy/object-key shorthand inherits this dependency (`60_System_Prompt_Sharing_Deduplication_Copy_on_Write_and_Continuations/design_implications.md:17-25`).

Use deterministic CBOR or a completely specified length-delimited binary frame with domain separation and golden vectors. SHA-256 does not remove an ambiguous preimage encoding.

### P2 — The historical measured transport claim does not cite its raw evidence directly — RESOLVED

Section 55 labels approximately 20.8 Gb/s historical USB4NET evidence `[MEASURED]` (`README.md:19`; `facts_and_constraints.md:17`) but its source register points to a feasibility plan and decision that are not the execution artifact (`sources.md:24-27`). The direct report exists at `C:/Users/britt/AgentRoot/10_projects/11_active/11.02_strix-halo-usb4-cluster/03_validation/output/2026-07-12__m0-usb4net-transport-baseline__report__v01.md` and records the raw-tree provenance.

Cite the measurement report and retained raw/environment manifest directly or downgrade the claim. The result remains historical and configuration-scoped, never a current HaloFPX control.

Section 51 similarly labels a source-tree hash observation `[MEASURED]` without a retained evidence receipt (`51_Existing_ggml_RPC_and_ROCmFPX_RDMA_Transport_Audit/facts_and_constraints.md:17`; `sources.md:24-26`). A reproducible source identity is better labeled `[VERIFIED]` unless an experiment receipt is intentionally retained.

### P2 — API and health contracts are not canonical across operations pages — RESOLVED

Section 66 proposes `/healthz` and `/readyz` (`66_OpenAI_Compatible_API_Server_Semantics_and_Error_Model/facts_and_constraints.md:33-34`); Section 69 uses `/health/live` (`69_CLI_Admin_API_Diagnostics_Health_Metrics_Logs_and_Traces/facts_and_constraints.md:25-26`). Section 66 also proposes a custom terminal streaming-error event without specifying endpoint, event name/payload, client negotiation, or whether strict OpenAI-compatible clients may receive it (`66_.../facts_and_constraints.md:40-42`).

Choose one canonical liveness/readiness/startup route set and define aliases/version behavior. For streaming failures, either prove endpoint-specific compatibility with pinned SDK/fixture tests or negotiate a namespaced HaloFPX extension; never advertise a custom event as generic compatibility.

### P2 — Research splits and source registers are incomplete — RESOLVED

Sections 61-65 do not explicitly separate completed source work, target-machine measurements, and decisions contingent on those measurements as required by `OUTPUT_STANDARD.md:86-92`. Sections 69-72 and much of 61-65 also omit access dates, publication/revision details, or limitations for individual source records despite `OUTPUT_STANDARD.md:57-67`.

Section 59 additionally cites mutable kernel “current docs” and BLAKE3 `master`; Section 60's S60-08 says “and context implementation” without an exact link. Replace mutable discovery sources with immutable revisions where feasible, record access dates, and narrow claims when the exact implementation file was not inspected.

### P2 — Linux source-audit and deployment-candidate identities are conflated — RESOLVED

Sections 49-52 inspect Linux `fce2dfa773ced15f27dd27cd0b482a7473cdcf2a`; Sections 53-55 inspect v7.2-rc2 `8cdeaa50eae8dad34885515f62559ee83e7e8dda`. Live comparison found `drivers/thunderbolt/stream.c` byte-identical at those pins, so there is no present code contradiction. The category still needs one note distinguishing inspected source pins from an approved machine kernel, plus a mandatory re-diff and machine capability test when the stable/deployment kernel is chosen.

### P3 — Category summaries and two scoped claims need cleanup — RESOLVED

- Category READMEs 08-10 still say `Research status: pending` despite populated sections.
- Section 50's config inventory regex at `procedures_and_checks.md:25` embeds `CONFIGFS_FS` inside a `CONFIG_` prefix group and therefore searches for `CONFIG_CONFIGFS_FS`; correct the expression.
- Section 64's claim that the implementation lacks per-user byte quotas, encryption, and secure deletion is supported only by an inspected `common/` subtree (`64_Eviction_Garbage_Collection_Quotas_User_Isolation_and_Privacy/README.md:15-16`; `sources.md:17`). Narrow it to “not identified in inspected cache files” or retain a whole-repository audit receipt.

## Per-section disposition

| Section | Status | Required action before promotion |
|---|---|---|
| 49 | **ACCEPT WITH CONDITIONS** | Wire traceability and fault authorization are documented; conformance and two-node tests remain open. |
| 50 | **ACCEPT WITH CONDITIONS** | Regex and source/deployment terminology are corrected; deployment-kernel evidence remains open. |
| 51 | **ACCEPT WITH SECURITY GATE** | Source mapping is verified; deployed artifact provenance/exposure remain open and RPC stays disabled. |
| 52 | **ACCEPT WITH CONDITIONS** | Global-epoch recovery is consistent; fault testing remains open. |
| 53 | **ACCEPT WITH CONDITIONS** | Bulk integrity and traceability are specified; TLC, crypto interoperability, fuzz, and fault tests remain open. |
| 54 | **ACCEPT WITH CONDITIONS** | Buffer paths remain machine-tested candidates and the conservative kernel-extension gate is retained. |
| 55 | **ACCEPT WITH CONDITIONS** | Historical evidence is directly scoped; matched HaloFPX measurements remain open. |
| 56 | **ACCEPT WITH CONDITIONS** | Preserve the bounded porting map and complete machine experiments before implementation promotion. |
| 57 | **ACCEPT WITH CONDITIONS** | Encoding is unambiguous; independent golden-vector reproduction remains open. |
| 58 | **ACCEPT WITH CONDITIONS** | Rank-local/all-ready semantics and disposable mutation boundaries are retained; fault tests remain open. |
| 59 | **ACCEPT WITH CONDITIONS** | Mode-aware recovery is documented; filesystem crash evidence remains open. |
| 60 | **ACCEPT WITH CONDITIONS** | Object framing and implementation pointers are repaired; integration tests remain open. |
| 61 | **ACCEPT WITH CONDITIONS** | Research split is explicit; exact continuation remains machine/model-specific and unproven. |
| 62 | **ACCEPT WITH CONDITIONS** | Pins and safe fault isolation are documented; target-machine kernel/filesystem tests remain open. |
| 63 | **ACCEPT WITH CONDITIONS** | Durability contract is reconciled; TLA+/TLC and crash/filesystem matrices remain open. |
| 64 | **ACCEPT WITH CONDITIONS** | Absence/privacy claims are scoped; principal, quota, encryption, and deletion decisions remain open. |
| 65 | **ACCEPT WITH CONDITIONS** | Destructive controls and provenance are documented; endurance/GC/privacy experiments remain open. |
| 66 | **ACCEPT WITH CONDITIONS** | Canonical health routes and negotiated streaming extension are documented; SDK compatibility corpus remains open. |
| 67 | **ACCEPT WITH CONDITIONS** | Per-field authority constrains overrides; schema and negative-policy tests remain open. |
| 68 | **ACCEPT WITH CONDITIONS** | Retain explicit rank ownership/fallback limits and validate lifecycle/admission state machines. |
| 69 | **ACCEPT WITH CONDITIONS** | Metadata, health routes, and fault isolation are repaired; observability experiments remain open. |
| 70 | **ACCEPT WITH CONDITIONS** | Cutover phases and safe boot/fault boundaries are explicit; deployment rehearsal remains open. |
| 71 | **ACCEPT WITH SECURITY GATE** | Source authority and bulk-integrity contract are repaired; deployed RPC proof, threat review, and abuse tests remain open. |
| 72 | **ACCEPT WITH CONDITIONS** | Runtime/data cutover and rollback phases are explicit; state migration and rollback rehearsal remain open. |

## New research prompts and acceptance questions

1. **Global epoch contract:** What exact state survives a rail failure, which operations are committed, and can a model checker plus two-rail fault suite prove no old-epoch chunk, credit, nonce, ACK, or rank state is accepted?
2. **Wire requirement trace:** Can one generated table map every Section 49 identity/order/cancellation/observability requirement to a negotiated connection property, v1 header field, authenticated upper-layer descriptor, metric, and negative test?
3. **Transport security profile:** What is the smallest audited record protection that authenticates every control and bulk record on direct USB4 while preserving useful performance, and what explicit threat acceptance is required to omit confidentiality?
4. **Durability-mode filesystem model:** For each target filesystem, mount, NVMe write-cache setting, and HaloKV mode, what failure points can lose/reorder data, and which flush/rename/directory-sync sequence makes the advertised guarantee true?
5. **Canonical identity encoding:** Can two independent implementations reproduce byte-identical compatibility manifests and object IDs for all boundary/Unicode/unknown-field cases, with an unambiguous domain-separated frame?
6. **Safe fault harness:** Can every cable, cgroup OOM, ENOSPC, EIO, corruption, process-kill, and power-loss case resolve a disposable exact target, refuse production paths, preserve out-of-band recovery, and emit a cleanup/evidence receipt?
7. **Field authority schema:** Which configuration authority owns every listener, identity, secret, artifact, plan, cache, durability, and telemetry field, and which override sources are explicitly prohibited for each?
8. **Runtime cutover proof:** Can a release rehearsal independently verify installed candidate, running process, loaded model/plan, listener/API, rank topology, migrated state, traffic commit, and rollback preimage?
9. **API compatibility corpus:** Which pinned OpenAI/SDK fixtures define the advertised subset, canonical health routes, SSE event grammar, tool/structured-output shapes, and retry/idempotency semantics under rank/link failures?
10. **Cache privacy and deletion:** What authenticated principal model, sharing classes, timing leakage budget, encryption/key-destruction policy, backup/export treatment, and physical-sanitization promise can be validated on the actual deployment?
11. **Evidence promotion validator:** Can CI reject `[MEASURED]` without a raw artifact/environment manifest and reject `[VERIFIED]` when its source URL/commit/path is unavailable?
12. **Manifest/schema validator:** The required permissive-core `section.yaml` contract is now enforced, including content-matched source/open-question counts. A future extension may machine-validate the prose research split without forcing one representation on deliberate per-section extensions.

## Acceptance gate

The original P1 transport, durability, source-authority, configuration-authority, and cutover findings are resolved at the design/documentation layer. The safe schema-validation P2 is also closed: focused valid/invalid self-tests pass and the stricter validator reports 86 complete, 86 schema-valid, and zero invalid sections. Promotion from research baseline to integrated implementation authority still requires the explicitly open wire conformance/fuzz/fault tests, TLA+/TLC gates, cache crash/corruption/privacy tests, API/operations compatibility tests, deployed RPC provenance/exposure proof, deployment/rollback rehearsal, and matched two-node machine experiments with retained raw evidence. Re-run structure/schema, generated-manifest, source-pin, local-link, claim-label, safety-boundary, and cross-section contract validators before that promotion.
