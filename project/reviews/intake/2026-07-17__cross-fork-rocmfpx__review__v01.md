---
type: intake-review
title: Cross-fork conformance and ROCmFPX technical inventory intake review
status: complete
decision: revise
created: 2026-07-17
reviewed_packages:
  - cross-fork-llama-conformance-wiki
  - ROCmFPX-llama-wiki
canonical_wiki_modified: false
---

# Cross-fork conformance and ROCmFPX technical inventory intake review

## Executive disposition

This review evaluates two preserved intake packages as evidence and implementation proposals. It does not approve their claims merely because their internal manifests validate, and it makes no changes to the canonical Wiki.

| Package | Verdict | What may move forward | What must not be promoted yet |
| --- | --- | --- | --- |
| `cross-fork-llama-conformance-wiki` | **REVISE** | Its conformance principles, identity envelope, evidence-retention model, comparator taxonomy, expected-rejection concept, failure-first reporting, and RPC isolation rule are useful design inputs for canonical section 78. | The 175-case matrix, fixtures, harness, fork applicability labels, and any implied conformance result. No fork or hardware execution occurred. |
| `ROCmFPX-llama-wiki` | **REVISE BEFORE PROMOTION** | Its exact fork pin, recent commit/PR/path inventory, format and backend maps, CPU-oracle recommendation, and migration dependency map are useful candidate evidence. | RETAIN/REFRESH/RETIRE labels as approved decisions, whole-fork equivalence, ABI stability, migration-base selection, performance, correctness, or compatibility claims. |

The packages are strong research scaffolds, not qualification evidence. The immediate promotion path is selective: preserve exact source facts and explicitly labeled recommendations; correct pin, scope, fixture, applicability, and project-authority defects before converting the material into canonical claims or executable gates.

## Review basis and authority

The review used, in order:

1. Root project instructions and [`README.md`](../../README.md).
2. [`PROJECT_GOAL.md`](../../PROJECT_GOAL.md), canonical Wiki routing, and relevant canonical sections 13, 15, 30, 51, 57, 61, and 78.
3. [`references/agent-harness.md`](../../references/agent-harness.md) and the Agent Harness architecture/review authority it routes to.
4. Preserved package contents and manifests under `sources/imports/2026-07-17-further-research-wikis/extracted/`.
5. Exact primary-source commit/tree/compare data for the pinned repositories.

The Agent Harness review contract requires an explicit accept/revise/defer/reject disposition based on evidence, scope, risk, dependencies, and evaluations. Accordingly, internal package polish and self-validation are treated as structural evidence only.

## Package integrity and provenance

| Package | Preserved ZIP SHA-256 from import receipt | Manifest result independently recomputed | Interpretation |
| --- | --- | --- | --- |
| Cross-fork conformance | `060a61cfe5372248f26283d27cec6e22412986264362d000c3ee07af3dc1369c` | 129 records; zero size or digest mismatches | The extracted package matches its internal manifest. This does not validate research claims or fixtures. |
| ROCmFPX inventory | `14e5b07a3d8d80348bbe4bb8858b7d0c26474ce8c3d4e08f531b75718bd72b4f` | 87 records; zero size or digest mismatches | The extracted package matches its internal manifest. The manifest excludes its own control files and does not prove source conclusions. |

The imported scripts were inspected as text and were not executed.

## Exact pin audit

| Authority or package | llama.cpp pin | ROCmFPX pin | CachyLlama pin | Finding |
| --- | --- | --- | --- | --- |
| Canonical Wiki/research baseline | `788e07dc91d266ad3162a1ce9037665656269689` | `a5605a72768c6562241b248e268e33dc92787394` | `6be745998f568e379ea197fcf827baec73ff9940` | Current project research baseline. |
| Both reviewed packages | `86d86ed4396b4130922f7b9af26e3d9fc11a591b` | `a5605a72768c6562241b248e268e33dc92787394` | Cross-fork package uses `6be745998f568e379ea197fcf827baec73ff9940` as a candidate | Upstream pin is not the canonical baseline. |
| `PROJECT_GOAL.md` | Not the controlling pin in the goal text | ROCmFPX declared the canonical integration repository | `6be745998f62b1d34f32da1f2c8a503936d142cf` | The recorded Cachy SHA does not resolve in the referenced repository and conflicts with the valid canonical Wiki pin. |

