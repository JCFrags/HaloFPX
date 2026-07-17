---
section_id: "70"
title: "Deployment and Cold-Boot Facts and Constraints"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["Custom_Inference_Project", "ROCmFPX"]
  software_versions: ["systemd 8009fa49845cd6fb7b7014ab06218b68fe702006", "ROCm 7.2.3 stable documentation snapshot"]
  hardware_revisions: ["dual Strix Halo target; exact nodes open"]
related_sections: ["67", "71", "72"]
---

# Facts and constraints

## Source-backed facts

- **[VERIFIED]** systemd `StateDirectory=`, `CacheDirectory=`, `LogsDirectory=`, and `ConfigurationDirectory=` create managed paths and mount dependencies [S70-01].
- **[VERIFIED]** `DynamicUser=` implies several sandbox settings and protects unmanaged filesystem areas, but its managed-directory ownership model must be evaluated against persistent GPU-service data [S70-01].
- **[VERIFIED]** systemd supports readiness notification, restart policy, credential loading, cgroup device access controls, and filesystem/process sandbox directives [S70-01].
- **[VERIFIED]** ROCm containers use the host kernel/driver and require explicit GPU device-node access; device-node selection can limit exposed GPUs [S70-02].
- **[VERIFIED]** AMD's ROCm compatibility matrix is release-, OS-, kernel-, and hardware-specific [S70-03].
- **[VERIFIED]** OCI image manifests/configuration are content-addressed and identify layers/configuration by digest [S70-04].

## Project constraints

- **[RECOMMENDATION]** Never treat a container image as a kernel/driver compatibility boundary. Record the host kernel, firmware, ROCm user space, device nodes, and image digest together.
- **[RECOMMENDATION]** Run neither coordinator nor worker as root. Grant only observed device/group permissions and no ambient capabilities.
- **[RECOMMENDATION]** Readiness must follow application-level peer/model/plan validation, not merely `After=` ordering or an open socket.
- **[RECOMMENDATION]** Uninstall preserves models, state, cache, experiments, and configuration by default. A separate explicit purge enumerates and confirms exact paths.
- **[OPEN]** AMD preview documentation mentions gfx1151/Strix Halo with particular kernels, but preview coverage is not production qualification for the project [S70-05].
