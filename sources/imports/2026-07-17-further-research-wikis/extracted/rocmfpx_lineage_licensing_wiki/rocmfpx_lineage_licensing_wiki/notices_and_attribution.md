# Notices and Attribution

## Required notice layers

[RECOMMENDATION] A canonical release should have four complementary records:

1. **Root `LICENSE`** for project-authored MIT material.
2. **`LICENSES/` directory** containing every license text used by shipped source or binaries.
3. **`THIRD_PARTY_NOTICES.md` or `NOTICE`** mapping components/files to copyright, license, source URL, and modifications.
4. **Provenance ledger** mapping exact files and commits to import method and source history.

## Current ROCmFPX notice coverage

[VERIFIED] ROCmFPX `THIRD_PARTY_NOTICES.md` mentions:

- llama.cpp / ggml authors under MIT;
- cpp-httplib under MIT;
- nlohmann/json under MIT;
- gguf-py under MIT;
- model weights as excluded from the repository license.

[VERIFIED] The current tree also contains material not enumerated there, including:

- stb_image;
- miniaudio;
- subprocess.h;
- xxHash;
- Apache-2.0 OpenVINO files;
- Apache-2.0 WITH LLVM-exception SYCL files;
- Apache-2.0 model templates and converter scripts;
- MIT Mozilla llamafile source;
- MIT Arm KleidiAI files;
- npm/WebUI transitive packages and generated assets;
- restored Hexagon/WebGPU snapshots;
- external Qualcomm SDK interfaces when Hexagon is built;
- ROCm runtime libraries when the self-contained packaging script is used.

[RECOMMENDATION] Replace the current notice file before any canonical binary release.

## Attribution for upstream/manual ports

Preserve:

- original Git author/committer metadata;
- `cherry-pick -x` source SHA when used;
- `Based on upstream ...` trailers for manual ports;
- `docs/UPSTREAM-ATTRIBUTION.md` and `docs/ROCmFPX-UPSTREAM-CREDITS.md` content or equivalent structured records;
- reverse-port attribution from upstream Hy3 commit `2969d6d15d67a08e7b83f26164b15350c79c5248` back to ROCmFPX;
- co-author and assisted-by trailers where present.

## Minimal component notice fields

```text
Component: nlohmann/json
Files: vendor/nlohmann/json.hpp
Version/commit: 3.12.0 / exact source commit when resolved
License: MIT
Copyright: Niels Lohmann and contributors
Source: immutable upstream URL
Local modifications: none | described
Included license: LICENSES/MIT-nlohmann.txt
```

## Apache-2.0 files

[VERIFIED] Apache-2.0 requires preservation of notices, a copy of the license, and prominent notice that modified files were changed. If an upstream work provides a `NOTICE`, relevant notice content must be carried as required by the license.

[RECOMMENDATION] Include one Apache-2.0 text plus component/file mapping. Keep `Apache-2.0 WITH LLVM-exception` distinguishable from plain Apache-2.0.

## BSD-2-Clause files

[VERIFIED] For binary redistribution, reproduce the copyright notice, conditions, and disclaimer in documentation and/or other materials provided with the distribution.

## Dual-license elections

[RECOMMENDATION] Record a deliberate license election:

- stb_image → MIT;
- miniaudio → MIT-0;
- public-domain-only source → preserve dedication text and obtain jurisdictional review.

Do not describe dual-option files merely as “public domain” when the release relies on the license alternative.

## GPL companion notices

If a GPL companion is distributed:

- keep its GPL notice and copyright headers;
- provide the exact GPL license text;
- identify source archive/commit;
- provide corresponding source for distributed binaries and scripts as required;
- include installation information where GPLv3 requires it;
- do not impose EULA or technical restrictions inconsistent with the GPL.

## CC-BY-NC-SA documentation notices

If authorized noncommercial copying/adaptation is used:

- identify author and source;
- link/include the CC license;
- state modifications;
- apply BY-NC-SA 4.0 or a permitted compatible license to adaptations;
- do not represent the result as commercially reusable or MIT documentation.

[RECOMMENDATION] Canonical commercial documentation should instead be independently written.

## Binary-bundle notice

A self-contained ROCm build must include a generated table for every copied library and kernel asset. The shell script's list of copied filenames is a starting point, not a license conclusion.

A notice template is in [`templates/THIRD_PARTY_NOTICES.template.md`](templates/THIRD_PARTY_NOTICES.template.md).
