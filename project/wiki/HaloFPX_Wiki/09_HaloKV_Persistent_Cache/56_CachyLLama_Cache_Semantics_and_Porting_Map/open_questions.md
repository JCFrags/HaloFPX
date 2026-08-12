---
section_id: "56"
title: "CachyLLama porting open questions"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["fewtarius/CachyLLama", "ggml-org/llama.cpp", "charlie12345/ROCmFPX"]
  software_versions: ["CachyLLama 6be745998f568e379ea197fcf827baec73ff9940"]
  hardware_revisions: ["dual gfx1151 Strix Halo; exact hosts pending inventory"]
related_sections: ["57", "58", "59", "60", "61", "62", "63", "64", "65"]
---

# CachyLLama porting open questions

| ID | Question | Resolution evidence |
|---|---|---|
| O56-01 | **[OPEN]** What is the exact merge ancestry and API delta among selected ROCmFPX, llama.cpp and CachyLLama pins? | Three-way source/API inventory and build. |
| O56-02 | **[OPEN]** Which serialized state components are necessary and sufficient per supported architecture? | Model-by-model save/restore/recompute tests; section 61. |
| O56-03 | **[OPEN]** Does CachyLLama v3 restore correctly after restart for transformer, hybrid/recurrent, MTP and speculative paths? | Deterministic suffix-only tests with raw evidence. |
| O56-04 | **[OPEN]** Which source-reported performance numbers are reproducible on the two gfx1151 hosts? | Matched benchmark distributions and manifests. |
| O56-05 | **[OPEN]** What exact fields currently feed `compat_hash`, and which omissions can yield false acceptance? | Call-chain audit and mutation tests; section 57. |
| O56-06 | **[OPEN]** How does source direct-final-path writing behave under torn writes, lost directory entries and stale index data? | Fault injection; section 63. |
| O56-07 | **[OPEN]** Can system-prefix boundary detection be replaced entirely by caller-supplied rendered token ranges? | Template corpus and API design; section 60. |
| O56-08 | **[OPEN]** What authenticated tenant identity exists above `llama_user_id`, and what is anonymous-sharing policy? | Threat/privacy decision; sections 60/64. |
| O56-09 | **[OPEN]** What tier budgets avoid serialized-state duplication between application RAM and kernel page cache? | Memory/NVMe telemetry under churn; section 62. |
| O56-10 | **[OPEN]** Which source components are cleanly portable versus too coupled to CachyLLama server slots? | Adapter prototype and dependency map. |
| O56-11 | **[OPEN]** Can legacy artifacts be identified and migrated without trusting their weak hashes or compatibility field? | Offline migration prototype with recomputation verification; section 65. |
| O56-12 | **[OPEN]** What license/provenance notices and patch history must accompany imported code? | Source/legal inventory tied to exact blobs. |

No question is resolved by repository README claims alone.

