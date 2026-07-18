# Dual-node transport and capacity constraints

## Measured target envelope

- **[MEASURED]** Both nodes expose approximately 124 GiB usable memory, one NUMA node, gfx1151/40 CU, kernel `7.1.3-1-cachyos`, ROCm 7.2.4-family packages, Mesa 26.1.4, and Crucial P310 1 TB NVMe. Package, swap, boot, and rollback-kernel skew remains. [Live comparison](../sources/measurements/2026-07-17-strix-halo-live-inventory/comparison.md)
- **[MEASURED]** nimo-1 is the private RPC worker and had about 43 GiB free while holding about 112 GiB of RPC tensor cache. nimo-2 is coordinator/LAN API and had about 318 GiB free. New persistent cache or a 200–230 GB artifact cannot be planned as if storage were symmetric.
- **[MEASURED]** The running model is 121,861,632,736 bytes and the server reported about 228.7B parameters. This proves that the current distributed stack can load that artifact; it does not prove a 200–230 GB stored artifact plus runtime, KV, staging, and safety headroom will fit.
- **[RECOMMENDATION]** Capacity plans use captured per-node available memory and measured peak deltas, not a nominal `2 x 128 GiB` profile or equal split arithmetic. `--tensor-split 1,1` is configuration evidence, not realized placement or ownership proof. [Large-model intake review](../reviews/intake/2026-07-17__dual-node-large-model__review__v01.md)

## Current fabric baseline

- **[MEASURED]** Two private USB4NET rails use `10.44.0.0/30` and `10.44.0.4/30`, MTU 9000, and one MPTCP connection with two active subflows. Each peer path reported two receive and two transmit lanes at 20.0 Gb/s per lane. These are negotiated/counter observations, not additive application goodput.
- **[MEASURED]** Interface-to-domain mapping is crossed between hosts. Bind rail identity by address and sysfs ancestry; never infer a physical path from `tb0`/`tb1` alone.
- **[MEASURED]** The 2026-07-17 control measured 9.4–9.8 Gb/s on one bound TCP flow per rail, 20.54–21.04 Gb/s with both rails active independently, and 20.69–20.71 Gb/s for one MPTCP connection with two observed subflows. Idle mean RTT was 0.129–0.135 ms; concurrent-load mean RTT was 0.317–0.576 ms with zero observed loss. This was one sample per throughput cell and is not a tail-latency, tensor, or GPU-to-peer qualification. [Current experiment](../experiments/2026-07-17-usb4-transport-baseline/RESULTS.md)
- **[RECOMMENDATION]** Keep TCP/MPTCP over `thunderbolt-net` as bring-up, control, fallback, and recovery baseline until a matched experiment proves a better carrier. [Wiki Section 50](../wiki/HaloFPX_Wiki/08_Fabric_and_Transport/50_USB4STREAM_and_thunderbolt_net_Implementation_Options/README.md)

## Candidate RPC bring-up

- **[MEASURED]** Candidate `ROCmFPX@61f2f2d` built with `GGML_RPC=ON` on both nodes and completed a Qwen3-4B request using rail A, explicit `RPC0,ROCm0` ordering, 1:1 layer split, and F16 KV. Remote allocation and ROCm graph execution occurred on nimo-1. This proves a bounded RPC bring-up path, not tensor parallelism, dual-rail use, security, resilience, or representative performance. [RPC smoke](../experiments/2026-07-17-open-pin-01-rpc-smoke/RESULTS.md)

## USB4STREAM boundary

- **[VERIFIED]** The pinned Linux 7.2-rc-era source exposes USB4STREAM through `thunderbolt-stream`, ConfigFS, and `/dev/tbstreamX`; it can coexist with `thunderbolt-net`.
- **[MEASURED]** The target `7.1.3-1-cachyos` kernel has USB4 and USB4NET enabled but no `thunderbolt_stream` module or `/dev/tbstream*`. USB4STREAM is not a deployed carrier.
- **[RECOMMENDATION]** Treat USB4STREAM as an optional reversible bulk-data experiment, not a prerequisite. It bypasses Ethernet/IP/TCP/MPTCP processing but still requires application framing, bounded queues/credits, short-I/O handling, integrity/authentication, epoch/reconnect semantics, copies/staging, and measured crossover benefit.
- **[RECOMMENDATION]** Use one carrier-neutral framed protocol for TCP and USB4STREAM candidates. Loss or reconnect of either rail ends the global session epoch; do not migrate a partial record silently to another rail. [Wiki Section 53](../wiki/HaloFPX_Wiki/08_Fabric_and_Transport/53_Message_Framing_Credits_Flow_Control_Integrity_and_Security/README.md)

## Ownership and failure rules

1. **[RECOMMENDATION]** Every plan declares coordinator, rank, sampler, sequence, KV/cache, transport, retry, and visible-output ownership.
2. **[RECOMMENDATION]** Preserve the observed nimo-1 worker / nimo-2 coordinator boundary until an approved plan changes it; device order and split are configuration authority.
3. **[RECOMMENDATION]** On required rank/link uncertainty, stop visible commits for the epoch, abort/drain, invalidate uncommitted steps, and recover only from a mutually validated checkpoint plus input/output ledger. Otherwise restart from the original prompt. [Wiki Section 48](../wiki/HaloFPX_Wiki/07_Distributed_Runtime/48_Distributed_Correctness_Determinism_Fault_Recovery_and_Degraded_Mode/README.md)
4. **[RECOMMENDATION]** Single-node fallback is valid only for a prequalified model/context plan that fits one node and begins a new epoch; partial distributed state is never accepted.

## OPEN measurement gates

- repeated single-rail capacity and latency distributions for each rail;
- repeated both-rail and simultaneous-bidirectional trials with system-wide CPU/IRQ cost and tail behavior;
- MPTCP construction, fallback, link-loss, reorder, and recovery proof;
- GPU-produced to peer-GPU-consumed end-to-end transfer and copy breakdown;
- isolated USB4STREAM probe on an approved additional kernel with stable rollback;
- carrier-neutral wire/parser/credit/fault campaign;
- realized model, tensor/layer/expert, KV, scratch, and staging placement under peak load;
- selected 200–230 GB artifact provenance, exact backend support, quality oracle, and safe fit.

The [transport intake review](../reviews/intake/2026-07-17__dual-usb4-transport__review__v01.md) owns the detailed candidate experiment program; none of its numeric thresholds are approved HaloFPX policy.
