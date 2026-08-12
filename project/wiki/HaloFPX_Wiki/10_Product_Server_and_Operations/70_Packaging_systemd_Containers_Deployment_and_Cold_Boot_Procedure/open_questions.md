---
section_id: "70"
title: "Deployment and Cold-Boot Open Questions"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["Custom_Inference_Project"]
  software_versions: ["deployment design candidate 2026-07-16"]
  hardware_revisions: ["dual Strix Halo target; exact nodes open"]
related_sections: ["67", "71", "72"]
---

# Open questions

1. **[OPEN]** Which exact distro, kernel, firmware, ROCm, compiler, and ROCmFPX commit form the first supported tuple?
2. **[OPEN]** What are the observed owners/modes for `/dev/kfd` and each intended render node on both hosts?
3. **[OPEN]** Which network manager owns USB4 addressing, routes, MPTCP, MTU, and firewall rules?
4. **[OPEN]** Is native systemd the sole supported tier or must OCI deployment have feature parity at launch?
5. **[OPEN]** Which package format and signing/update channel are supported?
6. **[OPEN]** Which models preload, and what boot-time/memory budget bounds preload?
7. **[OPEN]** Is automatic single-node fallback allowed for every API route and workload?
8. **[OPEN]** Can `DynamicUser=` satisfy device and durable-state requirements after testing?
