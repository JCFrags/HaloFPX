---
section_id: "12"
title: "Codebase Architecture Facts and Constraints"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories:
    - "ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689"
    - "charlie12345/ROCmFPX@a5605a72768c6562241b248e268e33dc92787394"
    - "fewtarius/CachyLLama@6be745998f568e379ea197fcf827baec73ff9940"
    - "fewtarius/llama-ai@1017f3dfdce3ca2b06aa9007b23295db3bb35722"
  software_versions: []
  hardware_revisions: []
related_sections: ["11", "13", "14", "16", "32", "36", "51", "56"]
---

# Facts and constraints

## Upstream module map

| Concern | Primary files at `llama.cpp@788e07d` | Verified call path or boundary |
|---|---|---|
| Public API | `include/llama.h`, `src/llama.cpp` | `llama_model_load_from_file()` enters `llama_model_load_from_file_impl()` and internal `llama_model_load()` [S12-001] |
| GGUF and model load | `src/llama-model-loader.{h,cpp}`, `src/llama-model.{h,cpp}`, `ggml/include/gguf.h`, `ggml/src/gguf.cpp` | Loader opens one or more GGUF shards with `gguf_init_from_file*()`, reads metadata/tensors, and assigns storage/backend buffers [S12-001][S12-002] |
| Architecture/graph construction | `src/llama-arch.*`, `src/llama-model.cpp`, `src/llama-graph.*`, architecture-specific `src/models/*` | `llama_context` asks `llama_model::build_graph()` for a graph parameterized by the current microbatch and memory contexts [S12-001] |
| Runtime context and memory | `src/llama-context.*`, `src/llama-memory.*`, `src/llama-kv-cache.*`, recurrent/architecture cache files | Context owns scheduler and model-specific memory; standard attention uses `llama_kv_cache`, while other architectures may use different memory implementations [S12-005] |
| Backend registration | `ggml/src/ggml-backend-reg.cpp`, `ggml/include/ggml-backend.h`, backend subdirectories under `ggml/src/` | Backend registry exposes devices/buffer types; static builds register compiled backends and dynamic builds can load backend libraries [S12-003] |
| Scheduler | `ggml/src/ggml-backend.cpp`, `src/llama-context.cpp` | Context creates `ggml_backend_sched`; graph allocation/splitting/copying and asynchronous compute are scheduler responsibilities [S12-003] |
| HIP/ROCm | `ggml/src/ggml-hip/CMakeLists.txt`, shared implementation in `ggml/src/ggml-cuda/`, `ggml/include/ggml-cuda.h` | HIP builds reuse the CUDA-named backend interface with `GGML_USE_HIP`; the runtime device name is `ROCm` [S12-006] |
| Vulkan | `ggml/src/ggml-vulkan/`, `ggml/include/ggml-vulkan.h` | Vulkan owns device/buffer/op support and shader generation; it registers through `ggml_backend_vk_reg()` [S12-006] |
| RPC | `ggml/include/ggml-rpc.h`, `ggml/src/ggml-rpc/`, `tools/rpc/rpc-server.cpp`, `common/arg.cpp` | CLI parsing calls `ggml_backend_rpc_add_server()`; the resulting remote devices enter the normal backend registry/scheduler surface [S12-004] |
| Server and scheduling | `tools/server/server.cpp`, `server-http.*`, `server-context.*`, `server-task.*`, `server-queue.*`, `server-stream.*` | HTTP handlers post `server_task` objects; `server_queue` drives slot/model work and `server_response` returns results [S12-007] |
| Speculative decoding | `common/speculative.*`, `tools/server/server-context.cpp`, speculative examples/tests | Server initializes target/draft state through `common_speculative_init_from_params()` and uses common speculative generation/sampling logic [S12-007] |
| Quantization/conversion | `conversion/`, `convert_hf_to_gguf.py`, `gguf-py/`, `tools/quantize/`, `src/llama-quant.cpp`, `ggml/src/ggml-quants.c` plus backend kernels | Conversion produces GGUF; `llama-quantize` selects tensor types and calls model quantization; each executable type needs layout and compute support [S12-008] |
| Tests/scripts/packaging | `tests/`, `ggml/tests/`, `scripts/`, `ci/`, `.github/workflows/`, root/ggml CMake and `cmake/*` | CMake independently gates tests, tools, server, shared libraries, dynamic backends, HIP, Vulkan, and installable library/config/pkg-config artifacts [S12-012] |

## Important call paths

### Load and execute

1. **[VERIFIED]** `tools/server/server.cpp` initializes backends and calls `server_context::load_model()`. [S12-007]
2. **[VERIFIED]** Common initialization calls the public model-loading API; `llama_model_loader` parses GGUF metadata/shards and maps or reads tensors. [S12-001][S12-002]
3. **[VERIFIED]** `llama_context` creates a backend scheduler from selected backend/device buffer types. [S12-003]
4. **[VERIFIED]** Decode builds a model graph, allocates/splits it through the scheduler, then calls `ggml_backend_sched_graph_compute_async()`. [S12-001][S12-003]