Primary compare data shows `86d86ed...` is six commits ahead of and zero commits behind `788e07d...`. The intervening changes touch build/backend and test surfaces, including `ggml/CMakeLists.txt`, BLAS/OpenCL material, and `tests/test-backend-ops.cpp`. Therefore the packages cannot silently substitute `86d86ed...` for the canonical baseline even if the delta appears small.

**Required decision:** either regenerate/recheck the packages against `788e07d...`, or approve an explicit baseline-advance decision that describes the six-commit delta and updates all dependent evidence together.

## Findings by priority

### P0 — Cross-fork applicability labels manufacture false equivalence

The cross-fork matrix contains 175 cases. Independently counting its applicability fields found 154 cases marked `required` for all four participants: upstream, ROCmFPX, CachyLlama, and the integration target. Those implementations do not share identical feature surfaces. Cache persistence, MTP, custom tensor formats, selected server APIs, and RPC combinations cannot all be presumed universally applicable.

Each case needs a capability-derived disposition at each exact pin:

- `required` when the source contract exists and the case is valid;
- `expected-reject` when incompatibility or unsupported input is the contract;
- `not-applicable` when the capability does not exist;
- `open` when source inspection has not established the contract.

Equivalence must be asserted only after identity, applicability, and oracle class are established. Capability absence is not failure, and shared test names are not shared semantics.

### P0 — Fixture IDs are not independently materialized

The fixture manifest lists 161 IDs, but only 26 unique paths. Of 143 entries labeled `included`, 131 IDs are not independently addressable in their referenced text files. For example, 20 logical IDs point to one `fixtures/api/requests.jsonl` file containing 14 generic records. Hashing the coarse file locks the container, not the exact per-case payload.

Other materialization counts are 11 `recipe`, five `operator-supplied`, one `generated`, and one `download`; 13 fixture entries have null digests. These are acceptable states for a proposal only if the unresolved inputs are explicit. They are not an executable, reproducible fixture corpus.

Every fixture ID must identify one immutable object through a dedicated file, byte range, JSON pointer, JSONL record key, or deterministic generation recipe. The record must carry provenance, license, expected schema, digest, and model/tokenizer/template dependencies.

### P0 — The harness does not execute the proposed cross-fork contract

The package contains generic process, normalization, comparison, selection, and validation utilities. It does not contain participant adapters, build orchestration, observation collectors, case-to-command mappings, backend telemetry capture, state capture/restore drivers, or RPC topology orchestration. Its tests exercise generic timeout and comparator behavior rather than any fork.

The package itself correctly reports that no fork, model, GPU, cache, MTP, or RPC execution occurred and that no numerical tolerance was approved. The matrix and harness therefore remain design artifacts. They must not be described as implemented conformance or qualification coverage.

### P0 — State and cache cases under-specify the owned state

Canonical section 61 requires recovery reasoning across target-model state, draft-model state, speculative controller state, sampler/grammar/RNG state, and server stream/accounting state. The proposed cache and speculative cases do not yet define atomic manifests and ownership for all of those streams. A target KV match can coexist with a corrupted continuation.

Restore equivalence must validate the complete state bundle, its identity envelope, atomic publication, corruption-to-miss behavior, and rank-local ownership. Cross-build or cross-fork state import should default to `expected-reject` until a versioned compatibility contract is proven.

### P0 — The ROCmFPX migration base conflicts with project authority

`PROJECT_GOAL.md` declares the ROCmFPX fork the canonical integration repository. The package's migration plan instead says to create a clean branch at upstream `86d86ed...`, avoid merging the ROCmFPX tree, and apply a minimal extension series.

The extension-series architecture may reduce maintenance cost, but it is a project-level change of source authority. It requires an ADR that chooses repository lineage, rollback, provenance preservation, patch ownership, and how existing verified ROCmFPX material remains recoverable. Until that decision exists, the plan is a recommendation, not an implementation instruction.

