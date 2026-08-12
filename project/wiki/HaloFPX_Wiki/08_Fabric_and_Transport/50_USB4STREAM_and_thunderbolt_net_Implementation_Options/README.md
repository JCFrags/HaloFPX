---
section_id: "50"
title: "USB4STREAM and thunderbolt-net Implementation Options"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: ["torvalds/linux@fce2dfa773ced15f27dd27cd0b482a7473cdcf2a"]
  software_versions: ["Linux 7.2-rc3-era master"]
  hardware_revisions: ["target USB4 controllers and cables unverified"]
related_sections: ["20", "23", "28", "49", "52", "53", "54", "55"]
---

# 50 - USB4STREAM and thunderbolt-net Implementation Options

**[RECOMMENDATION]** Use TCP over `thunderbolt-net` as the bring-up and recovery baseline. Treat direct USB4STREAM as an experimental data carrier gated by kernel availability, per-message framing, security, and matched on-machine results.

**[VERIFIED]** USB4STREAM exists in the pinned Linux 7.2-rc3-era source as `thunderbolt-stream`, exposes `/dev/tbstreamX`, supports multiple streams, and can coexist with `thunderbolt-net` [S50-01, S50-02].

**[MEASURED]** Both target machines currently run `7.1.3-1-cachyos` with `CONFIG_USB4=m` and `CONFIG_USB4_NET=m`; dual `thunderbolt-net` paths are active, but `thunderbolt_stream` is not installed and no `/dev/tbstream*` node exists [S50-L01]. TCP/MPTCP over USB4NET is therefore the only deployed project carrier in this snapshot.

The Linux implementation is new and no Strix Halo compatibility or performance is established here.

## Pages

- [Facts and constraints](facts_and_constraints.md)
- [Design implications](design_implications.md)
- [Procedures and checks](procedures_and_checks.md)
- [Open questions](open_questions.md)
- [Sources](sources.md)
