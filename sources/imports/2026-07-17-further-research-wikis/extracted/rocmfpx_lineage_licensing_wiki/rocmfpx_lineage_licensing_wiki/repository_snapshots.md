# Repository Snapshots

## Snapshot table

| Repository | Tip | Tip message | Default branch | Root license | Relationship |
|---|---|---|---|---|---|
| `charlie12345/ROCmFPX` | `a5605a72768c6562241b248e268e33dc92787394` | Merge PR #32 | `main` | MIT | llama.cpp-derived content with detached/reconstructed local history; snapshots, cherry-picks, manual ports, and reverse ports |
| `fewtarius/llama-ai` | `1017f3dfdce3ca2b06aa9007b23295db3bb35722` | Update CachyLLama gitlink after upstream merge | `main` | GPL-3.0-or-later source; CC-BY-NC-SA-4.0 docs | GPL orchestration superproject around separate MIT engine gitlink |
| `fewtarius/CachyLLama` | `6be745998f568e379ea197fcf827baec73ff9940` | Merge upstream/master, 61 commits | `master` | MIT | official GitHub fork of llama.cpp with direct upstream merge ancestry and local cache/server/GPU work |
| `ggml-org/llama.cpp` | `86d86ed4396b4130922f7b9af26e3d9fc11a591b` | OpenCL q4_K scale-layout optimization | `master` | MIT plus file exceptions | current upstream |

## ROCmFPX snapshot

- [VERIFIED] Repository: https://github.com/charlie12345/ROCmFPX
- [VERIFIED] Tip: https://github.com/charlie12345/ROCmFPX/commit/a5605a72768c6562241b248e268e33dc92787394
- [VERIFIED] License: https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/LICENSE
- [VERIFIED] `.gitmodules`: https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/.gitmodules — empty.
- [VERIFIED] ggml sync marker: https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/scripts/sync-ggml.last — `628249b398293fc8d2fa81a449ae2920a02c6523`.
- [VERIFIED] Vendor script: https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/scripts/sync_vendor.py
- [VERIFIED] Upstream attribution policy: https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/docs/UPSTREAM-ATTRIBUTION.md
- [VERIFIED] ROCmFPX upstream credits: https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/docs/ROCmFPX-UPSTREAM-CREDITS.md
- [VERIFIED] AI record: https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/AI_CHANGES.md

### Ordered ancestry anchors

| Child | Parent index | Exact parent | Meaning |
|---|---:|---|---|
| `a5605a72768c6562241b248e268e33dc92787394` | 1 | `25c71fc6e12d73bb3804127e032d29fb8976ae40` | PR #32 base |
| `a5605a72768c6562241b248e268e33dc92787394` | 2 | `a8b5fa906ccd13c6a8ca06d55aa287854c376868` | PR #32 topic head |
| `c2845bf86a5c1842d33bd9e990b2bcaf75779251` | 1 | `5b3956605309dd3e6beed49c8f3a41423ba71d25` | PR #27 base |
| `c2845bf86a5c1842d33bd9e990b2bcaf75779251` | 2 | `ccac6e55ec7c0fa8acdcb6e80b1a242e4f7d654e` | PR #27 experimental topic head |
| `2335e6a482b1601d71dff9e860c8feab108c3af2` | 1 | `221402af8574faf652b101b6afe225a3f329561f` | branch head before local remote-tracking merge |
| `2335e6a482b1601d71dff9e860c8feab108c3af2` | 2 | `5b3956605309dd3e6beed49c8f3a41423ba71d25` | merged remote-tracking branch object; historical remote URL not stored |

[VERIFIED] `2335e6a482b1601d71dff9e860c8feab108c3af2` and `c2845bf86a5c1842d33bd9e990b2bcaf75779251` are ancestors of the cutoff tip.

[VERIFIED] `5fd2dc2c41c342a75c26f9756ca6b1814ed05fb4` is not a common ancestor of current ROCmFPX in the inspected compare result. It remains a source-snapshot reference.

[CONSTRAINT] The string `upstream/main` in commit `2335e6a482b1601d71dff9e860c8feab108c3af2` is not a durable repository identity. A Git commit records parent objects and message text, not the URL assigned to a local remote name.

## llama-ai snapshot

