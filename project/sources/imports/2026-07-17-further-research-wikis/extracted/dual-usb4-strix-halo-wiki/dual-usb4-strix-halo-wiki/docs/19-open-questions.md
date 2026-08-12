# Open questions and future work

These are not blockers for the recommended USB4NET+MPTCP design; they are research targets.

## USB4 controller topology

- Do all shipping Strix Halo boards expose the two native USB4 ports as two Linux domains/NHI PCI functions?
- Which boards route both connectors through shared retimers, muxes, or power-management domains?
- Does simultaneous host-to-host use encounter firmware bandwidth arbitration not visible in sysfs?

## Driver scalability

- Would multiple ring/NAPI queues per `thunderbolt-net` interface improve one-link throughput or only CPU distribution?
- Can hardware expose more independent DMA rings/HopIDs safely to networking?
- Would busy-poll or threaded NAPI reduce tail latency for RPC-sized transfers?

## USB4STREAM ABI

- Can an upstream `mmap`, registered-buffer, `splice`, or io_uring-specific API remove the current user copy without weakening isolation?
- What stable discovery/session API should a production runtime use?
- How should permissions, namespaces, cgroups, and LSM policy apply to `/dev/tbstreamX`?
- What reconnect semantics should applications expect after XDomain disappearance?

## Runtime integration

- Should llama.cpp accept an explicit `--rpc-transport tcp|mptcp|rdma|tbstream` option?
- Should the RPC server support several bind addresses and expose source/interface binding?
- Can RPC transfers be scheduled as independent tensor chunks over two connections without duplicating devices?
- How much of inference time is transport-bound on realistic Strix Halo model partitions?

## Integrity and security

- Should the RPC protocol gain authentication, encryption, strict graph validation, and resource quotas before transport optimization?
- Can USB4 host identity/XDomain properties be safely incorporated into channel authentication?

## Measurement

- Which MTU minimizes CPU-seconds/GiB on current AMD NHI hardware?
- Does MPTCP match two independent TCP flows under equal RTT, or does scheduler/reassembly overhead dominate?
- Does USB4STREAM outperform USB4NET after normalizing for copies, chunk sizes, and CPU affinity?
- What shared memory-bandwidth ceiling appears when both links and the integrated GPU are busy?
