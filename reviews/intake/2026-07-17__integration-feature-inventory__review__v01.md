# Intake review: ROCmFPX/CachyLLama integration and feature inventory

- Review date: 2026-07-17
- Reviewer: Codex research track 11/13
- Intake packages:
  - `ROCmFPX-CachyLLama-Integration-Wiki`
  - `llama-ai-cachyllama-feature-inventory`
- Overall disposition: **REVISE before promotion**
- Scope: static, read-only evidence audit. No imported script or command was executed, no donor code was ported, and no canonical Wiki page was changed.

## Executive verdict

Both packages are valuable candidate research. Their immutable commit pins, source permalinks, internal checksums, explicit provenance gaps, and preference for extending ROCmFPX's stronger cache transaction base are materially better than a broad CachyLLama transplant. They should remain preserved under `sources/imports/` and be used to seed a governed implementation backlog.

They are not ready to become canonical decisions or implementation instructions. The integration package calls a still-proposed posture "approved," its ADRs and cache format have no maintainer approval or machine proof, and it assumes ROCmFPX should be the canonical fork despite the existing Section 15 recommendation to reconstruct on real llama.cpp ancestry. The feature package is static-only but assigns `M3` to 26 features and calls donor-oriented examples executable/directly consumable. Those labels can be mistaken for HaloFPX validation. The target and upstream branches also advanced after the assessed snapshots, so all port work requires a new delta audit.

Recommended action: accept the packages as **candidate evidence and planning inputs**, revise their status/maturity and operational wording, reconcile the repository-authority decision, then validate each selected capability through exact commit provenance, target-native tests, two-machine experiments where applicable, and license review.

## Review basis and integrity checks

The review used the project `AGENTS.md`, root `README.md`, `PROJECT_GOAL.md`, Wiki governance, relevant category material, Sections 11, 13, 14, 15, 16, 56, and 63, `references/agent-harness.md`, and the Agent Harness review guidance. Candidate claims were compared with the project's existing source and live-evidence boundaries rather than treated as authoritative on arrival.

Read-only integrity checks passed:

| Check | Result |
|---|---|
| Integration archive/import receipt | Preserved archive; receipt records SHA-256 `3802e84...` and 47/47 files. |
| Feature archive/import receipt | Preserved archive; receipt records SHA-256 `48a3dcc...` and 68/68 files. |
| Integration internal manifest | 46 entries checked; zero missing/mismatched files. |
| Feature internal manifest | 67 entries checked; zero missing/mismatched files. |
| Structured feature data | JSON/CSV parsed; 120 features, 118 evidence records, 245 links. |
| Disposition totals | 68 `RETAIN`, 39 `REDESIGN`, 13 `REJECT`; summary and structured data agree. |
| Imported execution | None. Static inspection only. |

The two packages consistently pin:

- ROCmFPX `a5605a72768c6562241b248e268e33dc92787394`;
- CachyLLama `6be745998f568e379ea197fcf827baec73ff9940`;
- llama-ai `1017f3dfdce3ca2b06aa9007b23295db3bb35722`.

The integration package additionally pins llama.cpp `86d86ed4396b4130922f7b9af26e3d9fc11a591b`. These are valid immutable research snapshots, not current branch heads. A read-only remote reconciliation on 2026-07-17 found ROCmFPX `main` at `61f2f2d7bc4955e9bca821095ef69125837133b5` and llama.cpp `master` at `6bdd77f13cf11b264b4231d320afc404f48d576e`; CachyLLama and llama-ai remained at the assessed pins. ROCmFPX's one new commit changes 14 paths and adds a material TurboQuant full-cache flash-attention staging fix, so it is relevant to Section 13 qualification. Current CachyLLama divergence is 53 commits ahead and 132 behind upstream, versus the dated 53/125 comparison. This is drift, not evidence that the packages are wrong, but it blocks direct application of the proposed port plan without a range/delta review.

## Priority findings

### P0 — The package confuses the external ROCmFPX source with the future writable HaloFPX fork

