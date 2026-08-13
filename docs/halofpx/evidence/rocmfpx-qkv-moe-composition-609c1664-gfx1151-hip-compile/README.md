# ROCmFPX Q/K/V and routed-MoE gfx1151 HIP composition qualification

Status: **[VERIFIED] GPU-less compile/link and host composition evidence** for
exact source commit `609c166421ecf3eecaa67340e4f40fcb750a0f48` on exact
accepted-main parent `3d9a0c3cc52168f696d600099742c7caf964161f`.

This receipt qualifies source composition among the default-off prompt-QKV,
strict n=1 decode-QKV, and routed-MoE preparation-reuse experiments. It is not
a GPU runtime, numerical-parity, reachability, or performance result.

## Host composition results

The Release host suite passed 17/17. It includes the existing prompt-QKV and
decode-QKV OFF/ON/source contracts, routed-MoE OFF/ON/source contracts, and
eight distinct executables covering every prompt/decode/MoE option state. The
interleaved graph checks decode-first ownership, prompt fallback, Q/K/V
adjacency only when the matching optimizer is enabled, MoE adjacency and
marker preservation, and topological order. `test-backend-ops` also compiled.

The focused Debug ASan/UBSan suite passed 11/11: routed-MoE OFF/ON/source plus
the same eight-state composition matrix. Raw CTest logs and the exact host
summary are retained beside this README.

## Pinned GPU-less boundary

The rootless Podman capture ran in WSL with no `/dev/kfd`, `/dev/dri`, or
device mount. Neither CachyOS Strix Halo target was contacted. It used only the
preinstalled immutable dependency image:

- image ID `67427cc410efa89e605039547cc43780f400381874da97ec6d6e4cd3263757c2`;
- image digest `sha256:9c7093f09068010487ad826e7677f9dc31b620db3a5544ef3c1da2c9d291dc80`;
- pinned upstream base `docker.io/rocm/dev-ubuntu-24.04@sha256:bdc8e61026cbb844ede93d44d2c50055f51ebb2041906b60182bf3bee3139054`.

The source was transferred as a Git bundle, cloned into a clean tree, and
asserted at the recorded head and parent before configuration. The container
used `--pull=never`; no package install or mutable image update occurred.

## Pinned compile/link results

| Build | Prompt QKV | Decode QKV | Routed MoE | Result | Three-TU contract | Linked library SHA-256 |
|---|---:|---:|---:|---|---|---|
| OFF/OFF/OFF | OFF | OFF | OFF | 170/170, linked | `gfx1151` present and all three macros absent in `ggml-cuda.cu`, `mmq.cu`, and `mmvq.cu` | `595a5cebe97000327e180bb33503d81cdce65fe7ff09c7578d7c716de631caea` |
| ON/ON/ON | ON | ON | ON | 170/170, linked | `gfx1151` and all three macros present in all three translation units | `3bb32cc0602eb781d3715cf8250804b8ef8347d30ee9c3e94d25fa4961acb20d` |

Both clean build directories used ROCm 7.2.4,
`CMAKE_HIP_ARCHITECTURES=gfx1151`, `GPU_TARGETS=gfx1151`, shared libraries,
and single-job compilation. The cache receipts independently bind all three
options to the mode above and keep dense-FFN reuse OFF.

The runner completed at `2026-08-13T08:33:42+00:00`; the retained container
exited zero. The initial `nohup`/`setsid` waiter did not survive the WSL command
boundary, so the already-completed container was finalized by rerunning the
exact wait script under a transient systemd user unit. This recovery and both
zero exit codes are recorded in `supervisor-finalization-recovery.txt`. It did
not rerun or alter either build.

## Retained evidence

[`podman-rocm-7.2.4-gfx1151/`](podman-rocm-7.2.4-gfx1151/) contains:

- exact setup, capture, launch, waiter, image, host, and container receipts;
- the Git-bundle hash/verification and exact source/blob identities;
- device-absence, toolchain, and installed-package records;
- raw configure and 170-step build output for both modes;
- both caches, configure logs, and compile databases;
- explicit architecture/macro/cache/link contract output;
- linked-library hashes;
- runner/container/supervisor exit and recovery receipts; and
- a post-finalization SHA-256 manifest.

The SHA-256 of `retained-evidence.sha256` is
`195b1a23df0cdcbd0e8f15f3a1d799bc37684f094590065d2697d25b03be7dbc`.
The manifest verifies after import into the repository. The linked libraries,
source bundle, and clean build trees are not committed.

## Claim boundary

This evidence establishes host graph/selector composition and exact-source
GPU-less compile/link only. It does not establish HIP runtime loading,
numerical parity, real-model reachability, runtime dispatch/counters, graph
replay, KV/output parity, or speed. Issue
[#41](https://github.com/JCFrags/HaloFPX/issues/41) continues to block target
work. The feature remains default-off.
