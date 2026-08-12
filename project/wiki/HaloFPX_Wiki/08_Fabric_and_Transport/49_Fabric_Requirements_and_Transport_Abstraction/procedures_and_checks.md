---
section_id: "49"
title: "Fabric Requirements - Procedures and Checks"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: ["ROCmFPX@a5605a7", "llama.cpp@788e07d"]
  software_versions: ["Linux"]
  hardware_revisions: ["two target Strix Halo nodes"]
related_sections: ["20", "50", "51", "52", "55"]
---

# Procedures and checks

## Internet/source-code research completed

1. Pinned Linux, llama.cpp, and ROCmFPX revisions in [sources.md](sources.md).
2. Inspected USB4STREAM semantics and ggml RPC framing/progress at those revisions.
3. Derived requirements only where the carrier evidence is explicit; no benchmark values were imported.

## Required on-machine work

### FT-49-E1 - path inventory and independence

Prerequisites: both machines, both cables, root for sysfs inspection. Read-only.

```bash
uname -a
lspci -nnk | grep -A4 -Ei 'USB4|Thunderbolt'
find /sys/bus/thunderbolt/devices -maxdepth 2 -type f \
  \( -name unique_id -o -name rx_speed -o -name tx_speed -o -name rx_lanes -o -name tx_lanes \) \
  -print -exec cat {} \;
ip -details link show
```

Record node, kernel commit/package, BIOS, controller/port/cable identity, IOMMU state, and whether simultaneous traffic on one path changes the other. Do not call paths independent until simultaneous tests pass.

### FT-49-E2 - service envelope

Run the Section 55 harness for one and two links with payloads 64 B through at least the largest expected activation chunk. Capture p50/p95/p99/p99.9 latency, goodput, CPU time, context switches, IRQ counts, copies if observable, queue depth, and loss/reconnect behavior. Retain raw output and environment metadata under `experiments/`.

### FT-49-E3 - API conformance fault matrix

Safety boundary: parser/checksum/cancellation faults run first in an unprivileged disposable harness. Process exits use dedicated test workers with no model or sole evidence copy loaded. Administrative interface changes and physical cable removal require an approved Section 80 fault plan, exact resolved rail/interface targets, a preserved out-of-band management path, declared privileges, stop conditions, and a cleanup/baseline-smoke receipt. Do not target a production service, boot/storage path, model store, cache, workspace, or sole evidence copy.

For each carrier: peer exit before/after receive, authorized exact-rail removal, stalled receiver, checksum failure, cancellation race, duplicate message, reordered chunks, and incompatible protocol version. Verify bounded completion, global-epoch reset after either rail fails, and no stale step application.

## Gate

**[RECOMMENDATION]** Do not freeze numeric SLOs or a default multipath policy until E1-E3 are reviewed with Sections 52, 53, and 55.
