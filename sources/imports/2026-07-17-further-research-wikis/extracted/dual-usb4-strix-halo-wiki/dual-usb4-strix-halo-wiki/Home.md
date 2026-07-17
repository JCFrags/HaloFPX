# Dual-USB4 Strix Halo Transport Wiki

This is the GitHub Wiki entry point. The canonical content is under [`docs/`](docs/index.md).

> **Preflight gate:** both cables must coexist as two USB4NET interfaces. Prefer a different `domainX` and NHI PCI BDF for each path on both nodes. In Linux's firmware/ICM connection-manager path, a second route to the same peer UUID within one USB4 domain can replace the first XDomain rather than create a concurrent path.

## Recommended architecture

```text
Node A                                      Node B
10.44.0.1/30 -- thunderbolt0 == cable 0 == thunderbolt0 -- 10.44.0.2/30
10.44.1.1/30 -- thunderbolt1 == cable 1 == thunderbolt1 -- 10.44.1.2/30
                         \____ one MPTCP connection ____/
```

Use the links as distinct Layer-3 paths. Do not put both USB4 interfaces in the same subnet unless there is a specific reason and a complete ARP/policy-routing design.

## Navigation

- [Executive conclusions](docs/00-executive-conclusions.md)
- [Topology and enumeration](docs/02-topology-and-enumeration.md)
- [Kernel prerequisites](docs/03-kernel-prerequisites.md)
- [MPTCP procedure](docs/07-mptcp.md)
- [llama.cpp RPC integration](docs/11-llama-cpp-rpc.md)
- [Transport comparison](docs/12-transport-comparison.md)
- [Measurement and proof](docs/14-measurement-and-proof.md)
- [One-page runbook](docs/18-runbook.md)
