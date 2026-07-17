# Design Implications

## Recommended canonical architecture

[RECOMMENDATION] Use a componentized, multi-license repository with an MIT core and explicit third-party boundaries:

```text
canonical-rocmfpx/
├── LICENSE                         # project-authored MIT material
├── LICENSES/                       # Apache-2.0, LLVM exception, BSD, MIT-0, vendor texts
├── NOTICE                          # generated release notice
├── provenance/                     # one record per imported component/change
├── core/                           # ROCmFPX/llama.cpp-derived MIT core
├── third_party/                    # unchanged or lightly modified permissive components
├── tools/                          # project-authored MIT tooling
├── packaging/                      # manifests only; no unaudited binary blobs
├── docs/                           # original MIT/CC-BY documentation, not copied NC prose
└── companions/
    └── llama-ai-gpl/               # optional separately distributed GPL integration, if adopted
```

## Distribution model choices

### Model A — MIT-oriented core, no llama-ai copying

**Use when:** commercial/permissive reuse and upstream contribution are priorities.

- [RECOMMENDATION] Start from ROCmFPX and/or current upstream/CachyLLama code under their MIT grants.
- [RECOMMENDATION] Reimplement useful llama-ai orchestration behavior from functional requirements; do not copy GPL script expression.
- [RECOMMENDATION] Write documentation independently from primary sources; do not adapt CC-BY-NC-SA prose.
- [RECOMMENDATION] Keep ROCm SDK acquisition out of release archives unless a complete component audit exists.

**Result:** Project-authored and MIT-origin source can remain MIT, while inherited Apache/BSD/etc. files retain their own licenses.

### Model B — GPL-covered integrated distribution

**Use when:** direct reuse of llama-ai scripts and tight integration are more important than permissive downstream use.

- [VERIFIED] MIT/Apache/BSD components can generally be included in a GPLv3-compatible distribution with their original notices.
- [RECOMMENDATION] Declare the covered combined work GPL-3.0-or-later only after confirming the actual grant scope.
- [RECOMMENDATION] Automate corresponding-source collection, build scripts, installation information when applicable, and license delivery.
- [LEGAL-REVIEW] Define which pieces are one covered work and which are separate aggregation.

**Result:** Do not market or publish the integrated derivative as MIT-only.

### Model C — Aggregated MIT engine plus separate GPL companion

**Use when:** users need the existing llama-ai behavior but the engine must stay permissive.

- Preserve separate repositories, archives, licenses, version numbers, and executable/process boundaries.
- Do not copy GPL source into MIT files.
- Do not make the GPL component a hidden mandatory library inside the MIT binary.
- Explain installation as separate components.

[LEGAL-REVIEW] Aggregation versus one combined work remains fact-specific; obtain counsel review of the actual IPC, packaging, and installer.

## Upstream strategy

[RECOMMENDATION] Treat current upstream llama.cpp as the integration target and ROCmFPX as the feature/provenance source, not as a permanently divergent monolith.

1. Preserve ROCmFPX commits that are genuinely local.
2. Replace manual snapshots with upstream commits where equivalent code now exists.
3. Avoid reimporting upstream Hy3 code without preserving its reverse provenance back to ROCmFPX.
4. Submit separable ROCmFPX features upstream under the existing MIT terms where technically viable.
5. Maintain a patch queue or topic branches rather than repeated whole-tree snapshot restoration.

## License architecture

[RECOMMENDATION] Add SPDX headers to all new project-authored files and do not add a single global SPDX expression to files with inherited exceptions.

Suggested policy:

- New canonical core source: `SPDX-License-Identifier: MIT`.
- Imported Apache source: retain `Apache-2.0` or `Apache-2.0 WITH LLVM-exception`.
- Imported BSD source: retain `BSD-2-Clause`.
- Dual-option source: document the chosen option in provenance metadata.
- GPL companion: `GPL-3.0-or-later`, physically and logically separate.
- Documentation: use MIT or CC-BY-4.0 for newly written docs; avoid NC terms if commercial reuse is intended.

## Provenance data model

Each imported change should record:

```yaml
id: PROV-0001
source_repository: https://github.com/owner/repo
source_commit: 40-character-sha
source_paths:
  - path/to/file
import_method: merge | cherry-pick-x | manual-port | blob-copy | generated
source_license: SPDX-expression
source_authors:
  - name
local_commit: 40-character-sha
modifications: concise description
verification:
  exact_blob: true | false
  tests:
    - command/result
legal_review: none | pending | approved
```

A template is included at [`templates/PROVENANCE_RECORD.template.yaml`](templates/PROVENANCE_RECORD.template.yaml).

## Vendor and supply-chain design

[RECOMMENDATION] Replace moving vendor URLs with immutable revisions and pinned digests. Store a small manifest, not only a Python synchronization script.

Minimum fields:

- upstream repository and immutable commit/tag;
- exact download URL;
- SHA-256 of source archive and checked-in blob;
- license expression and license-file path;
- notice token;
- local modifications;
- last reviewed date.

[RECOMMENDATION] Prefer building WebUI assets from a locked dependency graph with an auditable license report. A checksum hosted beside a prebuilt tarball protects against accidental corruption but does not independently establish origin or license completeness.

## ROCm runtime design

[RECOMMENDATION] The default release should require a separately installed ROCm runtime or an independently distributed, audited runtime package. Do not copy every library matched by a shell glob into the canonical project archive.

When a self-contained runtime is essential:

1. generate a file-level manifest before copying;
2. map every file to component repository/version/license;
3. include source links and required source offers;
4. include all license/NOTICE files;
5. record SHA-256 and dependency graph;
6. test that no system/proprietary file was accidentally captured;
7. obtain counsel approval for the exact archive.

## Documentation design

[RECOMMENDATION] Keep technical facts, benchmark methodology, and operational procedures in newly authored prose under a permissive documentation license. Use exact source links and short factual quotations only when necessary.

Do not:

- paste llama-ai README/AGENTS sections into canonical docs;
- assume code-license terms cover model cards, screenshots, datasets, or model weights;
- use vendor names/logos in a way that implies sponsorship;
- remove attribution files because Git history exists.

## Clean-room implementation design

For GPL/CC material that cannot be copied into the desired license:

1. evidence team writes functional requirements using public behavior and unprotectable facts;
2. implementation team has no access to copied source/prose during coding;
3. tests are written from observable inputs/outputs or independent specifications;
4. reviewers compare for suspicious expression-level similarity;
5. all participants sign a provenance declaration;
6. patent counsel reviews the functional area.

## Release decision rule

[RECOMMENDATION] No release is “canonical” until all blockers in [`legal_review_register.md`](legal_review_register.md) are closed or explicitly waived in writing by authorized counsel and the project owner.
