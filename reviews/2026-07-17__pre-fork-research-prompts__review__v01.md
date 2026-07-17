---
type: research-prompt-review
status: complete
created: 2026-07-17
subject: research/prompts/2026-07-17__pre-fork-further-internet-research-prompts.md
subject_sha256: 92318205f6aad6c3bee4f70f1b9af04a8c3cc55f188386db36222f29113a57eb
verdict: accept-after-revision
canonical_wiki_edited: false
---

# Independent review: pre-fork further Internet research prompts

## Verdict

**ACCEPT AFTER REVISION.** The dispatch artifact now contains 11 distinct Internet-research assignments. Every assignment explicitly requires a downloadable folder containing everything styled like an LLM Wiki, names the external evidence sought, identifies the later decision it informs, and preserves the boundary between returned candidate evidence and local or human approval.

No additional high-impact Internet-only track is missing from the reviewed preparation scope. The accepted implementation plan remains authoritative that no additional Internet research is required before local/read-only Phase 0A (L00A). These prompts prepare later source/toolchain, security, test-asset, persistence, transport, model, firmware, and optional-NPU gates; they are not a reason to delay L00A.

## Review basis

The prompt set was checked against:

- all nine intake reviews under `reviews/intake/`, including integrity, cross-fork conformance, donor patch mapping, integration features, HaloKV/cache, dual-USB4, dual-node/large-model, lineage/licensing/gfx1151, and ROCmFPX pin-to-head findings;
- accepted plan `reviews/plans/2026-07-17__rocmfpx-llama-ai-fork-plan__draft__v03.md` and final acceptance `reviews/plans/2026-07-17__rocmfpx-llama-ai-fork-plan__review__v03.md`;
- canonical Section 85's external-source authorities, open questions, freshness rules, severity model, and explicit separation from Section 84 machine experiments;
- exact repository identities in `sources/repositories/manifest.yaml` and the 2026-07-17 two-node evidence referenced by the backlog;
- the Agent Harness evidence route and the repository's preservation-first organization rules.

This review did not browse, execute imported scripts, contact target machines, or edit the canonical Wiki.

## Material findings and revisions

| ID | Finding | Revision |
|---|---|---|
| R1 | The original P0/P1 wording could imply that Internet research blocked accepted L00A, contradicting plan v03/review v03. PF-IR-02 and PF-IR-05 were also categorized as P0 even though they primarily serve transport and Phase 2. | Added the explicit L00A non-blocking boundary, assigned a priority to every prompt, and classified transport/model/firmware/persistence work as P1 while retaining security, official baseline evidence, licensing evidence, and conformance-asset provenance as P0 for their later gates. |
| R2 | PF-IR-02 and PF-IR-12 substantially duplicated public USB4/PCIe directionality, interoperability, errata, and Linux implementation mapping. Separate returns could diverge. | Merged the public standards/errata work into PF-IR-02 and removed PF-IR-12. The deduplication note now names PF-IR-02 as the single owner. |
| R3 | PF-IR-04 appeared to ask a remote Internet agent to inspect and approve assets from the local twelve-package intake. That agent may not possess the intake, and actual-artifact admissibility is a local scan plus human/legal decision. | Restricted PF-IR-04 to authoritative public license/provenance evidence and stated that it neither assumes local-intake access nor replaces the local full-tree/proposed-artifact scan or human decision. |
| R4 | PF-IR-10 could be read as allowing an external package to close local fixture applicability, static-review, isolated-execution, and promotion gates. | Restricted the claimed decision output to external provenance/licenses and candidate recipes; retained local source-derived applicability, staged qualification, and exact-manifest approval as mandatory. |
| R5 | Several “Decision unblocked” lines overstated what Internet documentation can prove about deployed binaries, both-node compatibility, filesystem durability, RCCL suitability, firmware rollout, service thresholds, and model choice. | Reworded each affected line to identify the exact external evidence contribution and the remaining local experiment or human-decision boundary. |
| R6 | The user's required delivery form needed to be mechanically obvious for every assignment. | Retained the exact `Expected output` sentence on all 11 prompts and added the one-folder-per-prompt dispatch rule. Counts now match: 11 prompts, 11 priorities, and 11 required downloadable LLM-Wiki outputs. |

## Coverage and deduplication assessment

| Required external domain | Owner | Assessment |
|---|---|---|
| Current security advisories and backport applicability | PF-IR-01 | Distinct; exact snapshots, symbols, reachability, and safe regression-test design are bounded. |
| USB4STREAM/kernel packaging, USB4NET/MPTCP semantics, public errata | PF-IR-02 | Distinct after PF-IR-12 merge; performance and failure independence remain local. |
| Official gfx1151/ROCm/Mesa/kernel/firmware component lane | PF-IR-03 | Distinct; source/artifact tuple only, not machine qualification. |
| Public donor/dependency/license and release obligations | PF-IR-04 | Distinct after local-intake scope removal; exact proposed-tree scan and legal approval remain local/human. |
| Immutable 200–230 GB model artifacts and publisher facts | PF-IR-05 | Distinct; fit, operator coverage, quality, and final workload choice remain local/human. |
| Filesystem/io_uring crash-semantics authorities | PF-IR-06 | Distinct; feeds the durability contract and fault matrix without claiming implementation proof. |
| Encryption, key lifecycle, deletion, backup standards | PF-IR-07 | Distinct; informs but does not make tenant/key policy decisions. |
| RCCL/two-host/network-plugin support boundary | PF-IR-08 | Distinct; no GPU-direct or target-machine capability is inferred. |
| Product-specific firmware, bulletins, errata, and RAS authorities | PF-IR-09 | Distinct from PF-IR-02's public transport standards and PF-IR-03's compute-stack tuple. |
| Redistributable or generated semantic fixtures | PF-IR-10 | Distinct; local harness/applicability/promotion stays outside the Internet assignment. |
| Optional XDNA2 auxiliary role | PF-IR-11 | Properly P2 and explicitly allowed to conclude `keep excluded`. |

No generic feature-inventory, whole-fork source archaeology, source-pin build qualification, benchmark, protocol model-check, cache prototype, machine inventory, deployment, fault injection, governance, or final policy task remains accidentally assigned to an Internet agent. Those tasks are explicitly retained in the local-only section.

## Remaining boundaries

1. A returned Wiki is candidate evidence, not an approved baseline, implementation decision, executable dependency, or machine result.
2. P0 in this dispatch file means priority for a later named gate; it does not supersede the accepted authority to begin L00A.
3. OPEN-PIN-01, OPEN-PROV-01, OPEN-BASE-01, OPEN-STATE-01, OPEN-API-01, OPEN-STORAGE-01, OPEN-ACCEPT-01, target-machine compatibility, and all performance claims remain local.
4. OPEN-GOV-01 and final license, clean-room, distribution, tenant/sharing, key ownership, model/workload, firmware rollout, and service-threshold choices remain human decisions.
5. Returned primary-source captures must still be preserved under `sources`, reviewed, and selectively promoted through the project evidence route; generated sites and unreviewed scripts remain non-canonical.

## Verification

- Subject SHA-256: `92318205f6aad6c3bee4f70f1b9af04a8c3cc55f188386db36222f29113a57eb`.
- Prompt headings: 11.
- Explicit priority records: 11.
- Exact required downloadable LLM-Wiki output statements: 11.
- Duplicate public USB4/PCIe standards prompt: removed and merged into PF-IR-02.
- Canonical Wiki edits: none.
