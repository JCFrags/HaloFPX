---
type: implementation-plan
status: final-review-candidate
created: 2026-07-17
target: future HaloFPX integration fork
risk: high
approval_required: human
canonical_base: charlie12345/ROCmFPX
implementation_started: false
---

# ROCmFPX + llama-ai/CachyLLama integration fork plan

## Verdict

**Ready to execute authorized local/read-only Phase 0A only; not ready to create or push a remote fork, execute imported candidate tooling, import donor code, or implement persistence.** The canonical repository choice, architectural boundary, patch ordering, safety invariants, validation shape, and rollback strategy are sufficiently defined for local source freezing, static archaeology, and baseline characterization. Remote repository creation is Phase 0B and requires its own destination/governance authorization. Actual feature integration remains blocked by the explicit OPEN gates in this plan, especially the ROCmFPX base-pin refresh, capability-level donor provenance, license approval, and target-machine baseline evidence.

This document is a review candidate, not an implementation authorization. It deliberately makes no build, compatibility, or performance claim.

## Objective and scope

Fork `charlie12345/ROCmFPX` as the canonical product repository. Preserve ROCmFPX's AMD Strix Halo, ROCmFPX quantization, server, RPC, and MTP/speculative behavior while adding selected, license-compatible cache/session/lifecycle capabilities evidenced in `fewtarius/CachyLLama` and operational requirements evidenced in `fewtarius/llama-ai`.

The initial implementation scope is a stable single-node-capable fork with a separately gated persistent context store. Dual-node tensor/pipeline/expert transport optimization for the 200–230 GB model class starts only after the integration fork passes its stability gates.

Non-goals for the first release:

- no merge of CachyLLama or llama-ai branch history into the canonical branch;
- no replacement or reinterpretation of ROCmFPX's existing per-run `--cache-disk*` behavior;
- no direct server loading of CachyLLama `KVRC`, `KVSM`, or `KVPG` records;
- no page-level SSD paging;
- no fuzzy or cross-tenant continuation reuse;
- no public C ABI for new cache/hybrid primitives before the internal contract is proven;
- no copied GPL llama-ai source in the intended MIT engine tree;
- no distributed-performance work used to waive Phase 1 correctness.

## Governing evidence and precedence

Use this order when evidence conflicts:

1. exact source objects in the preserved repository clones;
2. canonical project Wiki pages and live-machine evidence;
3. imported research candidates under `sources/imports/2026-07-17-further-research-wikis/extracted/`;
4. repository documentation and claims;
5. recommendations in this plan.

Important inputs:

- `PROJECT_GOAL.md`;
- canonical Wiki sections 11–16 and 56–65;
- `sources/measurements/2026-07-17-strix-halo-live-inventory/`;
- imported integration, feature-inventory, cache-design, conformance, and validation Wikis;
- `references/agent-harness.md` and the canonical Agent Harness architecture.

Imported Wikis remain candidate evidence. Their ADR labels do not approve HaloFPX decisions. Route implementation as evidence/requirement -> candidate change -> review -> validation -> published commit -> use observation -> improvement review.

## Source freeze and pin ledger

No implementation branch may use a moving branch name as its source identity.

| Role | Frozen research pin | Current observation | Required decision |
|---|---|---|---|
| canonical ROCmFPX candidate | `a5605a72768c6562241b248e268e33dc92787394` | local clone `main` is `61f2f2d7bc4955e9bca821095ef69125837133b5`; the extra commit changes 14 CPU/CUDA/HIP/TurboQuant test paths | **OPEN-PIN-01:** qualify `61f2f2d...` or deliberately freeze `a5605a...`; never silently advance |
| operational requirements donor | `fewtarius/llama-ai@1017f3dfdce3ca2b06aa9007b23295db3bb35722` | local clone matches | reference only unless a GPL distribution decision is approved |
| MIT engine donor | `fewtarius/CachyLLama@6be745998f568e379ea197fcf827baec73ff9940` | local clone and llama-ai gitlink match | capability provenance must reach P3 before code import |
| CachyLLama merged upstream parent | `ggml-org/llama.cpp@92366df30d4eaa4b85139b5fd694360237731b19` | proven second parent of donor merge | comparison anchor for the donor delta |
| llama.cpp upstream reference | `<OPEN-UPSTREAM-SHA>` | canonical/imported research snapshots used different later pins | choose only for upstream surveillance; it does not replace the user-directed ROCmFPX base |
| deployed operational baseline | `charlie12345/rocmfp4-llama@4860505ee322091f0f61eba77d6ad49be88cf4ea` | running on nimo-1/nimo-2 at live capture | preserve as rollback/performance baseline, not source base |

