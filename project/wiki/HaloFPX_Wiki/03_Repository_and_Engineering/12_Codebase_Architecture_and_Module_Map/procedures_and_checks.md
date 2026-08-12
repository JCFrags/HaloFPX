---
section_id: "12"
title: "Codebase Architecture Procedures and Checks"
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
related_sections: ["11", "13", "14", "16", "32", "51", "56", "74"]
---

# Procedures and checks

All commands are read-only unless a build directory or explicitly named experiment output is created. No command below requires root. Package installation and service deployment are out of scope.

## Internet/source refresh

Prerequisites: Git, network access, and an empty scratch directory.

```bash
git ls-remote https://github.com/ggml-org/llama.cpp.git HEAD
git ls-remote https://github.com/charlie12345/ROCmFPX.git HEAD
git ls-remote https://github.com/fewtarius/CachyLLama.git HEAD
git ls-remote https://github.com/fewtarius/llama-ai.git HEAD
```

Record the access time and do not replace pinned hashes merely because HEAD moved. First open a section 11/15 review comparing commits, file ownership, tests, and migration consequences.

For a reproducible source checkout:

```bash
git clone https://github.com/ggml-org/llama.cpp.git llama.cpp
git -C llama.cpp checkout 788e07dc91d266ad3162a1ce9037665656269689
git -C llama.cpp status --short
```

Repeat for the other three pinned commits. A clean status and exact `git rev-parse HEAD` are required.

## Static architecture checks

```bash
rg -n "llama_model_load_from_file_impl|llama_model::build_graph" src
rg -n "ggml_backend_sched_new|ggml_backend_sched_graph_compute_async" src ggml
rg -n "ggml_backend_rpc_add_server|ggml_backend_rpc_start_server" common ggml tools/rpc
rg -n "llama_state_seq_(get|set|save|load)" include src common tools/server tests
rg -n "common_speculative_init|common_speculative_are_compatible" common tools/server
```

Expected result: each public-to-internal path named in [facts and constraints](facts_and_constraints.md) remains locatable. If a symbol moved, update the map only after reading its callers and implementation.

For fork ownership, generate path manifests and semantic diffs from a known merge base; do not use independent HEAD-to-HEAD file counts as a patch inventory:

```bash
git fetch upstream master
base=$(git merge-base HEAD upstream/master)
git diff --name-status "$base"..HEAD > fork-paths.tsv
git diff --stat "$base"..HEAD
```

Preserve `base`, both commits, and remote URLs with the output in `sources/` or an experiment record.

## Build-surface validation

Use separate build directories and record the full CMake cache.

```bash
cmake -S . -B build-vulkan -G Ninja \
  -DGGML_VULKAN=ON -DGGML_HIP=OFF \
  -DLLAMA_BUILD_SERVER=ON -DLLAMA_BUILD_TESTS=ON
cmake --build build-vulkan --parallel
ctest --test-dir build-vulkan --output-on-failure

cmake -S . -B build-hip -G Ninja \
  -DGGML_HIP=ON -DGGML_VULKAN=OFF \
  -DLLAMA_BUILD_SERVER=ON -DLLAMA_BUILD_TESTS=ON
cmake --build build-hip --parallel
ctest --test-dir build-hip --output-on-failure
```

**[OPEN]** Exact ROCm toolchain flags and dependencies must come from section 16 and the frozen baseline; the commands are templates, not proof that the target builds.

## On-machine validation

Create one experiment record per item with hostname, BIOS/firmware, kernel, ROCm/Mesa, compiler, commit, CMake cache, model SHA-256, command, logs, and exit status.

### M12-01 - backend and type inventory on both nodes

Run the same binaries on both machines; capture device enumeration and backend-feature output for HIP and Vulkan. Confirm every selected ROCmFPX model type loads only where its required ops are supported. Unsupported types must fail clearly or use a recorded correct fallback.

### M12-02 - load/decode call-path trace

With a small licensed fixture, trace or instrument model load, graph build, scheduler split, buffer placement, copies, and backend compute. Compare both nodes and both backends. This is structural validation, not a performance claim.

### M12-03 - state round-trip and negative compatibility matrix

For each selected architecture, save sequence state, generate a deterministic continuation, restore into a fresh process, and compare tokens/logits under fixed sampling. Then attempt restore after changing one compatibility dimension at a time: model hash, runtime commit, K/V type, backend, context settings, draft model, and topology. Incompatible or corrupt state must be rejected or recomputed.

### M12-04 - RPC/dual-link behavior

Capture device ownership, graph partition, bytes/copies per link, command ordering, timeout behavior, remote-process exit, single-link loss, and complete peer loss. Verify the documented single-node fallback. Section 51 owns the detailed transport audit.

### M12-05 - server/cache concurrency and isolation

Exercise multiple slots and identities across restart. Verify that cache entries never cross declared user or rank boundaries, eviction does not resurrect invalid data, target/draft state is coordinated, and corruption results in a miss/recompute.

## Acceptance checklist

- [ ] Exact source commits and dirty state recorded.
- [ ] GGUF/type ID/layout manifest agrees across converter, loader, CPU, HIP, and Vulkan.
- [ ] Both backend builds complete and relevant tests run.
- [ ] Load -> graph -> scheduler -> backend path observed on both nodes.
- [ ] State restore passes positive cases and rejects every negative case.
- [ ] Rank ownership, failure behavior, and single-node fallback are documented.
- [ ] RPC/transport traces identify link use and copies.
- [ ] Raw logs and environment metadata are preserved under `experiments/`.
- [ ] No measured claim is promoted without matched configurations and raw evidence.

