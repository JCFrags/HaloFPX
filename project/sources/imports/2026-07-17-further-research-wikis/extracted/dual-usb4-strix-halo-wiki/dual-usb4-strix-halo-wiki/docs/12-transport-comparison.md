# Transport comparison

## Decision matrix

| Transport | Upstream maturity at snapshot | One logical ordered stream | Can use both links for one RPC stream | Failover | Userspace copies | App changes for llama.cpp | Security |
|---|---|---:|---:|---:|---|---|---|
| TCP over one USB4NET link | Stable | Yes | No | Reconnect only | Socket copies | None | No auth/encryption in RPC |
| Two explicit TCP sockets | Stable | No; runtime reassembles | Yes | Runtime-managed | Socket copies | Major/custom transport | Runtime responsibility |
| ECMP with ordinary TCP | Stable | Yes | No; one flow hashes to one route | Route-dependent | Socket copies | None | Same as TCP |
| Bond active-backup | Stable; use 7.0+ driver hooks | Yes | No | Yes | Socket copies | None | Same as TCP |
| Bond balance-xor / 802.3ad | Stable; use 7.0+ | Yes | No; multiple flows may spread | Yes | Socket copies | None | Same as TCP |
| Bond balance-rr | Stable but risky | Yes | Packet-striped, potentially | Yes | Socket copies | None | Same as TCP; reordering risk |
| MPTCP over USB4NET | Stable | Yes | Yes, through subflows | Yes | Socket copies | `mptcpize`, eBPF, or small patch | Join/address HMACs; no payload encryption or app identity |
| USB4STREAM | Linux 7.2 development | Yes, raw file stream | One stream per physical XDomain; custom striping can use two | Runtime-managed | Current driver copies on read/write | New transport adapter | No auth/encryption |
| Soft-RoCE/RXE over USB4NET | Experimental for this use | Verbs messages | One QP/path unless runtime creates several | Runtime-dependent | Software provider plus app staging | May trigger current RPC RDMA negotiation | No inherent channel security |
| Hardware RoCE NIC | Mature outside USB4 | Yes | Separate NIC paths possible | Runtime/RDMA-CM dependent | Lower-copy potential | Already supported by RPC negotiation | No inherent encryption unless added |
| Out-of-tree NHI/ibverbs | Research | Project-specific | Potentially | Project-specific | Project-specific | Major | Project-specific |
| CXL/NTB/ivshmem-style shared memory | Not provided by generic host-to-host USB4 | N/A | N/A | N/A | N/A | New hardware/platform | Platform-specific |

## Ranking by deployment objective

### Lowest engineering risk

1. TCP on one USB4NET link.
2. MPTCP on two routed USB4NET links.
3. Active-backup bond.

### Highest deterministic control

1. Two explicitly bound TCP connections with runtime striping.
2. Two USB4STREAM devices with application framing, after Linux 7.2 stabilizes.
3. A purpose-built kernel/NHI transport, with substantial maintenance burden.

### Lowest plausible CPU overhead

This cannot be declared without measurement. The hypotheses are:

- larger MTU/GSO over USB4NET reduces per-byte stack overhead;
- MPTCP adds meta-layer work but can use two controller/CPU contexts;
- USB4STREAM removes IP/TCP but retains copy and syscall/ring processing;
- Soft-RoCE adds software verbs layers and likely does not beat tuned TCP by default;
- a future mmap/registered-buffer USB4STREAM interface could change the result, but it does not exist in the current upstream driver.

## Why shared-memory claims require rejection

A transport can be DMA-backed without exposing shared memory. In both USB4NET and USB4STREAM, the local controller DMA engine reads/writes buffers owned by the local kernel. The remote host receives bytes through its own controller and buffers. There is no single cache-coherent address space, remote page faulting, or direct `mmap` of peer RAM.

The following are local-only primitives unless another transport is added:

```text
POSIX shm
memfd
tmpfs
anonymous mmap
dma-buf file descriptors
ROCm/HIP allocations
hugetlbfs
io_uring registered buffers
```

## Workload fit

| Workload shape | Best first experiment |
|---|---|
| One large ordered RPC stream | MPTCP |
| Many independent requests/sockets | Separate interfaces with policy routing or flow-hash bond |
| Bulk tensor shards with explicit offsets | Two TCP sockets in a custom runtime |
| Control + bulk data separation | TCP/MPTCP control plus one or two USB4STREAM data devices |
| Link redundancy, simple operations | Active-backup bond |
| Research on minimum protocol stack | USB4STREAM |
| Existing verbs-only runtime | RXE proof-of-concept, then compare against TCP |

## Acceptance gate for replacing TCP/MPTCP

An experimental transport should beat the stable baseline on all material dimensions:

- throughput and latency at the application phase that matters;
- CPU-seconds/GiB;
- bounded memory and queue behavior;
- reconnect/failure semantics;
- integrity and confidentiality requirements;
- observability and supportability;
- kernel/firmware upgrade risk;
- code maintenance burden.

A microbenchmark win alone is insufficient.
