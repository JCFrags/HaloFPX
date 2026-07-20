# P05 RPC small-command coalescing rejection

Status: **mechanism confirmed; performance candidate rejected and removed**

P05 tested a narrow control-plane hypothesis from P02-P04: whether collapsing the RPC command byte, eight-byte length, and payload into one TCP send for commands no larger than 256 bytes would improve serialized decode latency. The candidate reduced send syscalls exactly as designed, but its matched point estimates were not favorable. The source patch was therefore removed from the implementation repository and is not part of HaloFPX.

## Candidate authority and safety boundary

The exact node build source was HaloFPX commit `6c01b9769c3d0c0034acfdd3c1e47c3f632ea670`, tree `d0905b9c2d77290d1f0b4dcf898aaeb4c3c60432`, restored from source archive SHA-256 `6fed59764da4f3d9c02737382c6fc1da52c92e539a0fe9c487cf0971e8650154`. The logical implementation-repository tip at experiment time was `a179d16be57a0bfe5814a82691f54b5dafcf6de4`, tree `e4ae43af642ca5ea57012da6ee4ac7599c06473b`; the intervening P03 and P04 changes were documentation-only. The retained candidate patch SHA-256 is `f59dfe0cf261bbdccccab9bd49203b2308438ada6b97479c72f66e7b5637c182`.

The target-owned patch added `GGML_RPC_COALESCE_SMALL_COMMANDS=OFF`. When enabled, `send_rpc_cmd()` used one fixed stack buffer for TCP payloads up to 256 bytes and emitted the unchanged byte stream in one `send_data()` call. Larger payloads retained the original path. A transport query excluded negotiated RDMA because that transport preserves message boundaries and cannot safely consume a coalesced stream frame through the existing three receives. No heap allocation, protocol version, parser, server API, persistent-store path, WebUI, or donor code was introduced.

After evidence was sealed, all five modified source files in the implementation repository were restored to their exact indexed hashes. No P05 binary is deployed or running. Inert patched source and OFF/ON build artifacts remain under `/var/tmp/halofpx-p05-rpc-coalesce/` on each node as experiment evidence.

## Build and matched runtime

Fresh OFF and ON Release HIP+Vulkan+RPC gfx1151 builds completed on both Linux Strix Halo nodes with GCC 16.1.1 and CMake 4.3.4. Each mode produced byte-identical binaries and CMake caches across nodes.

| Artifact | OFF SHA-256 | ON SHA-256 |
| --- | --- | --- |
| `rpc-server` | `f2285df6bdddbd04eb9831036da8e87a3202b11b59fcb39c2df818f7dfc1fbec` | `2f42b36d4756b68c0237ca82690b4b4f96210de9a285b4fcd06062d61d622e16` |
| `llama-server` | `f3057ae963c183fd4c2a43e364c9a9627a22ebe1d81d8396e711fd253a0a9647` | `c5fcc2bb4d613b2ba5d87145a8d5c3e9fdd31be83f668d5bfdd3edd8cc31c71d` |
| `CMakeCache.txt` | `1c2db0462e21f7848ac41baae91db1ddc2b79b3a1da7eff90683d7096738976b` | `abafc9d6e5219ae376226418d3d8ce26437dea00754723ccb58df0527745572b` |

The exact pinned 160 GB primary model, request hash, runtime flags, Q8_0 K/V, two-node placement, and two-subflow MPTCP topology match P04. Execution order was OFF, ON, ON, OFF. Each block used one excluded warmup and three retained requests. All 16 admitted requests returned HTTP 200 with 1129 prompt and 128 generated tokens. All retained decoded outputs matched SHA-256 `3c9bcc5624f4a1d6558c51be89e2443ffba4b2206d10b632c89b7ef64fdd026f`.

Both rails carried traffic in every block. Nimo-1 received approximately 161.1-161.2 MB and transmitted 72.8-73.8 MB per four-request block, consistent with the matched P04 traffic volume.

## Mechanism check

One excluded request per variant ran under the same nimo-2 `strace -f -c -e sendto` diagnostic:

| Mode | HTTP | `sendto` calls | Diagnostic wall time |
| --- | ---: | ---: | ---: |
| OFF | 200 | 8,509 | 13.459726 s |
| ON | 200 | 8,257 | 13.457454 s |

ON removed 252 send calls, 2.96% of the observed count. That is consistent with eliminating two sends for 126 admitted small commands during the request. The diagnostic proves the intended syscall mechanism and wire compatibility for this workload; it is excluded from the performance result because tracing perturbs execution.

## Matched performance and decision

| Metric | OFF, mean +/- sample SD | ON, mean +/- sample SD | ON delta | Approx. 95% Welch CI |
| --- | ---: | ---: | ---: | ---: |
| Prompt processing | 203.9733 +/- 0.1467 tok/s | 203.8793 +/- 0.1377 tok/s | -0.0461% | -0.2772 to +0.0891 tok/s |
| Generation | 16.6587 +/- 0.0375 tok/s | 16.6578 +/- 0.0286 tok/s | -0.0057% | -0.0443 to +0.0424 tok/s |
| End-to-end curl wall time | 13222.395 +/- 17.681 ms | 13230.226 +/- 21.075 ms | +0.0592% | -17.295 to +32.957 ms |

Every interval crosses zero, so the data does not demonstrate a reproducible regression. However, all three ON point estimates are adverse. The promotion rule applied for P05 required a positive generation point estimate before promotion. P05 therefore rejects the candidate without rescue trials or default enablement. No P05 source remains integrated in the implementation repository, while node-local source/build artifacts remain inert evidence. Reducing 252 syscalls is too small to move complete-model inference measurably in this tuple.

Generation above 30 tok/s remains a stretch objective, not a claimed baseline. P05 neither advances nor disproves it.

## Rollback and provenance

All eight disposable experiment units stopped with result `success` and status 0 because each coordinator stopped before its worker. The preserved nimo-2 RPC worker was restored first, followed by the nimo-1 server. Both are active with zero restarts and original binary hashes; nimo-1 returned HTTP 200 `{"status":"ok"}`, and the restored connection again used both USB4 subflows.

All five immutable reference clones remain clean at their recorded commits and trees. No donor or GPL implementation entered the MIT engine, no P3 admission was needed for this target-owned experiment, and no remote, model, deployment, notice, license, or SBOM changed. Raw evidence remains node-local, hash-manifested, and separate from this synthesis.
