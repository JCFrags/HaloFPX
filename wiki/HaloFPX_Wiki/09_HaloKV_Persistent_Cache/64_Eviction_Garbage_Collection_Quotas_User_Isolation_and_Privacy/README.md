---
section_id: "64"
title: "Eviction, Garbage Collection, Quotas, User Isolation, and Privacy"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: ["fewtarius/CachyLLama@6be745998f568e379ea197fcf827baec73ff9940"]
  software_versions: []
  hardware_revisions: ["dual Strix Halo"]
related_sections: ["59", "60", "63", "65", "71"]
---

# 64 - Cache lifecycle and isolation

- **[VERIFIED]** CachyLLama provides hot/warm/cold demotion, checkpoint/conversation caps, LRU-like eviction, and hashed `user_id` namespaces that disable cross-user continuation lookup [S64-01][S64-02].
- **[VERIFIED]** No per-user byte-quota, encryption, or verifiable-secure-deletion mechanism was identified in the specifically inspected pinned cache/user-routing files [S64-01][S64-02]. This is a bounded source-audit result, not a whole-repository absence claim.
- **[RECOMMENDATION]** HaloKV eviction must be reachability-aware, quota-fair, active-session-safe, and throttled by latency/write-amplification budgets.
- **[OPEN]** Privacy retention and media-sanitization requirements need an operator policy.

## Research split

- **Internet/source-code research completed:** the pinned cache files and user-isolation design establish observed tier, pruning, namespace, and concurrency behavior; NIST/NVMe/Linux references establish external lifecycle mechanisms.
- **Target-machine work required:** run reachability/GC, quota/pressure, cross-principal isolation, deletion/backup, and endurance tests in disposable stores with actual filesystem permissions and identity plumbing.
- **Contingent decisions:** quota values/accounting, anonymous sharing, encryption/key scope, deletion promise, backup/export treatment, emergency reserve, and media-sanitization policy remain open.
