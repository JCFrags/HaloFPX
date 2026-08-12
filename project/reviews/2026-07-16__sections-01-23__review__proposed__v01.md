---
title: "Adversarial review of HaloFPX Wiki sections 01-23"
date: "2026-07-17"
status: "re-reviewed"
scope: "wiki/HaloFPX_Wiki sections 01-23"
review_type: "evidence, consistency, and integration audit"
---

# Adversarial review: sections 01-23

## Verdict

The 23 audited section directories are structurally complete: all 161 required section artifacts are present, their Markdown front matter and `section.yaml` files parse, and no broken relative Markdown links were found inside the audited section pages. The two P1 findings below were remediated and re-reviewed on 2026-07-17. The audited baseline still requires revision for the P2 label, identifier, and mutable-source findings.

| Category | Status | Basis |
|---|---|---|
| 01 Wiki Governance (01-05) | **REVISE** | Root path/artifact authority is now operational; question-label normalization and cross-platform validation remain. |
| 02 Project Definition (06-10) | **ACCEPT** | Section content consistently uses explicit claim labels, pinned repository revisions, local Agent Harness authority, and actionable validation gates. Shared category-index repair remains an integration task. |
| 03 Repository and Engineering (11-16) | **REVISE** | Source-code precedence is strong and contradictions are preserved, but section 16 uses an incompatible section-ID form and several open-question tables omit `[OPEN]`. |
| 04 Hardware and OS Platform (17-23) | **REVISE** | Sections 18-20 now meet the evidence/label gate, but several compatibility sources remain mutable `latest`/`master` pages. |

## Remediation re-review — 2026-07-17

**P1 verdict: RESOLVED.** `manifest.yaml` is now a deterministic canonical path and structural-artifact registry generated from the supplied section index and present `section.yaml` files. `manifest.schema.json` constrains the format, `wiki.yaml` routes authority without approving a version baseline, category READMEs 01-04 link their structurally complete section sets, and the generator's `--check` mode detects staleness. Final schema validation passed for 12 categories and 86 sections; all 86 required artifact sets were present. The manifest deliberately reports metadata validity separately and exposes pre-existing errors, including audited sections 11, 13, 14, 16, and 21-23, rather than silently normalizing them. Counts are intentionally left to the generated snapshot because concurrent remediation changes them.

**P1 measurement verdict: RESOLVED.** Nine cited local artifacts were copied without modifying their AgentRoot originals to `sources/measurements/2026-07-10_12-strix-halo-cluster/`; destination SHA-256 values matched the originals. Because the two July 12 system audits lack their underlying raw command-output bundle, sections 18 and 19 and the topology-report claim in section 20 were downgraded from `[MEASURED]` to scoped `[VERIFIED]` historical-report claims, with S18-E01, S19-E01, and S20-E01 retained as the required recaptures. Section 20's July 10 functional two-rail/MPTCP claim remains `[MEASURED]` because its raw logs, timestamped environment/state output, and checksum receipt are now preserved and directly linked.

**Remaining blockers.** The original P2 findings remain: mutable compatibility sources, incomplete `[OPEN]` normalization, and noncanonical identifiers/section metadata. Category 01 therefore remains `REVISE`; Category 04 remains `REVISE` for source immutability, not measurement provenance. Cross-platform generation/checking on Windows and both Halo nodes is still an explicit machine experiment, not a completed claim.

## Severity-ranked findings

### P1 — The declared wiki authority is not operational — **RESOLVED 2026-07-17**

**Evidence.** The wiki root says that root manifests and section registries determine authoritative paths and enumerates core ledgers (`wiki/HaloFPX_Wiki/README.md:7-16`). The only root machine manifest remains `status: "scaffold"` and lists unpinned repository URLs (`wiki/HaloFPX_Wiki/wiki.yaml:4-10`). Section 01 proposes `manifest.yaml` and `manifest.schema.json` but explicitly records that neither was created (`01_Wiki_Governance/01_Wiki_Architecture_Navigation_and_Root_Manifest/facts_and_constraints.md:35-52`). No root source, glossary, assumption, open-question, decision, experiment, compatibility, risk, or status ledger exists. All four audited category READMEs still say `Research status: pending` and list sections as plain text rather than links (for example, `01_Wiki_Governance/README.md:5-11` and `03_Repository_and_Engineering/README.md:5-11`).

