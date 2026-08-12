---
section_id: "12"
title: "Codebase Architecture Sources"
status: "verified"
last_verified: "2026-07-16"
applies_to:
  repositories:
    - "ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689"
    - "charlie12345/ROCmFPX@a5605a72768c6562241b248e268e33dc92787394"
    - "fewtarius/CachyLLama@6be745998f568e379ea197fcf827baec73ff9940"
    - "fewtarius/llama-ai@1017f3dfdce3ca2b06aa9007b23295db3bb35722"
  software_versions: []
  hardware_revisions: []
related_sections: ["11", "13", "14", "16"]
---

# Sources

Access date for every Internet source: **2026-07-16**. Repository HEADs were verified with `git ls-remote`, then shallow-cloned and inspected locally. GitHub blob/tree URLs are pinned to full commits.

## Primary source records

### S12-001 - llama.cpp core model/context sources

- Publisher/repository: ggml-org/llama.cpp
- Revision: `788e07dc91d266ad3162a1ce9037665656269689`
- URLs: [commit](https://github.com/ggml-org/llama.cpp/commit/788e07dc91d266ad3162a1ce9037665656269689), [`src/llama.cpp`](https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/src/llama.cpp), [`src/llama-model.cpp`](https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/src/llama-model.cpp), [`src/llama-context.cpp`](https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/src/llama-context.cpp)
- Supports: public/internal model-load path, graph construction, context execution.
- Limitations: snapshot only; commit timestamp falls on 2026-07-16 in project local time but 2026-07-17 in some author/committer offsets.

### S12-002 - GGUF and model loader

- Publisher/repository: ggml-org/llama.cpp
- Revision: `788e07dc91d266ad3162a1ce9037665656269689`
- URLs: [`src/llama-model-loader.cpp`](https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/src/llama-model-loader.cpp), [`ggml/src/gguf.cpp`](https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/ggml/src/gguf.cpp), [`ggml/include/gguf.h`](https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/ggml/include/gguf.h)
- Supports: GGUF parsing, shard loading, loader boundary.
- Limitations: does not by itself define HaloFPX format compatibility.

### S12-003 - backend registry and scheduler

- Publisher/repository: ggml-org/llama.cpp
- Revision: `788e07dc91d266ad3162a1ce9037665656269689`
- URLs: [`ggml/src/ggml-backend-reg.cpp`](https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/ggml/src/ggml-backend-reg.cpp), [`ggml/src/ggml-backend.cpp`](https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/ggml/src/ggml-backend.cpp), [`ggml/include/ggml-backend.h`](https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/ggml/include/ggml-backend.h)
- Supports: backend/device registration, buffer interfaces, scheduler creation and graph compute.
- Limitations: code structure is not evidence of target-machine behavior.

### S12-004 - RPC backend and server

- Publisher/repository: ggml-org/llama.cpp
- Revision: `788e07dc91d266ad3162a1ce9037665656269689`
- URLs: [`ggml/src/ggml-rpc/ggml-rpc.cpp`](https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/ggml/src/ggml-rpc/ggml-rpc.cpp), [`ggml/include/ggml-rpc.h`](https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/ggml/include/ggml-rpc.h), [`tools/rpc/rpc-server.cpp`](https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/tools/rpc/rpc-server.cpp)
- Supports: RPC registration/start interfaces and remote backend implementation.
- Limitations: no dual-link, security, or failure behavior was measured here.

### S12-005 - KV memory and state serialization

- Publisher/repository: ggml-org/llama.cpp
- Revision: `788e07dc91d266ad3162a1ce9037665656269689`
- URLs: [`include/llama.h`](https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/include/llama.h), [`src/llama-kv-cache.cpp`](https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/src/llama-kv-cache.cpp), [`src/llama-memory.cpp`](https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/src/llama-memory.cpp), [`src/llama-context.cpp`](https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/src/llama-context.cpp)
- Supports: sequence/whole-context APIs and context-memory ownership.
- Limitations: no cross-version portability guarantee was found or tested.

### S12-006 - HIP and Vulkan backends

- Publisher/repository: ggml-org/llama.cpp
- Revision: `788e07dc91d266ad3162a1ce9037665656269689`
- URLs: [`ggml/src/ggml-hip/CMakeLists.txt`](https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/ggml/src/ggml-hip/CMakeLists.txt), [`ggml/src/ggml-cuda/`](https://github.com/ggml-org/llama.cpp/tree/788e07dc91d266ad3162a1ce9037665656269689/ggml/src/ggml-cuda), [`ggml/src/ggml-vulkan/`](https://github.com/ggml-org/llama.cpp/tree/788e07dc91d266ad3162a1ce9037665656269689/ggml/src/ggml-vulkan)
- Supports: backend implementation locations and registry interfaces.
- Limitations: support varies by op/type/device; source presence is not validation.

### S12-007 - server and speculative decoding

- Publisher/repository: ggml-org/llama.cpp
- Revision: `788e07dc91d266ad3162a1ce9037665656269689`
- URLs: [`tools/server/`](https://github.com/ggml-org/llama.cpp/tree/788e07dc91d266ad3162a1ce9037665656269689/tools/server), [`common/speculative.cpp`](https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/common/speculative.cpp)
- Supports: server task/queue/context split and target/draft initialization.
- Limitations: API semantics and distributed speculation require dedicated sections.

### S12-008 - conversion and quantization

- Publisher/repository: ggml-org/llama.cpp
- Revision: `788e07dc91d266ad3162a1ce9037665656269689`
- URLs: [`conversion/`](https://github.com/ggml-org/llama.cpp/tree/788e07dc91d266ad3162a1ce9037665656269689/conversion), [`tools/quantize/`](https://github.com/ggml-org/llama.cpp/tree/788e07dc91d266ad3162a1ce9037665656269689/tools/quantize), [`src/llama-quant.cpp`](https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/src/llama-quant.cpp)
- Supports: conversion/quantization ownership and entry paths.
- Limitations: quality and performance are not inferred.

### S12-009 - ROCmFPX tree and feature code

- Publisher/repository: charlie12345/ROCmFPX
- Revision: `a5605a72768c6562241b248e268e33dc92787394` (committed 2026-07-16)
- URLs: [commit](https://github.com/charlie12345/ROCmFPX/commit/a5605a72768c6562241b248e268e33dc92787394), [`ggml/rocmfpx/`](https://github.com/charlie12345/ROCmFPX/tree/a5605a72768c6562241b248e268e33dc92787394/ggml/rocmfpx), [`ggml/rocmfp4/`](https://github.com/charlie12345/ROCmFPX/tree/a5605a72768c6562241b248e268e33dc92787394/ggml/rocmfp4), [`README.md`](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/README.md)
- Supports: ROCmFPX format/kernel/test/recipe surface and repository status claims.
- Limitations: repository benchmarks are environment-scoped claims; no result is promoted as a HaloFPX measurement.

### S12-010 - CachyLLama persistence implementation

- Publisher/repository: fewtarius/CachyLLama
- Revision: `6be745998f568e379ea197fcf827baec73ff9940` (committed 2026-07-08)
- URLs: [commit](https://github.com/fewtarius/CachyLLama/commit/6be745998f568e379ea197fcf827baec73ff9940), [`common/kv-ssd-cache.cpp`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/common/kv-ssd-cache.cpp), [`tools/server/server-context-ssd-cache.cpp`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/tools/server/server-context-ssd-cache.cpp), [`tools/server/server-context-page-manager.cpp`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/tools/server/server-context-page-manager.cpp)
- Supports: SSD cache modules, state API use, server integration.
- Limitations: no build or restore was executed; repository behavior claims remain unmeasured here.

### S12-011 - llama-ai operational wrapper

- Publisher/repository: fewtarius/llama-ai
- Revision: `1017f3dfdce3ca2b06aa9007b23295db3bb35722` (committed 2026-07-08)
- URLs: [commit](https://github.com/fewtarius/llama-ai/commit/1017f3dfdce3ca2b06aa9007b23295db3bb35722), [`README.md`](https://github.com/fewtarius/llama-ai/blob/1017f3dfdce3ca2b06aa9007b23295db3bb35722/README.md), [`.gitmodules`](https://github.com/fewtarius/llama-ai/blob/1017f3dfdce3ca2b06aa9007b23295db3bb35722/.gitmodules), [`scripts/`](https://github.com/fewtarius/llama-ai/tree/1017f3dfdce3ca2b06aa9007b23295db3bb35722/scripts), [`systemd/`](https://github.com/fewtarius/llama-ai/tree/1017f3dfdce3ca2b06aa9007b23295db3bb35722/systemd)
- Supports: submodule pin, wrapper/profile/build/benchmark/service ownership.
- Limitations: README performance claims and hardware defaults were not validated.

### S12-012 - llama.cpp CMake, tests, and installation

- Publisher/repository: ggml-org/llama.cpp
- Revision: `788e07dc91d266ad3162a1ce9037665656269689`
- URLs: [`CMakeLists.txt`](https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/CMakeLists.txt), [`ggml/CMakeLists.txt`](https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/ggml/CMakeLists.txt), [`tests/`](https://github.com/ggml-org/llama.cpp/tree/788e07dc91d266ad3162a1ce9037665656269689/tests), [`ci/`](https://github.com/ggml-org/llama.cpp/tree/788e07dc91d266ad3162a1ce9037665656269689/ci)
- Supports: build options, test entrypoints, installable artifacts.
- Limitations: packaging policy belongs in section 16.

### S12-013 - CachyLLama user-isolation design and code routing

- Publisher/repository: fewtarius/CachyLLama
- Revision: `6be745998f568e379ea197fcf827baec73ff9940`
- URL: [`docs/development/user-isolation-design.md`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/docs/development/user-isolation-design.md)
- Supports: intended identity, cache namespace, and scheduling boundaries; document states code is authoritative.
- Limitations: design documentation is not a security assessment or machine test.

## Local governance consulted

The project `AGENTS.md`, root/category wiki READMEs, research `PROJECT_CONTEXT.md` and `OUTPUT_STANDARD.md`, and `references/agent-harness.md` were followed. The external canonical Agent Harness `AGENTS.md` and `guide/architecture.md` informed evidence promotion and closeout review. They are governance inputs, not evidence for code behavior and are not included in `source_count`.

## Conflicts and freshness

- **[VERIFIED]** Commit hashes above were default-branch HEADs when queried on 2026-07-16, but "current" is not an authority after this access date.
- **[VERIFIED]** Repository snapshots are divergent. A fork-delta statement requires an explicit merge base and belongs in sections 11, 13, or 14.
- **[OPEN]** Licenses and third-party notices need the dedicated section 16 audit before code integration or redistribution.