The candidate makes `charlie12345/ROCmFPX` itself the canonical writable integration repository (`Home.md:15-16`, `Repository-and-Provenance.md:17-22`, `Git-Topology.md:31`, `ADR-001-ROCmFPX-Is-Canonical.md:19-28`). `PROJECT_GOAL.md:9-17` requires a project-controlled fork *from* ROCmFPX, while Section 86 still leaves the exact implementation repository/hosting authority unresolved. Following the package literally would direct integration PRs and release authority to an external donor repository.

**Required action:** reject the current topology wording. If the ROCmFPX-base option is selected, create a user-owned HaloFPX fork as writable `origin`; keep `charlie12345/ROCmFPX` as a named source remote; record hosting, branch protection, release, and named human decision authority in `HLX-ADR-*` governance.

### P0 — The proposed normal upstream merge is invalid for unrelated histories

The candidate calls for `git merge --no-ff --no-commit upstream/master` (`Git-Topology.md:37-44`, `Upstream-Synchronization.md:66-73`, `templates/SYNC-RECORD.md:12-17`, `checklists/Upstream-Sync.md:3-13`). ROCmFPX and llama.cpp have no merge base. A normal merge fails; forcing unrelated histories would create a two-root graph without establishing file-level provenance.

**Required action:** reject the procedure. Use Section 15's ancestry-normalization/semantic-port strategy, or approve a one-time reconstruction with file/commit provenance, generated artifacts, `range-diff`/patch-id evidence, build gates, and rollback.

### P1 — Repository authority is unresolved, so the core integration ADR cannot be promoted

The candidate says "Use ROCmFPX as the canonical fork" and forbids CachyLLama as a merge parent (`Executive-Decision.md:15-29`). This aligns with `PROJECT_GOAL.md`'s desired ROCmFPX base and is a coherent engineering posture. It conflicts with the current canonical Section 15 recommendation to build on real llama.cpp ancestry and replay ROCmFPX as a provenance-recorded port series (`wiki/.../15_.../README.md:23-29`). ROCmFPX has no merge base with pinned llama.cpp, making the choice consequential for long-term synchronization, attribution, generated artifacts, bisectability, and conflict cost.

**Required action:** create or update one maintainer-approved repository-authority ADR comparing at least:

1. ROCmFPX as canonical with explicit upstream import/reconciliation;
2. llama.cpp ancestry with ROCmFPX patch lanes replayed;
3. one-time ancestry reconstruction followed by normal upstream sync.

Measure two representative sync rehearsals, preserve `range-diff`/patch ledgers and generated-file provenance, and define an exit criterion. Until then, mark the candidate decision `PROPOSED`, not approved.

### P1 — GitHub commit visibility is incorrectly treated as ROCmFPX ancestry/equivalence

The candidate treats upstream commit `0bbc87b163ff7826656b1024dac5703e3f2bd6b6` as present/no-import-required in ROCmFPX (`Source-Register.md:215-220`, `Capability-Decision-Matrix.md:42-44,50-59`, `Upstream-Synchronization.md:79-83`, `checklists/Upstream-Sync.md:7`). GitHub exposes the commit object through repository URLs, but graph checks show no ancestry, and the ROCmFPX Vulkan file lacks the cited upstream `shader_core_count` submission-threshold behavior.

**Required action:** reject that conclusion. Classify it as unresolved until stable patch-ID and semantic content comparison prove equivalence, followed by target build/test evidence. Repository URL resolution is not ancestry proof.

### P1 — “Executable” examples mix donor, parent, current-target, and proposed interfaces

`18-API-Examples.md:3` says all examples are executable or directly consumable. The same page then presents donor/parent endpoints and flags such as `/slots`, `/models/load`, `/models/unload`, `/models/sse`, `/v1/stream`, `/v1/streams/lookup`, `/expert-stats`, and `--cache-ssd*` (`18-API-Examples.md:34-92`), alongside ROCmFPX's current `--cache-disk` example (`:94-100`) and a proposed persistent configuration (`:102-113`). Static source registration does not prove those endpoints coexist in the ROCmFPX target or that the scripts are safe runbooks.

