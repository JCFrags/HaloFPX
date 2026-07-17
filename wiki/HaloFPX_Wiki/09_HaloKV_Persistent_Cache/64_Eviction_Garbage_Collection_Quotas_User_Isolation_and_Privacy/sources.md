---
section_id: "64"
title: "Lifecycle and privacy sources"
status: "draft"
last_verified: "2026-07-17"
applies_to:
  repositories: ["fewtarius/CachyLLama@6be745998f568e379ea197fcf827baec73ff9940"]
  software_versions: []
  hardware_revisions: []
related_sections: ["56", "71"]
---

# Sources

Access date: 2026-07-17. The CachyLLama audit is deliberately limited to the linked cache and user-isolation paths; no whole-repository absence claim is made.

| ID | Primary source and revision | Supports | Limitations |
|---|---|---|---|
| S64-01 | CachyLLama [`common/kv-ssd-cache.h`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/common/kv-ssd-cache.h) and [`common/kv-ssd-cache.cpp`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/common/kv-ssd-cache.cpp), commit `6be745998f568e379ea197fcf827baec73ff9940` | tiers, caps, eviction, stats, and bounded absence audit within these files | not a whole-repository audit; not reachability GC or quota proof |
| S64-02 | CachyLLama [`user-isolation-design.md`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/docs/development/user-isolation-design.md) and [`server-context-page-manager.cpp`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/tools/server/server-context-page-manager.cpp), same commit | user namespace/routing and concurrency behavior | design can drift; caller authentication is external; audit scope remains bounded |
| S64-03 | [Linux cgroup v2](https://docs.kernel.org/admin-guide/cgroup-v2.html), accessed 2026-07-16 | memory/I/O control mechanisms | deployment policy unresolved |
| S64-04 | [NIST SP 800-88 Rev. 2](https://csrc.nist.gov/pubs/sp/800/88/r2/final), final September 2025 | media sanitization framework | organizational policy, not app deletion API |
| S64-05 | [NVM Express Base Specification 2.2](https://nvmexpress.org/wp-content/uploads/NVM-Express-Base-Specification-Revision-2.2-2025.03.11-Ratified.pdf), 2025-03-11 | storage management/log framework | controller features vary |

Source count is 5.
