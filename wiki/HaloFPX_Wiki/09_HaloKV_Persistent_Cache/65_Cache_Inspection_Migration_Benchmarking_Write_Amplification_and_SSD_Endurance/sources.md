---
section_id: "65"
title: "Cache operations sources"
status: "draft"
last_verified: "2026-07-17"
applies_to:
  repositories: ["fewtarius/CachyLLama@6be745998f568e379ea197fcf827baec73ff9940", "fewtarius/llama-ai@1017f3dfdce3ca2b06aa9007b23295db3bb35722"]
  software_versions: []
  hardware_revisions: []
related_sections: ["21", "56"]
---

# Sources

Access date: 2026-07-17. Code/tool links are fixed revisions. Source inspection and external measurements do not establish HaloFPX runtime behavior or installed-device identity.

| ID | Primary source and revision | Supports | Limitations |
|---|---|---|---|
| S65-01 | CachyLLama [`common/kv-ssd-cache.h`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/common/kv-ssd-cache.h) and [`common/kv-ssd-cache.cpp`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/common/kv-ssd-cache.cpp), commit `6be745998f568e379ea197fcf827baec73ff9940` | record/index/stat fields and bounded tooling-surface audit | not a whole-repository audit and not a portable admin contract |
| S65-02 | [llama-ai](https://github.com/fewtarius/llama-ai/tree/1017f3dfdce3ca2b06aa9007b23295db3bb35722), `1017f3df`, 2026-07-08 | integration and benchmark evidence shape | existing measurements are environment-specific |
| S65-03 | [NVM Express Base Specification 2.2](https://nvmexpress.org/wp-content/uploads/NVM-Express-Base-Specification-Revision-2.2-2025.03.11-Ratified.pdf), ratified 2025-03-11 | SMART/health and data-unit semantics | controller/vendor fields vary |
| S65-04 | [smartmontools commit `65de0aee36ed5c1d8b31f9ff2490aafcfbdc0140`](https://github.com/smartmontools/smartmontools/tree/65de0aee36ed5c1d8b31f9ff2490aafcfbdc0140), accessed 2026-07-16 | operational SMART collection tool | pin the installed executable and drive database versions before experiments |
| S65-05 | [NIST SP 800-88 Rev. 2](https://csrc.nist.gov/pubs/sp/800/88/r2/final), September 2025 | export/deletion media lifecycle context | not cache-format guidance |

Source count is 5.
