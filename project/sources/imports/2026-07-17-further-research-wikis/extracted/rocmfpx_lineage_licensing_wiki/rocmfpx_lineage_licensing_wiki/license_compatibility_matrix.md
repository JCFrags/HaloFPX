# License Compatibility Matrix

> The matrix describes the inspected facts and conservative release posture. “Allowed” means the identified license grants the relevant copyright permission when its conditions are satisfied; it does not resolve patents, trademarks, contributor authority, export rules, or derivative-work disputes.

## Action matrix for a canonical MIT-oriented project

| Material | Copy into MIT-oriented repository | Modify | Link/ship together | Clean-room reimplement | Document facts | Required handling | Review status |
|---|---|---|---|---|---|---|---|
| MIT code from ROCmFPX, CachyLLama, llama.cpp, ggml | **Yes** | **Yes** | **Yes** | Yes | Yes | Preserve copyright and permission notice in source/substantial portions. Keep provenance. | Routine notice review |
| Apache-2.0 files | **Yes, as Apache files in a multi-license distribution** | **Yes** | **Yes** | Yes | Yes | Include Apache-2.0 text; preserve notices; mark modified files; honor any NOTICE content; retain patent/termination terms. Do not relabel origin as MIT-only. | Notice and patent review |
| Apache-2.0 WITH LLVM-exception files | **Yes, with exact exception** | **Yes** | **Yes** | Yes | Yes | Preserve Apache-2.0 and LLVM exception text/SPDX expression; mark modifications. | Legal confirms exception packaging |
| BSD-2-Clause files such as xxHash | **Yes** | **Yes** | **Yes** | Yes | Yes | Preserve source notice; reproduce notice/disclaimer in binary documentation/materials. | Routine notice review |
| MIT-0 option, e.g. miniaudio alternative | **Yes** | **Yes** | **Yes** | Yes | Yes | Record election of MIT-0 rather than relying on uncertain public-domain effect; retain source/provenance. | Routine review |
| MIT/public-domain dual option, e.g. stb_image | **Yes** | **Yes** | **Yes** | Yes | Yes | Elect MIT for predictable global treatment and preserve notice. | Routine review |
| Public-domain/Unlicense-style-only source, e.g. subprocess.h | Usually | Usually | Usually | Yes | Yes | Preserve dedication/text; confirm enforceability and fallback rights in target jurisdictions. | `[LEGAL-REVIEW]` |
| llama-ai files with SPDX `GPL-3.0-or-later` | **No for MIT-only copy**; yes in GPL-covered component | Yes under GPL conditions | Linking/combined-work status is fact-specific; conservative position is GPL for combined derivative | **Yes**, from behavior/facts without copied expression | Yes, in original words | Preserve GPL notices; provide corresponding source and required installation information when applicable; no additional restrictions. | `[LEGAL-REVIEW]` before integration |
| Unmarked llama-ai source relying on README + GPLv3 `LICENSE` | Do not copy until scope confirmed | Same | Same | Yes | Yes | Confirm whether grant is GPL-3.0-only or GPL-3.0-or-later and which files are “source code.” | Release blocker |
| llama-ai documentation under CC-BY-NC-SA-4.0 | **No for commercially usable permissive docs without permission** | Only for noncommercial BY-NC-SA adaptation | N/A | Rewrite facts independently | **Yes**, factual reporting; do not copy protectable prose | Attribution, noncommercial use, ShareAlike, modification indication, license link. | Release blocker for copied prose |
| TheRock repository build-system source (MIT) | Yes | Yes | Yes | Yes | Yes | MIT notice for build-system files. | Routine |
| Downloaded TheRock/ROCm SDK tarball | **Unknown as a unit** | Component-specific | Component-specific | N/A | Yes | Inventory every component/file; preserve each license/source offer/notice; record checksums. Root TheRock MIT is not blanket coverage. | Release blocker |
| ROCm runtime libraries copied beside binaries | Component-specific | Usually not modified | Dynamic/static linkage and redistribution component-specific | N/A | Yes | Map each `.so`, kernel library, and bundled dependency to source/version/license; collect notices and source obligations. | Release blocker |
| Current llama.cpp WebUI source | Yes under repository/file licenses | Yes | Yes | Yes | Yes | Also audit npm dependencies and generated assets. | Conditional |
| npm transitive dependencies / built WebUI assets | Mixed | Mixed | Mixed | N/A | Yes | Produce lockfile-derived license report/SBOM; preserve asset notices; block prohibited/incompatible packages. | Release blocker if UI shipped |
| Prebuilt `llama-ui` tarball from remote bucket | Unknown until audited | N/A | Ship only after provenance and licenses verified | N/A | Yes | Verify signed/pinned digest, source correspondence, dependency notices, and build recipe. Same-origin `.sha256` is integrity, not identity/authenticity. | Release blocker |
| Apache-licensed model templates and converter scripts | Yes with exact notice | Yes | N/A | Yes | Yes | Preserve file header, source trail, and modification markers; check model-provider terms for copied prompt/template content. | File-specific review |
| Model weights, tokenizer files, chat templates downloaded with models | **Not covered by software repo license by default** | Separate terms | Separate terms | N/A | Yes | Obtain model-specific license/acceptable-use rights; track provenance and redistribution restrictions. | Release blocker when bundled |
| AI-assisted code already accepted under repository license | Copyright license appears to apply as committed | Yes | Yes | N/A | Yes | Confirm human review, contributor authority, provider terms, and provenance; retain assisted-by records. | `[LEGAL-REVIEW]` for material core code |
| Generated code/assets | Depends on generator and inputs | Depends | Depends | N/A | Yes | Track generator/input licenses, generated-file headers, reproducible build, and source form. | Conditional blocker |
| Names, logos, “ROCm,” “AMD,” “Llama,” “Meta,” “CachyLLama” | Code license does not grant trademark rights | N/A | N/A | N/A | Nominative factual use may be possible | Avoid endorsement; follow trademark policies; choose a distinct project name. | `[LEGAL-REVIEW]` |
| ROCmFPX algorithms/formats independently reimplemented | No source copying needed for ideas/methods | Yes | Yes | **Yes** | Yes | Clean-room records; patent search/opinion; avoid copied expression and confidential material. | Patent + clean-room review |

