# Linux MPTCP path manager, scheduler and fallback

## Selected controls and defaults

| Control | Default | Exact operational meaning |
|---|---:|---|
| `enabled` | 1 | permits MPTCP sockets/negotiation |
| `path_manager` | `kernel` | kernel PM owns endpoint signaling/subflow creation; `userspace` delegates decisions through netlink |
| `pm_type` | 0 | deprecated numeric PM selector retained for compatibility |
| `scheduler` | `default` | selects the in-kernel default scheduler unless another registered scheduler is chosen |
| `close_timeout` | 60 s | MPTCP-level make-after-break window after the last subflow is removed, before final close |
| `stale_loss_cnt` | 4 | stale threshold in counted no-progress MPTCP retransmission intervals; implementation uses strict `>` |
| `syn_retrans_before_tcp_fallback` | 2 | after initial MP_CAPABLE SYN plus two MP_CAPABLE retransmissions, a subsequent attempt may fall back to plain TCP |
| `add_addr_timeout` | 120 s | ADD_ADDR retransmission timeout; can be RTT-lowered; 0 disables retransmission |
| `allow_join_initial_addr_port` | 1 | permits MP_JOIN to the initial address/port where applicable |
| `blackhole_timeout` | 3600 s | suppresses repeated MPTCP attempts toward a detected blackhole for the timeout |
| `checksum_enabled` | 0 | MPTCP data checksum disabled by default |

## Path managers

The kernel PM and userspace PM are different control planes. The `mptcp_pm` generic-netlink family exposes endpoint and limit operations. Endpoint flags can request signaling, subflow creation, backup treatment and fullmesh behavior where implemented. Address availability alone does not create simultaneous paths: local PM policy, peer signaling, route reachability and subflow establishment all participate.

## Default scheduler

The default scheduler excludes unusable/stale paths, prefers non-backup active subflows, and then considers backup paths. Selection uses queued data and pacing-derived linger estimates plus send-window, memory and burst constraints. A blocking-estimation guard avoids selecting a path that would unnecessarily delay data relative to another. The fallback scheduler uses the first subflow.

## `stale_loss_cnt` nuance

The source increments a stale counter over MPTCP-level retransmission intervals when the subflow’s receive timestamp does not advance. Progress resets the counter and can reactivate the path. A path is marked stale only when:

1. the configured limit is nonzero;
2. `stale_count > stale_loss_cnt`; and
3. another active subflow has a lower stale count.

With default 4, the transition can occur on the fifth counted no-progress interval. Setting 0 disables this mechanism; it does not disable TCP loss detection or every other path-failure mechanism.

## Fallback boundary

Fallback is not one state:

1. **Active initial negotiation:** local policy can stop sending MP_CAPABLE after the configured SYN retransmissions and retry as TCP.
2. **Passive acceptance:** an MPTCP listener can accept a peer that does not negotiate MP_CAPABLE as a normal TCP flow.
3. **Established protocol handling:** specific MP_CAPABLE/DSS/mapping conditions can enter fallback behavior, while protocol errors may instead reset the MPTCP connection/subflow.
4. **Additional-subflow failure:** failure of an MP_JOIN path is a path failure. It does not automatically rewrite an already established MPTCP connection as ordinary TCP.

The transport remains a reliable ordered byte stream at the MPTCP data-sequence level. Reinjection and simultaneous subflows do not establish application goodput without measurement.


## Sources

- **S015** — MPTCP sysctl documentation (v7.2-rc3 blob b9b5f58e…)
- **S016** — MPTCP control defaults and close handling (v7.2-rc3 net/mptcp/ctrl.c and protocol.c)
- **S017** — MPTCP scheduler and stale-subflow source audit (v7.2-rc3 net/mptcp/sched.c and protocol.c)
- **S018** — MPTCP path-manager generic-netlink specification (mptcp_pm family)
- **S019** — RFC 8684 — TCP Extensions for Multipath Operation (RFC 8684; obsoletes RFC 6824)
- **S020** — RFC 8684 verified technical erratum (Errata ID 6609)
