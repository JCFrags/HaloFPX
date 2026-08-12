---
section_id: "20"
title: "USB4 topology and independence procedures"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: []
  software_versions: ["Linux USB4/Thunderbolt", "thunderbolt-net"]
  hardware_revisions: ["Nimo Direct MME3L / NIMO Mini PC (revision unknown)"]
related_sections: ["18", "49", "50", "52", "55", "75", "84"]
---

# Procedures and checks

Use dedicated maintenance windows, console recovery, clock synchronization, idle gates, and raw logs. Network setup must ensure management LAN is excluded from bulk routes/MPTCP endpoints.

## S20-E01 — Topology and identity capture

Run on both nodes before and after connecting each cable:

```bash
uname -a; cat /proc/cmdline
sudo lspci -Dtvnn
sudo lspci -Dnnvvk
find /sys/bus/thunderbolt/devices -maxdepth 4 -type f -readable -print -exec cat {} \;
udevadm info --export-db | grep -A20 -B5 -E 'thunderbolt|usb4'
ip -d link show; ethtool -i thunderbolt0; ethtool thunderbolt0
ethtool -i thunderbolt1; ethtool thunderbolt1
grep -Ei 'thunderbolt|usb4' /proc/interrupts
find /sys/kernel/iommu_groups -maxdepth 2 -type l -print
dmesg --ctime | grep -Ei 'thunderbolt|usb4|retimer|iommu'
```

Record PCI root/bridge/NHI function, NUMA node, MSI vectors/effective affinity, domain security, `iommu_dma_protection`, router generation, RX/TX lanes and speed, retimer count/NVM, XDomain identity hashes, power-control state, cable/port labels, and kernel/module hashes.

## S20-E02 — Cable and port swap matrix

Test each certified cable on each physical port pair, one cable at a time, in both orientations where meaningful. Cold boot at least once per physical mapping. This distinguishes domain numbering, port capability, cable, connector, and retimer effects. Do not hot-remove storage or other tunneled PCIe devices.

## S20-E03 — Simultaneous-load independence

Use two bound server ports and explicit `SO_BINDTODEVICE`/source addresses. Predeclare message sizes, stream counts, directions, duration, repetitions, warmup, CPU/IRQ policy, and thermal limits.

Randomized cells:

1. A only; 2. B only; 3. A+B same direction; 4. A+B opposite directions; 5. repeated controls after dual load.

Capture per-rail goodput, p50/p95/p99/max latency, retransmits/errors/drops, MPTCP subflow bytes/fallback, CPU cycles, softirq/IRQ counts and affinity, memory bandwidth/PSI, power/clocks/temperature, tunnel events, and retrains. Compare simultaneous per-rail result with its temporally adjacent single-rail control. Publish confidence intervals and raw samples. A suggested preregistration is ≥90% retained per-rail goodput and no material p99 regression, but Section 55 owns final acceptance thresholds.

## S20-E04 — Failure isolation

Under bounded traffic, disconnect one labeled cable at a time. Verify only its domain/netdev drops, the other rail continues, no controller/GPU reset occurs, transport reports degradation, management LAN carries no bulk fallback, and reconnection creates a new verified epoch.

## S20-E05 — Enumeration stability

Repeat E01 after three cold boots, reversed cable attach order, and port/cable swaps. Maintain durable identity mapping even if domain/netdev numbers change.
