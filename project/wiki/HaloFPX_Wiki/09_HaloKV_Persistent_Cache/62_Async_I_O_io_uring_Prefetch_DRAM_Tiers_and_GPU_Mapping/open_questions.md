---
section_id: "62"
title: "Async I/O open questions"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: []
  software_versions: []
  hardware_revisions: []
related_sections: ["19", "21", "28"]
---

# Open questions

| ID | Question | Evidence needed |
|---|---|---|
| O62-01 | What kernel/liburing/filesystem versions are pinned? | machine inventory |
| O62-02 | Which io_uring features/operations are available? | probe output |
| O62-03 | Does direct I/O beat page-cache reuse for checkpoint shapes? | M62-01 |
| O62-04 | What queue depths protect foreground inference? | mixed-load sweep |
| O62-05 | What token prefix triggers useful prefetch? | M62-02 |
| O62-06 | Can backend restore consume registered/pinned buffers directly? | M62-03 |
| O62-07 | What memory reserve prevents unified-memory pressure? | pressure/soak test |
| O62-08 | How are late completions fenced after slot reuse? | operation-generation design/test |

