# Open Questions

Every item in the open table is `[UNRESOLVED]` at the research cutoff.

## Resolved ancestry findings retained for audit history

| ID | Label | Finding | Evidence | Impact |
|---|---|---|---|---|
| RF-001 | [VERIFIED] | ROCmFPX merge 2335e6a482b1601d71dff9e860c8feab108c3af2 has first parent 221402af8574faf652b101b6afe225a3f329561f and second parent 5b3956605309dd3e6beed49c8f3a41423ba71d25. | Official GitHub compare resolution of child^1 and child^2; see ancestry_edges.csv A-005 and A-006. | Exact graph edge resolved; only the historical URL behind local remote name upstream remains open. |
| RF-002 | [VERIFIED] | CachyLLama merge 6be745998f568e379ea197fcf827baec73ff9940 has first parent c8ead677a7fe42fb0a67e6e866fb254cc338e9fd and second parent 92366df30d4eaa4b85139b5fd694360237731b19. | Official GitHub compare resolution of child^1 and child^2; second parent independently exists in ggml-org/llama.cpp. | Direct upstream merge ancestry established; former parent-order question closed. |

## Open items

| ID | Priority | Question | Evidence required | Impact |
|---|---|---|---|---|
| OQ-001 | medium | What repository URL did the local remote name `upstream` map to when ROCmFPX merge `2335e6a482b1601d71dff9e860c8feab108c3af2` was created? | Historical `.git/config`, archived clone metadata, reflog/config backup, or maintainer statement. | Controls lineage terminology; ordered parent identities are already verified and basic MIT reuse is unaffected. |
| OQ-002 | high | Is `charlie12345/ROCmFPX` an official GitHub fork-network member, a detached fork, or a standalone repository with imported objects? | Inspect raw REST `fork`, `parent`, `source`, network graph, and owner statement. | Affects terminology and commit-object interpretation. |
| OQ-004 | critical | Does the `GPL-3.0-or-later` declaration cover every llama-ai source file, including files without SPDX headers? | Obtain maintainer clarification or counsel interpretation; add per-file SPDX. | Blocks direct copying. |
| OQ-005 | critical | What exact file set is “Documentation” under CC-BY-NC-SA-4.0? | Maintainer-defined path policy and SPDX/document headers. | Blocks copied documentation. |
| OQ-006 | critical | What licenses and source obligations apply to every file in the exact TheRock nightly archive used by each build script? | Download exact tarball, manifest/SBOM scan, component source map. | Blocks self-contained binary release. |
| OQ-007 | high | Does the ROCm nightly service publish signed checksums, attestations, SBOMs, or consolidated notices for these tarballs? | Official AMD/TheRock release metadata or maintainer statement. | Supply-chain and license evidence. |
| OQ-008 | critical | What are all npm dependency and asset licenses in the built/prebuilt WebUI? | `npm ci`, SBOM/license report, asset scan, prebuilt source correspondence. | Blocks UI shipping. |
| OQ-009 | high | Are all model templates and legacy converter files authorized under their displayed headers, and do model-provider terms add restrictions? | Per-file upstream history and provider license review. | Blocks affected templates/tools. |
| OQ-010 | high | Which ROCmFPX files were substantially AI-generated versus assisted, and did contributors have authority under provider terms? | Session records, human review attestations, tool terms, contributor declarations. | Material core-code review. |
| OQ-011 | high | What contribution agreement, signoff, or inbound=outbound policy governs each repository? | Repository settings, maintainer policy, contributor records. | Chain-of-title confidence. |
| OQ-012 | critical | Do ROCmFPX formats, codebooks, scale selection, kernels, or cache techniques implicate patents? | Patent search and counsel freedom-to-operate review. | Blocks commercial release if material. |
| OQ-013 | high | What project name and branding can be used without implying AMD/Meta/other endorsement? | Trademark policy review and naming clearance. | Blocks branding. |
| OQ-014 | medium | Which license option should be elected for every dual/public-domain component? | Documented project policy and jurisdiction review. | Notice consistency. |
| OQ-015 | high | Which checked-in or embedded files are generated, and what source form is required to rebuild them? | Generator/input manifest and reproducible build. | License/source completeness. |
| OQ-016 | critical | Does a complete local license scan reveal additional file-specific licenses beyond those observed here? | Run ScanCode/REUSE/manual review on exact release tree. | Blocks final notice approval. |
| OQ-017 | high | Are Qualcomm SDK headers/libraries required or redistributed for the restored Hexagon backend, and under what terms? | Exact SDK package/license and binary distribution plan. | Blocks Hexagon builds/releases. |
| OQ-018 | medium | Are benchmark inputs, logs, recipes, and outputs cleared for publication and commercial use? | Dataset/model/output terms and privacy review. | Blocks affected documentation/data. |
| OQ-019 | medium | Does the canonical project need any llama-ai functionality that cannot be independently specified without consulting its CC-NC docs or GPL code? | Feature inventory and clean-room specification review. | Determines architecture. |
| OQ-020 | high | What exact upstream replacements now exist for ROCmFPX snapshot-restored code, and can snapshots be eliminated? | Path-by-path diff against current upstream and topic-branch migration plan. | Reduces provenance and maintenance risk. |

## Closure protocol

1. Add the new primary source to `data/source_ledger.csv`.
2. Update the relevant fact with `[VERIFIED]` or `[INFERENCE]`.
3. Store raw command/API output or counsel memorandum in the project compliance archive.
4. Move resolved factual questions into `data/resolved_findings.csv`; do not erase the audit trail.
5. Update `section.yaml` review date and regenerate `MANIFEST.sha256`.
