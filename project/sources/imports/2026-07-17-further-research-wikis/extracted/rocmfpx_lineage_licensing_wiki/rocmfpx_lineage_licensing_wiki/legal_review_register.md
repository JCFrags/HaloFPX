# Legal Review Register

> This register identifies every legal-review area observed in the inspected scope. It is not a universal list of all laws that could apply in every jurisdiction.

## Active and conditional review items

| ID | Severity | Area | Required decision/evidence | Release posture | Status |
|---|---|---|---|---|---|
| LR-001 | blocker | llama-ai GPL version/scope | Confirm whether every unmarked source file is GPL-3.0-or-later or GPL-3.0-only; reconcile README/SPDX declarations with bare GPLv3 LICENSE text. | Do not copy unmarked llama-ai source into canonical project until rights-holder or counsel confirms. | Open |
| LR-002 | blocker | llama-ai documentation boundary | Identify exactly which files and embedded snippets are CC-BY-NC-SA-4.0, and whether code comments/examples are documentation. | Exclude copied prose or obtain separate permission. | Open |
| LR-003 | blocker | GPL/MIT combination and linking | Determine whether proposed wrapper, library, plugin, IPC, installer, and source-tree arrangement is a combined/derivative work or aggregation. | Counsel-approved architecture and release license. | Open |
| LR-004 | blocker | GPL corresponding source | If any GPL binary/component is distributed, define complete corresponding source and installation-information obligations. | Automated source bundle and written offer/process where required. | Conditional |
| LR-005 | blocker | ROCm/TheRock tarball component rights | Audit every component/file in the exact nightly tarball; root TheRock MIT is not blanket coverage. | Component manifest, source revisions, licenses, notices, checksums. | Open |
| LR-006 | blocker | Bundled ROCm runtime redistribution | Review each copied .so, LLVM/OpenMP library, kernel library, and dependency from build script. | Approved binary manifest and notice/source package. | Open |
| LR-007 | high | Incomplete third-party notices | Current ROCmFPX notices omit observed file-level and vendored components. | Generated NOTICE covering all shipped source and binaries. | Open |
| LR-008 | high | Apache-2.0 and LLVM exception compliance | Preserve license/NOTICE, modification statements, patent terms, and exact LLVM exception. | File mapping and LICENSES bundle. | Open |
| LR-009 | high | BSD-2-Clause binary notice | Ensure xxHash notice/disclaimer accompanies binaries. | Notice test in release CI. | Open |
| LR-010 | high | Public-domain/Unlicense jurisdiction | Confirm fallback/enforceability for subprocess.h and any public-domain election. | Counsel-approved license election or replacement. | Open |
| LR-011 | high | Unpinned vendor sources | nlohmann latest and stb master produce non-reproducible future imports. | Pin commits/archive hashes and review new licenses before sync. | Open |
| LR-012 | blocker | npm/WebUI transitive licenses | Audit every dependency and generated asset in shipped UI. | SPDX SBOM, license report, notices, prohibited-license gate. | Open if UI ships |
| LR-013 | blocker | Prebuilt UI provenance | Same-origin SHA-256 does not establish source correspondence or signed provenance. | Reproducible build or trusted attestation plus notices. | Open if prebuilt UI ships |
| LR-014 | high | Model templates and converter provenance | Review Apache templates, HuggingFace-derived scripts, copied prompts, and provider terms. | Per-file source/license mapping and modification notices. | Open |
| LR-015 | blocker | Model weights/tokenizers | Software repository licenses do not automatically cover model assets. | Model-by-model license approval and distribution record. | Open if assets ship |
| LR-016 | high | AI-assisted code title and provider terms | Confirm human authorship/review, contributor authority, and tool terms for material AI-assisted changes. | Signed contributor/provenance review. | Open |
| LR-017 | high | Contributor authority / no located CLA-DCO | Many inherited and local contributors; no CLA/DCO text located in inspected files. | Confirm contribution terms and authority, especially for nontrivial local imports. | Open |
| LR-018 | blocker | Patent exposure | MIT lacks an express patent grant; quantization formats, kernels, and hardware interfaces may be patented. | Freedom-to-operate/patent opinion for commercial release. | Open |
| LR-019 | high | Trademark and endorsement | ROCm, AMD, Llama, Meta, product names and logos are not granted by code licenses. | Distinct naming and trademark-policy review. | Open |
| LR-020 | high | Clean-room reimplementation | Reimplementing GPL/CC behavior must avoid copied expression and preserve process evidence. | Approved clean-room protocol and similarity review. | Open when used |
| LR-021 | high | Generated files and embedded assets | Trace generated shaders, UI bundles, wrappers, codegen, fonts, icons, and compressed assets to inputs/licenses. | Reproducible build and generated-file manifest. | Open |
| LR-022 | medium | ROCmFPX remote-name and lineage terminology | Determine the historical URL assigned to local remote name `upstream` at merge 2335e6a482b1601d71dff9e860c8feab108c3af2; ordered parents are already verified. | Do not describe that merge as a ggml-org/llama.cpp merge absent historical remote evidence. | Open |
| LR-024 | medium | GitHub fork-network status | Repository page/API normalization did not conclusively establish whether ROCmFPX is in an official GitHub fork network. | Raw repository API parent/source fields or owner confirmation. | Open |
| LR-025 | high | Snapshot restoration provenance | Restored WebGPU/Hexagon files may lack per-file headers and depend on external SDKs. | Blob-level source map, external SDK licenses, notices. | Open |
| LR-026 | high | Qualcomm Hexagon SDK interfaces | Hexagon code includes HAP/AEE/dspqueue/rpcmem headers and requires external SDK/runtime terms. | SDK license and redistribution/build-use review. | Open if backend ships |
| LR-027 | medium | Documentation/data/benchmark rights | Benchmarks, CSV recipes, screenshots, datasets, logs, and model outputs can have separate rights/privacy issues. | Asset-specific provenance and publication approval. | Open if included |
| LR-028 | medium | Export controls and sanctions | Cross-border distribution of GPU/AI software and binaries may trigger jurisdiction-specific controls. | Trade-compliance classification for intended markets. | Conditional |
| LR-029 | high | Security/update authenticity | Nightly tarballs and some vendor downloads lack independent signatures/pins in observed scripts. | Pinned hashes, trusted signatures/attestations, update policy. | Open |
| LR-030 | high | Reverse-port attribution | Upstream Hy3 now derives from ROCmFPX; future imports can obscure original source chain. | Retain commit 2969d6d source statement and co-author credit. | Open governance item |

## Resolved factual item removed from the active gate list

- [VERIFIED] Former LR-023, CachyLLama merge-parent order, is closed: `6be745998f568e379ea197fcf827baec73ff9940` has first parent `c8ead677a7fe42fb0a67e6e866fb254cc338e9fd` and second parent `92366df30d4eaa4b85139b5fd694360237731b19`. The evidence remains in `data/resolved_findings.csv` and `data/ancestry_edges.csv`.

## Severity meanings

- **blocker:** no public/commercial release of the affected component until closed.
- **high:** material infringement/compliance risk; counsel or rights-owner decision normally required.
- **medium:** evidence/governance gap that can become a blocker depending on release scope.

## Closure requirements

A register item is closed only when the project stores:

1. the exact release artifact or source SHA reviewed;
2. the question presented to counsel/rights holder;
3. written conclusion and conditions;
4. implementation evidence satisfying those conditions;
5. approver name, authority, and date;
6. a trigger for re-review when source, dependency, packaging, or distribution model changes.

Machine-readable entries are in [`data/legal_review_register.csv`](data/legal_review_register.csv).
