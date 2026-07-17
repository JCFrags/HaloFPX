# PR #32 — HY3/ROCmFP2/cache patch map

- **PR:** [#32](https://github.com/charlie12345/ROCmFPX/pull/32)
- **Base:** `25c71fc6e12d73bb3804127e032d29fb8976ae40`
- **Head before merge:** `a8b5fa906ccd13c6a8ca06d55aa287854c376868`
- **Merge:** `a5605a72768c6562241b248e268e33dc92787394`
- **Authored commits:** 41
- **Changed paths:** 73
- **Canonical commits:** [PR commits](https://github.com/charlie12345/ROCmFPX/pull/32/commits)
- **Canonical diff:** [PR files](https://github.com/charlie12345/ROCmFPX/pull/32/files)

Primary source: [S-PR32](https://github.com/charlie12345/ROCmFPX/pull/32)

| Exact path | Subsystem | Decision | Inventory rationale |
| --- | --- | --- | --- |
| `.github/workflows/check-rocmfpx.yml` | Build / CI | **RETAIN** | Fork-specific correctness gate; update implementation details but preserve the gate. |
| `common/arg.cpp` | Common / parser / sampling | **REFRESH** | Shared common/server infrastructure: use current upstream and preserve only fork-specific policy/state behavior. |
| `common/chat-auto-parser-generator.cpp` | Common / parser / sampling | **RETIRE** | Generic parser/Jinja backport; use current upstream. |
| `common/chat-auto-parser.h` | Common / parser / sampling | **RETIRE** | Generic parser/Jinja backport; use current upstream. |
| `common/chat-diff-analyzer.cpp` | Common / parser / sampling | **RETIRE** | Generic parser/Jinja backport; use current upstream. |
| `common/common.h` | Common / parser / sampling | **REFRESH** | Shared common/server infrastructure: use current upstream and preserve only fork-specific policy/state behavior. |
| `common/jinja/value.cpp` | Common / parser / sampling | **RETIRE** | Generic parser/Jinja backport; use current upstream. |
| `common/speculative.cpp` | MTP / speculative decoding | **REFRESH** | Shared common/server infrastructure: use current upstream and preserve only fork-specific policy/state behavior. |
| `common/speculative.h` | MTP / speculative decoding | **REFRESH** | Shared common/server infrastructure: use current upstream and preserve only fork-specific policy/state behavior. |
| `convert_hf_to_gguf.py` | Conversion / GGUF | **REFRESH** | Shared GGUF/converter surface: use current upstream and re-add ROCmFPX IDs, quant mapping, and tests. |
| `docs/recipes/README.md` | Documentation / recipes | **RETAIN** | ROCmFPX-owned quantization recipe or model-quality evidence. |
| `docs/recipes/qwable-ultraquality-7p61bpw-attention-rank.csv` | Documentation / recipes | **RETAIN** | ROCmFPX-owned quantization recipe or model-quality evidence. |
| `docs/recipes/qwable-ultraquality-7p61bpw-rankleave32.md` | Documentation / recipes | **RETAIN** | ROCmFPX-owned quantization recipe or model-quality evidence. |
| `docs/recipes/qwen35-moe-rocmfpx-moequality.md` | Documentation / recipes | **RETAIN** | ROCmFPX-owned quantization recipe or model-quality evidence. |
| `docs/recipes/release-recipe-map.tsv` | Documentation / recipes | **RETAIN** | ROCmFPX-owned quantization recipe or model-quality evidence. |
| `docs/recipes/step35-rocmfpx-q3-qualityplus.md` | Documentation / recipes | **RETAIN** | ROCmFPX-owned quantization recipe or model-quality evidence. |
| `ggml/include/ggml.h` | Other | **REFRESH** | Shared ggml registry/dispatch: preserve ROCmFPX type behavior while rebasing onto current upstream. |
| `ggml/rocmfpx/README.md` | Quantization formats | **RETAIN** | ROCmFPX-owned format contract, CPU reference, codebook, and deterministic tests. |
| `ggml/rocmfpx/rocmfp2_reference.c` | Quantization formats | **RETAIN** | ROCmFPX-owned format contract, CPU reference, codebook, and deterministic tests. |
| `ggml/rocmfpx/rocmfp2_reference.h` | Quantization formats | **RETAIN** | ROCmFPX-owned format contract, CPU reference, codebook, and deterministic tests. |
| `ggml/rocmfpx/rocmfpx.c` | Quantization formats | **RETAIN** | ROCmFPX-owned format contract, CPU reference, codebook, and deterministic tests. |
| `ggml/rocmfpx/rocmfpx.h` | Quantization formats | **RETAIN** | ROCmFPX-owned format contract, CPU reference, codebook, and deterministic tests. |
| `ggml/rocmfpx/rocmfpx_hip_codebook.cuh` | Quantization formats | **RETAIN** | ROCmFPX-owned format contract, CPU reference, codebook, and deterministic tests. |
| `ggml/rocmfpx/test_rocmfp2_reference.c` | Quantization formats | **RETAIN** | ROCmFPX-owned format contract, CPU reference, codebook, and deterministic tests. |
| `ggml/src/ggml-cpu/ggml-cpu.c` | CPU / ggml core | **REFRESH** | Shared ggml registry/dispatch: preserve ROCmFPX type behavior while rebasing onto current upstream. |
| `ggml/src/ggml-cpu/ops.cpp` | CPU / ggml core | **REFRESH** | Shared ggml registry/dispatch: preserve ROCmFPX type behavior while rebasing onto current upstream. |
| `ggml/src/ggml-cuda/common.cuh` | ROCm/HIP kernels | **REFRESH** | Shared backend file: start from current upstream and re-port only ROCmFPX type/kernels and validated Strix tuning. |
| `ggml/src/ggml-cuda/convert.cu` | ROCm/HIP kernels | **REFRESH** | Shared backend file: start from current upstream and re-port only ROCmFPX type/kernels and validated Strix tuning. |
| `ggml/src/ggml-cuda/cpy-utils.cuh` | ROCm/HIP kernels | **REFRESH** | Shared backend file: start from current upstream and re-port only ROCmFPX type/kernels and validated Strix tuning. |
| `ggml/src/ggml-cuda/dequantize.cuh` | ROCm/HIP kernels | **REFRESH** | Shared backend file: start from current upstream and re-port only ROCmFPX type/kernels and validated Strix tuning. |
| `ggml/src/ggml-cuda/getrows.cu` | ROCm/HIP kernels | **REFRESH** | Shared backend file: start from current upstream and re-port only ROCmFPX type/kernels and validated Strix tuning. |
| `ggml/src/ggml-cuda/ggml-cuda.cu` | ROCm/HIP kernels | **REFRESH** | Shared backend file: start from current upstream and re-port only ROCmFPX type/kernels and validated Strix tuning. |
| `ggml/src/ggml-cuda/mmq.cu` | ROCm/HIP kernels | **REFRESH** | Shared backend file: start from current upstream and re-port only ROCmFPX type/kernels and validated Strix tuning. |
| `ggml/src/ggml-cuda/mmq.cuh` | ROCm/HIP kernels | **REFRESH** | Shared backend file: start from current upstream and re-port only ROCmFPX type/kernels and validated Strix tuning. |
| `ggml/src/ggml-cuda/mmvq.cu` | ROCm/HIP kernels | **REFRESH** | Shared backend file: start from current upstream and re-port only ROCmFPX type/kernels and validated Strix tuning. |
| `ggml/src/ggml-cuda/template-instances/generate_cu_files.py` | ROCm/HIP kernels | **REFRESH** | Shared backend file: start from current upstream and re-port only ROCmFPX type/kernels and validated Strix tuning. |
| `ggml/src/ggml-cuda/template-instances/mmq-instance-q2_0_rocmfpx.cu` | ROCm/HIP kernels | **REFRESH** | Shared backend file: start from current upstream and re-port only ROCmFPX type/kernels and validated Strix tuning. |
| `ggml/src/ggml-cuda/vecdotq.cuh` | ROCm/HIP kernels | **REFRESH** | Shared backend file: start from current upstream and re-port only ROCmFPX type/kernels and validated Strix tuning. |
| `ggml/src/ggml-quants.c` | CPU / ggml core | **REFRESH** | Shared ggml registry/dispatch: preserve ROCmFPX type behavior while rebasing onto current upstream. |
| `ggml/src/ggml-vulkan/ggml-vulkan.cpp` | Vulkan kernels | **REFRESH** | Shared backend file: start from current upstream and re-port only ROCmFPX type/kernels and validated Strix tuning. |
| `ggml/src/ggml-vulkan/vulkan-shaders/copy_to_quant.comp` | Vulkan kernels | **REFRESH** | Shared backend file: start from current upstream and re-port only ROCmFPX type/kernels and validated Strix tuning. |
| `ggml/src/ggml-vulkan/vulkan-shaders/flash_attn_dequant.glsl` | Vulkan kernels | **REFRESH** | Shared backend file: start from current upstream and re-port only ROCmFPX type/kernels and validated Strix tuning. |
| `ggml/src/ggml-vulkan/vulkan-shaders/types.glsl` | Vulkan kernels | **REFRESH** | Shared backend file: start from current upstream and re-port only ROCmFPX type/kernels and validated Strix tuning. |
| `ggml/src/ggml.c` | CPU / ggml core | **REFRESH** | Shared ggml registry/dispatch: preserve ROCmFPX type behavior while rebasing onto current upstream. |
| `gguf-py/gguf/constants.py` | Conversion / GGUF | **REFRESH** | Shared GGUF/converter surface: use current upstream and re-add ROCmFPX IDs, quant mapping, and tests. |
| `include/llama.h` | Other | **REFRESH** | Shared llama core/private API: use current upstream and re-port only extension-specific hooks. |
| `models/templates/tencent-Hy3.jinja` | Model architecture | **RETIRE** | Superseded by upstream HY3/MTP implementation; preserve only fork-specific tested deltas elsewhere. |
| `scripts/check-release-recipe-map.py` | Validation / tuning scripts | **REFRESH** | Shared or mixed file; inspect hunks and re-port only ROCmFPX-owned behavior. |
| `scripts/check-rocmfp2-reference.sh` | Validation / tuning scripts | **REFRESH** | Shared or mixed file; inspect hunks and re-port only ROCmFPX-owned behavior. |
| `src/llama-context.cpp` | Graph / context / KV | **REFRESH** | Shared llama core/private API: use current upstream and re-port only extension-specific hooks. |
| `src/llama-context.h` | Graph / context / KV | **REFRESH** | Shared llama core/private API: use current upstream and re-port only extension-specific hooks. |
| `src/llama-ext.h` | Graph / context / KV | **REFRESH** | Shared llama core/private API: use current upstream and re-port only extension-specific hooks. |
| `src/llama-model-loader.cpp` | Model loader / quantization | **REFRESH** | Shared llama core/private API: use current upstream and re-port only extension-specific hooks. |
| `src/llama-model.cpp` | Model loader / quantization | **REFRESH** | Shared llama core/private API: use current upstream and re-port only extension-specific hooks. |
| `src/llama-quant.cpp` | Model loader / quantization | **REFRESH** | Shared llama core/private API: use current upstream and re-port only extension-specific hooks. |
| `src/models/hyv3.cpp` | Model architecture | **RETIRE** | Superseded by upstream HY3/MTP implementation; preserve only fork-specific tested deltas elsewhere. |
| `src/models/models.h` | Model architecture | **REFRESH** | Shared llama core/private API: use current upstream and re-port only extension-specific hooks. |
| `tests/test-arg-parser.cpp` | Tests | **REFRESH** | Shared test/tool file: rebase then retain ROCmFPX-specific cases. |
| `tests/test-backend-ops.cpp` | Tests | **REFRESH** | Shared test/tool file: rebase then retain ROCmFPX-specific cases. |
| `tests/test-chat-auto-parser.cpp` | Tests | **RETIRE** | Tests for generic parser/Jinja backports; use current upstream equivalents. |
| `tests/test-jinja.cpp` | Tests | **RETIRE** | Tests for generic parser/Jinja backports; use current upstream equivalents. |
| `tests/test-quantize-fns.cpp` | Tests | **REFRESH** | Shared test/tool file: rebase then retain ROCmFPX-specific cases. |
| `tools/cli/README.md` | Other | **REFRESH** | Shared or mixed file; inspect hunks and re-port only ROCmFPX-owned behavior. |
| `tools/completion/README.md` | Other | **REFRESH** | Shared or mixed file; inspect hunks and re-port only ROCmFPX-owned behavior. |
| `tools/quantize/quantize.cpp` | Other | **REFRESH** | Shared test/tool file: rebase then retain ROCmFPX-specific cases. |
| `tools/server/README.md` | Server / cache | **REFRESH** | Shared server implementation: use current upstream and re-port disk-cache, strict verification, and request-override deltas. |
| `tools/server/server-context.cpp` | Server / cache | **REFRESH** | Shared server implementation: use current upstream and re-port disk-cache, strict verification, and request-override deltas. |
| `tools/server/server-task.cpp` | Server / cache | **REFRESH** | Shared server implementation: use current upstream and re-port disk-cache, strict verification, and request-override deltas. |
| `tools/server/server-task.h` | Server / cache | **REFRESH** | Shared server implementation: use current upstream and re-port disk-cache, strict verification, and request-override deltas. |
| `tools/server/server-tools.cpp` | Server / cache | **REFRESH** | Shared server implementation: use current upstream and re-port disk-cache, strict verification, and request-override deltas. |
| `tools/server/server.cpp` | Server / cache | **REFRESH** | Shared server implementation: use current upstream and re-port disk-cache, strict verification, and request-override deltas. |
| `tools/server/tests/unit/test_prompt_cache_disk.py` | Server / cache | **REFRESH** | Shared server implementation: use current upstream and re-port disk-cache, strict verification, and request-override deltas. |
| `tools/server/tests/utils.py` | Server / cache | **REFRESH** | Shared server implementation: use current upstream and re-port disk-cache, strict verification, and request-override deltas. |
