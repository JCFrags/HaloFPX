---
section_id: "57"
title: "Open compatibility and fingerprint questions"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: ["future HaloFPX integration tree"]
  software_versions: []
  hardware_revisions: ["dual gfx1151 Strix Halo; exact hosts pending inventory"]
related_sections: ["15", "26", "39", "43", "48", "58", "61", "63", "65", "78"]
---

<a id="s57-open"></a>
# Open compatibility and fingerprint questions

| ID | Question | Evidence needed / owner |
|---|---|---|
| O57-01 | **[OPEN]** Which exact integration commits and dependency locks become the first supported producer/reader baseline? | Section 11/15 baseline decision and reproducible builds. |
| O57-02 | **[OPEN]** What is the reviewed CDDL/schema and canonical-CBOR implementation for `halofpx.compat.v1`? | Two-encoder conformance corpus, parser limits, security review. |
| O57-03 | **[OPEN]** Which runtime/backend fields are true semantic invalidators versus operational provenance? | Source trace plus one-field mutation/continuation matrix on HIP and Vulkan. |
| O57-04 | **[OPEN]** What exact state streams, magic/version/flags and ABI boundaries exist after CachyLLama/ROCmFPX integration? | Final-fork serialization inventory; section 61. |
| O57-05 | **[OPEN]** Which schema transitions have safe adapters, and what exact continuation criterion authorizes them? | Old/new fixture corpus and section 78 equivalence decision. |
| O57-06 | **[OPEN]** Does physical device/host identity affect persistent state compatibility on the selected backends? | Cross-host/rank-remap restore experiments with matched and changed software. |
| O57-07 | **[OPEN]** What topology fields cover tensor, pipeline, MoE, replication, and remote-draft modes without ambiguous ranges? | Planner schema review with sections 41, 43, 44, 47, and 58. |
| O57-08 | **[OPEN]** Is SHA-256 throughput material at startup/commit, and is a faster secondary digest justified? | Streaming benchmarks with full model and cache objects; never weaken the authoritative root without ADR. |
| O57-09 | **[OPEN]** What writer-authentication/signature policy is required for shared or adversarial cache storage? | Threat model and tenant/privacy decision in section 64. |
| O57-10 | **[OPEN]** What retention/quarantine and diagnostic-redaction policy handles corrupt or impossible equal-root/different-manifest events? | Operations/security review with sections 63 and 77. |

No question is resolved by the current 64-bit CachyLLama `compat_hash`.