**Required action:** classify every example as `donor behavior probe`, `parent deployment example`, `ROCmFPX-a560 current`, or `proposed/not implemented`; add the exact executable commit and expected response schema; default all mutation/load/unload tests to disposable local instances. Reject the current bundle as a HaloFPX operational runbook.

The operational risk is concrete: launch examples omit authentication; slot/model/stream/admin requests omit credentials; slot 0 is saved, restored, and erased unconditionally; model load is followed by immediate unload without readiness/drain; stream-resume can block without a timeout; placeholder API keys default to `change-me`; and `examples/08-cachyllama-cache-flags.sh:12-19` sets `--cache-ssd-max-cold 0`, which is unlimited at the pinned donor. Quarantine these as forensic donor fixtures until destructive actions, authorization, bounded quotas, confirmations, timeouts, readiness, drain, rollback, and disposable targets are explicit.

### P1 — The port plan omits the defining dual-node/rank-local contract

`22-ROCmFPX-Porting-Plan.md:21-34,59-86,170-182` does not define logical-rank ownership, global checkpoint descriptors, distributed prepare/commit, rank-loss fencing, mixed-generation rejection, or separately compatible single-node fallback. "Two server instances sharing a configured root" is not the project's rank-local NVMe architecture. The proposed compatibility manifest also omits exact topology epoch, world size, rank/shard ownership, plan digest, and producer/runtime identity.

**Required action:** defer the port plan until it implements Sections 57/58: rank-local immutable objects, bounded all-rank readiness descriptors, one coordinator-visible generation commit, whole-restore miss if any required rank fails, and an explicit single-node compatibility/fallback path.

### P1 — Confirmed slot lifecycle defects are missing from the feature risk model

The feature inventory describes user caps and same-user affinity as implemented behavior with general race caveats (`07-Slot-Affinity-and-User-Isolation.md:75-93,117-135`). Canonical Section 14 found concrete defects at the exact pin: an HTTP fast path reads a moved-from task vector, `release()` clears `user_id_` before decrementing per-user counts, and prompt-similarity selection can expose residual slot state across users. These are cross-tenant correctness/security risks, not merely future hardening opportunities.

**Required action:** revise F-039/F-041/F-047 and add a critical contradiction. Reject donor user isolation, affinity, and concurrency accounting as implementation donors; derive ownership from authenticated context and test lifecycle ordering and residual-state isolation.

### P1 — The inventory does not separate inherited llama.cpp features from CachyLLama changes

The capability totals include standard APIs, TLS, embeddings, LoRA, slot APIs, and other inherited behavior without pinning the fork's merged upstream parent `92366df30d4eaa4b85139b5fd694360237731b19` or classifying the canonical 56-path delta. This overstates what CachyLLama contributed and weakens provenance and port decisions.

**Required action:** add `origin = inherited_upstream | cachyllama_delta | llama-ai | rocmfpx` to every feature/evidence row, plus the exact introduction/equivalence evidence. Retain the inventory as capability discovery, not a donor patch inventory, until this is complete.

### P1 — Static repository maturity is conflated with HaloFPX readiness

The inventory defines `M3` as an integrated path with focused tests or repeated operational evidence but explicitly states no binary was built or run (`00-Scope-and-Pins.md:50-55`). It nevertheless assigns `M3` to 26 features (`17-Maturity-and-Risk-Register.md:10`), including ROCmFPX baseline features and "published local validation" (`14-ROCmFPX-Target-Baseline.md:116-133`). Canonical Section 13 is explicit that repository benchmark tables and passed statements are not HaloFPX measured evidence and that the live cluster runs a different predecessor (`wiki/.../13_.../README.md:20-26`).

**Required action:** split maturity into at least `source integration maturity`, `upstream/donor test evidence`, and `HaloFPX validation state`. Retain source-level M3 only where exact tests exist, but label every package result `UNMEASURED ON HALOFPX` until retained raw evidence proves otherwise. Do not use F-110 as release-readiness evidence.

