---
section_id: "19"
title: "Unified-memory design implications"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: []
  software_versions: ["ROCm/HIP", "Linux amdgpu/TTM"]
  hardware_revisions: ["Radeon 8060S / gfx1151"]
related_sections: ["24", "39", "54", "59", "74", "80"]
---

# Design implications

- **[RECOMMENDATION]** Implement admission from live, multi-source telemetry rather than `hipMemGetInfo` or marketing capacity alone. Record total/available RAM, PSI, swap, TTM/GTT totals/use, VRAM totals/use, pinned buffers, and estimated model/KV/workspace peaks.
- **[RECOMMENDATION]** Reserve headroom per rank and reject before load. Calibrate the reserve through S19-E02/E05; do not encode the July historical 124 GiB cap as universal.
- **[RECOMMENDATION]** Keep model weights and rank-local KV in coarse-grained/device-oriented allocations when practical; use fine-grained pinned host memory only for small CPU/GPU synchronization or measured transport staging needs.
- **[INFERENCE]** Because CPU and GPU share backing memory, unnecessary host↔device copies may be avoidable, but API allocation/coherence and cache behavior—not physical sharing alone—determine whether access is correct and fast.
- **[RECOMMENDATION]** Treat IOMMU state as a benchmark dimension and security decision. `amd_iommu=off` must never be presented as a generic performance recommendation; it disables isolation and historically disabled the NPU.
- **[RECOMMENDATION]** On allocation failure, preserve logs/counters, free the partial rank allocation, and fall back to a smaller profile or single-node mode. Never continue with an incomplete distributed placement.
- **[OPEN]** Zero-copy transport choices depend on Section 54 measurements of pinned/coherent buffers and on Section 20 IOMMU/security state.

## Research split

1. Completed now: public architecture, kernel and HIP semantics.
2. On-machine: S19-E01 capability snapshot; E02 allocation staircase; E03 bandwidth/coherence; E04 fault/migration; E05 contention; E06 rebooted IOMMU A/B.
3. Contingent decisions: BIOS carve-out, TTM limits, IOMMU policy, allocator choice, reserve size, and fallback thresholds.
