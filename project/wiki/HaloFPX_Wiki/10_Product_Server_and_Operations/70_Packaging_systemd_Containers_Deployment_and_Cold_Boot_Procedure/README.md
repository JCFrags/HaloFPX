---
section_id: "70"
title: "Packaging, systemd, Containers, Deployment, and Cold-Boot Procedure"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["Custom_Inference_Project", "ROCmFPX", "llama.cpp"]
  software_versions: ["systemd 8009fa49845cd6fb7b7014ab06218b68fe702006", "ROCm 7.2.3 stable documentation snapshot"]
  hardware_revisions: ["dual Strix Halo target; exact nodes open"]
related_sections: ["67", "68", "69", "71", "72"]
---

# Packaging, systemd, Containers, Deployment, and Cold-Boot Procedure

**[RECOMMENDATION]** Ship a native systemd deployment first and an OCI image as an optional equivalent packaging surface. The host kernel, AMD driver, device nodes, USB4/network configuration, firmware, mounts, and service ordering remain host responsibilities even when inference runs in a container.

**[VERIFIED]** ROCm container guidance states that containers share the host kernel and require access to `/dev/kfd` and relevant `/dev/dri` render nodes [S70-02]. This does not prove a particular ROCm/Strix Halo combination is supported.

## Research split

- Online/source research completed: systemd directory/sandbox/service semantics, ROCm host/container constraints, OCI image identity, and upstream build pins.
- Machine validation required: supported OS/kernel/ROCm tuple, actual device ownership, unit hardening, cold-boot ordering, link naming, preload, recovery, and native/container parity.
- Contingent decisions: package format, static versus dynamic user, directory ownership, network manager integration, container runtime, preload policy, and uninstall retention.

See [facts and constraints](facts_and_constraints.md), [design implications](design_implications.md), [procedures and checks](procedures_and_checks.md), [open questions](open_questions.md), and [sources](sources.md).