**Impact.** Sections 01-23 can be retrieved individually, but there is no machine-valid global authority for stable IDs, supersession, contradictions, decisions, or unresolved questions. Important conflicts already preserved in sections 11 and 14 cannot be discovered from the root.

**Required revision.** Implement or explicitly reject the Section 01 manifest design; validate it in CI; generate category indexes and global ledgers from section metadata; change category status only from generated completeness evidence. Until then, treat `wiki.yaml` as a scaffold, not authority.

**Applied.** Added the generated `manifest.yaml`, its Draft 2020-12 schema, a deterministic generator/check command, scoped root authority routing, and linked category navigation. Artifact completeness is explicitly not research acceptance. The manifest exposes invalid section metadata instead of treating it as authoritative. Typed global claim/decision ledgers remain a P2 governance expansion because no approval or canonical records exist to populate them yet.

**Affected paths.** `wiki/HaloFPX_Wiki/README.md`, `wiki/HaloFPX_Wiki/wiki.yaml`, `wiki/HaloFPX_Wiki/01_Wiki_Governance/`, and category READMEs 01-04.

### P1 — `[MEASURED]` claims are traceable but not preserved as repository evidence — **RESOLVED 2026-07-17**

**Evidence.** Sections 18-20 promote historical machine/BOM, GTT/IOMMU, USB4, and MPTCP observations as `[MEASURED]` (`18_Exact_Machine_BOM_BIOS_Firmware_Cabling_and_Revisions/facts_and_constraints.md:17-25`; `19_Unified_Memory_GTT_GPUVM_IOMMU_and_Allocation_Limits/facts_and_constraints.md:35`; `20_USB4_Physical_Topology_and_Dual_Port_Independence/facts_and_constraints.md:25-34`). Their source records point outside this repository to synthesized/redacted AgentRoot audit pages and a receipt that refers to hashed logs (`20_USB4_Physical_Topology_and_Dual_Port_Independence/sources.md:39-41`). The project `sources/` tree does not preserve the cited raw logs, their environment manifests, or a checksum-bearing pointer manifest.

**Impact.** The observations may be legitimate and are responsibly scoped as historical, but a fresh clone cannot verify the measurement lineage. This falls short of the root requirement to retain raw data and environment metadata for performance claims and the Agent Harness rule that raw evidence precedes promotion.

**Required revision.** Add non-secret raw artifacts under `sources/`, or add a durable pointer manifest containing exact local authority, filenames, hashes, capture times, host/software/firmware context, and redaction notes. Link each measured row directly to the minimum supporting artifact. If preservation cannot be completed, downgrade the affected claims to `[INFERENCE]` or explicitly labeled historical reports.

**Applied.** Preserved available evidence and a checksum/provenance receipt under `sources/measurements/`. Claims backed only by synthesized audits are now `[VERIFIED]` statements about those historical reports, not measurements. Only the July 10 two-rail/MPTCP result retains `[MEASURED]`, backed by preserved raw logs and environment/state output.

**Affected paths.** Sections 18, 19, and 20; project `sources/`.

### P2 — Mutable documentation is used for version-sensitive compatibility claims

**Evidence.** Section 20 cites Linux `blob/master` and records the revision as “live master” (`20_USB4_Physical_Topology_and_Dual_Port_Independence/sources.md:25-26`). Sections 17, 19, 22, and 23 cite AMD `latest` routes or “current docs”; examples include section 19 source rows 19-25, section 22 lines 50-51, and section 23 lines 18-34, 59, 83, and 90. An access date limits ambiguity but does not make changing content reproducible. Section 11 demonstrates the stronger pattern by resolving exact repository commits and recording lineage discrepancies.

**Impact.** Kernel ABI, ROCm, HIP, and Ryzen support statements can silently drift after review, making `[VERIFIED]` claims and compatibility matrices irreproducible.

**Required revision.** Resolve source-code links to immutable commit URLs; record exact product/document versions; preserve a licensed local snapshot or hash when the publisher exposes no immutable revision. Keep access dates as supplemental metadata.

**Affected paths.** Sections 17, 19, 20, 22, and 23.

### P2 — Claim-label policy is not applied consistently to open-question ledgers

