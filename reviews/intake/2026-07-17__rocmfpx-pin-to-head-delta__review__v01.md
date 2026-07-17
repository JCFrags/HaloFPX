---
type: source-delta-review
status: open-machine-qualification-gate
created: 2026-07-17
target: charlie12345/ROCmFPX
from_commit: a5605a72768c6562241b248e268e33dc92787394
to_commit: 61f2f2d7bc4955e9bca821095ef69125837133b5
risk: high
approval_required: human-after-machine-evidence
implementation_started: false
---

# ROCmFPX frozen-pin to captured-HEAD delta review

## Verdict

**Do not silently advance the implementation baseline. Nominate `61f2f2d7bc4955e9bca821095ef69125837133b5` as the preferred implementation candidate, preserve `a5605a72768c6562241b248e268e33dc92787394` as the research/reference control, and keep `OPEN-PIN-01` open until the targeted machine gate below passes on both gfx1151 nodes.**

The newer commit is a direct child of the frozen research pin and contains one focused change: HIP TurboQuant flash attention no longer always stages the entire K/V cache as F16. That is highly relevant to long-context inference under the Strix Halo memory ceiling and is more likely to be required than optional for the eventual 200–230 GB workload. It also changes attention arithmetic and dispatch, adds a new online-softmax merge implementation, adds CPU traits, and changes backend-test selection. Static review cannot establish numerical correctness, graph-capture safety, memory reduction, or performance on gfx1151. Therefore HEAD is the right candidate to qualify, but it is not yet an approved stable baseline.

If the gate passes, freeze HEAD—not the older pin—as the Phase 0 implementation-fork baseline. If any mandatory case fails, retain the older pin temporarily and either fix the single commit in a narrow topic lane or explicitly disable the affected TurboQuant mode; do not discard the commit's bounded-staging requirement.

## Scope, authority, and method

This was a read-only audit of the immutable local reference clone at [`sources/repositories/charlie12345__rocmfpx`](../../sources/repositories/charlie12345__rocmfpx). No repository code was built or executed, no worktree or Wiki file was changed, no remote was fetched, and no branch was moved.

Evidence precedence followed the project and Agent Harness rules:

1. exact local Git objects and source at the two full commit IDs;
2. [`sources/repositories/manifest.yaml`](../../sources/repositories/manifest.yaml), which records capture and license provenance;
3. canonical Wiki sections 11, 13, and 15;
4. preserved intake research and existing reviews, which remain candidate evidence.

The exact commands used were read-only forms of `git show`, `git log`, `git rev-list`, `git merge-base --is-ancestor`, `git diff`, `git diff --check`, `git ls-tree`, `git grep`, `git rev-parse <commit>:<path>`, and `git patch-id --stable`. Findings below are source facts or explicitly labeled engineering assessments; no benchmark or compatibility claim is inferred from the commit message.

## Exact history identity

| Property | Frozen research pin | Captured HEAD |
|---|---|---|
| Commit | `a5605a72768c6562241b248e268e33dc92787394` | `61f2f2d7bc4955e9bca821095ef69125837133b5` |
| Tree | `6528a116ad015c316948d288b4ffd9c3586c00ad` | `0a35143f33a7b99a81c824fa8ffd8f743f7ae0dd` |
| Parent(s) | `25c71fc6e12d73bb3804127e032d29fb8976ae40`, `a8b5fa906ccd13c6a8ca06d55aa287854c376868` | `a5605a72768c6562241b248e268e33dc92787394` |
| Commit date | `2026-07-16T22:34:40-04:00` | `2026-07-17T15:41:08-04:00` |
| Subject | `Merge pull request #32 from charlie12345/qualify/hy3-on-github-main-20260716` | `hip : avoid full-cache TurboQuant flash-attention staging (#34)` |

**[VERIFIED]** `git merge-base --is-ancestor` returned success and `git rev-list --count a5605a..61f2f2d` returned `1`. There is no intervening merge, source refresh, or hidden commit range to interpret. The stable patch ID is `d163e5dcf4188dcf1cace0573b677376b7cd78f0`.