`PROJECT_GOAL.md`, the local CachyLLama clone, and the llama-ai gitlink now agree on `6be745998f568e379ea197fcf827baec73ff9940`.

Phase 0A produces a local annotated baseline manifest containing full commit and tree IDs, recursive gitlinks, dirty state, patch IDs, license inventory, build inputs, cryptographic artifact hashes, and preserved bundles. Signing is added only after the project approves a signing authority and key-retention policy.

Phase 0B creates/configures a remote only after **OPEN-GOV-01** closes: the human owner must approve the destination owner/name, visibility, GitHub fork-network versus independent integration-repository form, permissions, default branch, branch protections, force-push/deletion policy, tag/signing policy, evidence-retention location, and who may create/push. Suggested remotes after that approval are `origin` (writable HaloFPX), `rocmfpx`, `cachyllama`, `llama-ai` (reference only), and `llamacpp` (reference/sync surveillance).

## Candidate test-asset intake gate

Imported Wiki scripts and fixtures are preserved candidate evidence, not trusted executable dependencies. Intake has three distinct stages:

1. **Pre-execution review:** record package/file hashes, provenance, and licenses; statically review scripts, fixtures, subprocess/network behavior, paths, dependencies, and generated outputs; approve a bounded isolated qualification environment.
2. **Isolated qualification execution:** run the still-untrusted candidate only in that environment against synthetic fixtures, with no credentials, production services, target-node access, writable model/cache roots, or unapproved network access; retain commands, environment, raw results, and generated-file hashes.
3. **Promotion:** a human/reviewer approves the exact accepted asset manifest and canonical destination. Only that promoted subset becomes an executable project dependency.

Stages 1–2 authorize isolated qualification only, never production, target-node, credentialed, model-store, cache-store, or live-service execution. Before Stage 3, the cross-fork conformance package supplies requirements/fixture ideas and qualification candidates only. **OPEN-TEST-01** owns the staged intake decision and accepted asset manifest.

## Canonical architecture

The architecture separates behavior that the donor currently couples:

| Boundary | Owns | Must not own |
|---|---|---|
| request/API normalizer | protocol fields and stable error envelopes | authorization decisions or cache paths |
| trusted scope resolver | authenticated tenant/session inputs -> opaque keyed scope | raw identity in filenames/logs; client hints as authority |
| scheduler policy | admission, concurrency, optional affinity scoring | state parsing or storage layout |
| checkpoint state codec | capture/restore of target, draft, speculative, recurrent, and declared continuation streams | retention or tenant routing |
| match policy | exact compatible prefix selection in an authorized scope | file I/O or context mutation |
| store provider | atomic put/get/list/quarantine/delete of opaque validated components | token similarity or scheduling |
| retention policy | DRAM/SSD residency, quotas, reserve, expiry, eviction | state compatibility |
| telemetry provider | bounded counters/events, optional expert observations | control-path behavior |

Provider modes are explicit:

| Mode | Meaning | Initial state |
|---|---|---|
| `off` | no disk context-store provider | supported |
| `ephemeral` | existing ROCmFPX per-run disk cache adapter | current/default semantics preserved |
| `persistent-read-only` | validate and read the new canonical format | first canary mode |
| `persistent-read-write` | transactional read/write | disabled until crash/fault gates pass |

The deployed RPC tensor cache remains a different subsystem, namespace, schema, quota, and threat boundary. It caches model tensors, not attention/session state. Its current FNV filename/direct-write/read-without-rehash design is not inherited into either HaloKV or the fork's future RPC cache.

## Feature-by-feature treatment

Treatment codes: **CP** exact cherry-pick, **MP** attributed manual port, **IF** adapt behind an interface, **CR** clean-room reimplementation, **NA** retain canonical/upstream behavior, **DEFER** out of the first release.

