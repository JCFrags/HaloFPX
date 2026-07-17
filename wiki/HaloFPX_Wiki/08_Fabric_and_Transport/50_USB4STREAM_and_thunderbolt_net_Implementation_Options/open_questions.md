---
section_id: "50"
title: "USB4STREAM and thunderbolt-net - Open Questions"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["linux@fce2dfa"]
  software_versions: ["Linux 7.2 development line"]
  hardware_revisions: ["target machines unresolved"]
related_sections: ["20", "23", "49", "52", "54", "55"]
---

# Open questions

| ID | Question | Evidence needed |
|---|---|---|
| FT-50-Q1 | Does each target controller enumerate an XDomain and run USB4STREAM correctly? | FT-50-E1/E2 |
| FT-50-Q2 | Which distro/kernel will carry the feature, and is a backport maintainable? | packaging decision and exact patch series |
| FT-50-Q3 | How many usable HopIDs/streams remain with `thunderbolt-net` and two physical links? | configfs/source inspection on target |
| FT-50-Q4 | What ring size and throttling minimize tail latency without excessive IRQ cost? | FT-50-E3 sweep |
| FT-50-Q5 | Does simultaneous stream/net use contend for bandwidth or controller resources? | bidirectional coexistence benchmark |
| FT-50-Q6 | What is the effective userspace copy path, including pinned and GPU-visible buffers? | tracing plus Section 54 |
| FT-50-Q7 | What identity/security mechanism is required for a direct stream? | Section 53 threat model |
| FT-50-Q8 | Does direct stream materially outperform tuned TCP for HaloFPX payloads? | matched raw data, not line-rate claims |

**[OPEN]** USB4STREAM is a candidate, not the selected production carrier.

