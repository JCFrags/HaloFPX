---
section_id: "21"
title: "Storage sources"
status: "draft"
last_verified: "2026-07-17"
applies_to:
  repositories: []
  software_versions: []
  hardware_revisions: []
related_sections: ["18", "22", "28", "65", "77"]
---

# Storage sources

## S21-L01 — Live target storage and SMART inventory

- **Canonical source:** [`../../../../sources/measurements/2026-07-17-strix-halo-live-inventory/README.md`](../../../../sources/measurements/2026-07-17-strix-halo-live-inventory/README.md)
- **Capture:** both nodes, 2026-07-17 11:52–12:05 America/Los_Angeles.
- **Supports:** installed NVMe model/firmware, filesystem/mount policy, capacity/headroom, visible consumers, idle temperature, and SMART health/endurance counters.
- **Limitations:** serials redacted; `nvme-cli` absent; negotiated PCIe width, queue details, performance, write amplification, and power-loss behavior remain unmeasured.

## S21-01

- **Title/publisher:** NVM Express Base Specification 1.4c / NVM Express
- **URL:** https://www.nvmexpress.org/wp-content/uploads/NVM-Express-1_4c-2021.06.28-Ratified.pdf
- **Revision/date/access:** 1.4c, ratified 2021-06-28; accessed 2026-07-16
- **Supports:** SMART/health log semantics, percentage used, data units written.
- **Limitations:** Protocol semantics only; no installed-drive rating or behavior.

## S21-02

- **Title/publisher:** Open Source NVMe SSD Management Utility / NVM Express
- **URL:** https://nvmexpress.org/open-source-nvme-ssd-management-utility-nvme-cli/
- **Revision/date/access:** page current at 2026-07-16
- **Supports:** `nvme-cli` health, error, self-test, and endurance interpretation.
- **Limitations:** Example output is not project evidence.

## S21-03

- **Title/publisher:** Multi-Queue Block IO Queueing Mechanism / Linux kernel documentation
- **URL:** https://docs.kernel.org/block/blk-mq.html
- **Revision/date/access:** Linux 7.2 documentation; accessed 2026-07-16
- **Supports:** software/hardware queue and scheduler architecture.
- **Limitations:** Does not prescribe a HaloFPX scheduler.

## S21-04

- **Title/publisher:** Queue sysfs files / Linux kernel source documentation
- **URL:** https://www.kernel.org/doc/Documentation/block/queue-sysfs.rst
- **Revision/date/access:** repository document; accessed 2026-07-16
- **Supports:** queue attribute meanings and optionality.
- **Limitations:** Live kernel may expose a version-specific subset.

## S21-05

- **Title/publisher:** fio 3.41 documentation / fio project
- **URL:** https://fio.readthedocs.io/en/master/fio_doc.html
- **Revision/date/access:** fio 3.41-49-gde3d docs; accessed 2026-07-16
- **Supports:** direct I/O, queue depth, verification, steady state, JSON output.
- **Limitations:** Workload design and safe target selection remain project responsibilities.

## S21-06

- **Title/publisher:** `fsync(2)` / Linux man-pages project
- **URL:** https://man7.org/linux/man-pages/man2/fsync.2.html
- **Revision/date/access:** man-pages 6.15 rendering; accessed 2026-07-16
- **Supports:** file data/metadata sync and containing-directory requirement.
- **Limitations:** Hardware/device guarantees still apply.

## S21-07

- **Title/publisher:** AMD Ryzen AI Max+ 395 product specifications / AMD
- **URL:** https://www.amd.com/en/products/processors/laptop/ryzen/ai-300-series/amd-ryzen-ai-max-plus-395.html
- **Revision/date/access:** page current at 2026-07-16
- **Supports:** APU-level PCIe 4.0 and NVMe support.
- **Limitations:** Does not establish node model, slot wiring, installed SSD, or negotiated link.