- [VERIFIED] Repository: https://github.com/fewtarius/llama-ai
- [VERIFIED] Tip: https://github.com/fewtarius/llama-ai/commit/1017f3dfdce3ca2b06aa9007b23295db3bb35722
- [VERIFIED] Sole parent: `d8a07baad6ab175f8badbc4d496c9190b0cc3b2d`
- [VERIFIED] License text: https://github.com/fewtarius/llama-ai/blob/1017f3dfdce3ca2b06aa9007b23295db3bb35722/LICENSE
- [VERIFIED] Project license declarations: https://github.com/fewtarius/llama-ai/blob/1017f3dfdce3ca2b06aa9007b23295db3bb35722/README.md
- [VERIFIED] Agent reference declaration: https://github.com/fewtarius/llama-ai/blob/1017f3dfdce3ca2b06aa9007b23295db3bb35722/AGENTS.md
- [VERIFIED] Machine-readable summary declaration: https://github.com/fewtarius/llama-ai/blob/1017f3dfdce3ca2b06aa9007b23295db3bb35722/llms.txt
- [VERIFIED] Submodule config: https://github.com/fewtarius/llama-ai/blob/1017f3dfdce3ca2b06aa9007b23295db3bb35722/.gitmodules
- [VERIFIED] ROCm downloader: https://github.com/fewtarius/llama-ai/blob/1017f3dfdce3ca2b06aa9007b23295db3bb35722/scripts/rebuild.sh
- [VERIFIED] ROCm build wrapper: https://github.com/fewtarius/llama-ai/blob/1017f3dfdce3ca2b06aa9007b23295db3bb35722/src/cachy-llama-rocm/build.sh

### Submodule boundary

```text
fewtarius/llama-ai @ 1017f3dfdce3ca2b06aa9007b23295db3bb35722                 GPL wrapper / CC docs
└── CachyLLama gitlink @ 6be745998f568e379ea197fcf827baec73ff9940                separate MIT repository
```

[RECOMMENDATION] Preserve this as a legal and technical component boundary. Do not copy the GPL build/run scripts into an MIT distribution merely because they operate on an MIT submodule.

## CachyLLama snapshot

- [VERIFIED] Repository: https://github.com/fewtarius/CachyLLama
- [VERIFIED] Tip: https://github.com/fewtarius/CachyLLama/commit/6be745998f568e379ea197fcf827baec73ff9940
- [VERIFIED] License: https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/LICENSE
- [VERIFIED] Fork statement: https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/README.md
- [VERIFIED] `.gitmodules`: https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/.gitmodules — empty.
- [VERIFIED] ggml sync marker: https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/scripts/sync-ggml.last — `eced84c86f8b012c752c016f7fe789adea168e1e`.
- [VERIFIED] Vendor script: https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/scripts/sync_vendor.py

### Exact merge boundary

```text
first parent (local line):  c8ead677a7fe42fb0a67e6e866fb254cc338e9fd
second parent (llama.cpp):  92366df30d4eaa4b85139b5fd694360237731b19
merge tip:                  6be745998f568e379ea197fcf827baec73ff9940
```

[VERIFIED] `92366df30d4eaa4b85139b5fd694360237731b19` exists in `ggml-org/llama.cpp`; the merge is direct upstream graph ancestry, not merely textual provenance.

## Current upstream llama.cpp snapshot

- [VERIFIED] Repository: https://github.com/ggml-org/llama.cpp
- [VERIFIED] Tip: https://github.com/ggml-org/llama.cpp/commit/86d86ed4396b4130922f7b9af26e3d9fc11a591b
- [VERIFIED] Sole parent: `7d56da7e546f54fb1fa54ef2bc9ad9a872860ab0`
- [VERIFIED] License: https://github.com/ggml-org/llama.cpp/blob/86d86ed4396b4130922f7b9af26e3d9fc11a591b/LICENSE
- [VERIFIED] `.gitmodules`: https://github.com/ggml-org/llama.cpp/blob/86d86ed4396b4130922f7b9af26e3d9fc11a591b/.gitmodules — empty.
- [VERIFIED] ggml sync marker: https://github.com/ggml-org/llama.cpp/blob/86d86ed4396b4130922f7b9af26e3d9fc11a591b/scripts/sync-ggml.last — `9be313313c8ecb9488911bd64550190e3ed80f38`.
- [VERIFIED] Vendor script: https://github.com/ggml-org/llama.cpp/blob/86d86ed4396b4130922f7b9af26e3d9fc11a591b/scripts/sync_vendor.py
- [VERIFIED] Contribution policy: https://github.com/ggml-org/llama.cpp/blob/86d86ed4396b4130922f7b9af26e3d9fc11a591b/CONTRIBUTING.md
- [VERIFIED] Reverse Hy3 port: https://github.com/ggml-org/llama.cpp/commit/2969d6d15d67a08e7b83f26164b15350c79c5248

## ggml synchronization markers

| Consuming tree | ggml commit | Verified version marker | Meaning |
|---|---|---|---|
| ROCmFPX | `628249b398293fc8d2fa81a449ae2920a02c6523` | v0.11.1 | Copied/synchronized ggml baseline |
| CachyLLama | `eced84c86f8b012c752c016f7fe789adea168e1e` | v0.15.3 | Copied/synchronized ggml baseline |
| current llama.cpp | `9be313313c8ecb9488911bd64550190e3ed80f38` | v0.17.0 | Copied/synchronized ggml baseline |

[CONSTRAINT] A future sync can replace or relicense individual files. Repeat the file-level scan after every sync-marker change.