| Capability | Treatment | First-release decision |
|---|---|---|
| cross-restart conversation checkpoint persistence | IF + MP | add a separate persistent provider; keep existing per-run provider unchanged |
| target/draft/speculative paired state | NA + extend contract | retain ROCmFPX semantics and make component requirements explicit |
| hot/warm/cold retention and byte-bounded LRU | IF + MP/target-native implementation | adopt policy goals, not donor containers/native format |
| chunked I/O and readahead | MP; CP only after P3 provenance | correctness first; enable prefetch only after measured usefulness |
| durability/fsync policy | target-native redesign | strict default for committed persistence; any relaxed mode is visibly unsafe and independently tested |
| compatibility fingerprint | CR/target-native | strong canonical manifest and component digests; reject 64-bit FNV identity |
| persistent index recovery | CR/target-native | manifests/objects are authority; indexes rebuildable; startup bounded; malformed entries quarantined |
| system-prefix reuse | IF + MP | tenant-local by default; explicit/template-derived boundary; no heuristic marker scan |
| cross-conversation continuation | CR from safety contract | exact token-prefix and same authenticated scope only in v1; fuzzy matching deferred |
| destination sequence-ID remap | MP/retain semantic requirement | regression test required before hybrid/cold restore |
| hybrid attention/recurrent cleanup | internal IF + MP, CR fallback | per-architecture capability vector; no public ABI initially |
| sampler/grammar/RNG continuation | inventory then CR/MP | required streams must be declared; absent required state causes recomputation |
| tenant namespace | IF + target-native redesign | authority from trusted gateway/API key/JWT context; client `user` field is only a hint |
| per-tenant concurrency and HTTP 429 | MP after scope plumbing | one authoritative lifecycle; disabled by default until race/cancel tests pass |
| slot affinity | MP as optional scoring hint | separate lane; cannot create hard ownership or starvation |
| cache inspection/prune/invalidate | target-native | read-only inspect/validate first; destructive admin later with auth/dry-run |
| expert activation telemetry | IF + CR/MP after provenance | compile/runtime off; no placement dependency; overhead gate required |
| CPU ISA/profile behavior from llama-ai | CR | requirements-only reimplementation or separate GPL package |
| llama-ai runner, service, router, built-in tools, MCP proxy | requirements reference; DEFER/REJECT from core | no copied GPL scripts; unsafe tools stay outside the inference service |
| AMD Vulkan tuning already present | NA | assert ancestry/equivalence; do not port an older donor variant |
| broad HIP/Vulkan/kernel donor changes | DEFER pending symbol-level gap | retain ROCmFPX baseline; one narrow measured change per lane |
| donor cache-format migration | CR offline read-only importer only | server never auto-imports donor records |
| page-level SSD paging | DEFER | separate maturity/performance/security program |
| donor CLI aliases | DEFER | no silent equivalence between persistent and per-run semantics |

**Initial approved CP roster: empty.** A future CP requires exact introduction commit, dependency closure, upstream-equivalence classification, file/blob license, authorship, and a reviewer-approved P3 provenance record.

## License and provenance gate

Each capability/file progresses through:

- **P0 unidentified:** behavior claim only; no source import.
- **P1 repository classified:** repository license known; still no import.
- **P2 file/commit mapped:** exact introducing commits, parentage, paths, blobs, authors, license/SPDX, prerequisites, and upstream overlap recorded.
- **P3 approved:** treatment (CP/MP/IF/CR/NA/DEFER), attribution, notice change, reviewer, tests, and distribution consequence approved.

Rules:

1. MIT repository status is necessary but not sufficient for importing a mixed donor merge.
2. GPL-3.0-or-later llama-ai source and CC-BY-NC-SA documentation stay outside the intended MIT core unless the project owner explicitly changes the distribution decision.
3. GPL-derived behavior used in the MIT core requires an approved behavioral specification, provenance separation, and a clean-room implementation/review record. Record separate specification and implementation roles/contexts, prior-exposure attestations, permissible evidence, and the comparison-review method. An agent or person who inspected the GPL implementation is not silently reassigned as the clean-room implementer. The implementer must not copy or transform GPL source.
4. Every imported MIT unit preserves authorship/license and updates third-party notices.
5. Models, test corpora, Web UI artifacts, system packages, and runtime libraries receive separate license records.
6. Automated scanning supports but does not replace competent legal review.

## Cache safety invariants

These gates are non-negotiable:

