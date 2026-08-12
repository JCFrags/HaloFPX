---
section_id: "16"
title: "Build, dependency, license, CI, and workflow facts"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories:
    - "ROCmFPX@a5605a72768c6562241b248e268e33dc92787394"
    - "llama.cpp@788e07dc91d266ad3162a1ce9037665656269689"
    - "CachyLLama@6be745998f568e379ea197fcf827baec73ff9940"
    - "llama-ai@1017f3dfdce3ca2b06aa9007b23295db3bb35722"
  software_versions: ["CMake 3.14...3.28", "C 11", "C++ 17", "Python >=3.10 for ROCmFPX tools"]
  hardware_revisions: ["dual Strix Halo / gfx1151 (planned)"]
related_sections: ["03.11", "03.13", "03.14", "03.15", "04", "11"]
---

# Facts and constraints

## Frozen research baselines

| Repository | Branch HEAD inspected | Commit time | License at root | Build/control surface |
|---|---|---|---|---|
| `charlie12345/ROCmFPX` | `a5605a72768c6562241b248e268e33dc92787394` | 2026-07-16T22:34:40-04:00 | MIT | CMake; ROCmFPX scripts and custom reference CI |
| `ggml-org/llama.cpp` | `788e07dc91d266ad3162a1ce9037665656269689` | 2026-07-17T08:42:59+02:00 | MIT | CMake; broad cross-platform CI |
| `fewtarius/CachyLLama` | `6be745998f568e379ea197fcf827baec73ff9940` | 2026-07-08T20:17:28-04:00 | MIT | CMake; inherited llama.cpp CI plus cache fork |
| `fewtarius/llama-ai` | `1017f3dfdce3ca2b06aa9007b23295db3bb35722` | 2026-07-08T20:21:33-04:00 | GPL-3.0-or-later | Bash orchestration; no GitHub Actions in pinned tree |

**[VERIFIED]** `llama-ai` records CachyLLama as a branch-tracking submodule in `.gitmodules`, but its commit tree pins the gitlink to `6be745998f568e379ea197fcf827baec73ff9940`. Reproducible checkout must use the gitlink, never the declared `branch = master`. [S16-09]

## Build system and compiler contract

**[VERIFIED]** The three C/C++ trees require CMake `3.14...3.28`; the top-level Makefile intentionally fails and redirects users to CMake. `ggml/CMakeLists.txt` requires C11 and C++17 and exports `compile_commands.json`. Default release commands are `cmake -B build` and `cmake --build build --config Release`. [S16-02][S16-06]

**[VERIFIED]** The pinned ROCmFPX Strix script configures both `GGML_HIP=ON` and `GGML_VULKAN=ON`, targets `gfx1151`, disables CUDA and the Web UI, enables llama tests, and builds CLI, server, quantizer, benchmark, perplexity, and backend/quantization tests. Its optional rocWMMA include default contains a maintainer-local absolute path; it must be overridden or left disabled on HaloFPX hosts. [S16-03]

| Backend or feature | CMake switch | Principal external prerequisite | HaloFPX disposition |
|---|---|---|---|
| CPU/reference | `GGML_CPU=ON` | host C/C++ toolchain; OpenMP optional | mandatory correctness oracle |
| HIP/ROCm | `GGML_HIP=ON`; `CMAKE_HIP_ARCHITECTURES=gfx1151` | compatible ROCm/LLVM; optional rocWMMA headers | mandatory target backend; version unverified |
| Vulkan | `GGML_VULKAN=ON` | Vulkan headers/loader and `glslc` or `glslangValidator` | mandatory comparison/fallback backend |
| RPC | `GGML_RPC=ON` | network transport; optional RDMA | research/build only until distributed semantics are approved |
| dynamic backend loading | `BUILD_SHARED_LIBS=ON`, `GGML_BACKEND_DL=ON` | runtime library path discipline | optional; increases packaging surface |
| server TLS | `LLAMA_OPENSSL=ON` | OpenSSL development package | optional; build may omit SSL support |
| structured output | `LLAMA_LLGUIDANCE=ON` | external project pinned by commit | optional; provenance must record fetched source |
| other inherited backends | CUDA, MUSA, Metal, SYCL, OpenCL, CANN, WebGPU, OpenVINO, ZenDNN, Hexagon, etc. | backend-specific SDKs | not release requirements for dual Strix Halo |

**[VERIFIED]** Multiple backends may be compiled into one build, and runtime selection is exposed through device selection. That is upstream capability, not proof that a combined HIP+Vulkan HaloFPX artifact is correct or supportable on the target hosts. [S16-02]

## Dependencies, vendoring, and generated material

**[VERIFIED]** The C/C++ trees do not use Git submodules at the inspected commits. They vendor source such as `cpp-httplib`, `nlohmann/json`, `stb`, `miniaudio`, and subprocess helpers. Optional CMake paths may fetch external projects: llguidance is pinned to a full commit; ZenDNN is pinned to a full commit; CUDA CCCL uses a tag; other backend paths have their own SDK/source resolution. [S16-02][S16-06]