**Evidence.** The root rules require literal claim labels, and `OUTPUT_STANDARD.md` requires material conclusions to be labeled. Question rows in sections 01-05, 11-14, 16, and 18-20 have identifiers and unresolved wording but omit `[OPEN]`; sections 06-10, 15, 17, and 21-23 apply `[OPEN]` explicitly. Section 14 labels three prose conflicts `[OPEN]` (`14_llama_ai_and_CachyLLama_Feature_and_Patch_Inventory/open_questions.md:39-43`) while the table immediately above is unlabeled.

**Impact.** Machines cannot reliably distinguish unresolved claims by parsing labels, and readers must infer status from file location or prose.

**Required revision.** Define one schema field or mandatory label location for question rows, then normalize all ledgers and validate the rule. Avoid relying on directory name as an implicit claim label.

**Affected paths.** `open_questions.md` in sections 01-05, 11-14, 16, and 18-20; governance section 04.

### P2 — Stable identifiers are not actually stable across section implementations

**Evidence.** Most pages use two-digit section identifiers such as `"11"` and `"16"`; section 16 instead records `related_sections` as `"03.11"`, `"03.13"`, `"03.14"`, and `"03.15"` (`16_Build_Dependencies_Licensing_CI_and_AI_Agent_Workflow/section.yaml:17-21`) and repeats that convention across its pages. Open-question IDs also vary materially (`OQ-06-01`, `OQ12-01`, `S14-Q01`, `H16-O01`, `S18-OQ01`, `21-OQ01`) without a registry mapping.

**Impact.** Cross-section graph construction and automated ledger aggregation are ambiguous, directly undercutting governance sections 01 and 03.

**Required revision.** Select a single namespaced ID grammar, publish it in the manifest schema, create an old-to-new mapping, and validate every front-matter relationship and ledger ID.

**Affected paths.** Section 16, governance sections 01 and 03, and all open-question ledgers.

## Contradiction review

The research correctly preserves several contradictions rather than silently resolving them: the historical ROCmFPX integration reference does not cleanly match tag `b9438` in section 11; CachyLLama documents `--cache-ssd-max-cold` as `0` while low-level configuration and `llama-ai` use `32`; hybrid-cache correctness remains unvalidated; and `llama-ai` recommends Vulkan while HaloFPX targets ROCmFPX (`14_llama_ai_and_CachyLLama_Feature_and_Patch_Inventory/open_questions.md:39-43`). These are strengths. They require root-ledger promotion, owners, resolution evidence, and closure criteria. Commit dates should also be normalized to full timestamps plus timezone so UTC July 17 and America/Los_Angeles July 16 do not appear contradictory.

## New research prompts and acceptance questions

1. **Root authority integration:** Can a generated manifest/schema enumerate every category, section, canonical ID, source record, decision, assumption, question, experiment, and supersession while rejecting broken links and duplicate IDs?
2. **Measurement promotion audit:** Which exact raw files and hashes support every `[MEASURED]` row in sections 18-20, and does each capture include host identity, kernel, firmware, BIOS, ROCm/Mesa, command, timestamp, and redaction history?
3. **Volatile-source snapshot:** What immutable commit, product-version page, archive snapshot, or content hash replaces every `latest`, `current docs`, and `master` source used by sections 17-23?
4. **Identifier migration:** What canonical grammar covers section IDs, source IDs, question IDs, decisions, and experiments without ambiguity, and what redirect/mapping preserves existing references?
5. **Contradiction resolution experiments:** At the exact selected commits, what are the effective cache defaults, anonymous-user limits, hybrid/recurrent checkpoint semantics, and ROCm-versus-Vulkan portability boundaries?
6. **Category integration:** Can category READMEs be generated from validated section metadata so status, summaries, and links cannot remain stale after research completion?

## Acceptance gate

The two P1 gates are satisfied. Accept sections 01-23 as an integrated baseline only after the remaining P2 work is complete: normalize machine-readable `[OPEN]` state, migrate invalid/incompatible IDs and section metadata, and pin or snapshot-hash mutable compatibility sources. Cross-platform manifest checking and future typed-ledger validation should be retained under `experiments/` or `reviews/` as acceptance receipts; no unapproved decision or software baseline should be inferred from structural completeness.