### P1 — The ROCmFPX ledger scope is narrower than its presentation can imply

The package reports 100 post-baseline commit-graph nodes and 276 unique paths across PRs #27, #28, #31, and #32. Those are useful exact counts for the stated recent range. They are not an exhaustive semantic delta between the current ROCmFPX tree and its original/upstream source lineage, because the repositories lack a normal shared merge base and the path union is limited to selected PRs.

Every promoted count must retain the qualifier “recent audited range / selected PR union.” A separate full-snapshot semantic audit is required before claiming complete whole-fork ownership or deletion safety.

### P1 — RETAIN/REFRESH/RETIRE are review proposals, not established decisions

The inventory often distinguishes format-owned work from copied general llama.cpp surface well. However, path membership, filenames, and PR provenance do not establish hunk-level equivalence. In particular, retiring HY3/MTP, generic backend snapshots, server behavior, or RPC code requires semantic comparison and executable regression tests.

Promote the labels as `[RECOMMENDATION]` or candidate decisions with acceptance tests. Do not silently convert them into verified migration authority.

### P1 — RPC adoption needs compatibility and security gates

At the canonical upstream pin, `ggml-rpc.cpp` contains a relevant multi-context ownership improvement relative to the fork, while `CMakeLists.txt`, `transport.cpp`, and `transport.h` are identical at the inspected pins. This supports evaluating upstream RPC adoption; it does not prove safe substitution.

The package emphasizes same-build and tensor-type compatibility, but canonical section 51 also requires an explicit treatment of protocol versioning, endianness, size bounds, opaque identifiers, strong integrity checks, authentication/network trust boundaries, cleanup after partial failure, remote cancellation, and fallback behavior. Native RPC remains unsafe on an untrusted network. Qualification must cover local-plus-remote device ordering, target/draft multi-context behavior, disconnects, malformed frames, and unsupported custom types.

### P1 — Generic normalization can erase semantic differences

The cross-fork normalizer's default recursive removal of fields such as `id` and `slot_id` can hide semantic tool-call, object, request, or slot identity differences. SSE assembly uses that default path. Text comparison also normalizes line endings unless configured otherwise, which conflicts with byte-exact template and serialization cases.

Normalization must be case-schema-specific and field-path-specific. No generic key-name deletion should be allowed in an exact oracle. Raw observations must remain immutable alongside normalized views.

### P1 — Several upstream-reuse references are not upstream paths

After removing selectors, flags, and wildcards, 14 referenced entries do not resolve as paths at either reviewed upstream pin. They include `examples/quantize/README.md`, `examples/rpc/README.md`, `test_kv_page_manager.cpp`, `tests/test-turboquant.cpp`, several proposed server test modules, and one security-advisory label. Some appear to be fork-only references or conceptual work items placed in an upstream-reuse field.

Each reuse entry needs repository identity, exact commit, exact path, selector, and expected upstream behavior. Advisories and proposed tests need separate evidence/work-item fields.

### P1 — CachyLlama identity records need correction

The cross-fork status file says the requested `llama-ai/CachyLlama` identity is unresolved while naming `fewtarius/CachyLLama` as a candidate. Project evidence already resolves the `llama-ai` integration gitlink to `fewtarius/CachyLLama@6be745998f568e379ea197fcf827baec73ff9940`. The package should record that provenance chain rather than leave the participant ambiguous.

Separately, the non-resolving Cachy SHA in `PROJECT_GOAL.md` should be corrected through the project's normal decision/documentation process. This review does not edit it.

### P2 — Moving documentation URLs weaken long-lived provenance

ROCmFPX sources that point to AMD `develop` documentation can change after the review date. Where a versioned documentation page exists, promotion should pin the version and archive sufficient title/date/revision metadata. The versioned ROCm 7.2.0 Strix Halo reference is the better pattern.

## Coverage audit

