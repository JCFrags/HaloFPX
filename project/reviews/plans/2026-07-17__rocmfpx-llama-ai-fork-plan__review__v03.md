---
type: implementation-plan-review
status: complete
created: 2026-07-17
subject: 2026-07-17__rocmfpx-llama-ai-fork-plan__draft__v03.md
subject_sha256: 6c4a334bb4e55f9a28a1d7374ed3e232c23695a50745e34cdb3f7a72ce5c3697
verdict: accept
reviewer_scope: final independent evidence and readiness review
supersedes_review: 2026-07-17__rocmfpx-llama-ai-fork-plan__review__v02.md
---

# Final independent review: ROCmFPX + llama-ai/CachyLLama fork plan v03

## Verdict

**ACCEPT — authorized L00A / local read-only Phase 0A only.** V03 resolves the remaining test-asset and persistent-canary sequencing findings without introducing a new blocker. The plan is sufficiently precise to govern local source freezing, cryptographic inventories/bundles, and static archaeology. It correctly withholds authority for remote mutation, candidate-tool execution, donor import, persistence implementation, deployment, or disruptive machine work.

## Final finding verification

| Finding | Result | Evidence |
|---|---|---|
| R1 candidate-tool gate was circular | **Resolved** | Lines 75-84 define pre-execution review, isolated qualification execution, then human/reviewer promotion. `OPEN-TEST-01` at line 299 requires Stage 1 before qualification and exact-manifest approval after Stage 2 but before project/test-matrix execution. |
| R2 L08 canary lacked a lookup contract before L09 | **Resolved** | L08 is direct committed-ID/direct-session only; lines 211-214 prohibit prefix discovery, system-prefix reuse, fuzzy/cross-conversation lookup, and fallback until L09/L10. The execution sequence repeats the restriction. |
| Prior B1 state/scope dependency inversion | **Remains resolved** | L02 contracts precede L03-L05; L04-L05 remain offline; L06 trusted scope and L07 admitted codecs precede L08. |
| Prior B2 local versus remote authority | **Remains resolved** | Phase 0A is local; remote creation/configuration requires `OPEN-GOV-01`, G0B, and the separately authorized governance lane. |
| Prior B3 imported asset trust | **Fully resolved by R1** | Candidate code stays untrusted through bounded qualification; only an exact reviewed manifest is promoted. |
| Prior B4 stale donor pin | **Remains resolved** | `PROJECT_GOAL.md`, the local CachyLLama object, and llama-ai gitlink agree on `6be745998f568e379ea197fcf827baec73ff9940`. |

## Full checklist result

| Area | Final result |
|---|---|
| Canonical base and pins | Pass: ROCmFPX remains canonical; exact candidate/current/donor/upstream/deployed objects are distinguished; drift is gated. |
| Feature dispositions | Pass for planning: capability coverage is complete, wholesale merge is rejected, CP roster is empty, and each eventual treatment requires P3 evidence. |
| License/provenance | Pass: MIT/GPL/CC boundaries, clean-room role separation, notices, file/commit mapping, SBOM, and separate asset/runtime/model records are explicit gates. |
| Architecture and patch order | Pass: API, scope, scheduler, codec, match, store, retention, and telemetry boundaries have an implementable dependency order. |
| Cache/state/security invariants | Pass: corruption and incompatibility become miss/recompute; validation precedes mutation; required streams transact together; scope and filesystem failures fail closed. |
| API/state compatibility | Pass for planning: exact surfaces, fixtures, state vectors, and format evolution remain gated before implementation. |
| Build/test matrix | Pass: both nimo hosts/backends plus API, state, storage, lifecycle, isolation, quality, performance, and distributed regression cases are retained with exact manifests. |
| nimo rollout/rollback | Pass: the deployed baseline and RPC tensor cache are preserved; storage asymmetry, alternate ports, disposable stores, coordinated RPC switching, and rollback proof are explicit. |
| Acceptance gates and OPEN items | Pass: governance, test assets, pins, provenance, licensing, baseline, API, format, state, scope, storage, thresholds, and Phase 2 work remain conjunctive gates. |
| Further Internet research | Pass: none is required for L00A; later research is requested only for a named unresolved local or current-official-source dependency. |

## Exact authorized readiness scope

This acceptance authorizes only:

- verification of existing local repository objects, trees, parents, gitlinks, remotes, and dirty state;
- creation of local cryptographic inventories, bundles, manifests, provenance/license records, and review evidence inside approved workspace paths;
- static, non-executing source archaeology and capability/dependency mapping;
- non-executing Stage 1 inspection of candidate test assets, provided it does not promote or run them.

This acceptance does **not** authorize:

- creating, configuring, pushing, tagging, or otherwise mutating a remote repository;
- committing or pushing local project changes;
- executing imported candidate scripts or fixtures outside a separately approved Stage 2 isolation contract;
- importing, cherry-picking, adapting, or reimplementing donor code;
- implementing or enabling persistent storage, matching, scope, concurrency, telemetry, or clean-room behavior;
- changing target-node packages, kernels, services, binaries, configs, caches, models, network state, or boot state;
- stopping live inference, rebooting, deleting data, corrupting files, injecting faults, or performing deployment/cutover work.

All work beyond this scope remains deferred until its named OPEN item and acceptance gate close with the required human authorization and retained evidence.

## Nonblocking editorial note

The prose name “Phase 0B” refers to governed remote creation, while lane `L00B` refers to test-asset intake and `L00C` to remote governance. This is distinguishable in context and does not block L00A, but future issue/authorization records should always use the full lane ID and title; renaming one label would reduce operator ambiguity.

No further revision or Internet-research package is required before beginning the accepted L00A scope.
