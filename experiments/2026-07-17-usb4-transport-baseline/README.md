# Dual-USB4 transport baseline — 2026-07-17

Status: `COMPLETE`

## Requirement and decision link

- Supports the accepted implementation plan's local transport qualification work.
- Establishes a matched, reproducible baseline for the existing Thunderbolt-net plus MPTCP configuration before any USB4STREAM or custom transport work.
- Does not approve a transport change or predict tensor-parallel performance.

## Authorization and scope

The user explicitly authorized unloading the current model and performing local experiments on `nimo-1` and `nimo-2`. This run is limited to read-only inspection plus temporary `iperf3`, `ping`, and observation processes. It does not change kernel, NetworkManager, MPTCP endpoint, PM QoS, model, cache, or service configuration.

## Environment

- Initiator: `nimo-1`
- Peer: `nimo-2`
- Rail A: `nimo-1` `10.44.0.1` (`thunderbolt0`) ↔ `nimo-2` `10.44.0.2` (`thunderbolt0`)
- Rail B: `nimo-1` `10.44.0.5` (`thunderbolt1`) ↔ `nimo-2` `10.44.0.6` (`thunderbolt1`)
- Kernel at start: CachyOS `7.1.3-1-cachyos` on both nodes
- `iperf3`: `3.21` on both nodes
- MPTCP: enabled; two-subflow endpoint services active
- PM QoS service: active on both nodes
- Model services: deliberately stopped coordinator-first, both left enabled for exact rollback

## Test matrix

1. Idle ICMP latency on each rail.
2. Rail A TCP throughput, `nimo-1 → nimo-2` and reverse.
3. Rail B TCP throughput, `nimo-1 → nimo-2` and reverse.
4. Concurrent independent TCP streams on both rails in each direction.
5. One MPTCP connection using the existing two-subflow configuration in each direction.
6. Loaded ICMP observation during concurrent rail traffic.

Each throughput sample uses JSON output, a fixed 15-second measurement interval, and a 2-second omit interval. Raw outputs and exact commands belong under `raw/`; normalized findings belong in `RESULTS.md`.

## Stop rules

Stop immediately if any of the following occurs:

- either USB4 interface loses carrier unexpectedly;
- an existing network configuration or endpoint changes;
- kernel, MCE, PCIe, USB4, or network-driver errors appear;
- a test process survives its expected one-shot lifetime and cannot be identified exactly;
- node control over the management LAN becomes unreliable;
- unrelated services fail.

## Cleanup and rollback

- All `iperf3` servers are one-shot instances bound to experiment ports `55201`–`55204` and should exit after their client disconnects.
- Remove only experiment-owned temporary files in `/tmp/halofpx-usb4-20260717/` after copying evidence.
- Do not alter the active MPTCP or PM QoS services.
- After experiments, restore the model in dependency order: start the worker on `nimo-1`, verify `10.44.0.1:50052`, then start the coordinator on `nimo-2`, verify `/health` and idle slots.

The user authorized leaving the model unloaded while additional local qualification work proceeds. Restoration remains the final rollback step before handoff.