| Surface | Cross-fork package status | ROCmFPX package status | Promotion gate |
| --- | --- | --- | --- |
| Source/build identity | Good schema concepts; participant applicability incomplete | Exact fork/recent-range anchors; upstream baseline conflicts | One approved pin set and reproducible build manifests |
| Model/GGUF | Proposed cases; most real model artifacts operator-supplied | Useful format/type/file maps | Exact model and GGUF hashes, licenses, loader/error oracles |
| Tokenizer/template | Proposed exact cases; coarse fixtures | Not a primary inventory focus | Immutable tokenizer/template identity and byte/token exact fixtures |
| Logits/sampling | Comparator taxonomy only | No qualification results | Calibrated operation-specific error budgets plus deterministic controls |
| Server/API | Broad proposed matrix; applicability inflated | Fork behavior inventory and replacement proposals | Per-pin capability map, schema-aware normalization, error contract tests |
| Quantization/HIP/Vulkan | Generic cases | Strong candidate map of formats, CPU oracle, kernels, and CI | Golden vectors, round trips, backend-op matrix, real-device coherency |
| MTP/speculation | Proposed cases; incomplete total-state ownership | Useful replace/refresh hypotheses | Target-only oracle, acceptance trace, draft parity, cancellation and full-state gates |
| Persistent cache/state | Proposed corruption/restart cases; incomplete manifests | Identifies fork SSD cache and redesign need | Versioned atomic manifest, complete streams, identity checks, corruption-to-miss |
| RPC/distributed | Proposed topology/failure cases; no orchestrator | Supports upstream-replacement hypothesis | Protocol/security gates, custom types, multi-context, device order, failure cleanup |
| Performance | No measurements | No measurements | Matched environment/configuration, raw logs, repeated runs, stated uncertainty |

## Promotion candidates

The following may be promoted only with their stated claim class and provenance:

1. **Section 78, conformance design:** identity envelope, raw-evidence retention, exact/numerical/distributional oracle separation, no tolerance without calibration, expected rejection, capability probes, and first-class failure observations. Promote as `[RECOMMENDATION]` until implemented.
2. **Section 13, ROCmFPX inventory:** exact fork SHA, exact recent compare range, selected-PR path union, and subsystem maps. Preserve the recent-range qualifier and link raw ledgers.
3. **Sections 30/33, formats and kernels:** format IDs/layouts, CPU reference implementations as candidate oracles, HIP/Vulkan ownership map, and proposed golden-vector gates. Format “stability” remains `[OPEN]` until verified against artifacts and collision policy.
4. **Sections 15/36, upstream replacement:** HY3/MTP replacement and minimal extension-series ideas as `[RECOMMENDATION]`, with an ADR dependency and state-equivalence tests.
5. **Section 51, RPC:** the exact source-diff observation and same-build/custom-type test requirement. Add canonical security and failure gates before promotion.
6. **Sections 57/61, persistence:** versioned cache identity, atomic writes, corruption rejection, and complete target/draft/speculative state coverage as design requirements, not measured behavior.

## Required corrections before promotion

- Resolve `788e07d...` versus `86d86ed...` through one explicit baseline decision and rerun source/path checks against the chosen pin.
- Replace universal applicability with a source-derived capability matrix per participant and pin.
- Give every fixture ID an immutable, independently resolvable payload or deterministic recipe, with digest and provenance.
- Add participant adapters and case-to-command/observation mappings; keep raw outputs separate from normalized views.
- Scope normalization by schema and case; prohibit semantic-field deletion in exact comparisons.
- Define the full target/draft/speculative/sampler/grammar/RNG/server state manifest and atomic restore semantics.
- Mark cross-fork/cross-build state portability `expected-reject` unless explicitly proven.
- Correct upstream-reuse paths and separate source references, advisories, and proposed tests.
- Resolve CachyLlama repository and commit provenance; correct the non-resolving project-goal SHA separately.
- Preserve the recent-range qualifier on the ROCmFPX path ledger and perform hunk-level review before deletion decisions.
- Record RETAIN/REFRESH/RETIRE as recommendations with acceptance tests, not verified facts.
- Decide canonical repository lineage through an ADR before acting on the clean-upstream extension-series plan.
- Add RPC protocol, trust-boundary, integrity, cancellation, disconnect, and fallback gates.
- Pin moving documentation sources or capture dated/versioned source metadata.
- Execute qualification on the intended CPU/HIP/Vulkan/RPC topologies before any correctness, compatibility, or performance promotion.

