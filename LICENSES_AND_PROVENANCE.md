# HaloFPX license and provenance boundary

Status: **private preservation and engineering repository; not cleared for public redistribution**.

This file records a conservative boundary. It is not legal advice, does not grant
permission, and does not replace the license or terms that apply to an individual
file, component, model, document, capture, or binary. Preservation in this private
repository does not relicense third-party material.

## Repository boundary

The repository combines two materially different bodies of work:

1. The implementation source at the repository root is derived from
   `llama.cpp`/`ggml` and ROCmFPX. It is presented under the root
   [`LICENSE`](LICENSE) (MIT), subject to any more-specific file-level license,
   copyright notice, bundled-component terms, and provenance record. The current
   known-notice summary is [`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md),
   with original texts also retained for
   [`cpp-httplib`](vendor/cpp-httplib/LICENSE),
   [`nlohmann/json`](licenses/LICENSE-jsonhpp), and
   [`gguf-py`](gguf-py/LICENSE). That notice summary is not an exhaustive SBOM or
   a conclusion that every file in a release is MIT.
2. [`project/`](project/) is an engineering wiki, research record, governance
   history, evidence store, and collection of preserved imports. It has no
   blanket MIT grant. Imported packages and captures retain their original
   licenses or terms. Where no outbound license has been established, the
   status is `NOASSERTION`/unresolved rather than MIT by default.

The root MIT license therefore must not be read as covering all material under
`project/sources/imports/`, formal-verification tool archives, model artifacts,
vendor documents, standards captures, generated Web UI bundles, toolchains, or
runtime packages.

## Known license and terms families

The following families are present in, or recorded by the audits for, the
combined preservation set. A family appearing here is an inventory fact, not a
compatibility decision or permission to redistribute a particular artifact.

| Family | Recorded scope and required treatment | Primary local evidence |
|---|---|---|
| MIT | Root implementation assertion and several bundled/source components. Preserve copyright and permission notices. Check explicit headers and component boundaries before treating a file as MIT. | [`LICENSE`](LICENSE), [`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md), and the pinned donor/component inventory in [`components.csv`](project/sources/imports/2026-07-18-pre-fork-internet-research/extracted/PF-IR-04-licensing-dossier/PF-IR-04-licensing-dossier/manifests/components.csv) |
| GPL-3.0-or-later | Recorded for sampled `fewtarius/llama-ai` operational code. It is reference/provenance material, not approved for copying into the MIT implementation. Do not relabel it because it invokes an MIT engine. | [`components.csv`](project/sources/imports/2026-07-18-pre-fork-internet-research/extracted/PF-IR-04-licensing-dossier/PF-IR-04-licensing-dossier/manifests/components.csv) and [`03-code-license-evidence.md`](project/sources/imports/2026-07-18-pre-fork-internet-research/extracted/PF-IR-04-licensing-dossier/PF-IR-04-licensing-dossier/wiki/03-code-license-evidence.md) |
| CC-BY-NC-SA-4.0 | Recorded for `fewtarius/llama-ai` documentation by its README assertion. NonCommercial and ShareAlike conditions are separate from the code license. Treat copied prose as blocked unless its exact scope and use are approved; independently written factual summaries are not a license shortcut for copied expression. | [`components.csv`](project/sources/imports/2026-07-18-pre-fork-internet-research/extracted/PF-IR-04-licensing-dossier/PF-IR-04-licensing-dossier/manifests/components.csv) and the preserved canonical-terms receipt [`08-cc-by-nc-sa-4.0-canonical-terms.capture.md`](project/sources/imports/2026-07-18-pre-fork-internet-research/extracted/PF-IR-04-licensing-dossier/PF-IR-04-licensing-dossier/raw/upstream/08-cc-by-nc-sa-4.0-canonical-terms.capture.md) |
| Apache-2.0, including Apache-2.0 WITH LLVM-exception where stated | Recorded for multiple imported source families and file-level backend/tooling exceptions. Preserve exact notices, modification markings, applicable NOTICE content, and any exception text. Do not convert these files to MIT by aggregation. | [`09-licenses-and-provenance.md`](project/sources/imports/2026-07-18-pre-fork-internet-research/extracted/PF-IR-11_XDNA2_Linux_Wiki_2026-07-18/PF-IR-11_XDNA2_Linux_Wiki_2026-07-18/wiki/09-licenses-and-provenance.md), [`licenses.csv`](project/sources/imports/2026-07-18-pre-fork-internet-research/extracted/PF-IR-11_XDNA2_Linux_Wiki_2026-07-18/PF-IR-11_XDNA2_Linux_Wiki_2026-07-18/manifests/licenses.csv), and [`03-code-license-evidence.md`](project/sources/imports/2026-07-18-pre-fork-internet-research/extracted/PF-IR-04-licensing-dossier/PF-IR-04-licensing-dossier/wiki/03-code-license-evidence.md) |
| EPL-2.0 | Reported by the preservation audit among dependencies inside external verification-tool distributions. It is not an implementation-tree license assertion. The checked-in intake receipts identify the tool bytes but are not a complete dependency-license inventory; collect the exact embedded license/notice files before redistributing a tool archive. | Pinned tool receipts: [`Apalache v0.57.0`](project/sources/tools/apalache/v0.57.0/receipt.md) and [`TLA+ Tools v1.7.4`](project/sources/tools/tlaplus/v1.7.4/receipt.md) |
| GPLv2 with Classpath Exception | Reported by the preservation audit among Java runtime/tool dependencies used for formal verification. It does not apply to HaloFPX merely because a verifier was run. Preserve the exact runtime/package license and exception, and do not bundle a Java runtime until its exact distribution and notices have been inventoried. | The runtime identity and external-tool boundary are recorded in the [`TLA+ Tools v1.7.4 receipt`](project/sources/tools/tlaplus/v1.7.4/receipt.md); the receipt is not itself the complete runtime license text. |
| Vendor/EULA, custom, and noncommercial terms | Account-gated AMD packages, model-specific assets, and other vendor material require artifact-specific review. A repository or base-model license does not prove rights for a converted, quantized, packaged, or mirrored artifact. MiniMax-M2.7 is explicitly held in the research matrix because custom noncommercial and use restrictions remain. | [`XDNA2 licenses.csv`](project/sources/imports/2026-07-18-pre-fork-internet-research/extracted/PF-IR-11_XDNA2_Linux_Wiki_2026-07-18/PF-IR-11_XDNA2_Linux_Wiki_2026-07-18/manifests/licenses.csv) and the model [`LICENSE_MATRIX.csv`](project/sources/imports/2026-07-18-pre-fork-internet-research/extracted/PF-IR-05_llm_wiki_2026-07-18/PF-IR-05_llm_wiki_2026-07-18/licenses/LICENSE_MATRIX.csv) |
| All-rights-reserved or no-redistribution-license-inferred captures | Some vendor documents are retained only as evidence. No redistribution permission is inferred from public availability or local capture. | The Micron/Crucial source receipt records the all-rights-reserved notice and evidentiary-only treatment in [`CRUCIAL-P310-PRODUCT-FLYER.md`](project/sources/imports/2026-07-18-pre-fork-internet-research/extracted/PF-IR-09_NIMO-MME3L_authority-map_2026-07-18/PF-IR-09_NIMO-MME3L_authority-map_2026-07-18/04_storage_crucial_micron/sources/CRUCIAL-P310-PRODUCT-FLYER.md) |