**[VERIFIED]** The net delta is 14 paths, 753 insertions, and 26 deletions. `git diff --check` reported no whitespace errors.

## Changed-file ledger

| Surface | Paths | Net role |
|---|---|---|
| CPU type/quantization | `ggml/src/ggml-cpu/ggml-cpu.c`, `ggml/src/ggml-cpu/quants.h`, `ggml/src/ggml-quants.c`, `ggml/src/ggml.c` | Registers Turbo3/4 CPU vector-dot traits, exposes declarations, asserts supported dot shape, and allows generic chunk quantization by 128-wide Turbo rows. |
| Shared CUDA/HIP flash attention | `ggml/src/ggml-cuda/fattn-common.cuh`, `fattn-tile.cuh`, `fattn-vec.cuh`, `fattn.cu` | Adds HIP packed-Turbo VEC dispatch, H128 transforms, mixed Q8/Turbo handling, bounded K/V batching, and metadata-producing TILE behavior for partial-result merging. Most new runtime behavior is guarded by `GGML_USE_HIP`. |
| Template instances | two symmetric Turbo instance files | Excludes D=64 symmetric Turbo VEC instances for HIP while retaining 128/256; CUDA keeps the prior instances. |
| HIP build/new kernels | `ggml/src/ggml-hip/CMakeLists.txt`, new `fattn-kv-batched.cu`, new `fattn-vec-turbo-mixed.cu` | Compiles the bounded staging/merge implementation and six asymmetric/mixed packed-Turbo instance pairs into the ROCm backend. |
| Tests | `tests/test-backend-ops.cpp` | Adds Turbo quantization fixture handling, 14 packed/mixed attention cases, 4 bounded-slice cases, padded-mask coverage, and a backend-applicability filter. |

No file was renamed or deleted. No server, RPC, model graph, MTP script, prompt-cache implementation, GGUF enum, public RPC header, CI workflow, documentation, license, or notice file changed.

## Semantic change

### At the frozen pin

For every Turbo3/Turbo4 flash-attention call reaching this dispatch, `ggml_cuda_flash_attn_ext` allocated pool buffers large enough to pre-dequantize each Turbo K and/or V tensor to F16, replaced the source pointers, ran a standard FA kernel, and then restored the original pointers. The temporary allocation therefore scaled with the full visible Turbo cache for each affected K or V input.

### At captured HEAD

HEAD retains that old pre-dequantize path as a fallback but adds two HIP-specific paths:

1. **Small decode/speculative batches (`Q.ne[1] <= 8`)** use packed Turbo VEC kernels for eligible D=128/256 contiguous shapes. Turbo K rotates Q into the normalized H128 FWHT domain before the dot product; Turbo V rotates the accumulated output back afterward. Symmetric Turbo, mixed Turbo3/Turbo4, and Q8/Turbo pairings are instantiated.
2. **Larger prompt batches** that require bounded handling use a new HIP helper. It converts K/V in slices capped at 1,024 KV positions, invokes TILE attention on each slice, combines slice numerators with an online-softmax max/sum merge, and normalizes once at the end. This bounds the K/V F16 staging component by the slice size rather than full context length. A full-size next-result buffer and per-row metadata are still allocated, so this is not a zero-temporary-memory path.

The bounded helper activates only for supported Q8/Turbo K/V pairs, `Q.ne[1] > 8`, and either more than 1,024 KV positions or a mixed Q8/Turbo pair. Other shapes retain the previous fallback. Eligibility also requires D=128 or 256, contiguous element strides, K length aligned to the attention stride, and contiguous destination storage.

The commit also fixes/extends adjacent behavior:

- padded mask rows use their physical stride in Turbo VEC;
- TILE can emit unnormalized output plus max/sum metadata even when its normal `gridDim.y` condition would not;
- sinks are supplied only to the first K/V slice;
- Turbo CPU quantization and dot-product traits become reachable through generic test/backend infrastructure;
- CPU Turbo vector dot now asserts a multiple of the 128-wide head dimension and single-row invocation.

