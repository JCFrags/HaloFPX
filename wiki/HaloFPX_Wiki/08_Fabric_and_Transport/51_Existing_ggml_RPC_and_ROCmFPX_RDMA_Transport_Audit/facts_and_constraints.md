---
section_id: "51"
title: "ggml RPC and RDMA Audit - Facts and Constraints"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: ["ROCmFPX@a5605a7", "llama.cpp@788e07d"]
  software_versions: ["libibverbs auto-detected build"]
  hardware_revisions: []
related_sections: ["32", "39", "49", "53", "54"]
---

# Facts and constraints

## Provenance and identity

**[VERIFIED]** A local source comparison on 2026-07-16 found both pinned `transport.cpp` files hash to `8CE6C3FD5EA12751FC72D5B0C22FD36F6C2E06CDB0807025028EE4397E9DFB2C`; their RPC CMake files also matched. This is a reproducible inspection of immutable source identities, not a runtime measurement; no standalone experiment receipt was retained [S51-01, S51-02].

**[VERIFIED]** The only current `ggml-rpc.cpp` differences observed were placement/ownership of `last_graph_uid` between backend and device context; the wire transport file was identical.

## RPC security admission

**[VERIFIED]** GitHub advisory GHSA-j8rj-fmpv-wcxw documents unauthenticated remote code execution through `RPC_CMD_GRAPH_COMPUTE` in affected llama.cpp source and, as accessed 2026-07-17, identifies no patched version [S51-07]. Upstream commit `ba38f3becce7d1283585c73d796eb47d72bbbd30` adds the relevant null-buffer/non-null-data rejection; the pinned llama.cpp revision contains that commit, and exact-source comparison found the corresponding guards in pinned ROCmFPX [S51-08]. This supports only source-level containment of that documented path, not general RPC security.

**[OPEN]** No live executable, loaded library, listener, firewall, route, or process privilege was inspected for this section. A private USB4 address narrows reachability but does not authenticate RPC or prove parser safety.

**[RECOMMENDATION]** Default `GGML_RPC=OFF` and stop/disable port 50052 whenever either peer cannot prove executable and loaded-library provenance to reviewed fixed source. Enabling RPC requires exact artifact hashes, build options/toolchain, mechanical guard checks, isolated non-exploit rejection testing, intended-address-only binding, peer-only firewall reachability, and an unprivileged service account. Do not infer a patched release from repository tag ancestry or the source guard alone.

## RPC protocol

| Area | Pinned implementation |
|---|---|
| Handshake | `HELLO` is fixed command 14; request/response carry 24-byte connection capabilities; server returns major/minor/patch; client rejects different major or newer minor |
| Framing | request = 1-byte command + native `size_t` length + packed payload; response = 64-bit/native size field + payload |
| Commands | allocation/alignment/max/base/free/clear, set/hash/get/copy tensor, graph compute/recompute, memory, init, alloc-size, hello, device-count |
| Tensor identity | packed tensor metadata includes client pointer-derived IDs and remote buffer/data values |
| Hash cache | for sets above 10 MiB, client sends 64-bit FNV-1a hash; server cache file can avoid transfer |
| Graph reuse | first call serializes graph; nonzero matching graph UID sends `GRAPH_RECOMPUTE`; server stores one graph per backend/device |
| Progress | async tensor callbacks are null; backend `synchronize` is no-op because operations are synchronous |
| Failure | many client failures use `RPC_STATUS_ASSERT`; unsuccessful server graph compute is asserted unsupported |

**[INFERENCE]** Packed native structures and native-width fields make compatibility dependent on matched ABI/endianness/build expectations; the handshake does not by itself prove tensor-layout compatibility.

## verbs/RDMA path

**[VERIFIED]** On Linux with libibverbs, CMake enables `GGML_RPC_RDMA` by default when the dependency is found. The initial TCP connection carries capability exchange; matching nonzero QP information can upgrade data transfer, otherwise it stays on TCP [S51-03].

**[VERIFIED]** The code creates an RC QP, send/receive CQs, one 256 KiB registered transmit buffer, and 24 pre-posted registered 256 KiB receive slots (6 MiB). Device/GID can be influenced by `GGML_RDMA_DEV` and `GGML_RDMA_GID` [S51-03].

**[VERIFIED]** Large transfers are split into 256 KiB chunks. Every SEND is signaled and busy-polled to completion before the next; receive completion is busy-polled, copied from registered host RX staging into the caller buffer, then reposted. This is two-sided verbs SEND/RECV, not one-sided READ/WRITE and not proven GPU-direct [S51-03].

**[VERIFIED]** The receive loop subtracts `wc.byte_len` from the requested remaining size without an explicit `got <= rem` guard in the pinned source. This requires a targeted malformed/size-mismatch test and code review before trust.

**[OPEN]** No runtime evidence here establishes a usable RDMA device over USB4 on the target machines.
