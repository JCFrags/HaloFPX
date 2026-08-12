---
title: Source Register
description: Immutable source and evidence inventory used by this design.
status: Evidence locked
evidence-date: 2026-07-17
canonical-repository: charlie12345/ROCmFPX
---

# Source Register

> [!NOTE]
> Evidence is pinned to **2026-07-17**. Repository heads and source links are locked in [[Source-Register]] and `evidence/commit-lock.json`.


## Citation convention

Other pages cite `[Snn]`. Each entry below records repository, immutable ref, path/commit, evidence type, and the conclusion supported. URLs are permalinks where possible.

### S01 — Canonical repository identity

- Repository: `charlie12345/ROCmFPX`
- Observed branch/head: `main@a5605a72768c6562241b248e268e33dc92787394`
- Evidence: public repository metadata and commit history.
- Supports: canonical identity, default branch, evidence lock.

### S02 — Canonical MIT license

- Path: `LICENSE`
- Ref: `a5605a72768c6562241b248e268e33dc92787394`
- Blob: `e7dca554bcb802f98408383a864404e3aa4eacca`
- Permalink: https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/LICENSE
- Supports: MIT license text at observed canonical head.

### S03 — Canonical third-party notices

- Path: `THIRD_PARTY_NOTICES.md`
- Ref: `a5605a72768c6562241b248e268e33dc92787394`
- Blob: `4b2f877fe5011ac7eca70b5409a1cb7b032109ea`
- Permalink: https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/THIRD_PARTY_NOTICES.md
- Supports: llama.cpp base, bundled license inventory, ROCmFP4 additions under MIT unless noted.

### S04 — Canonical AI change process

- Path: `AI_CHANGES.md`
- Ref: `a5605a72768c6562241b248e268e33dc92787394`
- Blob: `e5acbf932092eef329b73c5748d56a6cc93f84eb`
- Permalink: https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/AI_CHANGES.md
- Supports: append-only record requirement for meaningful AI-assisted changes.

### S05 — Prior canonical upstream-integration practice

- Path: `ROCMFP4-UPSTREAM-INTEGRATION.md`
- Ref: `a5605a72768c6562241b248e268e33dc92787394`
- Blob: `68e5522cd9d0aa33121dba375d46bfc945ba6b2b`
- Permalink: https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ROCMFP4-UPSTREAM-INTEGRATION.md
- Supports: pinned upstream baseline, protected worktrees, dedicated build/validation gates.

### S06 — Canonical SSD prompt-cache introduction

- Commit: `c81c7c92233b6370b4eb7087398779a8dcb234a4`
- Title: `server: add SSD prompt cache for MTP`
- Permalink: https://github.com/charlie12345/ROCmFPX/commit/c81c7c92233b6370b4eb7087398779a8dcb234a4
- Related fixes: `756121a5e8e7da464aebd2ab344a2aefef6cecac`, `bdac1b3061a3bfc3af0161e0de431af92efd6c0a`.
- Supports: current `--cache-disk*` capability, UTF-8 path and Windows teardown follow-ups.

### S07 — Canonical prompt-cache data model

- Path: `tools/server/server-task.h`
- Ref: `a5605a72768c6562241b248e268e33dc92787394`
- Blob: `a3ebc1ef5a4a9c88d4045a32a30e857ba6af7087`
- Permalink: https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/tools/server/server-task.h
- Supports: target/draft/spec data, disk-state metadata, RAM/disk cache fields, circuit breaker.

### S08 — Canonical disk lifecycle and atomicity

- Path: `tools/server/server-task.cpp`
- Ref: `a5605a72768c6562241b248e268e33dc92787394`
- Blob: `6e0bb85d1080ac57746eeef0aacddfa293d90fd8`
- Permalink: https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/tools/server/server-task.cpp
- Supports: `.llama-prompt-cache-v1`, owner/lock run directories, cleanup, durable temporary files, atomic rename, conservative accounting.

### S09 — Canonical disk-cache tests

- Path: `tools/server/tests/unit/test_prompt_cache_disk.py`
- Ref: `a5605a72768c6562241b248e268e33dc92787394`
- Blob: `110ba252e96066a37cc3540a921e2d00c60ee2a1`
- Permalink: https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/tools/server/tests/unit/test_prompt_cache_disk.py
- Supports: disk-only restore, LRU limit, target/draft pairing, rejection/fallback, circuit breaker, cleanup.