## Compatibility conclusions

### MIT core + permissive file exceptions

[VERIFIED] MIT, Apache-2.0, Apache-2.0 WITH LLVM-exception, BSD-2-Clause, MIT-0, and appropriately handled public-domain alternatives can coexist in one distribution. The distribution is **multi-license** at file/component level; a root MIT file must not erase the other terms.

### MIT code inside a GPL distribution

[VERIFIED] GNU's official FAQ states that GPL-compatible permissive code can be combined into a larger GPL-covered program and distributed under the GPL conditions applicable to the combination. Apache's official compatibility statement describes Apache-2.0 as compatible with GPLv3 in that direction.

[RECOMMENDATION] A project choosing to copy llama-ai GPL code should publish a clear GPL distribution architecture rather than representing the combined source as MIT.

### GPL tooling beside an MIT engine

[INFERENCE] A GPL build/run script that invokes or compiles a separate MIT repository can remain a separately licensed component when distributed as genuine aggregation. Whether a particular tightly integrated packaging arrangement is one combined work is a legal determination. Keep process boundaries, repositories, licenses, and source archives explicit.

### CC documentation

[VERIFIED] CC-BY-NC-SA-4.0 grants sharing and adaptation only for noncommercial purposes and imposes ShareAlike. Creative Commons currently lists no non-CC compatible license for BY-NC-SA 4.0 adaptations.

[RECOMMENDATION] Write canonical documentation from primary facts and independently phrased procedures. Link to llama-ai documentation instead of copying its prose. Obtain separate permission for any excerpt beyond minimal factual quotation.

### Reimplementation

[VERIFIED] 17 U.S.C. §102(b) states that copyright does not extend to ideas, procedures, processes, systems, methods of operation, concepts, principles, or discoveries. It does protect original expression and derivative expression.

[LEGAL-REVIEW] Clean-room reimplementation does not eliminate patent, trademark, contract, trade-secret, or interoperability-law issues. Review the quantization formats and optimized kernels for patent exposure before a commercial release.
