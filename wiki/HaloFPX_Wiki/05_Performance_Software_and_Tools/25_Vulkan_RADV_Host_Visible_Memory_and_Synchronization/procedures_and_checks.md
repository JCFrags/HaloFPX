---
section_id: "25"
title: "Vulkan and RADV Procedures and Checks"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories:
    - "Mesa 20f4f9f45057559475600b60364b60643011990f"
    - "ggml-org/llama.cpp 788e07dc91d266ad3162a1ce9037665656269689"
  software_versions:
    - "Vulkan 1.4.357"
  hardware_revisions: []
related_sections: ["23", "24", "27", "47", "54", "73", "74", "75"]
---

# Procedures and checks

All commands are read-only unless a build/output directory is explicitly supplied. No root access is required for the listed inspection commands. Store final raw outputs through the section 73 experiment schema, not in this section directory.

## Internet/source-code refresh

### R25-I1 — Pin upstream snapshots

```powershell
git ls-remote https://github.com/KhronosGroup/Vulkan-Docs.git refs/heads/main
git ls-remote https://gitlab.freedesktop.org/mesa/mesa.git refs/heads/main
git ls-remote https://github.com/ggml-org/llama.cpp.git refs/heads/master
```

Record commit, retrieval UTC timestamp, and changed source paths. **[RECOMMENDATION]** Re-run when Mesa, the kernel, llama.cpp, or ROCmFPX baseline changes.

### R25-I2 — Re-audit implementation hotspots

At the pinned revisions inspect:

- Mesa: `src/amd/vulkan/radv_physical_device.c`, `src/amd/vulkan/radv_device.c`, `src/amd/common/amd_family.c`.
- llama.cpp: `ggml/src/ggml-vulkan/ggml-vulkan.cpp` and generated shader sources.

Search for memory property construction, extension gating, timeline syncobj, buffer device address, descriptor updates, command-buffer pooling, and `createComputePipeline`. Record line links at the exact commit.

## On-machine inventory

Run on **both** ranks and preserve stdout, stderr, exit code, package versions, kernel, firmware, BIOS, and device UUID.

```bash
uname -a
vulkaninfo --summary
vulkaninfo --json > vulkaninfo.json
```

If supported by the installed loader/tools, query device groups and profiles as well. Do not set debugging overrides for the baseline.

### R25-M1 — Feature and identity matrix

Extract and compare:

- API version, driver ID/name/info, vendor/device IDs, device UUID, driver UUID, and pipeline-cache UUID;
- every heap size/flags and memory type index/property flags;
- `memoryBudget`/`memoryUsage` when `VK_EXT_memory_budget` is available;
- `nonCoherentAtomSize`, allocation count/size limits, and buffer alignment limits;
- timeline semaphore, synchronization2, buffer device address/capture replay, subgroup size control, descriptor indexing/buffer, pipeline cache control/binary;
- external memory/semaphore FD handle-type import/export features and dedicated-only requirements.

**Pass criterion:** both rank records are complete and differences are explained. Equality is not assumed.

### R25-M2 — Heap pressure and stability

Allocate progressively larger arenas through the project test harness without exhausting the host. Record requested/committed bytes, heap budget before/after, allocation latency, failure code, and system pressure. Stop below a predeclared safety margin.

**Pass criterion:** allocation failure is handled without process corruption or system OOM; safe operating limits are defined per frozen machine profile.

## Correctness litmus tests

### R25-M3 — Mapped-memory visibility

For each compatible memory type, test sizes around 63/64/65 bytes and larger payloads:

1. CPU write -> optional aligned flush -> queue submit -> GPU checksum -> host wait.
2. GPU write -> device dependency -> host wait -> optional aligned invalidate -> CPU checksum.
3. Repeat with randomized offsets and guard regions.

For coherent types omit flush/invalidate; for non-coherent types use atom-aligned ranges. Validation must be enabled for the correctness run.

**Pass criterion:** all checksums and guards match across at least the predeclared iteration count; any single mismatch blocks direct mapped-buffer use.

### R25-M4 — Timeline and slot reuse

Exercise a bounded ring for at least two in-flight depths. Randomize completion delays, wrap slot indices while timeline values continue increasing, and inject timeout/cancellation. Verify no slot is mutated while pending and stale epochs are rejected.

**Pass criterion:** exact-once completion accounting, no deadlock, no early reuse, and deterministic recovery to a known state.

### R25-M5 — External-handle interoperability

On one host only, query and test each proposed FD handle type between two processes. Transfer FDs only with Unix-domain `SCM_RIGHTS`; record ownership/close behavior and whether semaphore payload import is temporary or permanent.

**Pass criterion:** capability query supports the exact handle combination and a bidirectional data/checksum test passes. **Do not** treat this as a cross-host test.

## Performance experiments

Follow section 73 controls: fixed clocks/power policy where authorized, warmup, repeated samples, raw per-iteration data, temperature, memory pressure, and confidence intervals. Validation and tracing are off for production-performance runs but the same binary/shaders must first pass validation.

### R25-M6 — Memory path matrix

Compare for representative activation/control sizes:

- device-local plus copy to/from host-visible cached;
- direct device-local+host-visible coherent;
- host-visible coherent cached and uncached/write-combined when enumerated;
- persistent mapping versus map/unmap.

Measure GPU producer time, dependency-to-host wake time, CPU read/write bandwidth, copy overlap, and full socket send/receive path. **No synthetic bandwidth result is sufficient to select the distributed path.**

### R25-M7 — Submission and descriptor overhead

Compare:

- re-record per token;
- reusable per-slot command buffers where legal;
- individual buffer reset versus pool reset;
- current per-dispatch descriptor updates versus preallocated per-slot sets;
- descriptor buffer only if supported and implemented.

Report CPU time per token, `vkQueueSubmit2` count, dispatches per submission, GPU idle gaps, and tail latency.

### R25-M8 — Pipeline/shader cache states

Use isolated cache directories, never the user's default cache:

```bash
export MESA_SHADER_CACHE_DIR="$PWD/mesa-cache-under-test"
export MESA_SHADER_CACHE_SHOW_STATS=true
```

Run cold, Mesa-warm, and application-`VkPipelineCache`-warm states. Fingerprint all inputs. Corrupt/truncate a copied application cache and verify it is rejected or treated as a miss. Do not corrupt Mesa's normal cache.

### R25-M9 — Subgroup and kernel variants

For kernels that support both, compare required subgroup sizes 32 and 64 with identical math, dispatch dimensions, and precision. Capture correctness, occupancy/counters where available, prompt processing, decode, and thermal steady state.

### R25-M10 — Matched HIP versus Vulkan baseline

Build the same pinned llama.cpp/ROCmFPX lineage with HIP and Vulkan. Match model hash, quantization, context, batch/ubatch, KV format, prompt tokens, generated tokens, sampling, thread affinity, power state, and cache state. Compare:

- single-rank prompt processing and decode;
- transport-size synthetic graph step;
- two-rank end-to-end mode only after sections 49–54 provide the same protocol semantics;
- cold/warm startup, median and tail latency, throughput, CPU overhead, energy, and correctness.

**Decision rule:** publish raw matched records; section 47 applies workload weights and thresholds. This section does not preselect a winner.