## Further research prompts

These prompts are independently assignable and should produce immutable evidence plus a concise promotion recommendation.

1. **Canonical upstream baseline decision.** Compare `788e07d...` and `86d86ed...` commit-by-commit, identify behavior-affecting changes for HaloFPX, and draft an ADR choosing one exact pin with rollback and evidence-refresh consequences.
2. **Per-pin capability extraction.** Inspect upstream, ROCmFPX, CachyLlama, and integration source at exact commits; produce a machine-readable capability/applicability map for all 175 cases with `required`, `expected-reject`, `not-applicable`, or `open` and source selectors.
3. **Fixture normalization and licensing.** Convert logical fixture IDs into independently addressable immutable objects; select legally redistributable tiny model/tokenizer/template artifacts; record hashes, licenses, generation recipes, and expected schemas.
4. **Complete state ownership.** Trace target, draft, speculative controller, sampler, grammar, RNG, slot, accounting, and server-stream ownership; define one atomic versioned manifest and corruption/recovery state machine.
5. **RPC compatibility and threat model.** Diff wire and backend semantics at the chosen pins; test custom tensor IDs, same/mismatched builds, target+draft contexts, local+remote placement, cancellation, disconnect, malformed sizes, integrity, and trusted-network enforcement.
6. **Full ROCmFPX semantic delta.** Build a source-snapshot-to-source-snapshot semantic inventory independent of the selected recent PR range; classify ownership at hunk level and identify deletion hazards.
7. **HY3/MTP replacement equivalence.** Compare fork and upstream graph construction, tensor mapping, tokenizer/template assumptions, target logits, acceptance traces, cancellation, and checkpoint state before recommending retirement.
8. **Numerical tolerance calibration.** For each format/backend operation, compare against the independent CPU oracle across adversarial and representative vectors; derive operation-specific error budgets and detect fallback paths.
9. **Format ABI provenance.** Inventory existing ROCmFPX GGUF artifacts, enum assignments, block sizes/layouts, endian assumptions, and possible upstream/fork collisions; distinguish pinned fork-local ABI from public stable ABI.
10. **Project authority repair.** Trace the valid CachyLlama gitlink/commit and draft the smallest correction for `PROJECT_GOAL.md`; separately draft the repository-lineage ADR required by the extension-series proposal.

## Acceptance conditions

The cross-fork package may be reconsidered for implementation acceptance when cases are capability-derived, fixtures are individually resolvable, adapters execute exact pinned participants, raw evidence is retained, full state/RPC failure semantics are represented, and a first qualification run is reproducible.

The ROCmFPX package may be promoted selectively when its upstream pin matches an approved baseline, all executive counts retain their exact range qualifiers, recommendations remain labeled, deletion candidates receive hunk/test review, the repository-lineage decision is explicit, and machine validation covers the intended Strix Halo backends and distributed topology.

## Review of this review

- **Correctness:** Counts and pins were independently checked; imported scripts were not executed. No performance or compatibility result is inferred.
- **Freshness:** The review is pinned to the 2026-07-17 intake and exact commits. Moving documentation remains explicitly flagged.
- **Clarity:** Package integrity, source evidence, design recommendations, and executable qualification are kept separate.
- **Provenance:** Local authorities and immutable intake paths are identified; primary commit/tree/compare data was used for pin and delta findings.
- **Residual risk:** This was a source-and-artifact audit, not a build or hardware run. Hunk-level whole-fork analysis and machine qualification remain open by design.
- **Reusable improvement:** The new prompts convert the principal gaps into independently assignable evidence tasks with explicit promotion boundaries.

**Final review verdict:** **REVISE.** Selective conceptual and exact-source promotion is justified after the listed corrections; implementation, equivalence, migration, and qualification claims remain deferred.
