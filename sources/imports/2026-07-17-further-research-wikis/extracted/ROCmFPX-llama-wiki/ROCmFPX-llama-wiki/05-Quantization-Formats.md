# Quantization formats

> **Audit snapshot:** fork `a5605a72768c6562241b248e268e33dc92787394`; upstream `86d86ed4396b4130922f7b9af26e3d9fc11a591b`; inventory date `2026-07-17`.  
> **Decision vocabulary:** **RETAIN** = capability/ABI must survive; **REFRESH** = preserve capability but re-port onto current upstream; **RETIRE** = remove the fork patch and use upstream or archive it. [S-FORK-HEAD](https://github.com/charlie12345/ROCmFPX/tree/a5605a72768c6562241b248e268e33dc92787394) [S-UP-HEAD](https://github.com/ggml-org/llama.cpp/tree/86d86ed4396b4130922f7b9af26e3d9fc11a591b)

## Fork-only type registry

The fork extends the GGML type enumeration with IDs `100`–`107`: `Q4_0_ROCMFP4`, `Q4_0_ROCMFP4_FAST`, `Q6_0_ROCMFPX`, `Q8_0_ROCMFPX`, `Q3_0_ROCMFPX`, `TURBO3_0`, `TURBO4_0`, and `Q2_0_ROCMFPX`. The pinned upstream enumeration does not define these ROCmFPX/Turbo IDs. [S-QT-ENUM-FORK](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/include/ggml.h) [S-QT-ENUM-UP](https://github.com/ggml-org/llama.cpp/blob/86d86ed4396b4130922f7b9af26e3d9fc11a591b/ggml/include/ggml.h)

| Type | ID | Weights/block | Bytes/block | Effective bits/weight | Primary role | Decision | Primary sources |
|---|---:|---:|---:|---:|---|---|---|
| `Q4_0_ROCMFP4` | 100 | 32 | 18 | 4.50 | Two 16-value groups with separate scale bytes. | **RETAIN** | [S-QT-ENUM-FORK](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/include/ggml.h) [S-ROCMFP4-H](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/rocmfp4/rocmfp4.h) |
| `Q4_0_ROCMFP4_FAST` | 101 | 32 | 17 | 4.25 | Single-scale speed/size variant. | **RETAIN** | [S-QT-ENUM-FORK](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/include/ggml.h) [S-ROCMFP4-H](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/rocmfp4/rocmfp4.h) |
| `Q6_0_ROCMFPX` | 102 | 32 | 26 | 6.50 | Six-bit ROCmFPX weight block. | **RETAIN** | [S-QT-ENUM-FORK](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/include/ggml.h) [S-ROCMFPX-H](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/rocmfpx/rocmfpx.h) |
| `Q8_0_ROCMFPX` | 103 | 32 | 33 | 8.25 | Eight-bit ROCmFPX weight block. | **RETAIN** | [S-QT-ENUM-FORK](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/include/ggml.h) [S-ROCMFPX-H](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/rocmfpx/rocmfpx.h) |
| `Q3_0_ROCMFPX` | 104 | 32 | 14 | 3.50 | Three-bit ROCmFPX weight block. | **RETAIN** | [S-QT-ENUM-FORK](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/include/ggml.h) [S-ROCMFPX-H](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/rocmfpx/rocmfpx.h) |
| `TURBO3_0` | 105 | implementation-defined in KV path | implementation-defined | — | TurboQuant KV-cache type. | **REFRESH** | [S-QT-ENUM-FORK](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/include/ggml.h) [S-PR27-FILES](https://github.com/charlie12345/ROCmFPX/pull/27/files) |
| `TURBO4_0` | 106 | implementation-defined in KV path | implementation-defined | — | TurboQuant KV-cache type. | **REFRESH** | [S-QT-ENUM-FORK](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/include/ggml.h) [S-PR27-FILES](https://github.com/charlie12345/ROCmFPX/pull/27/files) |
| `Q2_0_ROCMFPX` | 107 | 32 | 10 | 2.50 | Two-bit ROCmFPX/IFP2-family weight block. | **RETAIN** | [S-QT-ENUM-FORK](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/include/ggml.h) [S-ROCMFP2-H](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/rocmfpx/rocmfp2_reference.h) [S-ROCMFPX-H](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/rocmfpx/rocmfpx.h) |

The bits-per-weight values above are direct storage calculations (`8 × bytes / 32`); they describe block storage and do not include GGUF metadata or alignment overhead. [S-ROCMFP4-H](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/rocmfp4/rocmfp4.h) [S-ROCMFPX-H](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/rocmfpx/rocmfpx.h) [S-ROCMFP2-H](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/rocmfpx/rocmfp2_reference.h)

## ABI that must not change

The following are compatibility contracts for existing GGUF artifacts:

- numeric GGML type IDs;
- block length and byte size;
- scale-byte meaning and ordering;
- packed code ordering and sign/endpoint semantics;
- quantize/dequantize output for reference vectors.

Changing any of these without a new type ID would reinterpret existing model bytes. [S-QT-ENUM-FORK](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/include/ggml.h) [S-ROCMFP4-H](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/rocmfp4/rocmfp4.h) [S-ROCMFPX-H](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/rocmfpx/rocmfpx.h) [S-C-120](https://github.com/charlie12345/ROCmFPX/commit/120227d3d34a0d52cfee168964d0a7e3212960a9)

## Reference-first architecture

The fork keeps C reference implementations under `ggml/rocmfp4/` and `ggml/rocmfpx/`, then integrates them through shared type traits, CPU dispatch, model quantization, GGUF Python constants, and accelerated backends. The correct migration order is therefore **format structs → CPU oracle → type registry → quantizer/loader → accelerated kernels → server/model tests**. [S-ROCMFP4-C](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/rocmfp4/rocmfp4.c) [S-ROCMFPX-C](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/rocmfpx/rocmfpx.c) [S-GGML-QUANTS](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/src/ggml-quants.c) [S-CPU-REG](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/src/ggml-cpu/ggml-cpu.c) [S-QUANTIZER](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/src/llama-quant.cpp) [S-GGUF-CONSTANTS](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/gguf-py/gguf/constants.py)

## ROCmFP2 Phase-1 reference

The Phase-1 ROCmFP2 code freezes a 10-byte/32-weight representation and exposes deterministic reference selection/decode paths with MORD/MSM mappings. Its value is as a byte-level oracle and CI probe, even if optimized Q2 kernels evolve. [S-ROCMFP2-H](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/rocmfpx/rocmfp2_reference.h) [S-ROCMFP2-C](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/rocmfpx/rocmfp2_reference.c) [S-ROCMFP2-TEST](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/rocmfpx/test_rocmfp2_reference.c) [S-C-848](https://github.com/charlie12345/ROCmFPX/commit/8488bfc69f716b5aa34bf75d1a72466ac75cc5da)

## Backend operation contract

A format should not be declared production-ready merely because `MUL_MAT` works. The fork’s integration reaches CPU and GPU registration/dispatch surfaces for operations such as copy, row gathering/scattering, quantized matrix operations, and outer product; the exact operation matrix must be regenerated against the target upstream. [S-CPU-OPS](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/src/ggml-cpu/ops.cpp) [S-CUDA-DISPATCH](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/src/ggml-cuda/ggml-cuda.cu) [S-VULKAN](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/src/ggml-vulkan/ggml-vulkan.cpp) [S-BACKEND-TEST](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/tests/test-backend-ops.cpp) [S-C-D45](https://github.com/charlie12345/ROCmFPX/commit/d45ceff8d2b67fdebf73ebcb999807b0d322c73b) [S-C-A8B](https://github.com/charlie12345/ROCmFPX/commit/a8b5fa906ccd13c6a8ca06d55aa287854c376868)

## Retain/refresh boundary

| Component | Decision | Rationale | Primary sources |
|---|---|---|---|
| `ggml/rocmfp4/**`, `ggml/rocmfpx/**` | **RETAIN** | Owned block ABI, reference behavior, codebooks, tests. | [S-ROCMFP4-H](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/rocmfp4/rocmfp4.h) [S-ROCMFPX-H](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/rocmfpx/rocmfpx.h) [S-ROCMFP2-TEST](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/rocmfpx/test_rocmfp2_reference.c) |
| `ggml/include/ggml.h`, `ggml/src/ggml*.c`, CPU dispatch | **REFRESH** | Shared upstream files; re-port the custom registrations. | [S-QT-ENUM-FORK](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/include/ggml.h) [S-QT-ENUM-UP](https://github.com/ggml-org/llama.cpp/blob/86d86ed4396b4130922f7b9af26e3d9fc11a591b/ggml/include/ggml.h) [S-CPU-REG](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/src/ggml-cpu/ggml-cpu.c) [S-CPU-OPS](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/src/ggml-cpu/ops.cpp) |
| GGUF Python/converter and quantize tool | **REFRESH** | Shared upstream converter/tool; preserve custom IDs and quantization policy. | [S-GGUF-CONSTANTS](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/gguf-py/gguf/constants.py) [S-CONVERTER-UP](https://github.com/ggml-org/llama.cpp/blob/86d86ed4396b4130922f7b9af26e3d9fc11a591b/convert_hf_to_gguf.py) [S-QUANT-TOOL](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/tools/quantize/quantize.cpp) |
| TurboQuant integration | **REFRESH** | Preserve the format contract, rebase onto current KV-cache/FlashAttention internals. | [S-QT-ENUM-FORK](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/include/ggml.h) [S-PR27-FILES](https://github.com/charlie12345/ROCmFPX/pull/27/files) [S-UP-HEAD](https://github.com/ggml-org/llama.cpp/tree/86d86ed4396b4130922f7b9af26e3d9fc11a591b) |