**[VERIFIED]** Python conversion tooling uses requirement files and version ranges rather than a single resolved/hash-locked environment. ROCmFPX declares Python `>=3.10`, NumPy `>=1.25,<2`, and SentencePiece `>=0.1.98,<0.3`; its dependency bounds differ from current llama.cpp. UI subtrees have npm lockfiles, but there is no repository-wide lock covering OS packages, ROCm, Vulkan, CMake-fetched projects, and Python wheels. [S16-02][S16-06]

Generated or build-local material includes:

- `compile_commands.json` from CMake;
- configured `common/build-info.cpp` from `common/build-info.cpp.in`;
- build directories, generated Web UI assets, Vulkan shaders, benchmark reports/logs, and model files;
- downloaded SDKs or optional external-project source trees.

**[VERIFIED]** Build info captures commit, dirty state, compiler, target, and a Git-derived build number. Setting `SOURCE_DATE_EPOCH` disables native CPU optimization by default, improving portability, but the source does not itself prove bit-for-bit reproducibility. [S16-02][S16-15]

## Licensing and notices

| Material | Observed license/notice | Constraint |
|---|---|---|
| ROCmFPX / llama.cpp / CachyLLama source roots | MIT | retain copyright and permission notice in copies/substantial portions |
| ROCmFPX bundled third parties | MIT notices for cpp-httplib, nlohmann/json, gguf-py; `THIRD_PARTY_NOTICES.md` exists | keep the notice file synchronized with actual vendored/fetched content |
| llama.cpp / CachyLLama source files | mostly MIT plus observed Apache-2.0 SPDX files and embedded vendor terms | root MIT file is not an SBOM or complete per-file license conclusion |
| `llama-ai` orchestration repository | GPL-3.0-or-later; its scripts carry matching SPDX identifiers | copying/combining into a distributed work requires GPL compatibility analysis |
| `llama-ai` documentation | CC-BY-NC-SA-4.0 as declared by its README and `AGENTS.md` | do not copy its prose into project documentation without a deliberate compatibility/attribution review |
| models, tokenizers, test corpora, SDKs, drivers | not granted by the source-tree MIT license | record and satisfy each supplier's terms separately |

**[INFERENCE]** Keeping an unmodified GPL program as a separately invoked program is materially different from copying its source into HaloFPX. GNU's own FAQ says mere co-installation need not combine programs, while combinations may need compatible licensing; the exact HaloFPX boundary requires counsel if distributed. [S16-09][S16-14]

## CI and regression gates observed

**[VERIFIED]** ROCmFPX inherits broad platform jobs and adds `.github/workflows/check-rocmfpx.yml`. The custom job is path-filtered and CPU-hosted: it checks Python syntax/options, Bash syntax, released recipe mapping, builds `test-backend-ops` with `GGML_NATIVE=OFF`, runs ROCmFPX/FP2 reference probes, then selected CPU backend operations. It does not execute HIP or Vulkan kernels. [S16-04]

**[VERIFIED]** Inherited workflows cover combinations of CPU, sanitizers, Vulkan, HIP/ROCm, CUDA, SYCL, OpenCL, OpenVINO, RPC, cross-builds, Apple/Android, server, Python lint/type/requirements, code style, editorconfig, vendored-source checks, release, and UI. Coverage is extensive but branch/repository status must be checked; presence of a workflow file is not proof it passed at a commit. [S16-04][S16-08]

**[VERIFIED]** At least some inspected workflow actions use mutable semantic tags (for example `actions/checkout@v6` and `actions/cache@v5`) rather than full action commit SHAs. HaloFPX therefore needs an explicit action-pinning policy instead of assuming inherited workflow provenance is immutable. [S16-04][S16-08]

**[VERIFIED]** The upstream contribution guide asks contributors to run local CI, perplexity and performance checks, and `test-backend-ops` for operator work; it requires tests for new operators, focused PRs, squash merging, and commit titles of `<module> : <title> (#issue)`. Formatting guidance includes four spaces and clang-format 15+ when uncertain. [S16-05]

## AI-agent controls

**[VERIFIED]** ROCmFPX, llama.cpp, and CachyLLama carry the upstream policy: fully or predominantly AI-generated PRs are not accepted; meaningful AI use must be disclosed; humans must understand, debug, and maintain every submitted line; AI-written PR descriptions, commit messages, review replies, and automated submission are prohibited. Agents must not commit or push without explicit human approval for each action. [S16-05][S16-07]

**[VERIFIED]** `llama-ai/AGENTS.md` instead provides operational instructions for its Bash scripts, submodule, build directories, patch deprecation, and hardware assumptions. Those assumptions are scoped evidence, not HaloFPX truth. [S16-10]

**[VERIFIED]** None of the four inspected trees contains a purpose-built append-only AI change log. Therefore the log described in this section is a HaloFPX requirement, not an upstream feature.