### P1 — Port plan is snapshot-correct but stale against current ROCmFPX and llama.cpp heads

`22-ROCmFPX-Porting-Plan.md` and all target overlap conclusions are based on ROCmFPX `a5605a7...`. Both ROCmFPX and llama.cpp advanced by the live check above. The integration package's empty cherry-pick roster is appropriately cautious, but no exact feature-introduction SHA, current upstream-equivalence/patch-id classification, or new-head conflict audit is complete (`evidence/commit-lock.json:10-14`).

**Required action:** preserve the assessed snapshots; add a new immutable delta intake from `a5605a7...` to the chosen ROCmFPX target and from `86d86ed...` to the chosen llama.cpp anchor. Re-run source ownership, API, cache ABI/state codec, test, generated-file, and license comparisons before selecting any patch lane.

### P1 — Proposed cache format competes with the HaloKV architecture and lacks distributed identity

`Cache-Format-Versioning.md:15-18` and ADR-003 declare a new canonical ROCmFPX Context Store v1. Its bounded manifest, strong model-set identity, component digests, corruption-as-miss behavior, owner-only namespaces, and no in-place migration are useful. But it competes with Section 57's deterministic-CBOR compatibility fingerprint, Section 59's immutable page/segment/prefix-DAG design, and Section 63's generation-manifest commit protocol. Its identity omits execution plan, logical rank, shard ownership, world size, and distributed commit generation required by Sections 57/58. Its transaction relies on file/directory sync and atomic directory rename (`Cache-Format-Versioning.md:86-98`) without proof for the exact supported filesystems and operating systems.

**Required action:** reject ADR-003 as the canonical HaloKV format. It may survive as a named experimental whole-checkpoint prototype only after an approved `HLX-ADR-*` reconciles it with Sections 57/59/63. Specify distributed ownership/topology, state codecs, byte ordering, canonical serialization, platform storage semantics, concurrency, quota, migration, and rollback; validate every crash point on disposable stores.

### P1 — License strategy is cautious but not a legal conclusion or a complete clean-room procedure

The packages correctly distinguish MIT engine repositories from the GPL-3.0-or-later llama-ai parent, keep the initial direct cherry-pick roster empty, and demand per-file/commit provenance (`Executive-Decision.md:21-29`; `01-Executive-Inventory.md:45`). That is a sound gate. Statements that clean-room reimplementation is "required for GPL-parent behavior" (`Executive-Decision.md:26`) are project policy, not verified legal advice. Reviewers have already inspected GPL sources, so merely rewriting behavior does not establish an independently designed clean-room record.

**Required action:** obtain maintainer/legal approval for the license boundary. For any clean-room lane, preserve a source-independent behavioral specification, identify separated spec and implementation roles, record exactly what each role viewed, require independent tests/golden behavior rather than copied structure, and retain authorship/notices for any MIT-derived port. Treat parent documentation's separate content license independently from engine code.

### P2 — Candidate status language contradicts itself

`Executive-Decision.md:3-4` says both "Approved integration posture" and `status: Proposed for maintainer approval`; its approval record says maintainers still must approve the role model, cache format, empty cherry-pick roster, GPL boundary, dependency graph, and thresholds (`:65-73`). Candidate ADRs and the cache contract are likewise proposed.

**Required action:** replace `Approved` with `Proposed` everywhere unless an actual decision record, approver, date, scope, and supersession/rollback path exist.

The local `ADR-001` through `ADR-004` namespace is not project decision authority. Section 86 requires `HLX-ADR-*`, named human authority, evidence, rollback, and a reconsideration trigger. Convert useful material into proposals only after canonical ADR location and authority are approved.

### P2 — Feature dispositions are useful hypotheses, not acceptance decisions

The 120-row matrix is well structured and evidence-linked, and its recommendation to extend ROCmFPX's existing cache rather than transplant CachyLLama is strongly supported. Yet `RETAIN`, `REDESIGN`, and `REJECT` can read as project approvals. Several decisions depend on threat model, product API, portability, model state, or benchmark evidence not yet fixed.

