# Local experiment matrix

## Gate 0 — identity and eligibility

Capture host model, kernel, amdgpu/ROCr versions, exact ROCm packages, `librccl.so` path/hash/version log, `rocminfo` gfx target, library code objects, and whether the chosen RCCL build actually includes gfx1151. Failure here blocks all later conclusions.

## Gate 1 — two-rank Socket bring-up

Force `NCCL_NET=Socket`, an exact `NCCL_SOCKET_IFNAME`, and one IP family. Run two ranks with a small Broadcast, AllReduce, and paired Send/Recv. Preserve `NCCL_DEBUG=INFO` and `INIT,NET,ENV,GRAPH` logs. Passing requires logs that identify the intended provider and interface on both hosts.

## Gate 2 — correctness envelope

Run MPI-enabled `rccl-tests` over powers-of-two and awkward message sizes, multiple data types and reductions, in-place/out-of-place modes, repeated communicator creation/destruction, and sustained loops. Record corruption, hangs, error codes, latency, algorithm bandwidth, bus bandwidth, and host CPU utilization.

## Gate 3 — failure semantics

Inject separately:

| Fault | Expected observable |
|---|---|
| Peer absent at init | bounded harness timeout; initialization failure or async terminal state. |
| Wrong interface/family | deterministic failure with logs showing selection. |
| Kill one rank during collective | survivor detects async error/remote error or watchdog fires; no silent success. |
| Link down / interface removal | terminal async state or watchdog; coordinated abort. |
| Remote close after connection | source predicts `ncclRemoteError`. |
| Reinitialize | old communicator discarded; fresh unique ID and communicator succeed or fail cleanly. |
| Active-only revoke/shrink/grow | semantics match active docs; never attributed to 2.27.7. |

## Gate 4 — data-path and performance characterization

Measure wall time, throughput, CPU utilization, memory bandwidth, host copies where observable, pinned-memory behavior, and power. Treat the path as host-staged unless a non-Socket provider and direct registration are proven by logs/source and measurements.

## Gate 5 — application comparison

Compare identical model, quantization, context, batch, prompt, sampling, and power settings for: local single-host baseline; RCCL-backed tensor parallel adapter; ggml RPC; pipeline/layer split. Separate collective microbenchmarks from end-to-end tokens/s and time-to-first-token.

## Gate 6 — Phase 2 transport trigger

Prototype USB4STREAM only after recording a concrete unmet requirement: IP overhead, excessive host copies/CPU, insufficient bandwidth/latency, need for non-IP device semantics, or a direct-registration objective. Pin the exact Net ABI and add provider conformance tests before model work.
