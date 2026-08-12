# Facts and Constraints

## 1. Repository identities and tips

| ID | Repository | Branch | Immutable tip | Claim |
|---|---|---|---|---|
| R1 | `charlie12345/ROCmFPX` | `main` | `a5605a72768c6562241b248e268e33dc92787394` | [VERIFIED] Public active repository; top-level MIT file. |
| R2 | `fewtarius/llama-ai` | `main` | `1017f3dfdce3ca2b06aa9007b23295db3bb35722` | [VERIFIED] GPL wrapper/orchestration repository with one engine submodule. |
| R3 | `fewtarius/CachyLLama` | `master` | `6be745998f568e379ea197fcf827baec73ff9940` | [VERIFIED] Exact repository intended by “CachyLLama”; MIT fork of llama.cpp. |
| R4 | `ggml-org/llama.cpp` | `master` | `86d86ed4396b4130922f7b9af26e3d9fc11a591b` | [VERIFIED] Current upstream repository at cutoff. |

## 2. Git ancestry and upstream relationships

### ROCmFPX

[VERIFIED] `a5605a72768c6562241b248e268e33dc92787394` is the merge commit for PR #32, “Integrate HY3 MTP, ROCmFP2, and cross-platform server fixes.” PR metadata records 41 unsquashed topic commits and 73 changed files.

| Merge object | Ordered first parent | Ordered second parent | Verified interpretation |
|---|---|---|---|
| `a5605a72768c6562241b248e268e33dc92787394` | `25c71fc6e12d73bb3804127e032d29fb8976ae40` | `a8b5fa906ccd13c6a8ca06d55aa287854c376868` | PR #32 base and topic head; 41 topic commits plus merge. |
| `c2845bf86a5c1842d33bd9e990b2bcaf75779251` | `5b3956605309dd3e6beed49c8f3a41423ba71d25` | `ccac6e55ec7c0fa8acdcb6e80b1a242e4f7d654e` | PR #27 promotion base and experimental topic head; 53 topic commits plus merge. |
| `2335e6a482b1601d71dff9e860c8feab108c3af2` | `221402af8574faf652b101b6afe225a3f329561f` | `5b3956605309dd3e6beed49c8f3a41423ba71d25` | Local remote-tracking merge. The commit text names `upstream/main`, but no remote URL is stored in the commit. |

[VERIFIED] PR #27 states that original commits and authorship were preserved and that coherent source snapshots were restored against llama.cpp revision `5fd2dc2c41c342a75c26f9756ca6b1814ed05fb4`.

[VERIFIED] `2335e6a482b1601d71dff9e860c8feab108c3af2` is an ancestor of current tip `a5605a72768c6562241b248e268e33dc92787394`. Its second parent, `5b3956605309dd3e6beed49c8f3a41423ba71d25`, is a ROCmFPX PR #16 merge commit. The label `upstream` therefore must not be equated with `ggml-org/llama.cpp` without historical remote-configuration evidence.

[VERIFIED] Comparing llama.cpp snapshot `5fd2dc2c41c342a75c26f9756ca6b1814ed05fb4` with current ROCmFPX `a5605a72768c6562241b248e268e33dc92787394` returned **no common ancestor**. The snapshot is not proven current graph ancestry.

[INFERENCE] ROCmFPX is best described as a standalone/reconstructed llama.cpp-derived history whose upstream relationship is established through source snapshots, exact blob identity, cherry-pick/manual-port records, and reverse ports—not as a simple branch with a continuous parent chain to current llama.cpp.

[VERIFIED] Commit `e6009eb76d55062d22357e6f117a829e861be01b` records restored source revisions: WebGPU at `5fd2dc2c41c342a75c26f9756ca6b1814ed05fb4` and Hexagon at `d2c67959b32cc49e43de2256b7381feb9130a17a`.

[VERIFIED] ROCmFPX file `ggml/src/ggml-hexagon/htp/hmx-matmul-ops.c` at `a5605a72768c6562241b248e268e33dc92787394` is byte-identical, blob `5c37f24ff0049a0c8e67c19b2e712ce8b834102b`, to the same path in llama.cpp commit `d2c67959b32cc49e43de2256b7381feb9130a17a`.

[VERIFIED] Current upstream later imported ROCmFPX-origin Hy3 work in commit `2969d6d15d67a08e7b83f26164b15350c79c5248`. The commit states that the base implementation was ported from ROCmFPX `src/models/hyv3.cpp`, names `charlie12345` and Piotr Wilkin as co-authors, and adapts it to mainline APIs.

### llama-ai and CachyLLama

[VERIFIED] `fewtarius/llama-ai` contains a gitlink named `CachyLLama`; `.gitmodules` points to `git@github.com:fewtarius/CachyLLama.git`, branch `master`.

[VERIFIED] At `1017f3dfdce3ca2b06aa9007b23295db3bb35722`, whose sole parent is `d8a07baad6ab175f8badbc4d496c9190b0cc3b2d`, the gitlink changed from `c8ead677a7fe42fb0a67e6e866fb254cc338e9fd` to `6be745998f568e379ea197fcf827baec73ff9940`.

