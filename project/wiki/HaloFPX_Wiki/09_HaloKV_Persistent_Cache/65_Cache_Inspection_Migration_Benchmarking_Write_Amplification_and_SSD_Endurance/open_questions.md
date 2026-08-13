---
section_id: "65"
title: "Cache operations open questions"
status: "needs-machine-validation"
last_verified: "2026-08-12"
applies_to:
  repositories: []
  software_versions: []
  hardware_revisions: ["measured Crucial P310 CT1000P310SSD8 1 TB, firmware VACR001 on both targets; current state requires revalidation"]
related_sections: ["21", "57", "63"]
---

# Open questions

| ID | Question | Evidence needed |
|---|---|---|
| O65-01 | What rated-TBW source applies to the measured P310 part, and do current firmware/SMART identities still match? | dated inventory and exact vendor datasheet/warranty |
| O65-02 | Is device NAND-write telemetry available and trustworthy? | vendor docs/controller query |
| O65-03 | What store UUID/path/namespace authority prevents operator mistakes? | tooling ADR |
| O65-04 | Which format migrations are lossless? | version-pair round trips |
| O65-05 | Are exports encrypted/signed, and who may import them? | threat model/key policy |
| O65-06 | What write-amplification/endurance budgets trigger throttling? | M65-01/03 |
| O65-07 | How are user deletions represented in exports/backups? | privacy policy |
| O65-08 | What benchmark corpus may be preserved safely? | data-governance decision |
| O65-09 | Which admin operations may run online? | concurrency/fault tests |
