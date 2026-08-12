---
section_id: "25"
title: "Vulkan Design Implications for HaloFPX"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories:
    - "Mesa 20f4f9f45057559475600b60364b60643011990f"
    - "ggml-org/llama.cpp 788e07dc91d266ad3162a1ce9037665656269689"
  software_versions:
    - "Vulkan 1.4.357"
  hardware_revisions: []
related_sections: ["24", "37", "47", "49", "54", "74", "75"]
---

# Design implications

## Backend posture

**[RECOMMENDATION]** Treat Vulkan as a first-class experimental backend and single-node fallback, while retaining HIP as the initial reference for custom gfx1151 kernels and distributed token-path integration. This is not a performance verdict: it is a risk-control posture until matched measurements exist.

| Dimension | Vulkan/RADV | HIP | Current implication |
|---|---|---|---|
| Kernel interface | **[VERIFIED]** SPIR-V compute pipelines, descriptors or device addresses, explicit barriers, configurable subgroups. [S25-01] [S25-03] | **[VERIFIED]** C++ kernel launches, streams/events, mapped pinned memory, and graph capture are documented runtime mechanisms. [S25-07] [S25-08] | Vulkan offers portable explicit control; HIP aligns more directly with ROCmFPX kernel code. |
| Reuse | **[VERIFIED]** Reusable command buffers and pipeline caches exist; resource bindings and lifetimes constrain reuse. [S25-01] | **[VERIFIED]** HIP graphs capture kernel, copy, event, and external-semaphore nodes, subject to capture restrictions. [S25-07] | Compare pre-recorded command rings against instantiated HIP graphs using identical graph shapes. |
| Host-visible buffers | **[VERIFIED]** Runtime-selected host-visible/coherent/cached types; RADV may expose device-local+host-visible memory. [S25-01] [S25-03] | **[VERIFIED]** HIP exposes pinned/mapped host allocation with coherent/non-coherent and write-combined choices. [S25-08] | Neither API proves the best USB4 path. Section 54 must measure end-to-end. |
| Cross-host collectives | **[VERIFIED]** Core Vulkan has no cross-host collective abstraction; external FDs are local-platform handles. [S25-01] | **[INFERENCE]** HIP integrates naturally with ROCm code, but cross-host transport still requires the project's own protocol or a measured communication library path. | Backend choice cannot replace transport design. |
| Compilation | **[VERIFIED]** RADV compiles SPIR-V through NIR/ACO; pipeline and Mesa disk caches can reduce repeat work. [S25-04] [S25-05] | **[INFERENCE]** HIP's ahead-of-time/runtime toolchain has different startup and specialization costs. | Measure cold and warm startup separately; pin exact artifacts. |
| Debug surface | **[VERIFIED]** Vulkan validation can diagnose API misuse; it changes overhead and is not a production benchmark mode. [S25-09] | **[VERIFIED]** HIP supplies runtime error and synchronization APIs. [S25-07] | Run correctness with validation, performance without it, and record both configurations. |

## Proposed Vulkan execution shape

**[RECOMMENDATION]** If Vulkan remains competitive, use one persistent device/queue context per rank with bounded in-flight slots:

1. Pre-create compute pipelines for frozen shader variants and subgroup sizes.
2. Allocate large buffer arenas and suballocate; never allocate `VkDeviceMemory` in the per-token path.
3. Give each in-flight slot stable buffers, descriptor state, a command buffer, and timeline values.
4. Record stable graph regions once where bindings and dispatch dimensions remain valid; re-record only variable regions.
5. Signal a device-completion timeline value after the producer dispatch.
6. A host waiter observes completion before passing the mapped payload to the transport; the remote rank receives into its own rank-local buffer and signals its local GPU dependency.
7. On timeout/device loss/peer loss, invalidate the slot and fall back to single-node execution; never recycle a buffer whose completion state is unknown.

**[INFERENCE]** Stable per-slot resources avoid descriptor and command-buffer mutation while pending and make sequence numbers map cleanly to transport credits. The best slot count is measurement-dependent.

## Memory-path choices

| Path | Use candidate | Required proof |
|---|---|---|
| Device-local compute buffer + copy to cached host-visible buffer | Conservative baseline for GPU-produced activations | Copy bandwidth, queue overlap, CPU read latency, and end-to-end token latency. |
| Device-local+host-visible coherent buffer read by CPU | Potential copy elimination on UMA | GPU write throughput, cache visibility correctness, CPU read bandwidth, and contention under inference. |
| Cached host-visible buffer read/written by GPU | Control records and small payload candidate | GPU access penalty and whether cached type is compatible with each buffer usage. |
| Imported external host allocation | Same-rank interop experiment only | `vkGetMemoryHostPointerPropertiesEXT`, alignment, lifetime, page pinning, and transport-stack compatibility. |

**[RECOMMENDATION]** Keep control metadata and bulk tensors in separate allocations or cache-line-separated regions. A completion word must not share a cache line with payload data being concurrently produced or consumed.

## Synchronization rules

- **[RECOMMENDATION]** Prefer `vkQueueSubmit2`/synchronization2 and narrow compute/copy/host scopes over blanket all-commands barriers, after validation proves correctness.
- **[RECOMMENDATION]** Use monotonically increasing 64-bit slot epochs; reject stale or duplicate transport completions before exposing buffers to a GPU.
- **[RECOMMENDATION]** Host coherence is a memory-property optimization only. Every transition must still name producer completion and consumer start.
- **[RECOMMENDATION]** Use finite waits with fault telemetry. A permanently blocked wait is not recovery.
- **[RECOMMENDATION]** On distributed failure, each rank owns and frees only its local Vulkan resources. The coordinator cancels the distributed step and resumes from a defined single-node checkpoint or rejects the request.

## Descriptor, address, and cache policy

- **[RECOMMENDATION]** First optimize the existing descriptor-set path by preallocating per-slot sets and updating only changed bindings. Evaluate `VK_EXT_descriptor_buffer` only after CPU profiling shows descriptor updates are material.
- **[INFERENCE]** Buffer device address can reduce descriptor indirection for pointer-rich kernels, but increases lifetime, bounds, and validation risk. Never serialize an address into the cross-host protocol.
- **[RECOMMENDATION]** Add an application pipeline cache only with an atomic write/rename, bounded size, and a fingerprint including shader hash, specialization constants, Vulkan API version, vendor/device/driver identity, and `pipelineCacheUUID`. Invalid or corrupt data must become a cache miss.
- **[RECOMMENDATION]** Measure cold cache, Mesa-warm cache, and application-cache-warm startup separately. Do not publish mixed-cache results.

## Decision gate for section 47

Vulkan may be selected for a workload class only when it passes all of these gates:

1. Correctness matches the CPU/reference and HIP baseline for required models and quantizations.
2. Both machines expose the required feature/handle matrix under frozen kernel/Mesa/firmware versions.
3. Warm steady-state latency and throughput are competitive in matched configurations.
4. Cold start, shader/pipeline compilation, and cache invalidation meet operational targets.
5. Host-visible transport-buffer paths are correct and improve end-to-end distributed latency, not only a microbenchmark.
6. Failure handling returns to a known rank-local or single-node state.

**[OPEN]** Thresholds for “competitive” and the required workload weights depend on sections 09, 38, 47, 73, and 74.