### S10 — Canonical server/spec-state integration

- Path: `tools/server/server-context.cpp`
- Ref: `a5605a72768c6562241b248e268e33dc92787394`
- Blob: `f7d0bda8dfbdb1425a203ffa497b8f6f8061144f`
- Permalink: https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/tools/server/server-context.cpp
- Supports: target/draft state capture and required speculative-state validation on cache load.

### S11 — Donor engine identity and observed head

- Repository: `fewtarius/CachyLLama`
- Observed branch/head: `master@6be745998f568e379ea197fcf827baec73ff9940`
- Permalink: https://github.com/fewtarius/CachyLLama/tree/6be745998f568e379ea197fcf827baec73ff9940
- Supports: resolved donor identity and mixed upstream merge head.

### S12 — Donor capability claims

- Path: `README.md`
- Ref: `6be745998f568e379ea197fcf827baec73ff9940`
- Blob: `2a19230f301821314d4b061a1ae9dfd00c0254e1`
- Permalink: https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/README.md
- Supports: persistent cache, system cache, hybrid restore, user isolation, expert tracking, Vulkan/ISA claims, flags, donor-reported benchmarks.
- Confidence note: implementation claims are corroborated selectively by source; benchmark figures are not independently verified.

### S13 — Donor engine MIT license

- Path: `LICENSE`
- Ref: `6be745998f568e379ea197fcf827baec73ff9940`
- Blob: `e7dca554bcb802f98408383a864404e3aa4eacca`
- Permalink: https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/LICENSE
- Supports: MIT repository-level compatibility.

### S14 — Donor persistent checkpoint header/API

- Path: `common/kv-ssd-cache.h`
- Ref: `6be745998f568e379ea197fcf827baec73ff9940`
- Blob: `4c13dc525366d57b8da5c361fbc36f32480ee64a`
- Permalink: https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/common/kv-ssd-cache.h
- Supports: SPDX MIT/fewtarius notice, hot/warm/cold metadata, `KVID`/`KVRC`, versioned v3 record, token prefix, target/draft/spec blobs, namespace prefix.

### S15 — Donor persistent checkpoint implementation

- Path: `common/kv-ssd-cache.cpp`
- Ref: `6be745998f568e379ea197fcf827baec73ff9940`
- Blob: `9fe3d2bfb646cb00c1f50c06fde4321d3da8632d`
- Permalink: https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/common/kv-ssd-cache.cpp
- Supports: v3 constant, FNV token hash, native record I/O, readahead, directory scanning, tier promotion/demotion.

### S16 — Donor system-prefix cache

- Path: `common/kv-ssd-system-cache.h`
- Ref: `6be745998f568e379ea197fcf827baec73ff9940`
- Blob: `e4a360449c6c8f935d1d3c6d692023b56f5e730d`
- Permalink: https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/common/kv-ssd-system-cache.h
- Supports: `KVSM` v1 system record, global pool, expiry/LRU, boundary detection contract.

### S17 — Donor page manager

- Path: `common/kv_page_manager.h`
- Ref: `6be745998f568e379ea197fcf827baec73ff9940`
- Blob: `1a782370ac3bd39e2b9841ada51060c79c29a55c`
- Permalink: https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/common/kv_page_manager.h
- Supports: page states, `KVPG` header, background I/O/paging API; maturity not established by this source alone.

### S18 — Donor server cache integration

- Path: `tools/server/server-context-page-manager.h`
- Ref: `6be745998f568e379ea197fcf827baec73ff9940`
- Blob: `fe2e6d61beeb16b397a4fb2606ed436c7ee56a01`
- Permalink: https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/tools/server/server-context-page-manager.h
- Supports: per-conversation caches, target/draft/spec restore, continuation matching, user namespace, model compatibility.

### S19 — Donor user-isolation design

- Path: `docs/development/user-isolation-design.md`
- Ref: `6be745998f568e379ea197fcf827baec73ff9940`
- Blob: `8c85fe7de439ed27feff9a6e35ceebd456dc6421`
- Permalink: https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/docs/development/user-isolation-design.md
- Supports: request fields, validation, `u/` namespace, no cross-user lookup, concurrency cap, slot affinity, 429 path, touched files.

### S20 — Donor attention-only memory removal API

