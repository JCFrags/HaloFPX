---
section_id: "14"
title: "Procedures and Checks"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories:
    - "fewtarius/llama-ai@1017f3dfdce3ca2b06aa9007b23295db3bb35722"
    - "fewtarius/CachyLLama@6be745998f568e379ea197fcf827baec73ff9940"
  software_versions: []
  hardware_revisions:
    - "planned dual matched AMD Strix Halo nodes"
related_sections: ["11", "13", "15", "16"]
---

# Procedures and Checks

No command below requires root unless stated. Use a disposable clone and a disposable cache directory. Do not point fault-injection steps at a valuable cache.

## A. Reproduce the source inventory now

Prerequisites: Git, network access, PowerShell 7 or a POSIX shell.

```powershell
$auditRoot = Join-Path $env:TEMP 'halofpx-s14-audit'
git clone https://github.com/fewtarius/llama-ai.git $auditRoot
git -C $auditRoot checkout --detach 1017f3dfdce3ca2b06aa9007b23295db3bb35722
git -C $auditRoot config submodule.CachyLLama.url https://github.com/fewtarius/CachyLLama.git
git -C $auditRoot submodule update --init CachyLLama
git -C "$auditRoot/CachyLLama" rev-parse HEAD
git -C "$auditRoot/CachyLLama" show -s --format='%H %P %cI %s' HEAD
```

Expected pins: wrapper `1017f3d`, gitlink `6be7459`, and CachyLLama second parent `92366df`. A mismatch is a failed provenance check.

```powershell
$fork = Join-Path $auditRoot 'CachyLLama'
$upstreamParent = ((git -C $fork show -s --format='%P' HEAD) -split ' ')[1]
git -C $fork diff --name-status $upstreamParent HEAD
git -C $fork diff --stat $upstreamParent HEAD
```

Expected frozen delta: 56 paths, approximately 9,021 insertions and 400 deletions. Record tool version and full output under project `sources/` if this becomes an integration input.

## B. Static gates before porting

1. Map every selected function to a HaloFPX requirement and destination module.
2. Confirm the applicable MIT/GPL boundary with section 16; retain notices.
3. Search for format constants, declared byte lengths, direct `O_TRUNC`, `fsync`, and missing atomic rename.
4. Trace every user-count increment/decrement through normal, cancel, disconnect, exception, child-task, and shutdown paths.
5. Trace exact target, draft/MTP, speculative, recurrent, and attention state restored per architecture.
6. Require a test name and failure oracle for every selected patch.

## C. Build and smoke validation on each Halo machine

Prerequisites: exact integration commit, identical model bytes, build dependencies, at least 20 GiB free SSD space per disposable test model. Root: not required.

Record for both nodes: BIOS revision, kernel, Mesa/ROCm, firmware, CPU/GPU IDs, compiler/CMake, filesystem/mount options, NVMe model/firmware, runtime commit, model SHA-256, GGUF metadata, launch arguments, environment, and clocks/power policy.

Run build/unit suites first. If testing CachyLLama directly:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DLLAMA_BUILD_TESTS=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

**[OPEN]** The root `test_kv_page_manager.cpp` was not found registered in CMake. Compile/register it deliberately or port its cases into the project test framework; do not count its presence as a passing test.

## D. Deterministic checkpoint-equivalence matrix

For each supported architecture (dense transformer, attention+recurrent hybrid, pure recurrent if supported, MTP/speculative):

1. fix seed, sampler, prompt bytes, tokenizer, context, slots, backend, topology, and output length;
2. run an uncached baseline and save logits or token probabilities at the resume boundary;
3. save checkpoint, stop the server, restart, restore, and generate;
4. compare token IDs and logits within a documented numeric tolerance;
5. repeat across same slot, different slot, server restart, process kill during write, and node restart;
6. repeat with divergence before token 16, before 4,096, at 4,096, and after 4,096;
7. require mismatch to cause a miss/recompute, never accepted output.

Store raw requests, server logs, cache manifests, hashes, timings, and environment metadata. Do not report only a warm/cold speedup ratio.

## E. Corruption and crash tests

Use only a disposable cache copy.

| Injection | Required result |
|---|---|
| truncate header | miss/recompute; no crash |
| truncate each blob | miss/recompute; no oversized allocation |
| flip payload bit | digest failure and miss |
| alter length to huge value | bounded rejection before allocation |
| incompatible model/runtime/topology | compatibility miss |
| kill before/after payload flush, rename, directory flush, index publish | last committed record survives; partial record ignored |
| corrupt index | rebuild from validated records or start empty |

## F. Isolation and concurrency tests

Use two authenticated users plus anonymous requests. Submit identical and different prompts concurrently with `n_parallel >= 4`.

- prove no user A checkpoint, token prefix, timing detail, path, or expert data is visible to user B;
- verify slot affinity never overrides isolation;
- verify explicit-user cap returns 429 at exactly the configured boundary;
- decide and test anonymous policy explicitly;
- cover cancellation, SSE disconnect, child completions, overload, and process restart;
- inspect on-disk permissions and ensure user labels cannot be spoofed by request bodies.

## G. Two-node/rank-local experiments

Run the matrix separately for replication, remote speculative, tensor parallel, pipeline parallel, and the chosen MoE-aware mode.

1. record which rank creates each blob and the encoded topology identity;
2. stop one rank during save and restore;
3. verify all-rank restore is atomic at the logical checkpoint level;
4. prove missing/incompatible shard forces coordinated recomputation;
5. prove single-node fallback behavior matches the selected mode;
6. measure checkpoint size, write/read bandwidth, CPU time, peak RAM, SSD write amplification, TTFT, and restore-vs-recompute crossover;
7. saturate both USB4 links during checkpoint activity and record transport contention.

## H. Promotion gates

A capability may move from candidate to implementation decision only when:

- exact source and destination commits are recorded;
- license/notice review passes;
- correctness, crash, and isolation matrices pass;
- invalid state always becomes a miss;
- both machines have reproducible raw evidence;
- rank ownership, failure behavior, and single-node fallback are documented;
- performance is compared against a matched no-cache configuration.

