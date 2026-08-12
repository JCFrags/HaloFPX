---
section_id: "51"
title: "ggml RPC and RDMA Audit - Sources"
status: "draft"
last_verified: "2026-07-17"
applies_to:
  repositories: ["ROCmFPX@a5605a7", "llama.cpp@788e07d"]
  software_versions: []
  hardware_revisions: []
related_sections: ["11", "15", "49", "54"]
---

# Sources

| ID | Primary source and revision | Claims supported | Limitations |
|---|---|---|---|
| S51-01 | [ROCmFPX `ggml/src/ggml-rpc` at `a5605a72768c6562241b248e268e33dc92787394`](https://github.com/charlie12345/ROCmFPX/tree/a5605a72768c6562241b248e268e33dc92787394/ggml/src/ggml-rpc), commit 2026-07-16; accessed/cloned 2026-07-16 | fork RPC and transport snapshot | snapshot provenance does not establish upstream origin |
| S51-02 | [llama.cpp `ggml/src/ggml-rpc` at `788e07dc91d266ad3162a1ce9037665656269689`](https://github.com/ggml-org/llama.cpp/tree/788e07dc91d266ad3162a1ce9037665656269689/ggml/src/ggml-rpc), commit 2026-07-17 UTC+02; accessed/cloned 2026-07-16 local time | upstream comparison snapshot | fast-moving master |
| S51-03 | [ROCmFPX `transport.cpp` pinned](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/src/ggml-rpc/transport.cpp) and [CMakeLists](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/src/ggml-rpc/CMakeLists.txt), accessed 2026-07-16 | verbs detection, buffers, chunking, polling, TCP fallback | code behavior not runtime proof |
| S51-04 | [ROCmFPX `ggml-rpc.cpp` pinned](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/src/ggml-rpc/ggml-rpc.cpp), accessed 2026-07-16 | handshake, commands, framing, hash cache, graph reuse, synchronous API | protocol is not separately standardized |
| S51-05 | [libibverbs manual API index](https://man7.org/linux/man-pages/man7/libibverbs.7.html), Linux man-pages project, accessed 2026-07-16 | meanings of verbs objects used by code | documentation only; hardware support separate |
| S51-06 | Local Agent Harness `guide/architecture.md`, read 2026-07-16 | evidence promotion and reversible decisions | governance only |
| S51-07 | [GitHub advisory GHSA-j8rj-fmpv-wcxw](https://github.com/ggml-org/llama.cpp/security/advisories/GHSA-j8rj-fmpv-wcxw), published 2026-03-26; accessed 2026-07-17 | documented unauthenticated `RPC_CMD_GRAPH_COMPUTE` RCE path and advisory affected-range metadata | advisory still lists no patched version; does not attest HaloFPX binaries |
| S51-08 | llama.cpp commit [`ba38f3becce7d1283585c73d796eb47d72bbbd30`](https://github.com/ggml-org/llama.cpp/commit/ba38f3becce7d1283585c73d796eb47d72bbbd30), merged 2026-03-27; inspected through local follow-up `reviews/follow-ups/2026-07-16__llama-cpp-rpc-rce__research__v01.md` on 2026-07-17 | relevant serialization and server-side null-buffer/non-null-data rejection; exact-pin source mapping | source-level containment of one documented path only; not a patched-release or deployed-artifact claim |
| S51-L01 | [Candidate RPC smoke results](../../../../experiments/2026-07-17-open-pin-01-rpc-smoke/RESULTS.md), candidate `61f2f2d`, nimo-1/nimo-2, 2026-07-17 | identical RPC build artifacts, private rail-A listener, explicit device order/layer split, remote allocation/graph activity, one completed request, teardown | candidate only; one tiny model/request; no security, fault, dual-rail, quality, or performance qualification |

## Local comparison method

Shallow filtered clones at the full hashes were compared with `git diff --no-index` and SHA-256 on 2026-07-16. The source identity result is labeled **[VERIFIED]**. The later bounded network experiment is separately preserved as **[MEASURED]** evidence under S51-L01.
