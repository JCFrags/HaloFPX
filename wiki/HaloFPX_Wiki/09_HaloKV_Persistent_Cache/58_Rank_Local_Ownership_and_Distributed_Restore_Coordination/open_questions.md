---
section_id: "58"
title: "Rank-local restore open questions"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["charlie12345/ROCmFPX", "fewtarius/CachyLLama"]
  software_versions: ["ROCmFPX a5605a72768c6562241b248e268e33dc92787394", "CachyLLama 6be745998f568e379ea197fcf827baec73ff9940"]
  hardware_revisions: ["two gfx1151 Strix Halo hosts; exact topology pending"]
related_sections: ["50", "52", "54", "57", "59", "61", "63", "75"]
---

# Rank-local restore open questions

| ID | Question | Evidence required |
|---|---|---|
| O58-01 | **[OPEN]** What exact execution plans will HaloFPX support first? | Approved plan schemas tied to source implementation. |
| O58-02 | **[OPEN]** Which KV/recurrent/draft/spec bytes does each rank own under every plan? | Instrumented serialization/object map and suffix correctness. |
| O58-03 | **[OPEN]** Does current tensor-split state serialization already contain global or rank-local data? | Code trace, sizes/content and two-rank restore tests. |
| O58-04 | **[OPEN]** Is any topology transition safely reusable without full recomputation? | Formal mapping plus bit/semantic equivalence; default miss. |
| O58-05 | **[OPEN]** What common prefix boundary is valid across attention and recurrent components? | Model-specific tests; section 61. |
| O58-06 | **[OPEN]** What timeout, retry and coordinator election rules avoid hangs and split-brain publication? | Fault-injection state-machine tests. |
| O58-07 | **[OPEN]** How are global manifests atomically committed only after rank-local objects are durable? | Crash-consistency design/tests; section 63. |
| O58-08 | **[OPEN]** What control-plane byte ceiling prevents accidental bulk cache transfer? | Protocol schema, enforcement test and link trace. |
| O58-09 | **[OPEN]** Should rank-local objects be proactively duplicated, or is recomputation cheaper than repair? | Failure-frequency, NVMe, restore and recompute measurements. |
| O58-10 | **[OPEN]** What is the safe single-node fallback model/state identity? | Separate plan fingerprint and restored/recomputed equivalence. |
| O58-11 | **[OPEN]** How are MoE routing/expert placements fingerprinted, and is any routing history semantic state? | Source audit and model-specific experiments. |
| O58-12 | **[OPEN]** Can both NVMe devices sustain simultaneous restore without starving model/cache writes? | Two-host I/O traces and tail latency. |

Internet follow-up must pin the execution/transport implementation commits chosen by sections 49–54 and inspect any newly added distributed-state APIs before implementation.

