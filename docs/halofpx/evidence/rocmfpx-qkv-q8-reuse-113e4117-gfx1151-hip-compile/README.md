# ROCmFPX Q/K/V gfx1151 HIP compile qualification

Status: **[VERIFIED] GPU-less compile/link evidence** for source head
`113e411706e02704cf1c6c01d8973acbe0cab5b9`.

## Boundary

Both clean build trees used the exact pinned workflow image:

`docker.io/rocm/dev-ubuntu-24.04@sha256:bdc8e61026cbb844ede93d44d2c50055f51ebb2041906b60182bf3bee3139054`

The rootless Podman container ran in WSL without `/dev/kfd` or `/dev/dri` and
without any device mount. Neither Strix Halo node was contacted. The container
installed the workflow's exact bounded dependency list from the image's ROCm
7.2.4 repository, then configured two distinct build directories with
`CMAKE_HIP_ARCHITECTURES=gfx1151`, `GPU_TARGETS=gfx1151`, and the pinned
`/opt/rocm/lib/llvm/bin/clang++` compiler.

This evidence establishes compile and link only. It does not establish GPU
runtime loading, numerical parity, graph reachability, dispatch counters,
graph replay, or performance.

## Results

| Build | Q/K/V option | Result | Translation-unit contract | Linked library SHA-256 |
|---|---|---|---|---|
| feature OFF | `OFF` | 170/170, linked | `gfx1151` present; feature macro absent in `ggml-cuda.cu` and `mmq.cu` | `329447147ba978116240ca4e1e115c3c3c1e0daf54a2343af9c870935a461ae7` |
| feature ON | `ON` | 170/170, linked | `gfx1151` and feature macro present in both modified translation units | `b1f9258ef8a770b5c7bb5f23507304c4ef938f6b0fa3dc98df7c969df7c4749f` |

Each hash identifies `libggml-hip.so.0.11.1` in its scoped container build.
The libraries themselves are not committed.

## Retained evidence

[`podman-rocm-7.2.4-gfx1151/`](podman-rocm-7.2.4-gfx1151/) retains:

- the exact capture driver and pinned image pull/inspect records;
- source head, branch state, and implementation blob identities;
- GPU-device absence plus host/container/toolchain/package versions;
- raw dependency, configure, and 170-step build output for both modes;
- both CMake caches, configure logs, and compile databases;
- explicit macro/architecture/link contract output; and
- linked-library hashes plus a manifest covering every retained artifact.

The SHA-256 of `retained-evidence.sha256` is
`c9daf131398b7225faafa0cf8ffcca8a0ea52155889dedaea1cfcd7fbb82fcd5`.

The historical capture driver expects a prepared exact source checkout at
`/workspace/src`, writes fresh builds below `/workspace/work`, and records
evidence below `/workspace/evidence`. Its hard-coded source identity and Q/K/V
macro make it a receipt for this capture, not a generic future build script.

## Promotion boundary

Issue [#41](https://github.com/JCFrags/HaloFPX/issues/41) still blocks target
runtime work. Promotion still requires real `gfx1151` runtime parity, exact
whole-graph counters, real-model prompt reachability, graph-replay/concurrency
checks, and matched prompt-performance evidence. This receipt makes no
generation claim because the admitted MMQ slice excludes ordinary one-token
decode.
