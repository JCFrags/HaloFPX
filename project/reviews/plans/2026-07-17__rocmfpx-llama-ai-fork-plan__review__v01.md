---
type: implementation-plan-review
status: complete
created: 2026-07-17
subject: 2026-07-17__rocmfpx-llama-ai-fork-plan__draft__v01.md
verdict: revise
reviewer_scope: independent evidence and readiness review
---

# Independent review: ROCmFPX + llama-ai/CachyLLama fork plan v01

## Verdict

**REVISE.** The plan has the correct canonical base, a defensible no-wholesale-merge architecture, strong cache/state safety invariants, conservative provenance gates, a credible test shape, and a realistic nimo rollback boundary. Its stated readiness is justified only for **local, read-only Phase 0 source freezing and characterization**. It is not yet acceptable as authority to create/push the writable fork, execute imported research tooling, import donor code, or begin persistent-cache implementation.

## Blocking findings

### B1 — State and scope dependencies are ordered after consumers

The lane table places the v1 reader/writer/provider/match/system-prefix work in L03-L07, but the state-capability implementation is L08 and trusted scope identity is L09. This conflicts with the same plan's OPEN gates: `OPEN-STATE-01` and `OPEN-SCOPE-01` block persistent writes, while L06 claims an “authorized exact-prefix” policy before the trusted scope exists. It also conflicts with the canonical Wiki's requirement that required mutable streams, compatibility identity, and privacy scope be decided before a restore can be accepted.

Required revision: separate contract freeze from feature implementation and make the dependency order explicit. At minimum, freeze the state-capability vector and trusted scope contract before the format/schema reader and any writer; implement scope resolution before authorized matching/system-prefix reuse. If an early reader/writer is intentionally offline and synthetic-only, state that restriction and prevent it from entering a server or canary path.

Evidence:

- draft lines 151-175, 181-198, and 287-293;
- `wiki/HaloFPX_Wiki/03_Repository_and_Engineering/12_Codebase_Architecture_and_Module_Map/facts_and_constraints.md`;
- `wiki/HaloFPX_Wiki/09_HaloKV_Persistent_Cache/57_Compatibility_Fingerprints_Versioning_and_Topology_Identity/open_questions.md`;
- `wiki/HaloFPX_Wiki/09_HaloKV_Persistent_Cache/60_System_Prompt_Sharing_Deduplication_Copy_on_Write_and_Continuations/open_questions.md`;
- `wiki/HaloFPX_Wiki/09_HaloKV_Persistent_Cache/61_Attention_KV_Recurrent_MTP_Speculative_Sampling_and_RNG_State/README.md`;
- `wiki/HaloFPX_Wiki/09_HaloKV_Persistent_Cache/64_Eviction_Garbage_Collection_Quotas_User_Isolation_and_Privacy/open_questions.md`.

### B2 — Phase 0 conflates local preparation with externally mutating fork creation

L00 includes “fork” and the verdict says Phase 0 is ready, but the plan does not name the writable owner/repository, visibility, GitHub fork-network versus independent-integration-repository choice, default-branch protection, signing authority, or who is authorized to create/push it. The plan itself says human approval is required, and the canonical Wiki leaves the accepted base, signing authority, and evidence-retention location open.

Required revision: split Phase 0 into (a) authorized local/read-only source freeze, bundle, manifest, archaeology, and characterization, and (b) separately authorized remote repository creation/configuration. Add an explicit repository-governance OPEN item and acceptance gate for destination identity, visibility, permissions, branch protections, tag/signing policy, and immutable evidence location.

Evidence:

- draft lines 1-9, 16-18, 61-72, and 181-184;
- `PROJECT_GOAL.md` lines 15-17;
- `wiki/HaloFPX_Wiki/03_Repository_and_Engineering/11_Repository_Lineage_Branches_Commits_and_Frozen_Baselines/open_questions.md` (`OQ-11-08`);
- `wiki/HaloFPX_Wiki/03_Repository_and_Engineering/15_Integration_Patch_Stack_and_Upstream_Synchronization_Strategy/open_questions.md` (`OQ-15-12`);
- `wiki/HaloFPX_Wiki/03_Repository_and_Engineering/16_Build_Dependencies_Licensing_CI_and_AI_Agent_Workflow/facts_and_constraints.md`.

### B3 — Imported conformance tooling lacks a promotion/execution gate

The test matrix calls for the imported cross-fork conformance fixtures, but the plan does not require provenance, license, static review, sandboxing, and deterministic validation before executing candidate scripts. Imported Wikis remain candidate evidence under the project and Agent Harness rules; preservation does not make their code trusted.

Required revision: add a Phase 0 test-asset intake gate. Preserve exact hashes and licenses; statically review scripts and fixtures; run only in an isolated environment; record validation results; promote only the accepted subset into a canonical test-assets location. Until then, refer to the imported harness as a candidate, not a required executable dependency.

Evidence:

- draft lines 37-55 and 208-223;
- `sources/imports/2026-07-17-further-research-wikis/extracted/cross-fork-llama-conformance-wiki/`;
- root `AGENTS.md` source-to-Wiki promotion rule;
- `references/agent-harness.md` and `C:\Users\britt\Documents\Agent_Harness\guide\architecture.md`.

### B4 — The CachyLLama pin-mismatch statement is stale and false

Draft line 70 says `PROJECT_GOAL.md` contains `6be745998f62b1d34f32da1f2c8a503936d142cf`. The current goal file contains the valid donor/gitlink pin `6be745998f568e379ea197fcf827baec73ff9940`. Leaving the stale statement in a source-freeze plan creates ambiguity about the canonical ledger.