[VERIFIED] CachyLLama `6be745998f568e379ea197fcf827baec73ff9940` is a merge titled `Merge upstream/master: 61 commits ...` with ordered parents:

1. first parent `c8ead677a7fe42fb0a67e6e866fb254cc338e9fd`;
2. second parent `92366df30d4eaa4b85139b5fd694360237731b19`.

[VERIFIED] The second parent exists in `ggml-org/llama.cpp`. Compare evidence reports 61 imported commits plus the merge relative to the first parent, establishing direct upstream Git ancestry for this merge.

### Current upstream tip

[VERIFIED] llama.cpp tip `86d86ed4396b4130922f7b9af26e3d9fc11a591b` has sole parent `7d56da7e546f54fb1fa54ef2bc9ad9a872860ab0`.

Machine-readable ordered edges are in [`data/ancestry_edges.csv`](data/ancestry_edges.csv).

## 3. Top-level license declarations

| Repository/material | Declaration | Exact evidence | Constraint |
|---|---|---|---|
| ROCmFPX | MIT | `LICENSE` blob `e7dca554bcb802f98408383a864404e3aa4eacca` | Preserve copyright and permission notice. |
| CachyLLama | MIT | `LICENSE` blob `e7dca554bcb802f98408383a864404e3aa4eacca` | Local additions are stated to use the same terms unless otherwise noted. |
| llama.cpp | MIT | `LICENSE` blob `e7dca554bcb802f98408383a864404e3aa4eacca` | File-level exceptions remain separately licensed. |
| llama-ai source | GPL-3.0-or-later | README, `llms.txt`, `AGENTS.md`, and SPDX headers on scripts | Do not import into an MIT-only core without a deliberate GPL distribution decision. |
| llama-ai top-level `LICENSE` | GPLv3 text | Blob `f288702d2fa16d3cdf0035b15a9fcbc552cd88e7` | “Or later” scope for unmarked files should be confirmed. |
| llama-ai docs | CC-BY-NC-SA-4.0 | README, `llms.txt`, `AGENTS.md` | Noncommercial and ShareAlike; prose should not be copied into commercial/permissive docs without permission. |
| TheRock build-system source | MIT | `LICENSE` blob `5c22b7e0496700a306b37a37fcfb23bfaa6a986f` at `44164a5a330ebd8ae581c1d91f8f4a7251808666` | Does not automatically classify every file in assembled ROCm tarballs. |

## 4. File-level license exceptions observed

[VERIFIED] The inspected trees include at least the following material exceptions:

| File/family | Observed license or grant | Notes |
|---|---|---|
| `ggml/src/ggml-sycl/common.hpp` and SYCL family | MIT plus Apache-2.0 WITH LLVM-exception | Preserve both applicable notices and the LLVM exception text. |
| `ggml/src/ggml-openvino/**` | Apache-2.0 | File-specific license controls. |
| `examples/gguf-hash/deps/xxhash/xxhash.h` | BSD-2-Clause | Binary redistribution notice requirement. |
| `vendor/nlohmann/json.hpp` | MIT | Checked-in version 3.12.0; blob `82d69f7c5d044c9887c96b90c97f5639083ecd14`. |
| `vendor/stb/stb_image.h` | MIT or public-domain dedication | Version 2.30; blob `9eedabedc45b3e6fd88fae6f14a160b4d53272ec`; choose and document the MIT path for predictable compliance. |
| `vendor/miniaudio/miniaudio.h` | MIT-0 or public-domain dedication | Vendor input commit `9634bedb5b5a2ca38c1ee7108a9358a4e233f14d`; checked-in blob `c6d493ee81f553b8dd584e402453f130c20e0546`. |
| `vendor/sheredom/subprocess.h` | Public-domain dedication/Unlicense-style text | Vendor input commit `b49c56ee40e7b44706b719008e2385ea381177ac`; checked-in blob `3e40bae046a21339752bf15221ff7d1d39f6a8ce`; jurisdictional treatment requires review. |
| `vendor/cpp-httplib/LICENSE` | MIT | Current upstream vendor version 0.50.1; license blob `3e5ed359a2bb16f52c383745c25f14bb7a81c9e4`. |
| `ggml/src/ggml-cpu/llamafile/sgemm.cpp` | MIT, Mozilla Foundation | Preserve file notice. |
| `ggml/src/ggml-cpu/kleidiai/**` | MIT, Arm | Preserve file notices. |
| selected `models/templates/*.jinja` | Apache-2.0 | Template-specific notice; model-provider terms may also matter. |
| `tools/mtmd/legacy-models/minicpmv-convert-image-encoder-to-gguf.py` | Apache-2.0 | Header states Google AI/HuggingFace origin and copied-source trail. |
| Android Gradle wrapper | Apache-2.0 | Tooling component, not covered solely by root MIT. |
| `tools/ui/package-lock.json` dependency graph | Mixed transitive licenses | Lockfile blob `7216de682340876a48c0bc06647159cb1f25ba21`; requires generated npm license inventory for shipped UI assets. |