1. Corrupt, partial, short, oversized, digest-failing, version-mismatched, model/tokenizer/template-mismatched, ABI-mismatched, backend/layout-mismatched, rank/topology-mismatched, or unauthorized state is a miss/recomputation; it is never accepted.
2. No context mutation occurs until all mandatory components validate.
3. Target, draft, speculative, recurrent, and any required sampler/RNG streams publish and restore as one logical transaction.
4. Write to unique staging paths, verify length/digest, flush as required, atomically publish, sync the containing directory, then update rebuildable indexes.
5. A failed incoming write cannot destroy a prior committed entry.
6. Raw tenant IDs and prompt text never become paths or routine metric labels.
7. Explicit scopes never fall back to anonymous or other-scope search.
8. Persistent files are untrusted input: bounded parsing, duplicate-key rejection, fixed relative component names, no symlink/path traversal/device-file acceptance, and allocation bounds are required.
9. Quotas account separately for committed data, staging headroom, quarantine, indexes, and filesystem reserve.
10. Distributed checkpoints are rank-local. All required ranks validate and stage before an all-ready commit; one-rank success never publishes a partial restore.
11. Losing optional draft/speculative state may fall back to target-only decoding only when the state capability contract explicitly permits it; otherwise recompute.
12. Previous binaries are never pointed at a newer persistent root unless reader compatibility is proven.

## API and state compatibility contract

- Capture the exact ROCmFPX baseline CLI help, defaults, HTTP routes, JSON/SSE response shapes, error codes, health/model endpoints, slot/admin behavior, and state APIs before refactoring.
- Existing flags and defaults keep their behavior with all new features off. New persistent modes use new unambiguous names.
- API conformance compares structure, error behavior, stream termination, token/logit fixtures, cancellation, and concurrency; nondeterministic fields use declared tolerances rather than byte equality.
- State compatibility is an explicit vector: model and shard digests, tokenizer, effective chat template, adapters, quantization/K/V types, context/RoPE parameters, runtime/build/state-codec IDs, backend/device layout, target/draft identities, required state components, rank/world size, partition plan, and security scope.
- Format evolution is reader-before-writer, versioned, and never in-place. Unsupported newer required features miss; supported older entries are read-only inputs to a newly written generation.
- The canonical persistent format is new. The imported proposal `.rocmfpx-context-store-v1` is a candidate contract, not approved implementation until its ADR/schema/fuzz plan is reviewed.

## Ordered patch stack

Every integration commit must be buildable, bisectable, default-off for new behavior, linked to provenance, and accompanied by the smallest relevant tests.

| Order | Lane | Deliverable | Exit gate / rollback marker |
|---:|---|---|---|
| L00A | local source/provenance freeze | read-only pins, bundles, license records, baseline manifest; no remote mutation | approved local source lock |
| L00B | test-asset intake | hashes/licenses/static review/isolated validation and accepted asset manifest | `OPEN-TEST-01` closed |
| L00C | remote repository governance | authorized destination/configuration only; no donor code | `OPEN-GOV-01` closed; `integration-base/*` |
| L01 | characterization contracts | current cache/API fixtures, failure taxonomy, metrics schema | baseline behavior captured; `cache-contract-v0` |
| L02 | state, scope, and format contracts | required stream capability vector, trusted/anonymous scope rules, compatibility and schema contract; no server persistence | `OPEN-STATE-01`, `OPEN-SCOPE-01`, and `OPEN-FMT-01` contract decisions approved |
| L03 | provider seam and baseline codec | neutral codec/store/match contracts, no-op/current-cache adapters, admitted baseline state capture/restore | feature-off equivalence; `cache-provider-seam-v0` |
| L04 | v1 reader, offline first | bounded metadata/component validation and quarantine; only synthetic/disposable offline input until later dependencies pass | fuzz/corruption gates; `cache-format-v1-reader` |
| L05 | disabled v1 writer, offline first | transactional staging/publication/recovery; no server/canary path | crash-point and ENOSPC gates; `cache-format-v1-writer-off` |
| L06 | trusted scope implementation | principal -> opaque scope through request, scheduler, and store; anonymous policy | isolation gates; `cache-scope-v1` |
| L07 | admitted state codecs | transformer baseline, then separately approved draft/spec/recurrent/sampler streams | per-mode cold-vs-restore equivalence; `state-codec-v1` |
| L08 | persistent provider/retention | read-only then read-write storage canary, quotas, reserve, LRU, optional prefetch; depends on L04-L07; lookup is opaque committed-ID/direct-session reference from a disposable test harness only | restart/quota/rollback/direct-ID-only gates; `persistent-cache-v1-canary` |
| L09 | exact match policy/importer | authorized exact-prefix matching; optional offline donor importer | no partial/cross-scope restore; `persistent-cache-v1-match` |
| L10 | system-prefix provider | explicit/template boundary and tenant-local namespace | golden template/security gates; `system-prefix-cache-v1` |
| L11 | additional hybrid state | add only per-architecture streams not admitted in L07 | per-architecture equivalence; `hybrid-restore-v1` |
| L12 | concurrency | authoritative counters, cancel/error release, 429 mapping | race/lifecycle gates; `tenant-concurrency-v1` |
| L13 | affinity | optional fair scoring hint | scheduler equivalence/fairness; `slot-affinity-v1` |
| L14 | independent optional lanes | telemetry, clean-room ops, and upstream/backend surveillance as separately owned changes | each lane's provenance/build/overhead gate |
| L15 | policy/compatibility | documentation, any aliases, release defaults | separate human ADR; `persistent-cache-v1-stable` |