Required revision: remove the mismatch paragraph or replace it with a dated history note supported by a retained change record. The valid current pins are locally confirmed: ROCmFPX research pin `a5605a72768c6562241b248e268e33dc92787394`, current local/remote-tracking ROCmFPX tip `61f2f2d7bc4955e9bca821095ef69125837133b5`, llama-ai `1017f3dfdce3ca2b06aa9007b23295db3bb35722`, CachyLLama/gitlink `6be745998f568e379ea197fcf827baec73ff9940`, and donor merge second parent `92366df30d4eaa4b85139b5fd694360237731b19`.

Evidence:

- `PROJECT_GOAL.md` lines 59-61;
- `sources/repositories/charlie12345__rocmfpx/.git`;
- `sources/repositories/fewtarius__llama-ai/.git`;
- `sources/repositories/fewtarius__cachyllama/.git`;
- `wiki/HaloFPX_Wiki/03_Repository_and_Engineering/11_Repository_Lineage_Branches_Commits_and_Frozen_Baselines/README.md`.

## Nonblocking findings

1. **Feature dispositions are appropriately conservative but not implementation-ready.** The empty CP roster and P3 gate correctly prevent accidental bulk import. Several entries still use alternatives such as `MP/CR`, `IF + MP`, or “inventory then,” so each selected capability needs one final treatment, exact introducing commits, dependency closure, target files, tests, and rollback before its lane opens.
2. **The license boundary is directionally sound.** The MIT engine donor is separated from GPL llama-ai and separately licensed documentation. Before any clean-room lane, define separate specification and implementation roles/contexts, prior-exposure declarations, permissible evidence, and comparison review; agents that inspected GPL source must not silently become clean-room implementers. Exact release-tree dependencies, WebUI assets, models, ROCm packages, and test corpora remain release gates.
3. **Cache invariants are strong and consistent with project authority.** Corruption/mismatch becomes miss/recompute; validation precedes mutation; multi-component state is transactional; unsafe paths, quota reserve, rollback incompatibility, and rank-local distributed state are covered. Preserve these as normative requirements.
4. **API/state compatibility coverage is good but the compatibility surface remains open by design.** `OPEN-API-01`, `OPEN-FMT-01`, and `OPEN-STATE-01` correctly prevent implementation assumptions. Add exact fixture ownership and promotion once the surface is approved.
5. **The build/test matrix is credible but must freeze exact target manifests.** The current hosts share kernel `7.1.3-1-cachyos`, ROCm 7.2.4 family, Mesa 26.1.4, and gfx1151, but package/boot/swap skew exists. `OPEN-BASE-01` must choose the dependency-lock authority and record both native clean builds; imported benchmark thresholds are not admissible.
6. **The nimo rollout/rollback plan is realistic.** It preserves the running `rocmfp4-llama@4860505e...` baseline, protects the approximately 112 GiB RPC tensor cache, accounts for nimo-1's approximately 43 GiB free-space constraint, uses alternate ports/disposable stores, maintains nimo-1 worker/nimo-2 coordinator ownership, and avoids unproven mixed-version RPC operation. Add exact service/config backup and port-conflict receipts during execution.
7. **The plan correctly separates the deployed RPC tensor cache from HaloKV.** Its FNV/direct-write/no-read-rehash weaknesses are not inherited. Any future RPC cache hardening should remain its own lane, schema, quota, and acceptance matrix.
8. **Signing is not yet actionable.** “Signed/annotated baseline manifest” should be conditional on an approved signing authority/key-retention policy; otherwise require an annotated tag plus cryptographic artifact hashes and mark signing deferred.

## Requested checklist result

| Area | Result |
|---|---|
| User-directed ROCmFPX canonical base | Pass |
| Pin accuracy and drift handling | Revise: pins are accurate except stale line 70; base choice remains correctly OPEN |
| Feature-by-feature transplant/reimplementation decisions | Conditional pass: complete coverage, but per-capability P3 selections remain blocking |
| License/provenance gate | Conditional pass: conservative boundary; operational clean-room roles still needed |
| Patch order and architecture | Revise: state/scope dependency inversion |
| Cache invariants | Pass |
| API/state compatibility | Conditional pass: explicit OPEN gates are appropriate |
| Build/test matrix | Conditional pass: exact manifests and candidate-tool promotion remain open |
| nimo rollout/rollback | Pass with minor execution receipts added |
| Acceptance gates and risks | Pass after B1-B3 are reflected in gates |
| OPEN items and assumptions | Revise: add remote-repository governance and imported-tool promotion |

## Research and readiness conclusion

No new Internet research is required to perform the local/read-only portion of Phase 0. The immediate work is local source archaeology, capability provenance, test-asset review, exact builds, target-machine characterization, and human governance decisions. Further Internet research becomes necessary only when local history cannot resolve a capability/license dependency or when a current official toolchain, kernel, security, or licensing source is required for a named gate.

After B1-B4 are corrected in a preserved v02 draft, the plan can be accepted as **ready for authorized Phase 0 only**. It must remain **deferred for donor code import and persistent-cache implementation** until `OPEN-PIN-01`, `OPEN-PROV-01`, `OPEN-LIC-01`, `OPEN-BASE-01`, and `OPEN-API-01` close, and deferred for persistent writes until the format/state/scope/storage/acceptance gates close.
