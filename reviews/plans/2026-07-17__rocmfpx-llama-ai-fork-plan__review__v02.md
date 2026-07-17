---
type: implementation-plan-review
status: complete
created: 2026-07-17
subject: 2026-07-17__rocmfpx-llama-ai-fork-plan__draft__v02.md
verdict: revise
reviewer_scope: second independent evidence and readiness review
supersedes_review: 2026-07-17__rocmfpx-llama-ai-fork-plan__review__v01.md
---

# Second independent review: ROCmFPX + llama-ai/CachyLLama fork plan v02

## Verdict

**REVISE.** V02 resolves three prior blockers and the core of the fourth without regressing the canonical-base, provenance, cache-safety, API/state, test, or nimo rollback strategy. It remains ready for **authorized local/read-only Phase 0A source freeze and static archaeology only**. Two narrow sequencing ambiguities must be corrected before the test-asset intake lane or any persistence lane is executable.

## Prior blocker verification

| Prior finding | Result | Evidence |
|---|---|---|
| B1 state/scope consumers preceded contracts | **Resolved** | L02 now freezes state/scope/format contracts; L06 implements trusted scope; L07 admits state codecs; L04-L05 are explicitly offline/synthetic-only; server canary waits for L06-L07. Draft lines 194-216. |
| B2 local preparation conflated with remote fork mutation | **Resolved** | Phase 0A/0B are separated; `OPEN-GOV-01`, L00C, and G0B require destination, visibility, authority, protections, signing, and evidence decisions before create/push. Draft lines 14-18, 72-74, 194-200, 260-271, and 297-305. |
| B3 imported tooling lacked an intake/execution gate | **Partially resolved; revise R1** | Hash/license/static/isolation/validation/promotion controls and `OPEN-TEST-01` now exist, but their wording creates a circular gate. Draft lines 76-86. |
| B4 stale CachyLLama pin mismatch | **Resolved** | Draft line 70 now matches `PROJECT_GOAL.md`, the local donor object, and llama-ai gitlink: `6be745998f568e379ea197fcf827baec73ff9940`. |

## Required revisions

### R1 — Split the candidate-tool gate into pre-execution, qualification, and promotion stages

The text says all five steps occur “before any imported ... code runs,” but step 4 requires validating the candidate against synthetic fixtures, which ordinarily requires executing it. It then says the package remains requirements-only until the gate closes. Read literally, execution is forbidden until after the validation that requires execution, so `OPEN-TEST-01` cannot close.

Required correction:

1. **Pre-execution:** hashes, provenance/license review, static review, and approved isolated environment.
2. **Qualification execution:** run the still-untrusted candidate only inside that bounded environment against synthetic fixtures; retain raw results.
3. **Promotion:** human/reviewer approval of an exact accepted asset manifest and canonical destination.

State that steps 1-2 authorize only isolated qualification, never production, target-node, credentialed, model-store, cache-store, or service execution. This fully preserves the intended Agent Harness candidate-to-reviewed-to-published boundary.

Evidence: draft lines 76-86; root `AGENTS.md`; `references/agent-harness.md`; `C:\Users\britt\Documents\Agent_Harness\guide\architecture.md`.

### R2 — Define the L08 canary lookup contract before L09 adds exact-prefix matching

L08 enables a persistent read-only/read-write provider canary, while L09 introduces the authorized exact-prefix match policy. The provider/match separation is sound, but the plan does not say how L08 selects an entry without L09. An implementer could silently introduce an early match policy, undermining lane ownership and security review.

Required correction: either move the exact authorized match policy before the L08 canary, or explicitly restrict L08 to opaque committed-ID/direct session references supplied by the disposable test harness, with no prefix discovery, system-prefix reuse, fuzzy lookup, cross-conversation lookup, or fallback search until L09. Add that restriction to the L08 exit gate and execution sequence.

Evidence: draft architecture lines 89-101; lane table lines 194-216; cache-scope invariants lines 165-180; `wiki/HaloFPX_Wiki/09_HaloKV_Persistent_Cache/56_CachyLLama_Cache_Semantics_and_Porting_Map/design_implications.md`; `wiki/HaloFPX_Wiki/09_HaloKV_Persistent_Cache/60_System_Prompt_Sharing_Deduplication_Copy_on_Write_and_Continuations/open_questions.md`.

## Full checklist and regression audit

| Area | Result |
|---|---|
| Canonical base | Pass: `charlie12345/ROCmFPX` remains the product base; donors remain traceable inputs, never merge parents by default. |
| Pin accuracy/drift | Pass: exact pins and deployed baseline are coherent; `a5605a...` versus `61f2f2d...` remains correctly gated by `OPEN-PIN-01`. |
| Feature dispositions | Pass for planning: coverage is complete and conservative; the empty CP roster and P3 requirement correctly block code import until each mixed treatment becomes one reviewed disposition. |
| License/provenance | Pass: GPL/CC separation, clean-room role separation, per-file provenance, notices, SBOM, and separate asset/runtime/model records remain gated. |
| Architecture | Pass: API, scope, scheduler, codec, match, store, retention, and telemetry ownership are separated. |
| Patch order | Revise only for R2; prior state/scope inversion is fixed. |
| Cache/state/security invariants | Pass: corruption/mismatch is miss/recompute; validation precedes mutation; mandatory streams transact together; filesystem and scope attacks fail closed. |
| API/state compatibility | Pass for planning: exact surfaces and fixtures remain correctly blocked by `OPEN-API-01`, `OPEN-FMT-01`, and `OPEN-STATE-01`. |
| Build/test matrix | Pass after R1: exact manifests, both hosts/backends, state/storage/lifecycle/scope/quality/performance/distributed cases, and variance-derived thresholds are retained. |
| nimo rollout/rollback | Pass: production is preserved, nimo-1 capacity and the separate RPC tensor cache are protected, alternate-port/disposable canaries and coordinated RPC cutover rules remain explicit. |
| Acceptance gates | Pass after R1-R2 are reflected in G0C/L08. |
| Risks/dependencies/OPEN items | Pass: governance, test assets, pins, provenance, license, baseline, API, state, scope, storage, acceptance, and Phase 2 ownership/fabric/fit decisions remain explicit. |
| Further Internet research | Pass: none is needed for Phase 0A; later research is conditional on a named unresolved local or current-official-source gate. |

## Readiness scope

V02 may guide local source object verification, bundles, cryptographic inventories, static source archaeology, and non-executing candidate-tool review. It does **not** authorize remote fork creation/push, imported-code execution, donor-code import, persistent-cache implementation, target deployment, service interruption, or destructive/fault testing.

After R1 and R2 are corrected in a preserved v03 draft, the plan is suitable for **ACCEPT — authorized Phase 0A only**, while all later work remains deferred behind its explicit gates. No new Internet-research package is required to make these two corrections.
