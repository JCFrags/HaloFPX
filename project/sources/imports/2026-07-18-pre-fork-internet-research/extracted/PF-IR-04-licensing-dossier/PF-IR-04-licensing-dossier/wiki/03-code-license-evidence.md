# Code, SPDX, and notice evidence

## ROCmFPX

The root MIT license, README, and third-party notice files are unchanged between the two requested revisions. The later revision introduces two unheadered HIP files; those files have only the repository-wide MIT assertion unless a history/origin review establishes more specific evidence.

The tree also contains representative exceptions:

- Apache-2.0 WITH LLVM-exception sections in SYCL material.
- Apache-2.0 OpenVINO files.
- MIT KleidiAI material with Arm copyright.
- Unlicense/public-domain and separate public-domain notices.

The existing third-party notice file lists major MIT dependencies but does not fully enumerate these backend exceptions.

## CachyLLama

The root MIT license and README assertion are clear. The exact commit has no root `THIRD_PARTY_NOTICES.md`, while the same representative backend exceptions are present. A release notice must be generated from the exact tree.

## llama-ai

The root GPLv3 text and repeated `GPL-3.0-or-later` file headers support a GPL operational-code scope. The README separately labels documentation CC-BY-NC-SA-4.0 and references the MIT engine. These scopes must remain separate in manifests and notices.

## File-level manifest rule

Use repository-level declarations as one evidence layer. For each proposed file, record the strongest applicable evidence in this order: explicit SPDX/header, colocated license/notice, documented donor file history, repository assertion, then `NOASSERTION`.