## Surface-by-surface risk assessment

### gfx1151 / HIP: high relevance, high qualification risk

**Positive assessment.** This is directly aimed at the target backend. Avoiding full-cache F16 staging can materially reduce transient memory for long TurboQuant contexts. The existing pin's staging strategy is poorly matched to a machine already constrained by a roughly 124 GiB usable/GTT envelope. The new files are compiled only into the HIP source list, most dispatch changes are HIP-guarded, and the test additions explicitly exercise ROCm-only D=256 and bounded-slice cases.

**Unresolved risk.** Static source does not prove that gfx1151 compiles these templates under the project's exact ROCm 7.2.4 toolchain, that wavefront behavior and shared-memory use are correct, that pool allocations are graph-capture safe, or that numerical error remains within an approved tolerance. The new path contains bespoke FWHT, packed dequantization, mask-stride, partial-softmax, ALiBi/logit-softcap, sink, GQA, and permuted-layout handling. The commit adds tests but no retained test output or target-machine artifact in the local clone. CI workflow blobs are unchanged.

**Vulkan caveat.** A new test comment explicitly says CPU/HIP encode D=256 as two H128 chunks while the Vulkan runtime `SET_ROWS` path uses H256; the new D=256 fixtures are therefore filtered to ROCm. This is evidence of an unresolved cross-backend semantic boundary, not proof of Vulkan parity. If Vulkan remains a supported fallback for Turbo D=256, it needs a separate decision and test; qualifying HIP must not silently certify Vulkan.

### MTP/speculative decoding: indirect but important

No MTP model, graph, API, or script changes. However, canonical Section 13 records that ROCmFPX uses TurboQuant for target and draft caches, and the small-batch VEC branch explicitly covers decode/speculative-sized batches. The generic backend cases with `nb=1..3` are useful kernel coverage, but they do not reproduce an actual MTP target/draft graph, M-RoPE positions, acceptance/rejection loop, request-level speculative limits, graph capture, long-prompt transition, or state restore.

**Assessment:** no source-level MTP contract change, but runtime numerical and memory behavior can change whenever MTP uses Turbo K/V. MTP off/on end-to-end qualification is mandatory before baseline promotion.

### Disk prompt cache / persistent state: no direct format change, indirect compatibility concern

No `tools/server` or disk prompt-cache file changed. Turbo type identifiers and block structs are unchanged, so this delta does not introduce an on-disk ABI migration by itself. It changes how restored/in-memory Turbo K/V participates in attention, and the project has not proven that a cache produced by one binary is safely reusable by the other under all state fingerprints.

**Assessment:** the commit is not an SSD cache feature and does not resolve HaloKV integrity/durability requirements. Cross-binary prompt-cache reuse should be treated as incompatible unless the existing format's compatibility fingerprint proves otherwise; test pin-produced cache on HEAD only as disposable evidence, and corruption must remain a miss/recomputation.

### RPC / distributed execution: wire surface unchanged, remote compute can change

The exact blobs for `ggml/src/ggml-rpc/ggml-rpc.cpp` and the public RPC surface are unchanged. Custom type IDs/layouts are unchanged. There is therefore no source evidence of an RPC protocol or serialization delta.

The worker executes backend graphs, so a remote HIP worker at HEAD can take the new Turbo FA path even though RPC code is byte-identical. Mixed pin/HEAD coordinator-worker combinations are not approved by this audit. Both ends should use one exact binary/source manifest for the gate, followed by the current dual-node RPC/MPTCP smoke and failure test.

### Build and CI: localized build expansion, no proof of success

HIP CMake now appends two new translation units. This isolates 313 lines of HIP-specific code from the shared CUDA file, but also creates new template/linkage and device-code compilation boundaries. The commit touches no build workflow. Exact Git blobs for `.github/workflows/hip-quality-check.yml` and `.github/workflows/check-rocmfpx.yml` are unchanged, and neither contains Turbo-specific evidence in this audit. `scripts/check-rocmfpx-mtp-smoke.sh` is also unchanged.

