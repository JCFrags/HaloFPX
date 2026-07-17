---
section_id: "59"
title: "HaloKV format open questions"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["HaloKV design", "fewtarius/CachyLLama"]
  software_versions: ["format proposal v0"]
  hardware_revisions: ["dual Strix Halo NVMe hosts; exact filesystems pending"]
related_sections: ["57", "58", "60", "61", "62", "63", "64", "65"]
---

# HaloKV format open questions

| ID | Question | Evidence required |
|---|---|---|
| O59-01 | **[OPEN]** What component-native token/byte layout governs page boundaries? | Section 61 serialization schema and samples. |
| O59-02 | **[OPEN]** Which page size/alignment minimizes amplification and restore tail latency? | Real-state two-host size/I/O matrix. |
| O59-03 | **[OPEN]** What segment target balances file count, compaction and parallel reads? | Churn/restore benchmarks. |
| O59-04 | **[OPEN]** Should metadata use SQLite WAL, another proven engine, or a custom journal? | Failure model, dependency and fault-test comparison. |
| O59-05 | **[OPEN]** Which digest/encoding is fixed by section 57 and how are algorithm upgrades represented? | Canonical vectors and migration plan. |
| O59-06 | **[OPEN]** Is one-parent prefix lineage sufficient, or is true multi-parent DAG merging ever semantically valid? | Token/state equivalence proof; default one parent. |
| O59-07 | **[OPEN]** What is the exact durability boundary between local rank and global checkpoint publication? | Sections 58/63 protocol and crash tests. |
| O59-08 | **[OPEN]** Which I/O path wins for cold, warm and concurrent restore on each host? | Buffered/mmap/direct/async evidence. |
| O59-09 | **[OPEN]** How are readers pinned to generations during compaction/GC? | Concurrency design and kill tests. |
| O59-10 | **[OPEN]** What limits prevent hostile/corrupt manifests from exhausting memory or scan time? | Parser threat model and fuzzing. |
| O59-11 | **[OPEN]** Can CachyLLama v3 files be migrated without accepting weak identity or partial verification? | Offline verified migration prototype. |
| O59-12 | **[OPEN]** How are per-user deletion and shared-page references reconciled? | Reference/privacy model from sections 60/64. |

Internet follow-up must pin the exact metadata engine, digest implementation, filesystem and kernel docs selected for v0; moving documentation is not a format contract.

