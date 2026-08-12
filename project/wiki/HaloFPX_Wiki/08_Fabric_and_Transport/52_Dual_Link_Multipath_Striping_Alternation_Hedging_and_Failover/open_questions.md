---
section_id: "52"
title: "Dual-Link Multipath - Open Questions"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["linux@fce2dfa"]
  software_versions: ["Linux MPTCP"]
  hardware_revisions: ["dual USB4 topology unverified"]
related_sections: ["20", "38", "41", "42", "43", "49", "50", "55"]
---

# Open questions

| ID | Question | Resolution evidence |
|---|---|---|
| FT-52-Q1 | Are link failures and bandwidth actually independent? | FT-52-E1/E3 plus Section 20 |
| FT-52-Q2 | What payload/chunk size makes striping beneficial at p99? | FT-52-E2 with confidence intervals |
| FT-52-Q3 | Is direction separation better than symmetric striping? | bidirectional workload sweep |
| FT-52-Q4 | Which messages are safely idempotent and hedge-eligible? | protocol inventory/Section 53 |
| FT-52-Q5 | What health windows and hysteresis avoid flapping yet meet failover SLO? | repeated fault trials |
| FT-52-Q6 | How much reorder memory can each session consume? | workload envelope and admission limits |
| FT-52-Q7 | Does Linux MPTCP provide sufficient per-message policy control? | prototype comparison |
| FT-52-Q8 | What happens to a distributed decode step after one-link loss? | mode-specific experiments Sections 41-43 |
| FT-52-Q9 | Are thermal/memory/IRQ correlations strong enough to defeat two-link gain? | sustained telemetry |

**[OPEN]** Policy thresholds, health constants, and the default scheduler remain contingent.

