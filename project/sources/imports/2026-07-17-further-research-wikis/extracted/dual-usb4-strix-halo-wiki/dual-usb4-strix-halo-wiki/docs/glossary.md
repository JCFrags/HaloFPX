# Glossary

**AF_XDP** — Linux address family for high-performance userspace packet I/O, requiring driver/queue support for zero-copy operation.

**BDF** — PCI domain:bus:device.function identifier, for example `0000:c8:00.5`.

**Bond** — Linux virtual netdev combining member interfaces under one link-layer interface.

**CRC** — cyclic redundancy check; detects accidental errors but is not cryptographic authentication.

**DSS** — MPTCP Data Sequence Signal, mapping subflow sequence space to the connection-level data sequence.

**ECMP** — equal-cost multipath routing, normally hash-selecting a nexthop per flow.

**GRO/GSO/TSO** — receive/generic/transmit segmentation/coalescing mechanisms that reduce per-packet stack cost.

**HopID** — USB4/Thunderbolt path identifier used by tunneled traffic and DMA rings.

**IOMMU** — I/O memory-management unit restricting DMA to mapped memory.

**MPTCP** — Multipath TCP, one socket composed of one or more TCP subflows.

**NAPI** — Linux network event polling/processing abstraction used by drivers.

**NHI** — Native Host Interface for a Thunderbolt/USB4 host controller.

**NUMA** — non-uniform memory access topology exposed by the operating system.

**RPS/RFS/XPS** — software receive steering, receive flow steering, and transmit queue steering.

**RXE** — Linux Soft-RoCE provider implementing RDMA verbs over a normal Ethernet netdev in software.

**Subflow** — a regular TCP flow belonging to one MPTCP connection.

**USB4NET** — host-to-host networking protocol exposed as an Ethernet interface by `thunderbolt-net`.

**USB4STREAM** — raw host-to-host streaming protocol exposed by `thunderbolt-stream` as `/dev/tbstreamX`.

**XDomain** — a USB4/Thunderbolt cross-domain connection to another host and its advertised services.
