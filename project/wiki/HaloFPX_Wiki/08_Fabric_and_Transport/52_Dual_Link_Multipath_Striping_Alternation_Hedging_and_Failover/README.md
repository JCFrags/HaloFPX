---
section_id: "52"
title: "Dual-Link Multipath: Striping, Alternation, Hedging, and Failover"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: ["torvalds/linux@fce2dfa773ced15f27dd27cd0b482a7473cdcf2a"]
  software_versions: ["MPTCP v1 / RFC 8684", "Linux MPTCP"]
  hardware_revisions: ["two independent USB4 links assumed, not verified"]
related_sections: ["20", "38", "41", "42", "43", "49", "50", "53", "55"]
---

# 52 - Dual-Link Multipath: Striping, Alternation, Hedging, and Failover

**[RECOMMENDATION]** Start with explicit best-link selection plus one-link recovery. Under the proposed v1 application protocol, either rail failure barriers both rails and creates a new authenticated global epoch; “failover” never means continuing partial old-epoch work on the survivor. Add direction separation or alternating independent collectives only after link independence is measured. Enable proportional striping only above a measured size threshold; reserve hedging for rare deadline-critical idempotent messages.

**[ASSUMPTION]** Both physical USB4 links can be simultaneously active and fail independently. Section 20 and `FT-52-E1` must validate that premise.

## Pages

- [Facts and constraints](facts_and_constraints.md)
- [Design implications](design_implications.md)
- [Procedures and checks](procedures_and_checks.md)
- [Open questions](open_questions.md)
- [Sources](sources.md)

No default threshold or health timeout is claimed without measurement.
