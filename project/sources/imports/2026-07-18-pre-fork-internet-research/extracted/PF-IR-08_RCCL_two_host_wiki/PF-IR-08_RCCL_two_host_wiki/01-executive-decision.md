# Executive decision

## Decision unblocked

**Place RCCL in the local experiment matrix as a gated research candidate and the standard IP-socket collective baseline. Do not classify it as supported or suitable for the target APU pair until the exact two-host experiment passes.**

| Question | Determination |
|---|---|
| Does RCCL expose mechanisms worth adapting? | Yes: standard collectives, point-to-point operations, explicit interface/network selection, nonblocking async-error polling, abort/recreate, and version-dependent shrink/revoke/grow. |
| Does stock Socket provide a documented GPU-direct USB4 path? | No. The inspected Socket implementation advertises host pointers only, rejects non-host registration, and exports no DMA-BUF callback. |
| Is gfx1151 enabled upstream? | Yes on active `develop`; the enabling PR explicitly said distributed validation was pending. A later merged tuning PR reports multi-node Ethernet-switch execution, not USB4. |
| Is 7.2.x equivalent to active `develop`? | No. 7.2.x carries RCCL 2.27.7; active source is 2.30.4 with a larger fault-tolerance and plugin surface. |
| Is transparent reconnect provided? | No audited API or source path establishes transparent peer reconnection. Recovery is application-coordinated. |
| Is a custom USB4STREAM transport immediately necessary? | Not for the Phase 1 IP-over-USB4 baseline. It becomes a Phase 2 candidate only if non-IP/direct semantics or measured performance requirements justify it. |

## Hard boundaries

* `[MACHINE_EVIDENCE_REQUIRED]` This report does **not** claim RCCL works on the two target Strix Halo hosts.
* `[SOURCE_IMPLEMENTATION]` This report does **not** claim stock Socket performs GPU-direct or DMA-BUF transfer over USB4.
* `[NEGATIVE_EVIDENCE]` No exact upstream two-host Ethernet-over-USB4 test was found.
* `[MAINTAINER_REPORTED_MACHINE_EVIDENCE]` A merged 2026 tuning PR reports 1–4 Strix Halo nodes over a 10 Gbps Ethernet switch. That is relevant but not topology-equivalent.

## Adapter-worthy abstractions

The lowest-risk adapter surface consists of: unique-ID distribution; exact `NCCL_NET`/`NCCL_SOCKET_IFNAME`/family controls; nonblocking communicator creation; async-state polling; coordinated abort; fresh unique-ID reinitialization; and structured logs that prove the selected interface and network provider. Active-only revoke/grow experiments must remain separate from the 2.27.7 release lane.