- Path: `include/llama.h`
- Ref: `6be745998f568e379ea197fcf827baec73ff9940`
- Blob: `d55eca769ef64067b93402d6ee03bca7ba8f75ad`
- Permalink: https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/include/llama.h
- Supports: public `llama_memory_seq_rm_attn_only` declaration and hybrid semantics.

### S21 — Canonical public memory API comparison

- Path: `include/llama.h`
- Ref: `a5605a72768c6562241b248e268e33dc92787394`
- Blob: `4b6a4c563046260ec437466ebf63cd49ed00822d`
- Permalink: https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/include/llama.h
- Supports: observed canonical header lacks the donor attention-only declaration at the comparable API section.

### S22 — Donor expert telemetry API

- Path: `include/llama.h`
- Ref/blob: same as S20.
- Supports: expert stats struct and enable/get/reset/model-expert APIs.

### S23 — Donor parent GPL-3.0 license

- Repository: `fewtarius/llama-ai`
- Path: `LICENSE`
- Ref: `1017f3dfdce3ca2b06aa9007b23295db3bb35722`
- Blob: `f288702d2fa16d3cdf0035b15a9fcbc552cd88e7`
- Permalink: https://github.com/fewtarius/llama-ai/blob/1017f3dfdce3ca2b06aa9007b23295db3bb35722/LICENSE
- Supports: GPL clean-room boundary.

### S24 — Donor parent submodule relationship

- Repository/ref: `fewtarius/llama-ai@1017f3dfdce3ca2b06aa9007b23295db3bb35722`
- Commit title: `submodules(CachyLLama): merge upstream/master (61 commits, server fixes)`
- Permalink: https://github.com/fewtarius/llama-ai/commit/1017f3dfdce3ca2b06aa9007b23295db3bb35722
- Supports: parent-to-engine relationship and observed donor update.

### S25 — Upstream identity and observed head

- Repository: `ggml-org/llama.cpp`
- Observed branch/head: `master@86d86ed4396b4130922f7b9af26e3d9fc11a591b`
- Permalink: https://github.com/ggml-org/llama.cpp/tree/86d86ed4396b4130922f7b9af26e3d9fc11a591b
- Supports: upstream synchronization lock on evidence date.

### S26 — AMD small-GPU Vulkan fix already canonical

- Commit: `0bbc87b163ff7826656b1024dac5703e3f2bd6b6`
- Title: `vulkan: for small AMD GPUs, reduce submission threshold based on CU count (#25240)`
- Permalink: https://github.com/charlie12345/ROCmFPX/commit/0bbc87b163ff7826656b1024dac5703e3f2bd6b6
- Supports: no donor port required for this tuning.

### S27 — Canonical DeepSeek/MLA-related model paths

- Representative paths at `a5605a72768c6562241b248e268e33dc92787394`: `src/models/deepseek2.cpp`, `src/models/deepseek2ocr.cpp`, `src/models/deepseek32.cpp`, `src/llama-arch.*`.
- Permalink: https://github.com/charlie12345/ROCmFPX/tree/a5605a72768c6562241b248e268e33dc92787394/src/models
- Supports: verify upstream/canonical behavior before opening a donor MLA lane.

### S28 — Donor common build integration

- Path: `common/CMakeLists.txt`
- Ref: `6be745998f568e379ea197fcf827baec73ff9940`
- Blob: `d5dc830138464f868795dbbb83d9e5618e9c80ac`
- Supports: unconditional donor cache modules in `llama-common`.

### S29 — Donor server build integration

- Path: `tools/server/CMakeLists.txt`
- Ref: `6be745998f568e379ea197fcf827baec73ff9940`
- Blob: `32a9b35fd9a68b678967acb0f54b431f36b0fb5b`
- Supports: donor page-manager/SSD modules in server target.

### S30 — Canonical server build composition

- Path: `tools/server/CMakeLists.txt`
- Ref: `a5605a72768c6562241b248e268e33dc92787394`
- Blob: `9778e6341575858471aa28571ce658d6f62d3d00`
- Supports: architectural divergence and canonical diffusion/server composition.

## Machine-readable evidence

See:

- `evidence/repositories.yaml`
- `evidence/commit-lock.json`
- `evidence/capability-evidence.csv`
- `evidence/source-snapshot-notes.md`


---

**Wiki navigation:** [[Home]] · [[Executive-Decision]] · [[Capability-Decision-Matrix]] · [[Patch-Lanes-and-Dependency-Graph]] · [[Acceptance-Criteria]]