The `test-backend-ops` filter adds a virtual applicability hook to all cases and recognizes ROCm by backend registration name. Default behavior is `true`, so ordinary tests remain selected; ROCm-only cases are removed elsewhere. This is reasonable structure, but the test runner itself changed and must be checked for expected case counts so accidental skipping cannot look like success.

### CPU and CUDA: limited but not zero risk

HIP runtime behavior is guarded, but CPU type traits and generic Turbo quantization changed globally. The newly added non-ROCm backend cases can expose CPU behavior that was previously unavailable through this path. CUDA retains its prior D=64 symmetric Turbo instances and does not compile the HIP batching helper. A CPU build/test is required; a CUDA hardware gate is not required for the Strix Halo baseline, but the source should at least compile in a CUDA-disabled/default host configuration if that is a supported project build.

### Licensing and provenance: no observed delta

`LICENSE` and `THIRD_PARTY_NOTICES.md` have identical Git blob IDs at both revisions (`e7dca554...` and `4b2f877f...`). The repository manifest records the MIT root license expression for both revisions. This establishes no observed license-file change; it is not legal advice and does not replace attribution review for later donor integration.

## Why HEAD is the preferred candidate

1. It is a single, attributable direct-child patch, so it can be isolated, reverted, or repaired without importing unrelated moving-HEAD work.
2. It addresses a real architectural weakness visible in the older source: full-cache F16 staging scales with context and conflicts with the project's memory-constrained large-model goal.
3. It adds explicit symmetric, asymmetric, Q8/Turbo, D=128/256, padded-mask, multi-batch, long-KV, sink, softcap, ALiBi, GQA, and permuted-layout test cases rather than changing kernels without tests.
4. One preserved large-model intake Wiki independently pins this exact HEAD in its [`Runtime-Support.md`](../../sources/imports/2026-07-17-further-research-wikis/extracted/llm-wiki-200-230gb/llm-wiki-200-230gb/pages/Runtime-Support.md). That is consistency evidence only—not machine validation—but it avoids creating an implementation baseline older than the research package used for capacity planning. The broader ROCmFPX inventory and gfx1151 recipe remain pinned to `a5605a...`, so their conclusions must not be relabeled as HEAD validation.
5. Freezing the old pin would preserve a known full-cache staging design that the project would likely need to replace immediately. Qualifying HEAD first is more efficient and more aligned with Phase 0 characterization.

## Why HEAD is not yet approved

1. No build or test was run by this audit, by instruction.
2. No retained target-machine results are present for this commit.
3. The change affects numerical attention output and introduces a custom partial-softmax merge.
4. Actual MTP, graph-capture, prompt-cache restore, RPC, and long-context server behavior are not covered by the added unit cases.
5. The D=256 CPU/HIP versus Vulkan transform mismatch remains explicitly unresolved.
6. No measured transient-memory or performance comparison exists between the two commits on gfx1151.

## OPEN-PIN-01 qualification gate

Run matched, clean builds from both exact commits. Preserve commands, CMake cache, compiler/ROCm/kernel/firmware identity, binary hashes, test case counts, raw logs, model/cache hashes, and peak memory observations. Do not use pass summaries without raw evidence.

### Mandatory correctness/build cases

