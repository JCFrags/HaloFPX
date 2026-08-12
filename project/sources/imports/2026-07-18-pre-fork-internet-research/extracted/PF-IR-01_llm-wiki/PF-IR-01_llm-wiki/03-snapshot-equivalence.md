# Snapshot equivalence and divergence

## Exact snapshots

| Label | Snapshot ID | Repository | Exact commit | Role | Source-selection conclusion | Binary conclusion |
|---|---|---|---|---|---|---|
| [VERIFIED] | SNAP-ROCMFPX-BASE | charlie12345/ROCmFPX | `a5605a72768c6562241b248e268e33dc92787394` | baseline source snapshot | Resolved exact commit | [OPEN] No deployed-binary proof |
| [VERIFIED] | SNAP-ROCMFPX-CANDIDATE | charlie12345/ROCmFPX | `61f2f2d7bc4955e9bca821095ef69125837133b5` | candidate source snapshot | Resolved exact commit | [OPEN] No deployed-binary proof |
| [VERIFIED] | SNAP-CACHY | fewtarius/CachyLlama | `6be745998f568e379ea197fcf827baec73ff9940` | CachyLlama source snapshot | Resolved exact commit | [OPEN] No deployed-binary proof |
| [VERIFIED] | SNAP-LLAMA-AI | fewtarius/llama-ai | `1017f3dfdce3ca2b06aa9007b23295db3bb35722` | wrapper/submodule selection snapshot | Resolved exact commit | [OPEN] No deployed-binary proof |
| [VERIFIED] | SNAP-UPSTREAM-A | ggml-org/llama.cpp | `788e07dc91d266ad3162a1ce9037665656269689` | upstream comparison snapshot | Resolved exact commit | [OPEN] No deployed-binary proof |
| [VERIFIED] | SNAP-UPSTREAM-B | ggml-org/llama.cpp | `86d86ed4396b4130922f7b9af26e3d9fc11a591b` | upstream comparison snapshot | Resolved exact commit | [OPEN] No deployed-binary proof |
| [VERIFIED] | SNAP-DEPLOYED-SOURCE | charlie12345/rocmfp4-llama | `4860505ee322091f0f61eba77d6ad49be88cf4ea` | declared deployed predecessor source | Resolved exact commit | [OPEN] No deployed-binary proof |

[VERIFIED] `llama-ai@1017f3dfdce3ca2b06aa9007b23295db3bb35722` is a one-line submodule-pointer update selecting exactly `CachyLlama@6be745998f568e379ea197fcf827baec73ff9940`.

[OPEN] That pointer does not prove the submodule was initialized, that the checkout was clean, or that those bytes were compiled.

## File-level identity

| Label | Path | Snapshot group | Git blob ID | Interpretation |
|---|---|---|---|---|
| [VERIFIED] | ggml/src/gguf.cpp | upstream 788e | `7920b8100b61e2f2d903f55e7e0887fc07b65d4b` | byte identity only within that snapshot |
| [VERIFIED] | ggml/src/gguf.cpp | ROCmFPX a560 + candidate + rocmfp4 | `ab3cc9748678ce79ee171a22e426061a4235fa8f` | shared fork blob; semantically fixed |
| [VERIFIED] | ggml/src/gguf.cpp | CachyLlama | `5e1986182515b3baadd3ab4c45c7e09038d0b64e` | diverged blob; semantically fixed |
| [VERIFIED] | ggml/src/ggml-rpc/ggml-rpc.cpp | upstream 788e + CachyLlama | `d38057721834c95f4beda160dd89dc7f0c5ca86a` | byte-identical RPC source |
| [VERIFIED] | ggml/src/ggml-rpc/ggml-rpc.cpp | ROCmFPX a560 + candidate + rocmfp4 | `1cb8f563d8583db966c424b7f2fd58f65f8f9c7f` | shared fork blob; corrected RPC semantics |
| [VERIFIED] | src/llama-vocab.cpp | ROCmFPX a560 + candidate | `bccc4ecb99fae0c0c535c3d5ac65044734864b1e` | shared fork vocab blob |
| [VERIFIED] | src/llama-vocab.cpp | CachyLlama + upstream snapshots | `fdd447147d43daa42ea8d437924240bd02036b01` | byte-identical vocab source |
| [VERIFIED] | src/llama-vocab.cpp | rocmfp4 deployed-source snapshot | `a42cc74bfa6c40cd6a80330a6baa82497237408c` | older divergent blob with all three material guards |
| [VERIFIED] | vendor/cpp-httplib/httplib.h | ROCmFPX a560 + candidate + rocmfp4 | `45a55ae100056876a493eec87e6f8bfcea80d08d` | cpp-httplib 0.47.0 |
| [VERIFIED] | vendor/cpp-httplib/httplib.h | CachyLlama | `e7ef56370cbd5fc208c83c51099db90bcf58dff9` | cpp-httplib 0.49.0 |
| [VERIFIED] | vendor/cpp-httplib/httplib.h | upstream 788e + 86d | `de026a25326634026f082579e34675c6af897f35` | cpp-httplib 0.50.1 |

## Applicability matrix

| Label | Security family | ROCmFPX a560 | ROCmFPX candidate | CachyLlama | llama-ai | upstream snapshots | rocmfp4 source | Proof limit |
|---|---|---|---|---|---|---|---|---|
| [MEASURED] | GGUF overflow families | Fixed semantics | Fixed semantics | Fixed semantics | Inherits selected submodule | Fixed semantics | Fixed semantics | No binary proof |
| [MEASURED] | 2024 RPC GET/SET/type fixes | Present | Present | Present | Inherits selected submodule | Present | Present | No listener proof |
| [MEASURED] | 2026 RPC GRAPH_COMPUTE corrected fix | Present | Present | Present | Inherits selected submodule | Present | Present | Official advisory still lists no patched version |
| [MEASURED] | Vocabulary bounds families | Present | Present | Present | Inherits selected submodule | Present | Present | No dynamic test proof |
| [MEASURED] | Negative n_discard clamp | Present | Present | [INFERENCE] Later merged snapshot | Inherits selected submodule | Present | [INFERENCE] Later source lineage | No deployed configuration proof |
| [MEASURED] | cpp-httplib advisory floor | 0.47.0 | 0.47.0 | 0.49.0 | Inherits selected submodule | 0.50.1 | 0.47.0 | Loaded library/header proof remains local |

## Interpretation rules

[VERIFIED] A matching Git blob ID establishes byte identity for that file only.

[MEASURED] Different blobs can still provide source equivalence when the vulnerable symbol and required validation behavior are both inspected.

[INFERENCE] Commit date/ancestry can support inclusion when a direct symbol capture is unavailable, but it is weaker than a blob or source-line measurement.

[OPEN] None of these methods proves that an installed binary came from the selected tree, that a feature was compiled, that a different library was not linked, or that runtime configuration preserves the expected boundary.
