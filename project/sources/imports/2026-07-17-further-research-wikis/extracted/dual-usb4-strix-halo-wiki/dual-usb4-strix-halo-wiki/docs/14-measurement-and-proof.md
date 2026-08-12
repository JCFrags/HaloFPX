# Measurement and proof criteria

## Principle

A convincing result combines topology evidence, failure isolation, counter attribution, concurrent capacity, and transport-level path evidence. No single command is sufficient.

![Proof ladder](../diagrams/proof-ladder.svg)

## Phase 0 — freeze variables

Record on both hosts:

```text
board and BIOS version
kernel build and configuration
linux-firmware and USB4/retimer firmware
iproute2, ethtool, iperf3, llama.cpp revision
cable make, length, and certification
power profile and thermal state
CPU governor and irqbalance state
MTU and offload settings
```

Run and retain the exact directory printed by the script:

```bash
CAPTURE=$(sudo scripts/capture-topology.sh phase0 | tail -n 1)
printf '%s\n' "$CAPTURE"
```

## Phase 1 — topology proof

Pass criteria on **each host**:

- two USB4NET interfaces exist **at the same time**;
- attaching cable 2 does not remove or replace cable 1's XDomain/netdev;
- each resolves to a different service/XDomain path;
- preferred: each resolves to a different `domainX`;
- preferred: each resolves to a different NHI PCI BDF;
- lane/speed attributes are present and plausible;
- both interfaces can be up simultaneously.

Use:

```bash
python3 tools/verify_report.py "$CAPTURE"
```

The verifier reports evidence; it does not invent independence when sysfs data is incomplete.

## Phase 2 — path correctness

```bash
ping -I 10.44.0.1 -c 10 10.44.0.2
ping -I 10.44.1.1 -c 10 10.44.1.2
ip route get 10.44.0.2 from 10.44.0.1
ip route get 10.44.1.2 from 10.44.1.1
```

Pass criteria:

- no packet loss in the unloaded lab;
- the route lookup chooses the intended interface;
- the source address remains path-specific;
- no traffic appears on the other link's counters during a path-bound ping beyond unrelated control traffic.

## Phase 3 — isolated throughput baselines

On node B, start two servers:

```bash
iperf3 -s -p 5201
iperf3 -s -p 5202
```

Use separate terminals or `examples/start-iperf-servers.sh`.

Node A:

```bash
mkdir -p results
iperf3 -c 10.44.0.2 -B 10.44.0.1 -p 5201 -P 4 -t 30 -O 3 -J > results/link0.json
iperf3 -c 10.44.1.2 -B 10.44.1.1 -p 5202 -P 4 -t 30 -O 3 -J > results/link1.json
```

Repeat in reverse direction with `-R`. Record at least five steady-state runs per MTU.

## Phase 4 — simultaneous capacity

```bash
scripts/proof-independent-paths.sh results/dual
```

The script runs the two source-bound iperf3 clients concurrently and creates JSON plus a Markdown summary.

Pass criteria:

- both links carry substantial bytes at the same time;
- neither link is accidentally idle;
- aggregate exceeds the best isolated-link result by a meaningful margin;
- CPU, memory bandwidth, or application limits are documented if scaling is not additive.

**Project heuristic, not a standard:** a strong capacity-independence result is aggregate throughput at least `1.7 ×` the best isolated-link baseline, while each loaded link retains at least `80%` of its own isolated throughput. Platforms can be genuinely controller-independent and still miss this heuristic because of a shared SoC/memory/CPU ceiling.

## Phase 5 — failure isolation

Start continuous traffic on both paths. Unplug cable 0:

- only the cable-0 netdev/XDomain should lose carrier or disappear;
- cable 1 should continue transferring;
- its counters and route should remain valid;
- no controller reset should remove both domains.

Reconnect and repeat with cable 1.

Capture:

```bash
sudo journalctl -kf
ip monitor link address route
udevadm monitor --kernel --udev
```

For MPTCP, one logical transfer should survive either single-link loss.

## Phase 6 — IRQ and counter attribution

Before a link-0-only transfer:

```bash
ip -s link show thunderbolt0 > before-tb0.txt
ip -s link show thunderbolt1 > before-tb1.txt
cat /proc/interrupts > before-irqs.txt
```

Repeat after transfer. Pass criteria:

- target link bytes rise by the expected amount;
- non-target payload bytes remain near baseline control noise;
- the corresponding NHI/USB4 interrupt activity rises;
- error, CRC, missed, overrun, and drop counters do not grow materially.

## Phase 7 — one-connection MPTCP proof

Run one MPTCP iperf3 stream or llama.cpp RPC operation. Save:

```bash
ss -Mani > results/ss-meta.txt
sudo ss -tani > results/ss-subflows.txt
ip mptcp endpoint show > results/mptcp-endpoints.txt
ip mptcp limits > results/mptcp-limits.txt
nstat -asz > results/nstat.txt
sudo timeout 30 tcpdump -ni thunderbolt0 -w results/tb0.pcap 'tcp port 50052' &
sudo timeout 30 tcpdump -ni thunderbolt1 -w results/tb1.pcap 'tcp port 50052' &
```

Pass criteria:

1. one MPTCP meta-socket;
2. at least two established subflows;
3. intended address pairs, not cross-cable routes;
4. payload packets and increasing byte counters on both interfaces;
5. no silent fallback to one ordinary TCP flow;
6. connection survives single-cable removal.

## MTU proof

For each candidate MTU:

```bash
ping -I 10.44.0.1 -M do -s $((MTU-28)) -c 3 10.44.0.2
tracepath -b 10.44.0.2
```

Do not call `iperf3 -M` an MTU setting; it sets TCP MSS. Set link MTU with `ip link` first.

## CPU and latency proof

Collect `perf stat`, `mpstat`, `pidstat`, softnet counters, and application latency in the same interval. Normalize CPU by bytes or work units. For inference, include prompt tokens/s and inter-token latency distributions.

## Integrity proof

- zero unexplained netdev CRC/error growth;
- no application hash mismatch;
- no unexpected TCP/MPTCP retransmission spike;
- for raw USB4STREAM, deterministic payload plus per-chunk and final hash;
- cable removal produces a defined error/recovery outcome, not silent truncation.

## Report template

Use `examples/proof-checklist.md` and preserve raw captures. A final claim should state:

```text
Topology independence: interface / XDomain / controller / unknown
Failure independence: passed / failed / not tested
Capacity independence: ratio and raw Gbit/s
Application multipath: subflow pairs and bytes
Kernel and firmware: exact versions
Security envelope: listener, firewall, IOMMU, encryption
```
