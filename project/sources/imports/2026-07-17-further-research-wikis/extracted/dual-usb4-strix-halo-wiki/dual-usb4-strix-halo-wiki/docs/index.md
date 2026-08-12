# Dual-USB4 topology and transport research

![Wiki logo](assets/logo.svg)

This wiki answers a narrow systems question: **how can two host-to-host USB4 links between two Linux Strix Halo nodes be exposed, proven independent, and used concurrently by llama.cpp RPC or a custom runtime?**

## Decision

<div class="decision">
First require two simultaneously present `thunderbolt-net` interfaces; if attaching cable 2 removes cable 1's XDomain, the topology fails before routing. Put the surviving links in separate /30 subnets. Prove that each maps to a separate XDomain and preferably a separate USB4 domain/NHI PCI function on both nodes. Use MPTCP for one ordered RPC byte stream. For a custom runtime, use two explicitly bound sockets and stripe framed chunks. Evaluate USB4STREAM only on Linux 7.2+ and only after establishing a USB4NET baseline.
</div>

## Why this design

The Linux software connection manager discovers XDomains by route, but the firmware/ICM event path de-duplicates a remote UUID within one USB4 domain and replaces the old route. Separate `domainX`/NHI instances are therefore the most robust way to expose two same-peer links concurrently.

A second physical cable does not automatically increase one TCP connection's throughput. Linux routing and common bond hash modes choose a path per flow. `balance-rr` can stripe packets but often creates harmful reordering. MPTCP instead presents one socket to the application while creating regular TCP subflows on both links, with sequence mapping and reinjection at the MPTCP layer.

USB4STREAM removes the IP and Ethernet layers and exposes `/dev/tbstreamX`, but the current upstream implementation still copies between userspace and page-backed DMA rings. It is a raw byte stream without authentication, encryption, remote `mmap`, shared cache coherence, or a llama.cpp transport adapter.

## Research boundaries

- Snapshot date: **2026-07-17**.
- Current stable kernel at snapshot: **7.1.3**.
- Current mainline pre-release at snapshot: **7.2-rc3**.
- Ryzen AI Max+ 395 advertises two native USB4 40 Gbit/s ports, but board topology and firmware determine what Linux actually exposes.
- Performance numbers are intentionally not prescribed. Cable quality, firmware, IOMMU, power policy, kernel, board routing, CPU affinity, and workload materially affect results.

## Fast navigation

| Question | Page |
|---|---|
| Are the links genuinely independent? | [Topology](02-topology-and-enumeration.md) and [proof criteria](14-measurement-and-proof.md) |
| Which kernel is required? | [Kernel prerequisites](03-kernel-prerequisites.md) |
| How should addresses and routes be configured? | [USB4NET](04-usb4net-configuration.md) and [routing](05-routing-and-policy-routing.md) |
| Bonding or MPTCP? | [Bonding and ECMP](06-bonding-and-ecmp.md), [MPTCP](07-mptcp.md) |
| How does llama.cpp RPC behave? | [llama.cpp RPC](11-llama-cpp-rpc.md) |
| Is shared memory possible? | [Experimental transports](13-usb4stream-and-experimental.md) |
| Which commands prove both paths carry data? | [Measurement and proof](14-measurement-and-proof.md) |
