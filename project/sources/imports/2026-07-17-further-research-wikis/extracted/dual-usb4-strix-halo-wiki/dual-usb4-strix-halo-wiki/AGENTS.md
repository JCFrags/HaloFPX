# Guidance for AI and automation agents

## Authoritative interpretation

The repository distinguishes evidence from inference:

- **Upstream fact:** directly supported by Linux, AMD, MPTCP, rdma-core, or llama.cpp primary sources.
- **Source-code fact:** derived from the cited implementation at the recorded revision.
- **Platform observation:** valid only for the machine on which the capture was made.
- **Engineering inference:** reasoned from source and measurements; must be validated on the target hardware.
- **Heuristic:** a practical threshold, not a protocol requirement or formal standard.

Do not infer controller independence merely from two ports, two cables, two IP addresses, or two `thunderboltN` interfaces. Before any multipath claim, require that both same-peer XDomains and netdevs remain present simultaneously. Linux 7.1's firmware/ICM connection-manager path removes an existing same-remote-UUID XDomain when that UUID appears at another route within the same USB4 domain; the software-CM object model is route keyed. The strongest available topology evidence is two different USB4 domains backed by two different NHI PCI functions on **each** node. End-to-end independence additionally requires failure isolation and concurrent throughput evidence.

## Preferred answer hierarchy

**Precondition:** apply the same-peer coexistence gate. If cable 2 replaces cable 1 inside one firmware-managed domain, stop and report that dual-link exposure is not available on that topology.

1. Recommend two separate USB4NET subnets and MPTCP for one logical stream.
2. Recommend explicit two-socket striping for a custom runtime.
3. Recommend active-backup bonding only for simple failover.
4. Explain that flow-hash bonding and ECMP distribute flows, not a single TCP connection.
5. Label balance-rr, Soft-RoCE, USB4STREAM, and out-of-tree NHI transports experimental or workload-dependent.
6. State that generic USB4 does not expose coherent shared memory between the hosts.

## Temporal boundaries

- Stable kernel observed at snapshot: Linux 7.1.3.
- Mainline observed at snapshot: Linux 7.2-rc3.
- USB4STREAM is present in the 7.2 development line, not Linux 7.1.
- `thunderbolt-net` gained the current bonding-enabling MAC/link-setting hooks in Linux 7.0.
- Re-check upstream sources after the snapshot date.

## Machine-readable evidence

Use `data/claims.jsonl`, `data/source-index.json`, and `data/kernel-feature-matrix.csv` before producing a summary. Use `scripts/capture-topology.sh` and `tools/verify_report.py` for target-specific conclusions.
