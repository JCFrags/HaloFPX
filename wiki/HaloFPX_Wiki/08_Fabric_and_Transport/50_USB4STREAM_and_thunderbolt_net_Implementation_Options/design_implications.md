---
section_id: "50"
title: "USB4STREAM and thunderbolt-net - Design Implications"
status: "draft"
last_verified: "2026-07-16"
applies_to:
  repositories: ["linux@fce2dfa"]
  software_versions: ["Linux 7.2-rc3-era master"]
  hardware_revisions: ["dual USB4 premise"]
related_sections: ["49", "52", "53", "54", "55"]
---

# Design implications

## Option matrix

| Criterion | TCP over thunderbolt-net | Direct USB4STREAM |
|---|---|---|
| Bring-up | **[RECOMMENDATION]** preferred | experimental |
| Message semantics | ordered reliable byte stream; app framing still needed | raw byte stream; app framing, recovery, integrity required |
| Kernel maturity | long-standing documented path | new in 7.2 development line |
| Tooling | `ip`, sockets, MPTCP, `ss`, `tcpdump`, `iperf3` | file operations, poll, configfs; custom diagnostics needed |
| Multiple logical lanes | sockets/ports | multiple named stream devices |
| Coexistence | can coexist with stream | can coexist with net |
| Zero-copy | not assumed | not assumed; driver maps kernel pages but userspace `read/write` is not proof of GPU-direct |
| Security | standard socket security options possible | application authentication/encryption required if threat model demands |

**[RECOMMENDATION]** Keep control and recovery on `thunderbolt-net`. Evaluate a distinct USB4STREAM `bulk` lane only after framing and fault tests pass. A second direct stream for latency traffic is contingent on HopID/ring resource availability and measured isolation.

**[RECOMMENDATION]** One writer must serialize a logical stream, or writes must be framed with a synchronization-safe header. The driver preserves bytes, not application write boundaries.

**[RECOMMENDATION]** Use nonblocking FDs with `poll/epoll`, bounded queues, and explicit CLOSE/error handling. Never let direct-stream progress block the rank compute loop.

**[RECOMMENDATION]** Begin with default ring/throttling values. Tune one variable at a time through Section 55; higher ring size can increase memory and queueing, while lower throttling can raise interrupt cost.

## Contingent decision gate

Choose direct stream only if it demonstrates a material benefit for the actual message distribution, maintains control-plane tail latency under bulk load, survives cable/peer faults without stale delivery, and has an operationally maintainable kernel path.