L04 and L05 are offline/synthetic-only until L06 trusted scope and at least one L07 state codec pass their gates. They cannot enter a server, persistent canary, or production build path merely because their parsers/writers pass isolated tests. L08 proves storage by opaque committed ID or direct test-session reference only: it has no prefix discovery, system-prefix reuse, fuzzy lookup, cross-conversation lookup, or fallback search. Those behaviors remain impossible until L09/L10 land. L09/L10 cannot precede L06. Candidate branches merge dependent lanes in the order above with explicit merge boundaries. Private topic branches may rebase; reviewed candidates and releases do not. Conflicts are classified and reviewed by affected owners; no blanket ours/theirs policy. Rollback uses revert or provider flags, never destructive history rewriting.

## Build and test matrix

### Build identities

Every result records source/tree/patch IDs, dirty state, compiler/toolchain/package versions, kernel/firmware, CMake cache, environment allowlist, device identity, model/fixture hashes, executable hash, and command line. The two nodes' package/boot/swap skew must be normalized or explicitly controlled before comparison.

### Required matrix

| Layer | Required cases before stable |
|---|---|
| host CPU | Linux GCC and Clang; unit tests; parser sanitizers/fuzzing; feature off/on |
| nimo-1 HIP | clean `gfx1151` build, backend ops, ROCmFPX quant/reference, server smoke, MTP off/on as supported |
| nimo-2 HIP | same manifest and gates as nimo-1 |
| Vulkan | build and correctness on both nodes where retained as supported fallback/comparison |
| server/API | approved/promoted conformance fixtures only; the imported cross-fork package remains candidate until L00B; JSON/SSE/errors/cancel/health/models/slot behavior |
| state | target-only, target+draft, MTP/spec, supported hybrid/recurrent, adapter on/off, destination sequence remap |
| storage | empty/full/read-only root, short I/O, rename/sync/permission/EIO failures, truncated/corrupt/oversized/manipulated metadata, stale locks, concurrent processes |
| lifecycle | clean stop, SIGTERM, SIGKILL at each publication step, restart/reconcile, upgrade, downgrade, rollback |
| scope | same tenant, other tenant, anonymous, changed principal, conversation boundary, public admin prefix if supported |
| quality | deterministic token/logit equivalence and matched perplexity/output checks against cold recomputation |
| performance | feature-off overhead, cold, DRAM hit, SSD hit after restart, write latency/bytes, hit rate, p95/p99 under load, endurance counters |
| distributed regression | current RPC/MPTCP topology, rank/link loss, timeout/cancel, explicit single-node recovery; persistent distributed restore remains off until Phase 2 |

Numeric performance thresholds are **OPEN-ACCEPT-01**. Establish variance from matched baseline repetitions before approving thresholds; do not inherit donor percentages as HaloFPX facts. Correctness, integrity, isolation, provenance, and rollback gates are conjunctive and cannot be waived by a speedup.

## nimo-1 / nimo-2 rollout and rollback

The live service is an operational baseline and must not be overwritten in place.

