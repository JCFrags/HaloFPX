---
section_id: "45"
title: "Persistent Rank Protocol Sources"
status: "verified"
last_verified: "2026-07-16"
applies_to:
  repositories: ["charlie12345/ROCmFPX", "ggml-org/llama.cpp", "torvalds/linux"]
  software_versions: ["HIP documentation 7.2.53210", "RCCL documentation 2.30.4", "NVMe Base 2.0c"]
  hardware_revisions: []
related_sections: ["32", "39", "48", "53", "54"]
---

# Sources

All Internet sources were accessed 2026-07-16. Exact source-code links are immutable commits.

| ID | Primary source and revision | Claims supported | Limitations / conflicts |
|---|---|---|---|
| S45-01 | [ROCmFPX `ggml-rpc.cpp`](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/src/ggml-rpc/ggml-rpc.cpp), [`ggml-rpc.h`](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/include/ggml-rpc.h), commit `a5605a72768c6562241b248e268e33dc92787394` | RPC 4.0.1, framing/HELLO, graph serialization, UID recompute, server allocations and single stored graph/device | prototype RPC; no epochs, command/completion rings, cancellation, or recovery contract established |
| S45-02 | [llama.cpp `llama-context.cpp`](https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/src/llama-context.cpp), commit `788e07dc91d266ad3162a1ce9037665656269689` | prompt/token graph reserve; decode reset/alloc/async compute | not a distributed protocol; actual gfx1151 behavior unmeasured |
| S45-03 | [llama.cpp `ggml-backend.cpp`](https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/ggml/src/ggml-backend.cpp), same commit | scheduler reserve, allocation, reset, split, asynchronous backend compute | internal interface can change |
| S45-04 | [llama.cpp HIP graph option](https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/ggml/CMakeLists.txt), [HIP build](https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/ggml/src/ggml-hip/CMakeLists.txt), [graph capture implementation](https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/ggml/src/ggml-cuda/ggml-cuda.cu), same commit | HIP graphs enabled option and capture/update/launch path | source support is not target-machine validation |
| S45-05 | [AMD HIP graphs](https://rocm.docs.amd.com/projects/HIP/en/docs-7.2.0/how-to/hip_runtime_api/hipgraph.html) and [Graph Management API](https://rocm.docs.amd.com/projects/HIP/en/docs-7.2.0/doxygen/html/group___graph.html), documentation build `7.2.53210` | create/capture, instantiate, update, launch, destroy, preallocation guidance | documentation capability does not guarantee every graph/update works on gfx1151 |
| S45-06 | [Linux `io_uring(7)`](https://man7.org/linux/man-pages/man7/io_uring.7.html), Linux man-pages; [UAPI header](https://github.com/torvalds/linux/blob/fce2dfa773ced15f27dd27cd0b482a7473cdcf2a/include/uapi/linux/io_uring.h), Linux commit `fce2dfa773ced15f27dd27cd0b482a7473cdcf2a` | fixed entries, SQ/CQ ownership, correlation, acquire/release publication | kernel/user shared-memory analogue, not a cross-host wire protocol |
| S45-07 | [NVM Express Base Specification 2.0c](https://nvmexpress.org/wp-content/uploads/NVM-Express-Base-Specification-2.0c-2022.10.04-Ratified.pdf), ratified 2022-10-04, sections 3.3.3.2.2-3.3.3.3 | completion phase-tag wrap and bounded queue precedent | storage-controller standard, used only as an analogue |
| S45-08 | [AMD RCCL all API](https://rocm.docs.amd.com/projects/rccl/en/develop/api-reference/api-library.html) and [environment variables](https://rocm.docs.amd.com/projects/rccl/en/develop/api-reference/env-variables.html), RCCL documentation `2.30.4` | communicator error/abort and multi-communicator ordering concerns | project may use a specialized transport instead; target topology unverified |
| S45-09 | Local [Agent Harness architecture](../../../../references/agent-harness.md) routing to `C:\Users\britt\Documents\Agent_Harness\guide\architecture.md`, read 2026-07-16 | evidence promotion, uncertainty, reversibility, closeout review | governance source, not protocol evidence |

## Contradictions and provenance notes

**[VERIFIED]** ROCmFPX already avoids repeat serialization for one unchanged last graph UID [S45-01]; therefore “RPC always serializes every graph” would be false at the pinned commit. **[INFERENCE]** A bounded multi-graph table with explicit generations is still required because the prototype’s one-graph/device reuse and process-local UID do not define distributed identity, replay, or recovery.

No source establishes the proposed 128/64-byte ABI, epoch rules, deadline values, ring depths, or performance benefit. Those remain recommendations and experiments.