1. Host CPU build plus relevant unit/backend tests at both commits; prove the new Turbo CPU traits do not regress generic operation selection.
2. HIP `gfx1151` build on nimo-1 and nimo-2 using the same locked source/toolchain manifest.
3. Run `test-turboquant` and the relevant `test-backend-ops` FA filters; retain enumerated/selected/skipped case counts. Include every new pair, D=128/256, padded mask, `kv=1280`, `nb=9/17`, sink, softcap, ALiBi, GQA, and permuted case.
4. Compare outputs against CPU/cold recomputation using a predeclared tolerance; do not choose tolerance after seeing HEAD results.
5. Run actual server smoke with Turbo3/Turbo4 symmetric and the recommended Q8/Turbo4 asymmetric profile at MTP off and on. Cover decode batch <=8 and prompt batch >8, including crossing 1,024 KV positions.
6. Exercise graph capture/replay if enabled by the production build and verify no allocation/capture error, stale pointer, or divergent output.
7. Use disposable prompt-cache data to test cold run, save, same-binary restore, and controlled pin-to-HEAD restore. Any incompatible or corrupt state must miss/recompute, never be accepted silently.
8. Run same-commit coordinator/worker dual-node RPC smoke with current device ordering and split. Include cancellation and clean single-node development fallback; then verify no RPC protocol/type regression.
9. If Vulkan Turbo D=256 remains supported, run an independent cross-backend parity gate or record a human-approved exclusion. HIP success alone cannot close this item.

### Mandatory resource/performance observations

Use identical model, prompt tokens, context, batch, slots, K/V types, MTP settings, warmup, and repetitions for pin and HEAD.

- record peak backend/pool allocation and process/cgroup/GTT memory during prefill and decode at increasing contexts;
- prove that HEAD's staging peak is bounded as intended for `kv > 1024`, while recording the full output/metadata temporaries too;
- record prefill/decode latency and throughput, CPU use, failures, and thermal state as observations, not release claims;
- test at least one context large enough for the old full-cache behavior to be meaningful, but stop before unsafe system pressure or swap thrash;
- treat a memory win as insufficient if correctness, recovery, or tail behavior regresses.

### Pass/fail decision

**Promote HEAD** only if both nodes build, all mandatory correctness cases pass, no state/RPC regression appears, memory staging is demonstrably bounded, and performance is not materially worse under a human-approved threshold. Record the final source/tree/binary hashes and freeze them in the implementation baseline manifest.

**Do not promote HEAD** if there is any unexplained numerical divergence, build/link failure, graph-capture failure, skipped mandatory test, incorrect mask/sink/GQA behavior, cache acceptance error, or distributed regression. Preserve all failures. Because the change is one commit, open a narrow repair lane based on HEAD or temporarily remain at the pin with Turbo long-context usage disabled; do not silently claim the old full-cache path is adequate.

## Required updates after the gate—not performed here

If HEAD passes, a separate reviewed documentation change should:

- close `OPEN-PIN-01` in [`reviews/plans/2026-07-17__rocmfpx-llama-ai-fork-plan__draft__v01.md`](../plans/2026-07-17__rocmfpx-llama-ai-fork-plan__draft__v01.md);
- update canonical Wiki applicability in sections 11 and 13 from the research pin to a clearly distinguished implementation baseline without erasing the older research snapshot;
- add the new HIP Turbo FA/bounded-staging capability and Vulkan D=256 caveat to the source-backed inventory;
- attach the machine experiment IDs, raw-artifact hashes, and binary manifests;
- keep the deployed `rocmfp4-llama@4860505...` baseline separate as rollback/comparison evidence.

Until that happens, `a5605a...` remains the canonical Wiki's frozen research pin and `61f2f2d...` remains the preferred but unapproved implementation candidate.

## Evidence summary

| Evidence | Result |
|---|---|
| Local repository manifest | Captured HEAD and pin both present; clean at capture; no submodules; root MIT license metadata recorded. |
| Exact ancestry/count | HEAD is one non-merge commit directly after pin. |
| Exact diff | 14 paths; +753/-26; no rename/delete; diff check clean. |
| Unchanged critical blobs | License, notices, two ROCmFPX/HIP workflows, MTP smoke script, server context, and RPC implementation are identical. |
| Runtime delta | HIP Turbo packed VEC for small batches; 1,024-position bounded K/V conversion and online-softmax merge for eligible larger batches; old full-cache fallback retained. |
| Test delta | New Turbo/mixed/padded/long-KV backend cases, but no retained execution evidence. |
| Final disposition | `61f2f2d...` preferred candidate; baseline promotion remains OPEN pending matched gfx1151 qualification. |