**Required action:** rename package-level decisions to `candidate_disposition` and add `decision_owner`, `decision_record`, `validation_state`, `target_commit`, and `superseded_by`. Promote only individually reviewed rows.

### P2 — Tenant hints must never become authorization or cache ownership

The examples correctly warn that request user fields are hints, not credentials (`18-API-Examples.md:3`), and the feature inventory rejects raw body-provided identity as authentication. The proposed cache architecture must carry that rule into its key derivation, quotas, scheduling, logs, purge, and administrative APIs.

**Required action:** derive tenant/cache scope only from authenticated middleware, bind it cryptographically or through an opaque server-side scope ID, and test cross-tenant collision, enumeration, restore, purge, quota, and timing isolation. HMAC directory names reduce disclosure but do not replace authorization or encryption at rest.

### P2 — No implementation patch exists, so no code-level integration claim is reviewable

The integration package explicitly supplies plans, gates, ADRs, and format proposals rather than a patch. This is appropriate for intake but means interface fit, ABI stability, compile behavior, rollback, and regression risk are all unproven.

**Required action:** implement the smallest provider seam first behind default-off flags, retain the current ROCmFPX adapter, and land one bisectable capability per lane with unit, corruption, restart, concurrency, tenant, model-mismatch, target/draft/MTP-state, and rollback tests.

### P2 — Configuration validators do not enforce the prose security contract

`19-Configuration-Schemas.md:36-46` requires atomic commit, owner-only modes, authenticated tenant binding, corruption quarantine, and bounded startup. In `schemas/rocmfpx-persistent-cache-port-plan.schema.json`, several of those controls remain optional and a configuration omitting them validates. The proposed example is therefore syntactically valid without proving a safe product configuration. The Cachy schema also mishandles documented zero sentinels/value domains.

**Required action:** make security invariants schema-required or conditionally required, add negative fixtures, align exact pinned defaults/sentinels, and label `examples/10-rocmfpx-proposed-persistent-config.json` non-executable until a pinned parser exists.

### P2 — Acceptance thresholds and fault procedures lack project authority and safety gates

Candidate material introduces percentage regressions, 1%/10% traffic, and release-cycle durations (`Acceptance-Criteria.md:44-51,88-104`, `Operations-Runbook.md:31-39`, `Integration-Order.md:101-110`, `Feature-Flags.md:67-77`) while Sections 82/86 leave them unratified. Fault checklists omit Section 63's resolved disposable scratch target/store UUID, isolated service, ceilings, recovery access, stop conditions, and explicit authorization.

**Required action:** defer numbers to owned Section 84 experiment/decision cards. Reject fault procedures for operational use until the disposable-target boundary and rank/single-node fallback are explicit.

### P2 — Package provenance does not itself license reuse of package-authored material

Exact source permalinks, commit/blob IDs, and internal manifests are strong. The integration package does not include a license for its own prose, diagrams, CSS, templates, and examples, and it does not retain every primary-source snapshot or exact retrieval timestamp despite suggesting Wiki copying.

**Required action:** preserve and cite the intake as candidate evidence, but defer copying package-authored content until authorship/license is known. Add origin, exact retrieval time, authority tier, limitations, and retained evidence where promotion requires it.

### P2 — A narrow source-level state adapter is feasible, but the APIs are not ABI-equivalent

Live source reconciliation found both CachyLLama and ROCmFPX expose `llama_state_seq_{get_size,get_data,set_data}_ext` with matching flag values, which partially resolves the source-side question of whether a narrow semantic adapter seam exists. The surrounding interfaces diverge: CachyLLama adds attention-only removal and expert APIs, while ROCmFPX adds storage-object initialization, clone, free, size, and extended-storage operations. This supports a manual adapter, not a header/source transplant or ABI-compatibility claim.

**Required action:** record the exact shared and repository-only symbols at the selected target commits; design against ROCmFPX's storage API; prove state equivalence with golden restore/recomputation tests for every supported model state.

