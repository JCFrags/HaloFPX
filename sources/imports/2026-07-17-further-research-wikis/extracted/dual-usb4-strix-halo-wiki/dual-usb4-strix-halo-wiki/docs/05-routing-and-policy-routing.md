# Routing and policy routing

## Why separate subnets are the default

Linux owns IP addresses at the host level, not strictly per interface. Placing two interfaces in one subnet can produce ARP flux, ambiguous source selection, and return-path asymmetry. Two /30 subnets make each cable a distinct Layer-3 path and simplify MPTCP endpoint pairing.

## Connected routes

After address assignment, the main table should contain:

```text
10.44.0.0/30 dev thunderbolt0 proto kernel scope link src 10.44.0.1
10.44.1.0/30 dev thunderbolt1 proto kernel scope link src 10.44.1.1
```

Verify:

```bash
ip -4 route show table main
ip -4 route get 10.44.0.2 from 10.44.0.1
ip -4 route get 10.44.1.2 from 10.44.1.1
```

## Source-specific policy tables

Policy routes make the intended egress explicit even if additional routes, bridges, VPNs, or defaults are added later.

Node A:

```bash
sudo ip route replace table 144 10.44.0.0/30 dev thunderbolt0 src 10.44.0.1
sudo ip route replace table 145 10.44.1.0/30 dev thunderbolt1 src 10.44.1.1
sudo ip rule del priority 10440 2>/dev/null || true
sudo ip rule del priority 10441 2>/dev/null || true
sudo ip rule add priority 10440 from 10.44.0.1/32 lookup 144
sudo ip rule add priority 10441 from 10.44.1.1/32 lookup 145
```

Node B uses `.2` source addresses. The supplied script implements both roles.

## Reverse-path filtering

Strict reverse-path filtering (`rp_filter=1`) can discard valid packets in asymmetric or multipath designs. MPTCP guidance recommends loose mode (`2`) on participating interfaces.

```bash
sudo sysctl -w net.ipv4.conf.thunderbolt0.rp_filter=2
sudo sysctl -w net.ipv4.conf.thunderbolt1.rp_filter=2
sudo sysctl -w net.ipv4.conf.all.rp_filter=2
```

Loose mode still rejects sources unreachable through any interface. In a fully isolated lab, `0` is another option, but it removes this source-reachability check.

## Application path selection

### Bind by source address

```c
bind(fd, local_usb4_address, ...);
connect(fd, peer_usb4_address, ...);
```

This is the most portable method for a custom runtime.

### Bind by device

`SO_BINDTODEVICE` forces a socket to a Linux netdev but normally requires privilege or a suitable capability. It is useful for tests and tightly controlled services.

### Command-line examples

```bash
ping -I 10.44.0.1 10.44.0.2
iperf3 -c 10.44.0.2 -B 10.44.0.1 -p 5201
curl --interface 10.44.0.1 http://10.44.0.2:8000/
```

llama.cpp RPC currently does not expose a source-bind option. Connecting to the path-specific remote address chooses the initial route; MPTCP or a source change is needed for transparent multi-link use.

## ECMP

An equal-cost multipath route can spread multiple flows over equal nexthops:

```bash
sudo sysctl -w net.ipv4.fib_multipath_hash_policy=1  # Layer-4/5-tuple hash
```

ECMP is flow-hashed. It does not normally alternate packets from one TCP connection across both links. A single llama.cpp RPC socket therefore remains on one nexthop. ECMP is useful when a custom runtime opens multiple connections with varying source ports.

Directly connected path-specific peer addresses do not need ECMP. Adding an extra virtual service address and ECMP nexthops is possible, but MPTCP offers a clearer one-stream abstraction.

## Same-subnet fallback

Only use this when an external constraint requires it. In addition to source-specific routing, evaluate:

```bash
sudo sysctl -w net.ipv4.conf.all.arp_filter=1
sudo sysctl -w net.ipv4.conf.all.arp_announce=2
sudo sysctl -w net.ipv4.conf.all.arp_ignore=1
```

This is more fragile than distinct subnets and can interact poorly with bonding, NetworkManager, and MPTCP address announcements.

## Route proof

For every intended address pair, save:

```bash
ip -4 rule show
ip -4 route show table all
ip -4 route get REMOTE from LOCAL ipproto tcp sport 40000 dport 50052
```

A route lookup that chooses the wrong interface invalidates any later “path independence” result.
