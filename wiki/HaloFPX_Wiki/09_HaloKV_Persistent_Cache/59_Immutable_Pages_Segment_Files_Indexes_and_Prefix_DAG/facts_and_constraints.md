---
section_id: "59"
title: "HaloKV on-disk format facts and constraints"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["HaloKV design", "fewtarius/CachyLLama"]
  software_versions: ["CachyLLama 6be745998f568e379ea197fcf827baec73ff9940", "format proposal v0"]
  hardware_revisions: ["dual Strix Halo NVMe hosts; exact filesystems pending"]
related_sections: ["57", "58", "60", "61", "62", "63", "65"]
---

# HaloKV on-disk format facts and constraints

- **[VERIFIED]** SQLite demonstrates that a durable database state may span main, journal/WAL and index files, and that commit markers plus recovery rules—not file extension—define committed state. [S59-02][S59-03]
- **[VERIFIED]** POSIX `rename()` changes directory entries, while `fsync()` requests synchronized completion for an open file. [S59-04][S59-05] **[INFERENCE]** HaloKV must test file sync, atomic replacement and parent-directory durability on its selected filesystem rather than assuming a rename alone survives power loss.
- **[VERIFIED]** Linux buffered I/O uses page cache; direct I/O bypasses it and imposes filesystem/device alignment and coherence behavior. [S59-06]
- **[VERIFIED]** CachyLLama stores up to a bounded token-prefix array plus target/draft/spec byte counts in each v3 checkpoint record and scans `ckpt-*.bin` at startup. [S59-01]
- **[RECOMMENDATION]** Derived indexes never establish trust. Readers verify object headers, bounds, content digest, compatibility fingerprint, token identity and ownership after index lookup.
- **[RECOMMENDATION]** Unknown required fields/schema/components cause a miss; optional extension fields use length-delimited namespaces and are ignored only when semantics explicitly allow it.

## Proposed object invariants

| Object | Immutable identity | Required validation |
|---|---|---|
| Page | digest of canonical header subset + uncompressed payload | magic/version/type/length/token range/component/rank/digest |
| Segment | segment UUID plus immutable sealed footer digest | complete records, offsets, footer and no overlap/out-of-bounds |
| DAG node | digest of parent IDs, exact token suffix digest and page references | compatibility/topology/tenant scope, token interval continuity, referenced page digests |
| Checkpoint manifest | digest of canonical global/rank manifests and generation | complete expected rank/component set and committed metadata state |
| Index entry | no trust identity; maps lookup key to object locator | locator validated against target object |

## Page sizing and alignment

**[RECOMMENDATION]** Make page payload size a format parameter, not a hard-coded 4 KiB assumption. Initial candidates should include 64 KiB, 256 KiB, 1 MiB and component/native block multiples. Align record starts for selected buffered/direct I/O paths; retain exact payload length so padding is never hashed as semantic data.

**[OPEN]** Token counts do not map to constant bytes across architectures, layers, KV types and recurrent components. Page boundaries therefore require component-specific token-to-byte metadata.

## WAL versus journal

**[RECOMMENDATION]** Data segments are append-only and independently verifiable. Metadata publication may use a proven embedded transactional engine or a small explicit WAL, but not an ad hoc mutable index. If custom WAL is chosen, it needs transaction IDs, length/checksum, prepare/commit records, replay idempotence, torn-tail detection and compaction/checkpoint rules.

