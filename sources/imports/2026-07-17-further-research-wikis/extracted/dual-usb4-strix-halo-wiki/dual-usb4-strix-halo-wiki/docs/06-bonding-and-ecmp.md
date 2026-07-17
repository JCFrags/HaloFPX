# Bonding, ECMP, and multipath behavior

## The central distinction

Bonding and ECMP usually distribute **flows**. MPTCP distributes one **logical connection** through several TCP subflows. This distinction determines whether one llama.cpp RPC stream can use both cables.

## Kernel-version boundary

Linux 6.18's `thunderbolt-net` implementation lacked the live MAC-change and ethtool link-setting support present in Linux 7.0 and later. The Linux 7.0 networking changes explicitly targeted bonding support. On older kernels, active-backup may still be usable, while 802.3ad and other modes can fail capability checks or behave inconsistently.

Use Linux 7.1 stable or newer for a new bonded deployment, then verify `/proc/net/bonding/bond0` rather than relying on version alone.

## Mode comparison

| Mode | Single-flow aggregation | Failover | Direct Linux-to-Linux suitability | Principal risk |
|---|---:|---:|---|---|
| `active-backup` | No | Yes | Good | Idle second link |
| `balance-xor`, `xmit_hash_policy=layer3+4` | No; different sockets may spread | Yes | Reasonable if both peers mirror the bond | One flow pins to one member |
| `802.3ad` | No; flow-hashed | Yes | Possible when both peers negotiate LACP correctly | Added state/capability requirements; same-speed requirement |
| `balance-rr` | Yes, at packet level | Yes | Lab-only | Packet reordering, TCP congestion response, jitter |
| `balance-tlb` / `balance-alb` | Workload-dependent | Yes | Not recommended as first design | MAC/ARP asymmetry and difficult observability |

The kernel bonding documentation states that `layer3+4` can spread traffic to a peer across slaves while a single connection remains on one slave. It also identifies `balance-rr` as the only standard bond mode that stripes one TCP connection, with out-of-order delivery as the cost.

## Active-backup example

This is the safe bond starting point when transparent interface failover is more important than aggregate bandwidth:

```bash
sudo CONFIRM=YES MODE=active-backup BOND=bond0 \
  IF0=thunderbolt0 IF1=thunderbolt1 ROLE=A \
  scripts/configure-bond.sh
```

Node B uses `ROLE=B`. The script removes path addresses from the members and assigns one bond subnet. Inspect before production use:

```bash
cat /proc/net/bonding/bond0
ip -d link show bond0
```

## Flow-hash bond example

```bash
sudo CONFIRM=YES MODE=balance-xor XMIT_HASH_POLICY=layer3+4 \
  BOND=bond0 IF0=thunderbolt0 IF1=thunderbolt1 ROLE=A \
  scripts/configure-bond.sh
```

Open several independent TCP sockets to test distribution. A single iperf3 stream or one llama.cpp RPC connection is expected to use one member.

## 802.3ad caveats

- Both Linux peers must create compatible LACP bonds.
- Member speeds and duplex must be reported consistently. Linux 7.0 added `thunderbolt-net` link-setting support for this class of use.
- LACP actor/partner state must show both members in one aggregator.
- A direct two-host bond is less common than a host-to-switch LACP design; test failover and restart behavior.
- Aggregate throughput still requires several flows because transmit selection is hashed.

## Why balance-rr is not the default

USB4 links may have similar nominal latency, but ring interrupt timing, NAPI batching, host-controller scheduling, and CPU placement still differ. Packet striping can reorder segments enough to trigger duplicate acknowledgements, retransmissions, and congestion-window reduction. Increasing `net.ipv4.tcp_reordering` can mask some symptoms but does not remove the architectural problem.

Use `balance-rr` only as a controlled comparison and collect:

```bash
nstat -az | grep -E 'TcpRetransSegs|TcpExtTCPLostRetransmit|TcpExtTCPDSACK'
ss -ti dst PEER
ip -s link show bond0
cat /proc/net/bonding/bond0
```

## ECMP versus bonding

| Property | ECMP | Bond |
|---|---|---|
| Layer | L3 route | L2 netdev |
| Addressing | Can retain per-link addresses | Members usually lose their addresses |
| Path visibility | Strong; route lookup shows nexthop | Hidden behind bond master |
| One flow | One hashed nexthop | One hashed member except round-robin |
| MPTCP compatibility | Natural: endpoints retain addresses | Possible but usually defeats path visibility |
| Failure detection | Route/neighbour/application dependent | Bond monitor and mode dependent |

For this two-node research topology, separate interfaces plus MPTCP are easier to prove and tune than a bond.

## Proof for a bond

A bond is not proven merely because both members show `up`. Require:

1. Both members in the expected aggregator or active/backup state.
2. Per-member TX/RX counters that match the selected mode.
3. Single-link failure without session loss where failover is claimed.
4. Multi-flow distribution for hash modes.
5. Reordering and retransmission measurements for round-robin.