[VERIFIED] ROCmFPX `THIRD_PARTY_NOTICES.md` identifies llama.cpp, cpp-httplib, nlohmann/json, and gguf-py, but does not enumerate all exceptions above. It is not sufficient as the release notice file for the current tree.

## 5. Submodules and synchronized source

| Repository | `.gitmodules` | Gitlink status | Synchronized `ggml` marker |
|---|---|---|---|
| ROCmFPX | Empty blob | No configured submodule observed | `628249b398293fc8d2fa81a449ae2920a02c6523` (ggml v0.11.1) |
| llama-ai | One `CachyLLama` entry | Gitlink `6be745998f568e379ea197fcf827baec73ff9940` | Engine marker lives in submodule |
| CachyLLama | Empty blob | No configured submodule observed | `eced84c86f8b012c752c016f7fe789adea168e1e` (ggml v0.15.3) |
| llama.cpp | Empty blob | No configured submodule observed | `9be313313c8ecb9488911bd64550190e3ed80f38` (ggml v0.17.0) |

[VERIFIED] The `ggml/` directories are copied/synchronized source, not submodules. Their provenance and notices must travel with the source distribution.

## 6. Vendor synchronization state

| Repository | cpp-httplib | nlohmann/json | stb_image | miniaudio | subprocess.h |
|---|---|---|---|---|---|
| ROCmFPX | v0.47.0 | unpinned `releases/latest` | unpinned `master` | `9634bedb5b5a2ca38c1ee7108a9358a4e233f14d` | `b49c56ee40e7b44706b719008e2385ea381177ac` |
| CachyLLama | v0.49.0 | unpinned `releases/latest` | unpinned `master` | `9634bedb5b5a2ca38c1ee7108a9358a4e233f14d` | `b49c56ee40e7b44706b719008e2385ea381177ac` |
| current llama.cpp | v0.50.1 | unpinned `releases/latest` | unpinned `master` | `9634bedb5b5a2ca38c1ee7108a9358a4e233f14d` | `b49c56ee40e7b44706b719008e2385ea381177ac` |

[VERIFIED] The checked-in blobs are immutable at each repository SHA, but two update inputs are moving targets. Re-running the vendor script later is not reproducible without recording downloaded blob hashes or replacing those URLs with immutable revisions.

## 7. ROCm/TheRock bundle constraint

[VERIFIED] `llama-ai/scripts/rebuild.sh`, blob `dedf0d299169a2898331289bc0009554a75b1952`, downloads and extracts a multi-component TheRock nightly tarball into gitignored `deps/`, without an observed checksum/signature or captured component-license manifest.

[VERIFIED] ROCmFPX `scripts/build-rocmfp4-rocm714-local.sh` downloads `therock-dist-linux-gfx120X-all-7.14.0a20260624.tar.gz`, builds with the extracted SDK, then copies numerous runtime libraries—including HIP, rocBLAS, hipBLAS, hipBLASLt, rocSOLVER, HSA, COMGR, LLVM, OpenMP, and kernel libraries—beside the binaries and recommends archiving the complete build directory.

[LEGAL-REVIEW] Redistribution of that bundle is blocked until each copied file is mapped to its component, source revision, license, required notice/source offer, and checksum.

## 8. AI-assisted and contributor provenance

[VERIFIED] ROCmFPX maintains `AI_CHANGES.md`, blob `e5acbf932092eef329b73c5748d56a6cc93f84eb`, as an append-only record of AI-assisted changes. PR #32 also carries assisted-by trailers.

[VERIFIED] Current upstream `CONTRIBUTING.md` permits assistive AI use with disclosure and human review, while rejecting fully or predominantly AI-generated pull requests.

[UNRESOLVED] No CLA or DCO text was located in the inspected llama.cpp repository files. This does not prove none exists outside the inspected files or platform settings.

[LEGAL-REVIEW] Before commercial release, confirm contributor authority and tool/provider terms for material AI-assisted changes, especially core ROCmFPX quantization and generated code.

## 9. Core constraints for a canonical project

1. [VERIFIED] MIT-origin code may be copied and modified if the MIT notice is preserved.
2. [VERIFIED] File-level Apache/BSD/MIT-0/public-domain terms do not disappear under the root MIT file.
3. [VERIFIED] GPL wrapper source cannot be represented as MIT-only source.
4. [VERIFIED] CC-BY-NC-SA documentation is unsuitable for unrestricted commercial documentation without separate permission.
5. [INFERENCE] Executing a GPL script as a separate process does not itself copy that script into the MIT engine; distribution and integration facts still require counsel review.
6. [VERIFIED] Build-time and runtime downloads are separate third-party components whose licenses must be audited at the exact artifact level.
7. [VERIFIED] Model weights, tokenizers, and downloaded model assets are outside the software-repository declarations unless separately licensed.
8. [RECOMMENDATION] Preserve Git authorship, source commit URLs, and manual-port records in every imported commit or provenance record.