### State persistence

1. **[VERIFIED]** The public API exposes whole-context and per-sequence get/set/save/load functions in `include/llama.h`. [S12-005]
2. **[VERIFIED]** `llama_context` serializes model/context memory through its memory interfaces; state is broader than raw attention K/V bytes. [S12-005]
3. **[VERIFIED]** CachyLLama's `server-context-ssd-cache.cpp` obtains per-sequence target and optional draft blobs with `llama_state_seq_get_data_ext()`, stores them through `kv_ssd_*`, then restores with `llama_state_seq_set_data_ext()`. [S12-010]
4. **[INFERENCE]** Persisted state must be treated as runtime- and configuration-coupled until a compatibility contract proves otherwise, because its producer/consumer is internal context-memory serialization and CachyLLama adds its own metadata/indexing around that blob. [S12-005][S12-010]

## Fork maps

### ROCmFPX

**[VERIFIED]** The pinned tree adds `ggml/rocmfp4/` and `ggml/rocmfpx/` reference format implementations, additional GGML type/layout handling, HIP/CUDA template instances, Vulkan dequant/copy/attention shaders, quantization policies, build scripts, regression scripts, and recipe documentation. [S12-009]

**[VERIFIED]** Its format support is coupled across at least these boundaries:

- GGML type identifiers and block structures;
- byte-size/type-trait tables and CPU reference conversions;
- GGUF read/write and `llama-quantize` tensor-selection policy;
- HIP/ROCm and Vulkan op/type support;
- FlashAttention/MMQ template instantiation;
- conversion scripts, tests, recipes, and CI. [S12-009]

**[INFERENCE]** A format patch cannot be considered integrated if a model merely loads. Correctness needs reference dequantization, representative ops, backend dispatch, quantize/dequantize round trips, and matched logits/quality checks.

### CachyLLama

**[VERIFIED]** The pinned tree adds `common/kv-ssd-cache.*`, `kv-ssd-system-cache.*`, `kv_page_manager.*`, server adapters/page management, cache CLI flags in `common/arg.cpp`, and server integration in the already large `server-context.cpp`. [S12-010]

**[VERIFIED]** The cache code owns disk/index/tier behavior, while libllama owns serialization semantics and the server owns slot identity, request identity, checkpoint timing, target/draft coordination, and restore decisions. [S12-005][S12-010]

**[VERIFIED]** The design document describes first-class `llama_user_id` routing and concurrency isolation, but explicitly says code is the behavioral source of truth. [S12-013]

### llama-ai

**[VERIFIED]** `llama-ai` pins CachyLLama as a Git submodule, with wrapper/build logic in `llama-run.sh`, `scripts/`, three backend build wrappers under `src/`, benchmark orchestration, and `systemd/llama-server.service`. [S12-011]

**[INFERENCE]** Hardware profiles and operational defaults belong in this outer layer or a HaloFPX equivalent, not in GGML kernels or persistent-state formats.

## Ownership and coupling constraints

| Boundary | Proposed owner | Inputs/outputs | Coupling risk |
|---|---|---|---|
| GGUF/type ABI | upstream-derived core plus ROCmFPX patch owner | type IDs, block layout, metadata | Critical: an ID/layout mismatch silently corrupts interpretation |
| Graph/model semantics | libllama model owners | graph, memory context, logits | Critical: recurrent/MoE/speculative state differs by architecture |
| Backend op execution | ggml backend owners | tensors, buffers, events | High: HIP and Vulkan support matrices need parity or explicit fallback |
| Graph placement | ggml scheduler / future Halo planner | device/buffer assignment, copies | High: existing scheduler is not a complete two-node policy engine |
| RPC transport | ggml RPC owner / Halo transport adapter | remote allocation, tensor data, graph commands | High: protocol, security, failure, and dual-link behavior are cross-cutting |
| Persistent checkpoint | HaloKV owner using a narrow state adapter | fingerprinted state blobs and metadata | Critical: compatibility, corruption, privacy, rank ownership |
| HTTP/session scheduling | server owner | tasks, slots, responses | High: Cachy additions currently touch central server paths |
| Packaging/profiles | HaloFPX operations layer | binaries, configs, services | Medium: should consume stable lower-layer interfaces |

## Constraints and conflicts

- **[VERIFIED]** The four snapshots are not the same upstream revision; file-count or path comparison is orientation only, not a semantic patch inventory. Exact lineage belongs in section 11. [S12-001][S12-009][S12-010][S12-011]
- **[VERIFIED]** HIP and Vulkan are separate build-time backend choices with different implementation and validation surfaces. [S12-006][S12-012]
- **[INFERENCE]** ROCmFPX and CachyLLama cannot safely be combined by copying their unique files: both also modify shared core/common/server/build files.
- **[OPEN]** Current RPC device support does not by itself prove dual-link striping, rank ownership, recovery, or tensor-parallel collectives on the Halo pair.
- **[OPEN]** No on-machine evidence in this section establishes ROCmFPX type support across both target backends or CachyLLama restore correctness for target model families.