1. Resolve nimo-1 storage headroom first. It had about 43 GiB free while its separate RPC tensor cache used about 112 GiB. Do not delete that cache without explicit authorization and retained evidence.
2. Build/test in isolated versioned worktrees and output directories. Stage exact binaries/configs by digest; preserve the deployed binary/config/service and model paths.
3. Use nimo-2's greater free space for initial non-deployed builds if needed, but require a native clean build/test receipt from both machines before release.
4. Run alternate-port, loopback/private-network canaries with the production service untouched. Start with all new cache features off, then the current ephemeral adapter, then a disposable persistent-read-only store.
5. During an approved maintenance window, drain requests and capture `/health`, active processes, service units, executables, commits, configs, model identity, MPTCP/subflows, disk/SMART, and logs.
6. Qualify single-node mode independently on each host. No two-node cutover occurs until each host can run the approved reduced fixture/model and revert to the deployed baseline.
7. Validate RPC protocol compatibility. If mixed versions are not proven compatible, stop both processes and switch the worker/coordinator as one coordinated maintenance transaction rather than running an unapproved mixed cluster.
8. Preserve role ownership: nimo-1 private RPC worker; nimo-2 coordinator/LAN API. Explicit device order, split, rank ownership, and failure semantics are configuration authority.
9. Canary persistent reads with disposable data; shadow/disabled writes next; bounded read-write only after crash and corruption gates. Persistent stores are versioned and never shared with the old binary.
10. Roll back by disabling persistent reads/writes, returning to `ephemeral`/`off`, then restoring the prior versioned service/binary. Preserve suspect stores read-only for forensics; do not delete or migrate during emergency recovery.
11. Verify rollback with cold correctness, health/models endpoints, RPC device visibility, both USB4/MPTCP rails, and the known deployed large-model readiness procedure. Performance comparison occurs only after correctness and matched environment checks.

## Acceptance gates

| Gate | Pass condition |
|---|---|
| G0A local source identity | canonical base candidate recorded; all exact objects/bundles available; valid clean worktrees; no remote mutation |
| G0B repository governance | destination/visibility/permissions/protections/signing/evidence authority approved before create/push |
| G0C test assets | imported executable assets hashed, licensed, statically reviewed, isolated, validated, and explicitly promoted |
| G1 provenance/license | every imported/reimplemented capability P3; notices/SBOM/distribution decision reviewed |
| G2 baseline | frozen ROCmFPX builds/tests on both nodes; API/cache/backend characterization retained |
| G3 feature-off | defaults, APIs, scheduling, current cache, ROCmFPX quant/MTP/backend behavior match approved baseline |
| G4 persistence correctness | deterministic cold-vs-restore equivalence; all mandatory streams transactional; mismatch/corruption recomputes |
| G5 crash/storage safety | fault matrix accepts no partial entry, preserves old committed data, and recovers cold |
| G6 isolation | no cross-scope enumerate/match/load/evict/metric leakage; authoritative admission lifecycle |
| G7 operations | quotas/reserve/metrics reconcile; disk-full behavior safe; inspect/quarantine/rollback rehearsed |
| G8 target matrix | both nimo hosts pass exact build/runtime matrix with retained evidence |
| G9 performance | human-approved thresholds derived from matched variance; SSD benefit and write/endurance cost measured |
| G10 release | immutable manifest/tag/artifacts, deployment receipt, rollback proof, and independent review accepted |

## Major risks and controls

| Risk | Control / rollback trigger |
|---|---|
| wrong or drifting base pin | freeze full SHA/tree; block on OPEN-PIN-01 |
| GPL contamination | P3 provenance and clean-room boundary; quarantine/revert on similarity concern |
| broad donor merge overwrites ROCmFPX safety work | donor branch never becomes a merge parent; empty initial CP roster |
| unstable/native donor format becomes ABI | new canonical versioned format; offline importer only |
| partial state restore changes output | all-or-nothing component validation; any divergence disables persistent restore |
| cross-tenant disclosure | authenticated opaque scope; exact isolation tests; violation is a security rollback |
| cache parser attack | bounded untrusted-input parser, fuzzing, safe-open/path rules; compile/provider kill switch |
| disk full/crash destroys valid state | stage-before-commit, reserve/quota, circuit breaker, fault injection |
| nimo-1 capacity exhaustion | no write enablement before capacity/reserve decision |
| backend/MTP regression | feature-off matrix and per-lane bisectability; revert owning lane |
| scheduler races/starvation | identity, concurrency, and affinity in separate lanes; disable affinity then cap |
| false performance conclusion | matched configs, raw evidence, variance-based thresholds, donor claims excluded |
| long-term fork burden | stable interfaces, small lanes, upstream surveillance, conflict/machine-hour ledger |