### P2 — The llama-ai checkpoint option defect is at the wrapper boundary, not the CachyLLama engine

The pinned llama-ai service passes `--checkpoint-every-n-tokens` to `llama-run.sh`, but the wrapper parser accepts `--checkpoint-min-step` and rejects the former as unknown. The underlying CachyLLama argument parser still accepts both flags. Any canonical contradiction record should therefore describe a stale/broken wrapper-to-engine interface, not an engine capability removal.

**Required action:** keep the GPL wrapper as a policy donor, test the exact CLI contract independently, and do not copy its `eval`-assembled arguments, hard-coded paths, or force-kill behavior into the MIT product layer.

## Accept / revise / defer / reject matrix

| Artifact or claim group | Disposition | Rationale and promotion condition |
|---|---|---|
| Preserved archives, receipts, manifests, checksums | **ACCEPT** | Provenance-preserving intake is sound; keep immutable. |
| Exact snapshot pins and permalinks | **ACCEPT** | Valid evidence for the assessed snapshots; never relabel as current head. |
| Integration `Source-Register.md` and commit lock | **ACCEPT with revision** | Strong traceability; fill listed gaps and add current-head delta records. |
| Feature inventory JSON/CSV and evidence links | **ACCEPT as candidate dataset** | Structurally consistent and useful for queries; add decision/validation ownership fields. |
| Extend ROCmFPX cache rather than transplant donor storage | **ACCEPT as recommendation** | Preserves stronger target atomic-pair/failure behavior; still needs target-head verification. |
| Corruption/mismatch causes miss or recomputation | **ACCEPT as requirement** | Matches `PROJECT_GOAL.md` and Sections 56/63; test every mandatory state stream. |
| Donor final-name writes, split lifecycle maps, body user IDs, raw IDs/log risks | **ACCEPT as candidate findings** | Strong source-based concerns; verify exact line/commit during per-capability promotion. |
| Provider-first seam, default-off flags, independent rollback | **ACCEPT as design direction** | Reversible and bisectable; interface remains unimplemented. |
| Integration executive decision and local ADR set | **REJECT for direct promotion** | Wrong authority/identifier namespace; repository role and maintainer approval unresolved. |
| External ROCmFPX repository as writable canonical project repo | **REJECT** | The project needs its own fork/authority; external source remains a remote/donor. |
| ROCmFPX codebase as canonical source base | **DEFER** | Aligns user goal but conflicts with canonical Section 15 ancestry recommendation. |
| Direct donor cherry-picks | **DEFER** | Empty roster is correct until introduction commits, patch-id overlap, and provenance are locked. |
| ROCmFPX Context Store v1 / ADR-003 | **REJECT as canonical; defer as prototype** | Conflicts with Sections 57/59/63, omits distributed identity, and lacks portability/crash proof. |
| Donor-format offline importer | **DEFER** | Only after format ownership, threat model, sandbox, fuzzing, license, and migration demand are approved. |
| Page-level SSD paging | **DEFER** | Correctly excluded from v1; dead/unlinked donor page-manager code is not product evidence. |
| Feature maturity labels | **REVISE** | Split source maturity from project validation; no HaloFPX M3 inference from static inspection. |
| `RETAIN` / `REDESIGN` / `REJECT` labels | **REVISE** | Keep as candidate dispositions until named decision owner and evidence gate approve them. |
| F-110 "published local validation" as project evidence | **REJECT** | Repository-reported evidence is not retained HaloFPX measurement. |
| API examples as a unified executable ROCmFPX runbook | **REJECT** | Interfaces come from different repositories/states; reclassify as scoped probes. |
| Unauthenticated/destructive example scripts and unlimited cold cache | **REJECT for operations** | Require auth, bounds, confirmations, disposal, readiness/drain, timeout, and rollback. |
| Normal merge of llama.cpp into unrelated-history ROCmFPX | **REJECT** | Fails without forced unrelated histories and does not establish provenance. |
| Upstream `0bbc87b...` classified present in ROCmFPX | **REJECT** | GitHub object visibility is not ancestry or semantic equivalence. |
| Raw request user IDs as tenant ownership/authentication | **REJECT** | Hints are untrusted; derive ownership from authenticated server context. |
| Broad merge of CachyLLama or llama-ai into ROCmFPX | **REJECT** | Provenance, GPL boundary, architectural overwrite, and rollback risks. |
| Copying GPL parent scripts/code into MIT target | **REJECT pending explicit relicensing/legal decision** | Keep separate GPL deployment layer or use an approved independent implementation process. |
| Donor benchmarks as HaloFPX acceptance evidence | **REJECT** | Requires matched local measurements with raw data and environment metadata. |

