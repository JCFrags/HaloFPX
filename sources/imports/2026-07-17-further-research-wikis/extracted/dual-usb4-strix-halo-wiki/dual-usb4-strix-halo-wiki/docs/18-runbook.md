# One-page deployment runbook

## 1. Update and capture

On both nodes:

```bash
PREFLIGHT=$(sudo scripts/capture-topology.sh preflight | tail -n 1)
python3 tools/verify_report.py "$PREFLIGHT"
```

Require Linux 7.1+ for the stable path. Use Linux 7.2+ only for USB4STREAM experiments.

## 2. Connect and map one cable at a time

Record for each connector:

```text
netdev -> service/XDomain -> domain -> NHI PCI BDF
```

Required exposure result: both netdevs coexist when both cables are connected. Preferred independence result: two different domains and BDFs on each node. If connecting cable 1 removes cable 0's XDomain/netdev, stop; the platform/connection-manager topology cannot expose the requested two paths concurrently.

## 3. Configure addresses

Node A:

```bash
sudo ROLE=A IF0=thunderbolt0 IF1=thunderbolt1 MTU=9000 scripts/configure-links.sh
sudo ROLE=A IF0=thunderbolt0 IF1=thunderbolt1 scripts/configure-policy-routing.sh
```

Node B:

```bash
sudo ROLE=B IF0=thunderbolt0 IF1=thunderbolt1 MTU=9000 scripts/configure-links.sh
sudo ROLE=B IF0=thunderbolt0 IF1=thunderbolt1 scripts/configure-policy-routing.sh
```

## 4. Validate each path

Node A:

```bash
ping -I 10.44.0.1 -c 5 10.44.0.2
ping -I 10.44.1.1 -c 5 10.44.1.2
```

## 5. Measure isolated and simultaneous throughput

Node B:

```bash
examples/start-iperf-servers.sh
```

Node A:

```bash
LOCAL0=10.44.0.1 PEER0=10.44.0.2 \
LOCAL1=10.44.1.1 PEER1=10.44.1.2 \
  scripts/proof-independent-paths.sh results/pre-mptcp
```

## 6. Configure MPTCP

Node B:

```bash
sudo ROLE=server IF0=thunderbolt0 IF1=thunderbolt1 scripts/configure-mptcp.sh
```

Node A:

```bash
sudo ROLE=client IF0=thunderbolt0 IF1=thunderbolt1 scripts/configure-mptcp.sh
```

## 7. Validate one MPTCP stream

Node B:

```bash
mptcpize run iperf3 -s -p 5203
```

Node A:

```bash
mptcpize run iperf3 -c 10.44.0.2 -p 5203 -t 60
```

During transfer:

```bash
ss -Mani
sudo ss -tani
ip -ts mptcp monitor
```

Require two intended subflows and payload bytes on both links.

## 8. Start llama.cpp RPC

Node B:

```bash
sudo nft -f configs/nftables/dual-usb4-rpc-node-b.nft
mptcpize run ./build/bin/ggml-rpc-server -H 0.0.0.0 -p 50052 -c
```

Node A:

```bash
mptcpize run ./build/bin/llama-cli -m model.gguf -ngl 99 --rpc 10.44.0.2:50052
```

## 9. Failure proof

During a long RPC or iperf operation, unplug each cable in turn. The session must survive one link loss. Save `ss`, counters, logs, and packet captures.

## 10. Final capture

```bash
POST=$(sudo scripts/capture-topology.sh post-test | tail -n 1)
python3 tools/verify_report.py "$POST"
```

Do not publish “dual 40 Gbit/s” based on negotiated speed labels. Publish measured payload throughput, CPU cost, and the exact independence level proven.
