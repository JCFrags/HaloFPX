# Research Snapshot — 2026-07-17

> **Wiki status:** Proposed · **Evidence state:** D0 — design only · **Last reviewed:** 2026-07-17  
> **Machine-validation status:** Not run. Missing measurements are `INSUFFICIENT_EVIDENCE`, never an implicit pass.


## Load-bearing findings

1. AMD documentation identifies Ryzen AI Max/Strix Halo as `gfx1151` and publishes explicit kernel requirements because missing KFD limit fixes can cause queue creation or memory-availability failures. This is a lab-readiness check, not an optional tuning note. [[SRC-003]](Sources.md#src-003) [[SRC-004]](Sources.md#src-004)
2. AMD's documentation currently exposes both a ROCm Core SDK 7.14 stream and Ryzen-specific support matrices. A run must pin the exact supported stack rather than write only “ROCm latest.” [[SRC-001]](Sources.md#src-001) [[SRC-002]](Sources.md#src-002) [[SRC-003]](Sources.md#src-003)
3. AMD has published a multi-node Ryzen AI Max+ `llama.cpp` RPC example over 5 GbE. It demonstrates an upstream integration path but does not validate a two-node USB4 topology, release thresholds, correctness, or stability. [[SRC-006]](Sources.md#src-006)
4. Upstream `llama.cpp` explicitly calls RPC proof-of-concept, fragile, and insecure. The release program therefore requires a dedicated trusted link/firewall and blocks untrusted exposure. [[SRC-007]](Sources.md#src-007)
5. Linux supports host-to-host USB4NET and exposes negotiated speed/lane data. USB4 utilization should be normalized against same-link `iperf3` payload goodput, not nominal signaling bandwidth. [[SRC-010]](Sources.md#src-010) [[SRC-011]](Sources.md#src-011) [[SRC-012]](Sources.md#src-012)
6. AMDGPU can expose busy, power, thermal, frequency, throttling, fan, and APU CPU metrics, while current ROCm documentation lists Strix/Halo profiler support. These are suitable synchronized diagnostics; external wall power remains the preferred energy reference. [[SRC-013]](Sources.md#src-013) [[SRC-014]](Sources.md#src-014) [[SRC-029]](Sources.md#src-029)
7. `llama-bench` separates prompt processing and token generation, and modern serving stacks distinguish TTFT, TPOT/ITL, E2E, and throughput. The program adopts those boundaries and records both client and engine views. [[SRC-008]](Sources.md#src-008) [[SRC-015]](Sources.md#src-015) [[SRC-016]](Sources.md#src-016)

## Evidence caveat

None of these upstream statements proves the target pair, cable, firmware, model, partition, or workload. They justify the test design and watch list only.
