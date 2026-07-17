---
section_id: "65"
title: "Inspection and endurance facts"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["fewtarius/CachyLLama@6be745998f568e379ea197fcf827baec73ff9940"]
  software_versions: []
  hardware_revisions: []
related_sections: ["21", "56"]
---

# Facts and constraints

**[VERIFIED]** CachyLLama records checkpoint IDs, positions, token counts/hash/prefix, compatibility hash, target/draft/spec sizes, access counters, tiers, and aggregate hit/miss/store/evict/load statistics [S65-01]. The format is implementation-specific and lacks the checksummed portable bundle defined by sections 57/63.

**[VERIFIED]** NVMe SMART/health counters are controller-defined standardized fields, including host data units written and percentage used [S65-03]. Host data units written are not NAND physical bytes and therefore do not alone reveal device-internal write amplification.

## Write accounting

**[RECOMMENDATION]** Track:

- logical new continuation bytes;
- application bytes submitted by stream/type;
- filesystem allocated bytes and metadata where observable;
- device host writes delta from NVMe counters;
- NAND writes only if trustworthy vendor/device telemetry exists;
- compaction/GC rewrite and discarded bytes.

Application write amplification = application bytes / logical new bytes. Host-device amplification = NVMe host bytes / application bytes. NAND WAF must be labeled unavailable unless directly measured.

**[INFERENCE]** Checkpoint-per-turn designs may rewrite overlapping state and amplify writes substantially; immutable page/segment deduplication can reduce logical duplication but compaction can add background writes.

