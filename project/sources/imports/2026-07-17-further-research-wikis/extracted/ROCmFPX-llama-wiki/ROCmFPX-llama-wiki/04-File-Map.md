# File map

> **Audit snapshot:** fork `a5605a72768c6562241b248e268e33dc92787394`; upstream `86d86ed4396b4130922f7b9af26e3d9fc11a591b`; inventory date `2026-07-17`.  
> **Decision vocabulary:** **RETAIN** = capability/ABI must survive; **REFRESH** = preserve capability but re-port onto current upstream; **RETIRE** = remove the fork patch and use upstream or archive it. [S-FORK-HEAD](https://github.com/charlie12345/ROCmFPX/tree/a5605a72768c6562241b248e268e33dc92787394) [S-UP-HEAD](https://github.com/ggml-org/llama.cpp/tree/86d86ed4396b4130922f7b9af26e3d9fc11a591b)

## Exact changed-path inventory

The exact PR file lists produce **276 unique paths**. The path-level decision ledger contains **47 RETAIN**, **105 REFRESH**, and **124 RETIRE** entries. These are path classifications; shared-file hunks still require code review during re-port. [S-PR27-FILES](https://github.com/charlie12345/ROCmFPX/pull/27/files) [S-PR28](https://github.com/charlie12345/ROCmFPX/pull/28) [S-PR31-FILES](https://github.com/charlie12345/ROCmFPX/pull/31/files) [S-PR32-FILES](https://github.com/charlie12345/ROCmFPX/pull/32/files)

| Subsystem | Unique paths | RETAIN | REFRESH | RETIRE |
| :--- | ---: | ---: | ---: | ---: |
| OpenCL | 34 | 0 | 0 | 34 |
| Hexagon | 33 | 0 | 0 | 33 |
| Validation / tuning scripts | 29 | 24 | 3 | 2 |
| WebGPU | 24 | 0 | 0 | 24 |
| ROCm/HIP kernels | 20 | 0 | 20 | 0 |
| Documentation / recipes | 16 | 11 | 5 | 0 |
| Vulkan kernels | 13 | 0 | 13 | 0 |
| Build / CI | 12 | 3 | 5 | 4 |
| Common / parser / sampling | 12 | 0 | 4 | 8 |
| Graph / context / KV | 12 | 0 | 12 | 0 |
| Server / cache | 11 | 0 | 11 | 0 |
| Model architecture | 10 | 0 | 1 | 9 |
| Quantization formats | 9 | 9 | 0 | 0 |
| Other | 8 | 0 | 7 | 1 |
| Tests | 8 | 0 | 5 | 3 |
| Model loader / quantization | 7 | 0 | 7 | 0 |
| Conversion / GGUF | 6 | 0 | 5 | 1 |
| CPU / ggml core | 5 | 0 | 5 | 0 |
| WebUI | 3 | 0 | 0 | 3 |
| MTP / speculative decoding | 2 | 0 | 2 | 0 |
| Housekeeping | 1 | 0 | 0 | 1 |
| Metal | 1 | 0 | 0 | 1 |

## Highest-value source clusters

| Cluster | Affected source paths | Decision | Required treatment | Primary sources |
|---|---|---|---|---|
| Format ABI and CPU oracle | `ggml/rocmfp4/**`, `ggml/rocmfpx/**` | **RETAIN** | Preserve block structs, codebooks, quant/dequant semantics, and deterministic tests. | [S-ROCMFP4-H](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/rocmfp4/rocmfp4.h) [S-ROCMFP4-C](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/rocmfp4/rocmfp4.c) [S-ROCMFPX-H](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/rocmfpx/rocmfpx.h) [S-ROCMFPX-C](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/rocmfpx/rocmfpx.c) [S-ROCMFP2-TEST](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/rocmfpx/test_rocmfp2_reference.c) |
| Shared type/quant integration | `ggml/include/ggml.h`, `ggml/src/ggml*.c`, `ggml/src/ggml-cpu/**`, `gguf-py/**`, `src/llama-quant.cpp`, `tools/quantize/**` | **REFRESH** | Start from current upstream; re-port custom types and tests without renumbering. | [S-QT-ENUM-FORK](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/include/ggml.h) [S-QT-ENUM-UP](https://github.com/ggml-org/llama.cpp/blob/86d86ed4396b4130922f7b9af26e3d9fc11a591b/ggml/include/ggml.h) [S-GGML-QUANTS](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/src/ggml-quants.c) [S-CPU-REG](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/src/ggml-cpu/ggml-cpu.c) [S-QUANTIZER](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/src/llama-quant.cpp) [S-QUANT-TOOL](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/tools/quantize/quantize.cpp) |
| HIP/ROCm kernels | `ggml/src/ggml-cuda/**`, `ggml/rocmfpx/*hip*` | **REFRESH** | Preserve ROCmFPX device primitives and validated Strix tuning; inherit newer generic backend code. | [S-MMVQ](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/src/ggml-cuda/mmvq.cu) [S-MMQ](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/src/ggml-cuda/mmq.cu) [S-MMQ-H](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/src/ggml-cuda/mmq.cuh) [S-VECDOT](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/src/ggml-cuda/vecdotq.cuh) [S-HIP-CODEBOOK](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/rocmfpx/rocmfpx_hip_codebook.cuh) [S-UP-HEAD](https://github.com/ggml-org/llama.cpp/tree/86d86ed4396b4130922f7b9af26e3d9fc11a591b) |
| Vulkan kernels | `ggml/src/ggml-vulkan/**` | **REFRESH** | Preserve custom formats, shader math, and packed FP6 coherency; update generator/backend interfaces. | [S-VULKAN](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/src/ggml-vulkan/ggml-vulkan.cpp) [S-VULKAN-TYPES](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/src/ggml-vulkan/vulkan-shaders/types.glsl) [S-C-62F](https://github.com/charlie12345/ROCmFPX/commit/62f7508b12c6b8510fd7a77dfc5d9519fa026d82) |
| MTP/server | `common/speculative.*`, `src/llama-{context,ext,graph}*`, `tools/server/**` | **REFRESH** | Adopt upstream model/private APIs and server base; re-port strict verification, request overrides, and cache state. | [S-SPEC-UP](https://github.com/ggml-org/llama.cpp/blob/86d86ed4396b4130922f7b9af26e3d9fc11a591b/common/speculative.cpp) [S-EXT-UP](https://github.com/ggml-org/llama.cpp/blob/86d86ed4396b4130922f7b9af26e3d9fc11a591b/src/llama-ext.h) [S-C-7D7](https://github.com/charlie12345/ROCmFPX/commit/7d7b06bc5e6fd4a128af8efa954caa7409376a79) [S-C-B56](https://github.com/charlie12345/ROCmFPX/commit/b56ad79d77bc1eb9fe6407640eec8b8edfc04900) [S-C-C81](https://github.com/charlie12345/ROCmFPX/commit/c81c7c92233b6370b4eb7087398779a8dcb234a4) |
| Generic copied backends | `ggml/src/ggml-{hexagon,opencl,webgpu}/**` | **RETIRE** | Replace wholesale with current upstream. | [S-C-807](https://github.com/charlie12345/ROCmFPX/commit/80732f992f7c75e9154cfe184041f1384c59a0fb) [S-PR27-FILES](https://github.com/charlie12345/ROCmFPX/pull/27/files) [S-UP-HEAD](https://github.com/ggml-org/llama.cpp/tree/86d86ed4396b4130922f7b9af26e3d9fc11a591b) |
| Generic WebUI/platform backports | `tools/server/webui/**`, `build-xcframework.sh`, parser/Jinja units | **RETIRE** | Remove from the fork patch stack and inherit upstream. | [S-C-FF8](https://github.com/charlie12345/ROCmFPX/commit/ff8e7b8cf9dab714951df49d71f5835a7322404a) [S-C-A8C](https://github.com/charlie12345/ROCmFPX/commit/a8c41d31423e3e0b6f2dc7138af1d4fba8edb295) [S-C-01D](https://github.com/charlie12345/ROCmFPX/commit/01d463bb23c3f290688c4529c13b3b467fa2f7dc) [S-C-FE2](https://github.com/charlie12345/ROCmFPX/commit/fe2b7dc5e19a5e24c276593368a1bb41d0e27b1d) |

## Full machine-readable ledger

Open [`data/file-map.csv`](data/file-map.csv) for every path, PR membership, category, decision, and rationale. The static HTML site also exposes the same records through the left-navigation search.

## Path-level decision rule

A `RETAIN` path is ROCmFPX-owned and should remain independently reviewable. A `REFRESH` path is a shared integration point and should be recreated from upstream, not merged blindly. A `RETIRE` path should disappear from the extension patch stack unless a fresh reproducer demonstrates a current need.
