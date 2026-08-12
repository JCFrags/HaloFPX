# Source ledger

> **Audit snapshot:** fork `a5605a72768c6562241b248e268e33dc92787394`; upstream `86d86ed4396b4130922f7b9af26e3d9fc11a591b`; inventory date `2026-07-17`.  
> **Decision vocabulary:** **RETAIN** = capability/ABI must survive; **REFRESH** = preserve capability but re-port onto current upstream; **RETIRE** = remove the fork patch and use upstream or archive it. [S-FORK-HEAD](https://github.com/charlie12345/ROCmFPX/tree/a5605a72768c6562241b248e268e33dc92787394) [S-UP-HEAD](https://github.com/ggml-org/llama.cpp/tree/86d86ed4396b4130922f7b9af26e3d9fc11a591b)

Every factual claim in this wiki links to one or more entries below. Repository code links are pinned to the audited fork or upstream SHA unless the source is a PR/commit page; AMD links are official documentation.

| Source ID | Description | Evidence type | Primary source |
| --- | --- | --- | --- |
| `S-AMD-LLAMA` | ROCm llama.cpp compatibility | Official AMD documentation | [open](https://rocm.docs.amd.com/en/develop/compatibility/ml-compatibility/llama-cpp-compatibility.html) |
| `S-AMD-ROCWMMA` | rocWMMA supported architectures | Official AMD documentation | [open](https://rocm.docs.amd.com/projects/rocWMMA/en/develop/api-reference/api-reference-guide.html#supported-gpu-architectures) |
| `S-AMD-STRIX` | AMD Strix Halo system optimization | Official AMD documentation | [open](https://rocm.docs.amd.com/en/docs-7.2.0/how-to/system-optimization/strixhalo.html) |
| `S-BACKEND-TEST` | ROCmFPX backend operation test coverage | Primary test code | [open](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/tests/test-backend-ops.cpp) |
| `S-BASELINE` | ROCmFP4 upstream-integration baseline note | Primary repository documentation | [open](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ROCMFP4-UPSTREAM-INTEGRATION.md) |
| `S-BUILD-STRIX` | Strix Halo build script | Primary build code | [open](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/scripts/build-strix-rocmfp4-mtp.sh) |
| `S-C-01D` | XCFramework script restoration | Primary commit | [open](https://github.com/charlie12345/ROCmFPX/commit/01d463bb23c3f290688c4529c13b3b467fa2f7dc) |
| `S-C-063` | CI/WebUI isolation | Primary commit | [open](https://github.com/charlie12345/ROCmFPX/commit/0631515b0bca7859387e3467a6f6ac6379622a02) |
| `S-C-0D7` | Portable disk-cache failure probes | Primary commit | [open](https://github.com/charlie12345/ROCmFPX/commit/0d7ac512e5eb511843797c07a16bc7d97d3bc4f8) |
| `S-C-0E5` | Metal GET_ROWS capability correction | Primary commit | [open](https://github.com/charlie12345/ROCmFPX/commit/0e5159daa31132e16a1cc45f724c410b1d236185) |
| `S-C-0FF` | Backend token sampling across slots | Primary commit | [open](https://github.com/charlie12345/ROCmFPX/commit/0ff0b4d03e74d9bc8a092ce2dd177d24cfdea007) |
| `S-C-120` | FP6 endpoint semantics across backends | Primary commit | [open](https://github.com/charlie12345/ROCmFPX/commit/120227d3d34a0d52cfee168964d0a7e3212960a9) |
| `S-C-16E` | Bounded HY3 WebGPU matrix | Primary commit | [open](https://github.com/charlie12345/ROCmFPX/commit/16ed874b3d950a67be0d14311708879a435ad3ca) |
| `S-C-276` | One-slot automatic policy for strict HY3 | Primary commit | [open](https://github.com/charlie12345/ROCmFPX/commit/2766f419526ea14ba1be8f31eca21263cfc52813) |
| `S-C-62F` | ROCm/Vulkan ROCmFPX coherency repair | Primary commit | [open](https://github.com/charlie12345/ROCmFPX/commit/62f7508b12c6b8510fd7a77dfc5d9519fa026d82) |
| `S-C-630` | HY3 MTP/NextN model graph | Primary commit | [open](https://github.com/charlie12345/ROCmFPX/commit/630fa5a0f8fc04689b86d1b0a3d75b2b7d546d07) |
| `S-C-756` | UTF-8 disk-cache paths | Primary commit | [open](https://github.com/charlie12345/ROCmFPX/commit/756121a5e8e7da464aebd2ab344a2aefef6cecac) |
| `S-C-7D7` | Strict greedy HY3 MTP verification | Primary commit | [open](https://github.com/charlie12345/ROCmFPX/commit/7d7b06bc5e6fd4a128af8efa954caa7409376a79) |
| `S-C-807` | Backend/parser source-snapshot restoration | Primary commit | [open](https://github.com/charlie12345/ROCmFPX/commit/80732f992f7c75e9154cfe184041f1384c59a0fb) |
| `S-C-848` | ROCmFP2 reference CI probe | Primary commit | [open](https://github.com/charlie12345/ROCmFPX/commit/8488bfc69f716b5aa34bf75d1a72466ac75cc5da) |
| `S-C-8FC` | Chat whitespace upstream backport | Primary commit | [open](https://github.com/charlie12345/ROCmFPX/commit/8fcff76bca27ea2818761adad681cb792eb2fa26) |
| `S-C-A8B` | Q2 ROCmFPX CPU OUT_PROD | Primary commit | [open](https://github.com/charlie12345/ROCmFPX/commit/a8b5fa906ccd13c6a8ca06d55aa287854c376868) |
| `S-C-A8C` | WebUI provisioning upstream backports | Primary commit | [open](https://github.com/charlie12345/ROCmFPX/commit/a8c41d31423e3e0b6f2dc7138af1d4fba8edb295) |
| `S-C-B133` | TurboQuant portability/type-handling repair | Primary commit | [open](https://github.com/charlie12345/ROCmFPX/commit/b1331a2dd4ca774833a26d7682b6ce1f9c022d4a) |
| `S-C-B56` | Per-request speculative overrides | Primary commit | [open](https://github.com/charlie12345/ROCmFPX/commit/b56ad79d77bc1eb9fe6407640eec8b8edfc04900) |
| `S-C-BB7` | Portable cache file synchronization | Primary commit | [open](https://github.com/charlie12345/ROCmFPX/commit/bb7d9cb5965e3be1ce2073134ba14787bf378113) |
| `S-C-C81` | SSD prompt cache for stateful MTP | Primary commit | [open](https://github.com/charlie12345/ROCmFPX/commit/c81c7c92233b6370b4eb7087398779a8dcb234a4) |
| `S-C-CCAC` | ROCmFPX README/MTP results update | Primary commit | [open](https://github.com/charlie12345/ROCmFPX/commit/ccac6e55ec7c0fa8acdcb6e80b1a242e4f7d654e) |
| `S-C-D45` | CPU ROCmFPX OUT_PROD routing | Primary commit | [open](https://github.com/charlie12345/ROCmFPX/commit/d45ceff8d2b67fdebf73ebcb999807b0d322c73b) |
| `S-C-D52` | Server read_file indentation upstream backport | Primary commit | [open](https://github.com/charlie12345/ROCmFPX/commit/d52c96a8339325417624351bebad194c3864cb26) |
| `S-C-DF3` | ROCm 7.2 negative-infinity semantics | Primary commit | [open](https://github.com/charlie12345/ROCmFPX/commit/df3b8a0efa4dbdfcc2e29dac367c69eed310ed24) |
| `S-C-E600` | WebGPU/Hexagon coherent snapshot restoration | Primary commit | [open](https://github.com/charlie12345/ROCmFPX/commit/e6009eb76d55062d22357e6f117a829e861be01b) |
| `S-C-EFF` | Native MTP hardening and effective limits | Primary commit | [open](https://github.com/charlie12345/ROCmFPX/commit/eff9987923b58d1a6b7e54610c667803ac2d0ea7) |
| `S-C-F961` | HY3 MTP conversion/split export | Primary commit | [open](https://github.com/charlie12345/ROCmFPX/commit/f961404519a2ed286b750ba1419d40318a6b9a92) |
| `S-C-FE2` | Jinja CRT base guard upstream backport | Primary commit | [open](https://github.com/charlie12345/ROCmFPX/commit/fe2b7dc5e19a5e24c276593368a1bb41d0e27b1d) |
| `S-C-FF8` | WebUI build repair | Primary commit | [open](https://github.com/charlie12345/ROCmFPX/commit/ff8e7b8cf9dab714951df49d71f5835a7322404a) |
| `S-C-FUND` | Funding-link commit | Primary commit | [open](https://github.com/charlie12345/ROCmFPX/commit/1b49f4a004b8a2e0183c6cda7a61e7d322ac8d60) |
| `S-C-PR27-M` | PR #27 merge | Primary commit | [open](https://github.com/charlie12345/ROCmFPX/commit/c2845bf86a5c1842d33bd9e990b2bcaf75779251) |
| `S-C-PR28-M` | PR #28 merge | Primary commit | [open](https://github.com/charlie12345/ROCmFPX/commit/6bf20cd688ba0af882d1f68ba50b292edf646ab4) |
| `S-C-PR31-M` | PR #31 merge | Primary commit | [open](https://github.com/charlie12345/ROCmFPX/commit/25c71fc6e12d73bb3804127e032d29fb8976ae40) |
| `S-C-PR32-M` | PR #32 merge/current head | Primary commit | [open](https://github.com/charlie12345/ROCmFPX/commit/a5605a72768c6562241b248e268e33dc92787394) |
| `S-CMAKE-PRESETS` | Strix ROCmFPX CMake preset | Primary build code | [open](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/CMakePresets.json) |
| `S-COMPARE` | Exact 100-commit post-baseline compare | Primary commit graph | [open](https://github.com/charlie12345/ROCmFPX/compare/5b3956605309dd3e6beed49c8f3a41423ba71d25...a5605a72768c6562241b248e268e33dc92787394) |
| `S-CONTEXT-FORK` | Fork context/graph execution integration | Primary code | [open](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/src/llama-context.cpp) |
| `S-CONVERTER-FORK` | Fork HF-to-GGUF converter | Primary code | [open](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/convert_hf_to_gguf.py) |
| `S-CONVERTER-UP` | Current upstream HF-to-GGUF converter | Primary code | [open](https://github.com/ggml-org/llama.cpp/blob/86d86ed4396b4130922f7b9af26e3d9fc11a591b/convert_hf_to_gguf.py) |
| `S-CPU-OPS` | CPU operation dispatch | Primary code | [open](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/src/ggml-cpu/ops.cpp) |
| `S-CPU-REG` | CPU type-trait registration | Primary code | [open](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/src/ggml-cpu/ggml-cpu.c) |
| `S-CUDA-DISPATCH` | Shared CUDA/HIP backend dispatch | Primary backend code | [open](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/src/ggml-cuda/ggml-cuda.cu) |
| `S-DEQUANT` | ROCmFPX device dequantization | Primary kernel code | [open](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/src/ggml-cuda/dequantize.cuh) |
| `S-EXT-FORK` | Fork private llama extension API | Primary code | [open](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/src/llama-ext.h) |
| `S-EXT-UP` | Current upstream private llama extension API | Primary code | [open](https://github.com/ggml-org/llama.cpp/blob/86d86ed4396b4130922f7b9af26e3d9fc11a591b/src/llama-ext.h) |
| `S-FORK-HEAD` | ROCmFPX pinned source tree | Primary code | [open](https://github.com/charlie12345/ROCmFPX/tree/a5605a72768c6562241b248e268e33dc92787394) |
| `S-GGML-QUANTS` | Quant traits and conversion integration | Primary code | [open](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/src/ggml-quants.c) |
| `S-GGUF-CONSTANTS` | GGUF Python type constants | Primary code | [open](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/gguf-py/gguf/constants.py) |
| `S-GRAPH-FORK` | Fork graph plumbing | Primary code | [open](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/src/llama-graph.cpp) |
| `S-HIP-CODEBOOK` | ROCmFPX HIP codebook helpers | Primary kernel code | [open](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/rocmfpx/rocmfpx_hip_codebook.cuh) |
| `S-HIP-QUALITY` | HIP kernel quality workflow | Primary CI code | [open](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/.github/workflows/hip-quality-check.yml) |
| `S-HYV3-FORK` | Fork HY3 model and MTP graph | Primary code | [open](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/src/models/hyv3.cpp) |
| `S-HYV3-UP` | Current upstream HY3 model | Primary code | [open](https://github.com/ggml-org/llama.cpp/blob/86d86ed4396b4130922f7b9af26e3d9fc11a591b/src/models/hy-v3.cpp) |
| `S-MMQ` | ROCmFPX MMQ/GEMM integration | Primary kernel code | [open](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/src/ggml-cuda/mmq.cu) |
| `S-MMQ-H` | ROCmFPX MMQ tile loaders | Primary kernel code | [open](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/src/ggml-cuda/mmq.cuh) |
| `S-MMVQ` | ROCmFPX MMVQ/GEMV dispatch and Strix tuning | Primary kernel code | [open](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/src/ggml-cuda/mmvq.cu) |
| `S-PR27` | PR #27 — promote validated experimental branch | Primary patch series | [open](https://github.com/charlie12345/ROCmFPX/pull/27) |
| `S-PR27-COMMITS` | PR #27 commit list | Primary commit list | [open](https://github.com/charlie12345/ROCmFPX/pull/27/commits) |
| `S-PR27-FILES` | PR #27 file diff | Primary patch map | [open](https://github.com/charlie12345/ROCmFPX/pull/27/files) |
| `S-PR28` | PR #28 — funding link | Primary patch series | [open](https://github.com/charlie12345/ROCmFPX/pull/28) |
| `S-PR29` | Draft PR #29 — SSD prompt cache | Primary patch series | [open](https://github.com/charlie12345/ROCmFPX/pull/29) |
| `S-PR30` | Draft PR #30 — HY3 IFP2/adaptive MoE/cache | Primary patch series | [open](https://github.com/charlie12345/ROCmFPX/pull/30) |
| `S-PR31` | PR #31 — per-request speculative overrides | Primary patch series | [open](https://github.com/charlie12345/ROCmFPX/pull/31) |
| `S-PR31-FILES` | PR #31 file diff | Primary patch map | [open](https://github.com/charlie12345/ROCmFPX/pull/31/files) |
| `S-PR32` | PR #32 — HY3 MTP, ROCmFP2, server fixes | Primary patch series | [open](https://github.com/charlie12345/ROCmFPX/pull/32) |
| `S-PR32-COMMITS` | PR #32 commit list | Primary commit list | [open](https://github.com/charlie12345/ROCmFPX/pull/32/commits) |
| `S-PR32-FILES` | PR #32 file diff | Primary patch map | [open](https://github.com/charlie12345/ROCmFPX/pull/32/files) |
| `S-QT-ENUM-FORK` | ROCmFPX ggml type enumeration | Primary code | [open](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/include/ggml.h) |
| `S-QT-ENUM-UP` | Current upstream ggml type enumeration | Primary code | [open](https://github.com/ggml-org/llama.cpp/blob/86d86ed4396b4130922f7b9af26e3d9fc11a591b/ggml/include/ggml.h) |
| `S-QUANT-TOOL` | llama-quantize CLI integration | Primary code | [open](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/tools/quantize/quantize.cpp) |
| `S-QUANTIZER` | Model quantizer integration | Primary code | [open](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/src/llama-quant.cpp) |
| `S-README-FORK` | ROCmFPX project README | Primary repository documentation | [open](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/README.md) |
| `S-RECIPES` | ROCmFPX quantization recipes | Primary repository documentation | [open](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/docs/recipes/README.md) |
| `S-ROCMFP2-C` | ROCmFP2 Phase-1 reference algorithms | Primary code | [open](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/rocmfpx/rocmfp2_reference.c) |
| `S-ROCMFP2-H` | ROCmFP2 Phase-1 frozen format/reference API | Primary code | [open](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/rocmfpx/rocmfp2_reference.h) |
| `S-ROCMFP2-TEST` | ROCmFP2 deterministic reference test | Primary test code | [open](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/rocmfpx/test_rocmfp2_reference.c) |
| `S-ROCMFP4-C` | ROCmFP4 CPU reference implementation | Primary code | [open](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/rocmfp4/rocmfp4.c) |
| `S-ROCMFP4-H` | ROCmFP4 on-disk block definitions | Primary code | [open](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/rocmfp4/rocmfp4.h) |
| `S-ROCMFPX-C` | ROCmFPX CPU reference implementation | Primary code | [open](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/rocmfpx/rocmfpx.c) |
| `S-ROCMFPX-CI` | ROCmFPX validation workflow | Primary CI code | [open](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/.github/workflows/check-rocmfpx.yml) |
| `S-ROCMFPX-H` | ROCmFPX block definitions | Primary code | [open](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/rocmfpx/rocmfpx.h) |
| `S-ROCMFPX-README` | ROCmFPX format/backend notes | Primary repository documentation | [open](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/rocmfpx/README.md) |
| `S-RPC-FORK` | Fork RPC implementation | Primary code | [open](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/src/ggml-rpc/ggml-rpc.cpp) |
| `S-RPC-IGPU-FIX` | Upstream RPC+iGPU/Strix Halo device-selection fix | Primary upstream commit | [open](https://github.com/ggml-org/llama.cpp/commit/1738129bee5c81b06fa1850daf3f958813c76f5f) |
| `S-RPC-LIGHTNING` | Upstream Lightning Indexer and RPC version bump | Primary upstream commit | [open](https://github.com/ggml-org/llama.cpp/commit/00f5442cc4e805293280c8f85d21d8f9d4aad206) |
| `S-RPC-MTP-FIX` | Upstream RPC multi-context graph UID fix | Primary upstream commit | [open](https://github.com/ggml-org/llama.cpp/commit/c3e9ade6dd3ff2a1ceafd2d59062634715b472c4) |
| `S-RPC-UP` | Current upstream RPC implementation | Primary upstream code | [open](https://github.com/ggml-org/llama.cpp/blob/86d86ed4396b4130922f7b9af26e3d9fc11a591b/ggml/src/ggml-rpc/ggml-rpc.cpp) |
| `S-SERVER-ARGS` | Fork server arguments | Primary code | [open](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/common/arg.cpp) |
| `S-SERVER-CTX` | Fork server context/cache/MTP integration | Primary code | [open](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/tools/server/server-context.cpp) |
| `S-SERVER-DOC` | Fork serving documentation | Primary repository documentation | [open](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/docs/ROCmFPX-SERVING.md) |
| `S-SERVER-SCHEMA-UP` | Current upstream server schema | Primary upstream code | [open](https://github.com/ggml-org/llama.cpp/blob/86d86ed4396b4130922f7b9af26e3d9fc11a591b/tools/server/server-schema.cpp) |
| `S-SERVER-TASK` | Fork request parsing/task behavior | Primary code | [open](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/tools/server/server-task.cpp) |
| `S-SERVER-TEST` | Fork disk-cache server tests | Primary test code | [open](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/tools/server/tests/unit/test_prompt_cache_disk.py) |
| `S-SPEC-FORK` | Fork speculative decoding implementation | Primary code | [open](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/common/speculative.cpp) |
| `S-SPEC-UP` | Current upstream speculative decoding implementation | Primary code | [open](https://github.com/ggml-org/llama.cpp/blob/86d86ed4396b4130922f7b9af26e3d9fc11a591b/common/speculative.cpp) |
| `S-SWEEP-BACKEND` | ROCmFPX backend sweep | Primary validation code | [open](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/scripts/sweep-rocmfpx-backend-ops.sh) |
| `S-TUNE-FLAGS` | ROCmFPX decode tuning flags | Primary build/tuning code | [open](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/scripts/rocmfp4-decode-tune-flags.sh) |
| `S-UP-HEAD` | llama.cpp pinned upstream tree | Primary code | [open](https://github.com/ggml-org/llama.cpp/tree/86d86ed4396b4130922f7b9af26e3d9fc11a591b) |
| `S-UP-HY3-MERGE` | Upstream HY3 merge commit | Primary upstream commit | [open](https://github.com/ggml-org/llama.cpp/commit/2969d6d15d67a08e7b83f26164b15350c79c5248) |
| `S-UP-PR25395` | Upstream PR #25395 — HY3 with MTP | Primary upstream patch series | [open](https://github.com/ggml-org/llama.cpp/pull/25395) |
| `S-UP-SPLIT-MTP` | Upstream split MTP export | Primary upstream commit | [open](https://github.com/ggml-org/llama.cpp/commit/cb489bc0fb789c2cb7a9cc9dc44fa71893fe0988) |
| `S-VECDOT` | ROCmFPX quantized dot products | Primary kernel code | [open](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/src/ggml-cuda/vecdotq.cuh) |
| `S-VULKAN` | ROCmFPX Vulkan backend integration | Primary backend code | [open](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/src/ggml-vulkan/ggml-vulkan.cpp) |
| `S-VULKAN-FP6` | ROCmFPX FP6 Vulkan dequant shader | Primary shader code | [open](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/src/ggml-vulkan/vulkan-shaders/dequant_rocmfpx_fp6.comp) |
| `S-VULKAN-TYPES` | ROCmFPX Vulkan shader types | Primary shader code | [open](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/src/ggml-vulkan/vulkan-shaders/types.glsl) |
