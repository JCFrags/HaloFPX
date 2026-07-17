---
section_id: "70"
title: "Deployment and Cold-Boot Procedures and Checks"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["Custom_Inference_Project"]
  software_versions: ["deployment design candidate 2026-07-16"]
  hardware_revisions: ["dual Strix Halo target; exact nodes open"]
related_sections: ["69", "71", "72"]
---

# Procedures and checks

## OPS-70-E1 — cold-boot matrix

For each node and supported mode, record firmware, kernel, boot parameters, ROCm packages, device nodes/ACLs, link configuration, release digest, model/plan hashes, and timestamps. Use a disposable deployment/cache/store, copied fixtures, exact resolved targets, resource ceilings, stop conditions, preserved recovery access, declared privileges, and cleanup record. Test clean boot, peer late, peer absent, safely simulated link unavailability, corrupt disposable cache, missing copied model, and bounded ENOSPC on a loopback/sacrificial filesystem. Physical cable, kernel/device, or actual power-loss tests are separate Section 80-authorized experiments and must never target the boot disk, production cache/model store, workspace, or sole evidence copy. Pass only when readiness/degradation and fallback match policy without manual cleanup.

## OPS-70-E2 — unit hardening and device access

Start from maximal proposed sandboxing. Verify inference/device discovery, then remove only controls with a documented failing syscall/path/device and a narrower mitigation. Capture `systemd-analyze security`, effective unit properties, device ACLs, journal, and successful restart after forced termination.

## OPS-70-E3 — container parity

Run the same pinned model/plan and readiness/fault suite natively and in an image referenced by digest. Record host/kernel/driver and image SBOM. Pass if API semantics, rank/device selection, health, signals, shutdown, and recovery match; performance is reported separately as measured evidence.

## OPS-70-E4 — uninstall/reinstall

Install, operate, uninstall without purge, inventory preserved paths/hashes, reinstall the same release, and verify recovery. Separately test explicit purge only in a disposable environment with exact-target preview.

## OPS-70-E5 — activation and rollback phase rehearsal

Record the old pointer, running process executable/build identity, traffic assignment, and durable-state generation. Rehearse all eight activation phases in order. Inject a bounded failure between each adjacent phase and prove that abort/rollback restores a coherent pointer, process, traffic, and compatible state combination. A pointer-only change is neither deployment success nor rollback success.

## Release checklist

- **[RECOMMENDATION]** Verify package signatures/digests, config schema, dependency tuple, service user, permissions, firewall listeners, rollback release, and cold-boot experiment before activation.
- **[RECOMMENDATION]** Do not publish a generic install command until the supported distro/kernel/ROCm matrix has passed on both physical nodes.