## Implementation implications

If maintainers choose the ROCmFPX-canonical direction, the safe sequence is:

1. Freeze a new ROCmFPX target commit and llama.cpp comparison anchor; retain the old snapshots.
2. Create/identify the project-owned writable fork and distinguish `origin`, ROCmFPX source remote, and llama.cpp upstream.
3. Decide repository authority and ancestry normalization/upstream synchronization in one approved ADR.
4. Build an exact per-capability origin/provenance/overlap ledger; keep direct cherry-pick empty until each row passes.
5. Add an internal cache-provider interface without changing default behavior.
6. Reconcile any whole-checkpoint prototype with the HaloKV fingerprint/page/segment/generation architecture.
7. Specify identity for models, tokenizer, template, backend/state codecs, target/draft/MTP/speculative/recurrent state, runtime ABI, topology, rank/shard ownership, and critical layout parameters.
8. Implement manifest-first validation and transactional publication with platform-specific storage primitives; any uncertainty must cold-fallback.
9. Add authenticated opaque scope derivation, quotas, purge, metrics, and audit behavior.
10. Add restart, torn-write, bit-flip, mismatch, missing-state, rank/world mismatch, concurrent-writer, disk-full, path, tenant-crossing, and rollback tests.
11. Run matched single-node and dual-node experiments on disposable stores, retaining commands, commits, model hashes, environment, raw outputs, and failure logs.
12. Promote only individually accepted claims and decisions into canonical Wiki pages.

## Missing research questions

### Repository and upstream

- Which current ROCmFPX commit is the intended implementation base, and what changed since `a5605a7...` in server cache, state codecs, APIs, build scripts, generated sources, HIP/Vulkan, quant formats, and MTP?
- What changed in llama.cpp since `86d86ed...`, and which candidate donor features are now upstream-equivalent, obsolete, or conflicting?
- Can ROCmFPX history be reconstructed with trustworthy authorship and generated-file lineage, or is a permanent semantic port ledger cheaper and safer?
- What is the measured maintenance cost of two upstream-sync rehearsals for each repository-authority option?

### Cache correctness and format

- What exact byte-level state codecs are stable enough for cross-restart persistence, and which are deliberately invalidated on every build/commit?
- Which state streams are mandatory for attention KV, recurrent/Mamba, MLA, MoE routing, MTP, draft speculation, sampler, RNG, grammar, and adapters?
- Does template identity belong in the cache key for raw-token APIs, chat APIs, or both? How are canonicalization and tokenizer-added special tokens handled?
- What are the exact commit/visibility guarantees on ext4, btrfs, XFS, NTFS, APFS, and any deployed network filesystem after each sync/rename step?
- How are multiple server processes fenced? Are locks advisory, mandatory, lease-based, or single-writer by configuration?
- How are monotonic time, wall-clock changes, crash recovery, LRU, retention, and generation numbers reconciled?
- What data must be encrypted at rest, and who owns key rotation, deletion, backup, and incident response?
- Is donor-format migration worth its attack surface, or should the project cold-start the new store?

### API, tenancy, and operations

