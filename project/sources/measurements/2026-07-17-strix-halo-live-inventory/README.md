---
id: S-LOCAL-2026-07-17-STRIX-HALO-LIVE
type: source
status: raw-reviewed
title: Live nimo-1 and nimo-2 target inventory
origin: ssh nimo-1 and ssh nimo-2
retrieved_at: 2026-07-17T11:52:00-07:00
authority: primary
sensitivity: internal
---

# Live Strix Halo target inventory

This folder preserves a normalized, redacted snapshot of the two target machines collected over SSH on 2026-07-17. It is the primary local source for Wiki claims promoted during this capture.

## Scope

- Hosts: `nimo-1` (`192.168.40.11`) and `nimo-2` (`192.168.40.12`).
- Operator path: Windows OpenSSH aliases using the unprivileged `connorb` account.
- Collection mode: read-only system inspection plus five ICMP echo requests in each direction on each private USB4 rail.
- No service, package, filesystem, network, firmware, boot, or runtime configuration was changed.
- No inference request, throughput benchmark, stress test, or fault injection was run because the dual-node model service was active.
- NVMe SMART was read through passwordless `sudo`; device serials were observed for pair identity but intentionally omitted from this project copy.

## Artifacts

- [`nimo-1.md`](nimo-1.md) — normalized node-1 snapshot.
- [`nimo-2.md`](nimo-2.md) — normalized node-2 snapshot.
- [`comparison.md`](comparison.md) — matched-pair comparison and project consequences.
- [`rpc-cache-audit.md`](rpc-cache-audit.md) — deployed RPC tensor-cache behavior and integrity gap.
- [`commands.md`](commands.md) — command families and interpretation boundaries.

## Known limitations

- Point-in-time counters and thermals describe the capture window, not sustained inference.
- The physical connector labels, cable makes/serials, retimer firmware, board revision, EC firmware, and power/cooling assembly were not available through this remote session.
- Five ICMP samples per rail prove reachability and give a diagnostic RTT snapshot; they are not a fabric benchmark.
- USB4 interface counters are cumulative since interface creation and are not attributable to one workload.
- No large artifact was hashed during the live workload. Active executable hashes and Git commits were captured; the 121.86 GB model and 112 GiB RPC cache were not read end-to-end.
- The raw Codex terminal transcript contained device serials. This canonical source package retains the decision-relevant values and redacts identity fields by design.

