# 08 — Fabric and Transport

Defines the dual-USB4 communication layer and the experiments needed before specializing it.

Research status: populated; integrated protocol and machine validation remain pending as of 2026-07-17.

## Evidence and kernel authority

The Linux commits cited in Sections 49-52 (`fce2dfa773ced15f27dd27cd0b482a7473cdcf2a`) and Sections 53-55 (`8cdeaa50eae8dad34885515f62559ee83e7e8dda`, Linux v7.2-rc2) are inspected **source snapshots**, not an approved deployment kernel. The inspected `drivers/thunderbolt/stream.c` was byte-identical at those two pins during the 2026-07-17 review, but that does not establish packaging, target-hardware compatibility, or the behavior of a future stable kernel. When a deployment candidate is selected, re-diff the driver and testing ABI against the cited pins, record the exact package/config/module hashes, and run the Section 50/55 capability and crossover gates before promotion.

Section 53 is the proposed wire-contract authority. Its v1 recovery rule is global: loss or reconnect of either rail barriers all rails, ends the epoch, and permits retry only as a whole idempotent upper-layer operation in a newly authenticated epoch. Sections 49, 52, and 55 defer to that rule.

- 49 — Fabric Requirements and Transport Abstraction
- 50 — USB4STREAM and thunderbolt-net Implementation Options
- 51 — Existing ggml RPC and ROCmFPX RDMA Transport Audit
- 52 — Dual-Link Multipath: Striping, Alternation, Hedging, and Failover
- 53 — Message Framing, Credits, Flow Control, Integrity, and Security
- 54 — GPU-Visible Buffers, Coherence, Copies, and Zero-Copy Options
- 55 — Fabric Microbenchmark Plan and USB4 Kernel-Patch Decision
