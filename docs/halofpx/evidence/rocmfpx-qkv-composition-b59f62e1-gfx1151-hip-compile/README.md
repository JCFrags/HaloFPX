# ROCmFPX prompt/decode Q/K/V gfx1151 HIP composition qualification

Status: **[VERIFIED] GPU-less compile/link evidence** for exact source head
`b59f62e1e34d6a3176d25583ff6d8f311b91e242` on exact base
`9bfccf25d43af0c446df591035e9cdac0b74d6c0`.

This receipt qualifies source composition between the merged prompt/MMQ Q/K/V
reuse feature and the proposed strict n=1 generation/MMVQ feature. It is not a
GPU runtime or performance result.

## Boundary

The rootless Podman capture ran in WSL with no `/dev/kfd`, `/dev/dri`, or
device mount. Neither CachyOS Strix Halo target was contacted. The capture
reused the installed ROCm 7.2.4 dependency image retained by the prompt lane:

- image ID `67427cc410efa89e605039547cc43780f400381874da97ec6d6e4cd3263757c2`;
- image digest `sha256:9c7093f09068010487ad826e7677f9dc31b620db3a5544ef3c1da2c9d291dc80`;
- pinned upstream base `docker.io/rocm/dev-ubuntu-24.04@sha256:bdc8e61026cbb844ede93d44d2c50055f51ebb2041906b60182bf3bee3139054`.

The upstream-base and dependency-image relationship is also preserved by the
[prompt compile receipt](../rocmfpx-qkv-q8-reuse-113e4117-gfx1151-hip-compile/README.md).
The source was transferred as an exact Git bundle, cloned into a clean tree,
and asserted clean at the recorded head before configuration.

## Results

| Build | Prompt option | Decode option | Result | Three-TU contract | Linked library SHA-256 |
|---|---|---|---|---|---|
| OFF/OFF | `OFF` | `OFF` | 170/170, linked | `gfx1151` present and both macros absent in `ggml-cuda.cu`, `mmq.cu`, and `mmvq.cu` | `61c4ef835547457608d16c030c15b0c9784028e23a427aee41339a9171d1a7f2` |
| ON/ON | `ON` | `ON` | 170/170, linked | `gfx1151` and both macros present in all three translation units | `da321dfc68323c5a8e9ba4ab94cdd616c18a17f862a58c27a8b9d5221dac2cc2` |

Both clean build directories used `CMAKE_HIP_ARCHITECTURES=gfx1151`,
`GPU_TARGETS=gfx1151`, the pinned ROCm Clang compiler, shared libraries, and
single-job compilation. The cache receipts independently require the two
options to equal the mode shown above.

The retained container exited zero at `2026-08-13T06:51:03+00:00`. The runner
and detached supervisor each recorded exit code zero and an independent PASS
marker. The container was intentionally retained long enough to inspect its
final exit state; it was not an ephemeral `--rm` run.

## Retained evidence

[`podman-rocm-7.2.4-gfx1151/`](podman-rocm-7.2.4-gfx1151/) contains:

- capture and detached-supervisor scripts, image/launch/host receipts, and
  final container inspection;
- exact source head, clean branch state, and relevant blob identities;
- device-absence, container, toolchain, and installed-package records;
- raw configure and 170-step build output for both modes;
- both CMake caches, configure logs, and compile databases;
- explicit architecture/macro/cache/link contract output;
- linked-library hashes; and
- runner/supervisor exit receipts plus a SHA-256 manifest.

The SHA-256 of `retained-evidence.sha256` is
`ab7d46cdb0f556810cb874a780f687b28698848a1a5e5983e8b6595f10d563dc`.
The libraries themselves and the clean build trees are not committed.

## Claim boundary

This evidence establishes exact-source GPU-less compile and link only. It does
not establish HIP runtime loading, numerical parity, real-model reachability,
graph dispatch, exact runtime counters, replay, KV/output parity, or speed.
Issue [#41](https://github.com/JCFrags/HaloFPX/issues/41) continues to block
target work. Issue [#42](https://github.com/JCFrags/HaloFPX/issues/42) remains
open until the target promotion gates are satisfied.
