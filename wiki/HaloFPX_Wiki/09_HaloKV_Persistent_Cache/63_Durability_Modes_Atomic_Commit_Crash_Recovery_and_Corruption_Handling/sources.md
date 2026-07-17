---
section_id: "63"
title: "Durability sources"
status: "draft"
last_verified: "2026-07-17"
applies_to:
  repositories: ["fewtarius/CachyLLama@6be745998f568e379ea197fcf827baec73ff9940"]
  software_versions: []
  hardware_revisions: []
related_sections: ["56", "59"]
---

# Sources

Access date: 2026-07-17. Repository code is pinned; standards/man-pages identify interfaces but cannot establish the project filesystem/device failure contract.

| ID | Primary source and revision | Supports | Limitations |
|---|---|---|---|
| S63-01 | [CachyLLama SSD cache](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/common/kv-ssd-cache.cpp), `6be74599`, 2026-07-08 | current write/fsync/scan/header behavior | no power-loss guarantee |
| S63-02 | [fsync(2)](https://www.man7.org/linux/man-pages/man2/fsync.2.html), Linux man-pages 6.18, 2026-02-08 | file versus directory persistence | filesystem/device behavior still varies |
| S63-03 | [rename(2)](https://www.man7.org/linux/man-pages/man2/renameat.2.html), Linux man-pages 6.18 | atomic replacement namespace semantics | does not alone guarantee durability |
| S63-04 | [NVM Express Base Specification 2.2](https://nvmexpress.org/wp-content/uploads/NVM-Express-Base-Specification-Revision-2.2-2025.03.11-Ratified.pdf), ratified 2025-03-11 | NVMe controller/flush/log framework | device implementation must be tested |
| S63-05 | [llama.cpp state API](https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/include/llama.h), `788e07dc` | opaque runtime state interface | not storage transaction semantics |
| S63-06 | Project research follow-up, [Distributed protocol model-checking strategy](../../../../reviews/follow-ups/2026-07-16__distributed-protocol-model-checking__research__v01.md), researched 2026-07-17; grounded in TLA+ Tools [stable `v1.7.4`](https://github.com/tlaplus/tlaplus/releases/tag/v1.7.4), tag commit `5a47802b5c391f59ecdd44117981f4ff8c0656ba`, released 2024-08-05, and pinned [README](https://github.com/tlaplus/tlaplus/blob/5a47802b5c391f59ecdd44117981f4ff8c0656ba/README.md) | proposed finite-state safety/liveness gate and exact primary tool pin | proposed research only; no HaloKV model has been written or executed, and model checking does not prove implementation/filesystem conformance |
| S63-L01 | [Live deployed RPC tensor-cache audit](../../../../sources/measurements/2026-07-17-strix-halo-live-inventory/rpc-cache-audit.md), source `charlie12345/rocmfp4-llama@4860505e`, captured 2026-07-17 | current tensor-cache purpose, write/read integrity behavior, live size/count/headroom | separate predecessor cache; no destructive corruption test was run |

Source count is 7.