Other file-level families are also represented in research captures, including
GPL-2.0 variants, LGPL variants, BSD variants, MIT-0, public-domain/Unlicense-
style grants, Linux syscall-note expressions, model-specific terms, standards
terms, and documentation-site terms. The source-specific inventory remains
authoritative for those files; see, for example,
[`SOURCE-LICENSE-INVENTORY.md`](project/sources/imports/2026-07-18-pre-fork-internet-research/extracted/PF-IR-06-HaloKV-crash-durability-wiki-2026-07-18/PF-IR-06-HaloKV-crash-durability-wiki-2026-07-18/licenses/SOURCE-LICENSE-INVENTORY.md).

The portable Qwen3-0.6B ROCmFPX fixture is a separately identified model-
artifact lane. Its exact-revision Unsloth distribution card declares
Apache-2.0 and names `Qwen/Qwen3-0.6B` as the base model. The tracked
[`fixture license and provenance record`](docs/halofpx/fixtures/qwen3-0.6b-rocmfpx/README.md)
preserves the license text and exact artifact identities. The card does not
encode the upstream base-checkpoint revision used to create the BF16 GGUF, so
the immutable distribution revision and GGUF hash—not an inferred base-model
revision—are the source authority. No model bytes are included in Git or yet
approved/published as release assets.

## Provenance rules for continued work

- Use the most specific available evidence in this order: explicit file SPDX or
  header; colocated license/notice; exact donor-history record; repository-level
  assertion; then `NOASSERTION`.
- Preserve exact source URL, commit/tag, blob or artifact hash, capture date,
  original path, and license/notice files. Do not replace a pinned receipt with a
  moving “latest” reference.
- Keep GPL and noncommercial documentation material as reference-only unless a
  separately approved distribution design says otherwise. Behavioral
  reimplementation requires the project's documented provenance and clean-room
  controls; renaming or paraphrasing copied code is not clean-room work.
- Treat model weights, tokenizers, templates, generated fixtures, test corpora,
  downloaded SDKs, runtime libraries, firmware, and binary packages as separate
  artifacts. Their publisher, exact revision, transformation chain, hashes,
  terms, and required notices must be recorded before distribution.
- Treat build outputs and generated bundles as compositions of their exact
  inputs. A source-tree license does not automatically cover the generated
  artifact or all dependencies embedded in it.
- Preserve third-party notices and license texts with archives. If a receipt
  states that the license set is incomplete, that archive remains blocked for
  redistribution.

## Public-release gate

This repository and any GitHub Releases attached to it **must remain private**
until an authorized redistribution review examines the exact proposed public
tree and artifacts. Making the repository private is a containment measure, not
permission to possess, copy, or distribute material under third-party terms.

Before changing visibility or exporting a release, the reviewer must at least:

1. generate an exact file/component SBOM and complete license/notice inventory;
2. resolve `NOASSERTION`, custom/noncommercial, all-rights-reserved, EULA,
   standards, model, firmware, and binary-package items for the exact bytes;
3. decide the GPL and CC-BY-NC-SA boundaries and verify that restricted material
   is absent from an MIT-oriented public tree unless deliberately licensed and
   packaged under its own terms;
4. assemble and verify every required license, NOTICE, source offer, attribution,
   modification statement, and provenance record;
5. review trademarks, patents, contracts, privacy identifiers, export controls,
   and confidential/restricted evidence separately from copyright licensing;
6. record a dated, scoped approval naming the exact commit and release-asset
   hashes.

Until those steps are complete, use this repository for private engineering and
evidence preservation only. Do not describe the combined monorepo as “MIT
licensed” without the implementation/research boundary stated above.
