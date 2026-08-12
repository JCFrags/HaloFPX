---
section_id: "70"
title: "Deployment and Cold-Boot Sources"
status: "verified"
last_verified: "2026-07-16"
applies_to:
  repositories: ["systemd", "ROCmFPX", "Custom_Inference_Project"]
  software_versions: ["source snapshot 2026-07-16"]
  hardware_revisions: []
related_sections: ["71", "72"]
---

# Sources

| ID | Source | Revision/access | Scope and use | Limitations |
|---|---|---|---|---|
| S70-01 | [systemd source/manuals](https://github.com/systemd/systemd/tree/8009fa49845cd6fb7b7014ab06218b68fe702006/man) | Commit `8009fa49845cd6fb7b7014ab06218b68fe702006`; accessed 2026-07-16 | `systemd.exec`, `systemd.service`, resource control, credentials, sandboxing | Mechanisms only; unit behavior and effective hardening require machine tests |
| S70-02 | [ROCm 7.2.3 container documentation](https://rocm.docs.amd.com/projects/install-on-linux/en/docs-7.2.3/how-to/docker.html) | Version 7.2.3 documentation; accessed 2026-07-16 | Host kernel/driver and device mapping | General ROCm guidance; does not qualify the HaloFPX node tuple |
| S70-03 | [ROCm compatibility matrix](https://rocm.docs.amd.com/en/latest/compatibility/compatibility-matrix.html) | Mutable current matrix; accessed 2026-07-16 | Discovery authority for release/OS/kernel/hardware compatibility | Must be frozen to the selected release before deployment approval |
| S70-04 | [OCI image specification](https://github.com/opencontainers/image-spec/tree/af26a05fba5ee648512f4ea3c9fda1fcc1b6d6dc) | Commit `af26a05fba5ee648512f4ea3c9fda1fcc1b6d6dc`; accessed 2026-07-16 | Digest-addressed manifests, configuration, and layers | Image identity does not prove runtime or state compatibility |
| S70-05 | [ROCm 7.13.0 preview compatibility matrix](https://rocm.docs.amd.com/en/7.13.0-preview/compatibility/compatibility-matrix.html) | Preview 7.13.0 documentation; accessed 2026-07-16 | Discovery evidence for `gfx1151` | Preview evidence only; not production qualification |
| S70-06 | [ROCmFPX source](https://github.com/charlie12345/ROCmFPX/tree/a5605a72768c6562241b248e268e33dc92787394) | Commit `a5605a72768c6562241b248e268e33dc92787394`; accessed 2026-07-16 | Exact candidate source lineage | Product subset, build recipe, and physical-node qualification remain open |

**[VERIFIED]** The deployment proposal combines source-backed mechanisms with unvalidated project choices. Only the recorded machine experiments can promote the latter.