## Dependencies and explicit OPEN items

### Blocks before any donor code import

- **OPEN-PIN-01:** approve `a5605a...` versus current `61f2f2d...` after the TurboQuant/FA delta passes the selected baseline matrix.
- **OPEN-GOV-01:** approve remote owner/name, visibility, fork-network form, permissions, protections, signing/tag policy, evidence location, and create/push authority.
- **OPEN-TEST-01:** complete Stage 1 review before isolated qualification; then approve the exact promoted asset manifest after Stage 2 and before any project/test-matrix execution.
- **OPEN-PROV-01:** identify the exact introduction commits and dependency closure for every selected CachyLLama behavior; promote each source record to P3.
- **OPEN-LIC-01:** approve the product distribution/license model, file-level notices, and the GPL clean-room procedure.
- **OPEN-BASE-01:** build and characterize the selected ROCmFPX base on both target nodes from a complete manifest.
- **OPEN-API-01:** approve which ROCmFPX server/API surfaces are compatibility requirements and freeze their fixtures.

### Blocks before persistent writes

- **OPEN-FMT-01:** approve the canonical v1 schema, state capability vector, parser bounds, compatibility/invalidation, and migration policy.
- **OPEN-STATE-01:** inventory required mutable state for each admitted transformer, hybrid/recurrent, MTP/speculative, sampler, grammar, and RNG mode.
- **OPEN-SCOPE-01:** approve authentication/principal binding, anonymous policy, tenant/system/public-prefix sharing, and key rotation.
- **OPEN-STORAGE-01:** approve nimo-1 capacity, filesystem reserve, quota, durability mode, and disposable fault-test location.
- **OPEN-ACCEPT-01:** derive numeric performance/regression thresholds from matched baseline variance.

### Blocks before two-node optimization

- define process/rank/control/sampling/KV ownership for each execution mode;
- prove RPC/fabric protocol compatibility, cancellation, rank/link failure, and single-node recovery;
- measure single-rail, dual-rail, simultaneous bidirectional, CPU cost, and GPU-to-peer end-to-end behavior;
- decide whether USB4STREAM graduates from research while retaining TCP/MPTCP control/fallback;
- prove the selected 200–230 GB stored model plus runtime/KV/headroom fits the actual pair safely.

## Execution issue sequence

Create reviewable issues/ADRs in this order:

1. local base-pin refresh and source-bundle manifest (L00A; read-only);
2. candidate test-asset intake and promotion decision (L00B);
3. remote repository governance decision, then creation only if authorized (L00C);
4. target baseline build/API/cache characterization;
5. donor capability provenance and license dispositions;
6. state/scope/format contracts and threat model (L02);
7. provider seam and admitted baseline codec (L03);
8. offline reader/fuzz/quarantine (L04);
9. offline disabled writer/fault injection (L05);
10. trusted scope and admitted state-codec implementations (L06-L07);
11. nimo disposable persistent-store experiment;
12. bounded direct-ID-only storage canary and retention (L08; no discovery/fallback matching);
13. authorized exact-prefix matching, then optional L10-L14 capabilities one lane at a time;
14. two-node regression qualification;
15. Phase 1 release review;
16. only then open Phase 2 distributed optimization work.

## Further Internet research verdict

No additional Internet research is required to begin Phase 0A. The immediate blockers are local source archaeology, human decisions, exact builds, and target-machine experiments. New Internet research should be requested only if local history cannot establish a donor introduction/dependency/license record, or if an official current toolchain, kernel, security, or licensing source is needed immediately before the corresponding decision.

## Review instructions

An independent reviewer must return **accept**, **revise**, **defer**, or **reject**, with evidence for:

- compliance with the user-directed ROCmFPX canonical base;
- pin correctness and conflict handling;
- feature disposition completeness;
- license/provenance sufficiency;
- cache/state/security invariants;
- patch-lane dependency and bisectability;
- test/rollout/rollback realism on the measured hosts;
- missing assumptions or hidden implementation decisions;
- whether the readiness verdict is justified.

The plan must not approve itself. Any substantive revision increments the version and preserves this draft.
