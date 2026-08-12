---
section_id: "64"
title: "Lifecycle and privacy facts"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["fewtarius/CachyLLama@6be745998f568e379ea197fcf827baec73ff9940"]
  software_versions: []
  hardware_revisions: []
related_sections: ["56", "59"]
---

# Facts and constraints

**[VERIFIED]** CachyLLama tracks hot/warm bytes, access time/count, turn age, a cold-checkpoint cap, conversation cap, and cache statistics. Demotion removes RAM copies while retaining disk files; cold pruning unlinks checkpoint files [S64-01].

**[VERIFIED]** With a valid user identity, CachyLLama routes to `u/<fnv1a(user_id)>`, disables cross-user fuzzy lookup, and enforces a per-user concurrency cap. Anonymous requests keep content-derived routing [S64-02]. FNV-1a naming is not encryption or authentication.

**[INFERENCE]** A path namespace prevents accidental application-level reuse only if every lookup, administrative API, filesystem permission, backup, and log path preserves the same boundary.

**[VERIFIED]** NIST SP 800-88 Rev. 2 treats sanitization as making access infeasible for a defined effort; ordinary file deletion is not a media sanitization guarantee [S64-04]. Flash translation/wear leveling makes overwrite-based assurance especially deployment-dependent.

## Protected versus reclaimable

Active session generations, in-flight restore/write objects, pinned prefixes, and the newest committed fallback generation are protected. Unreachable temp objects, superseded generations beyond retention, expired entries, and over-quota cold data are reclaimable after a grace period.

