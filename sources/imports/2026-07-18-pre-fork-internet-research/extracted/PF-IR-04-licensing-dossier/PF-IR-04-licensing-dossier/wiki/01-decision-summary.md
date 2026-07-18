# Decision summary

## What the external evidence unblocks

- Candidate MIT donor records for the two ROCmFPX snapshots and the CachyLLama snapshot.
- Concrete evidence that repository-level MIT labels are incomplete without file-level exception and notice handling.
- A technical and licensing evidence boundary between the GPL `llama-ai` operational layer and the separately pinned MIT `CachyLLama` gitlink target.
- A conservative promotion matrix for templates, tokenizers, models, test corpora, Web UI outputs, and distribution dependencies.
- Release-control requirements for SBOM, source-to-binary mapping, corresponding source, notices, generated assets, and clean-room roles.

## High-impact findings

| Finding | Evidence status | Default disposition |
|---|---|---|
| ROCmFPX root license is MIT at both commits | exact-byte, same Git blob | candidate, subject to exceptions/full-tree scan |
| CachyLLama root license is MIT | exact-byte Git blob | candidate, but create notice inventory |
| `llama-ai` operational files are GPL-3.0-or-later | root license + repeated file SPDX | GPL release-model decision |
| `llama-ai` documentation is separately asserted CC-BY-NC-SA-4.0 | exact README | reference-only by default |
| `CachyLLama` is a gitlink to exact commit `6be745...` | exact `.gitmodules` + commit diff | separate component record |
| SYCL/OpenVINO backend files include Apache/LLVM exceptions | exact file blobs/headers | preserve terms and supplement notices |
| Models/tokenizers/templates are not licensed by repository placement alone | generation/download scripts | blocked or reference-only |
| Web UI is generated and embedded; direct dependencies are mixed MIT/BSD/Apache | exact package-lock blob and build files | pinned regeneration with SBOM/notices |

## Human decisions still required

Final admissibility, release aggregation/linking, GPL corresponding-source method, clean-room role design, documentation distribution, model/test-data permission, and the exact source/binary release model remain human decisions over the locally proposed tree.