- Which endpoints and flags exist at the selected ROCmFPX target, which are donor-only, and which are product requirements rather than inherited interfaces?
- What authenticated principal and authorization policy controls save, restore, enumerate, purge, quota, and administrative cache operations?
- What are the single-node fallback and dual-rank ownership rules for persistent entries, including failed-rank fencing and recomputation?
- Which metrics are safe to expose without leaking prompt, model, or tenant identity, and which cardinality limits apply?

### Licensing and provenance

- For every selected MIT file/change, what exact introduction commit, author, license text, and upstream overlap apply?
- Which useful behaviors originate only in the GPL parent, which are generic requirements, and which can remain in a separate GPL deployment layer?
- What independent-specification and implementer-separation record is required for the project's clean-room policy?
- Does the parent documentation content license constrain copied diagrams, prose, schemas, examples, or test vectors?

### Validation

- Which source tests were actually run at each pin, on which OS/backend, and with what retained raw output?
- What golden recomputation comparison proves restored target/draft/MTP/recurrent state is semantically valid rather than merely parseable?
- What matched workload shows useful TTFT or recomputation savings without throughput, quality, privacy, durability, or tail-latency regression?
- What bounded stop conditions and cleanup receipts govern disk-full, corruption, crash, and power-loss experiments?

## Exact source pointers

Candidate integration package:

- `sources/imports/2026-07-17-further-research-wikis/extracted/ROCmFPX-CachyLLama-Integration-Wiki/ROCmFPX-CachyLLama-Integration-Wiki/Executive-Decision.md:15-29,65-73`
- `.../Cache-Format-Versioning.md:15-18,55-98,100-131`
- `.../Source-Register.md:17-31,58-95,99-212`
- `.../Git-Topology.md:31-44,109-115`
- `.../Upstream-Synchronization.md:66-83`
- `.../ADR-001-ROCmFPX-Is-Canonical.md:19-28`
- `.../ADR-003-New-Canonical-Cache-Format.md:19-28`
- `.../Capability-Decision-Matrix.md:42-59`
- `.../evidence/commit-lock.json:10-34`

Candidate feature package:

- `sources/imports/2026-07-17-further-research-wikis/extracted/llama-ai-cachyllama-feature-inventory/llama-ai-cachyllama-feature-inventory/00-Scope-and-Pins.md:5-12,50-55`
- `.../01-Executive-Inventory.md:30-45`
- `.../14-ROCmFPX-Target-Baseline.md:116-135`
- `.../07-Slot-Affinity-and-User-Isolation.md:75-93,117-135`
- `.../17-Maturity-and-Risk-Register.md:7-30`
- `.../18-API-Examples.md:1-113`
- `.../20-Evidence-Index.md:1-20`
- `.../21-Contradiction-Register.md`
- `.../22-ROCmFPX-Porting-Plan.md:1-24,165-190`
- `.../schemas/rocmfpx-persistent-cache-port-plan.schema.json:37-67,146-185`
- `.../examples/08-cachyllama-cache-flags.sh:7-19`
- `.../data/feature-inventory.json`
- `.../data/evidence.json`

Canonical comparison points:

- `PROJECT_GOAL.md:17-34,55-63`
- `wiki/HaloFPX_Wiki/03_Repository_and_Engineering/13_ROCmFPX_Feature_Kernel_Format_and_Patch_Inventory/README.md:20-29`
- `wiki/HaloFPX_Wiki/03_Repository_and_Engineering/15_Integration_Patch_Stack_and_Upstream_Synchronization_Strategy/README.md:23-29,61-77`
- `wiki/HaloFPX_Wiki/09_HaloKV_Persistent_Cache/56_CachyLLama_Cache_Semantics_and_Porting_Map/README.md:19-21`
- `wiki/HaloFPX_Wiki/09_HaloKV_Persistent_Cache/63_Durability_Modes_Atomic_Commit_Crash_Recovery_and_Corruption_Handling/README.md:15-27`

## Closeout

The packages pass preservation and candidate-evidence review but fail canonical-promotion review until the P1 items are resolved. No verified project material should be replaced silently. Preserve this review with the intake, convert the P1/P2 items into owned decisions/experiments, and promote only corrected, source-pinned, individually validated claims.
