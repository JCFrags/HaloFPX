---
section_id: "36"
title: "Speculative decoding test matrix"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689", "charlie12345/ROCmFPX@a5605a72768c6562241b248e268e33dc92787394"]
  software_versions: []
  hardware_revisions: ["dual Strix Halo"]
related_sections: ["41", "73", "76", "80"]
---

# Procedures and checks

## M36-01 matched mode matrix

Pin target/draft GGUF hashes, binary commit, backend, clocks/power policy, prompt corpus, seeds, chat template, sampling, context, output length, concurrency, and warmup. Run randomized repeated cells:

| Axis | Required levels |
|---|---|
| mode | none; native MTP; local external draft; node-2 external draft |
| phase/context | short/long prompt; short/long decode; low/high context occupancy |
| concurrency | 1, 2, 4, 8 slots as memory permits |
| sampling | greedy; project-default stochastic; high-entropy; grammar/tool constraint |
| draft depth | 1, 2, 4, 8 or supported subset |
| workload | prose, code, reasoning, structured output; multimodal if supported |

Record TTFT, target tokens/s, inter-token p50/p95/p99, proposal length, accepted length histogram, generated/accepted counts, target verification batch size/time, draft time, rollback time, link bytes/RTT, peak memory per rank, power/thermals, errors, and output-quality result.

**Acceptance:** greedy tokens match target-only; stochastic mode passes the project's distributional test; claimed win has confidence intervals and improves the chosen end-to-end metric without SLO regression.

## M36-02 failure and restore

During remote drafting, inject delayed, duplicated, reordered, stale, and dropped proposals plus draft-node restart. Acceptance: target emits no unverified token, session continues target-only, and restored speculation begins only from a committed prefix fingerprint.

## Internet follow-up

On every rebase, diff `docs/speculative.md`, `common/speculative.*`, model MTP graph builders, CLI defaults, and ROCmFPX's experiment log. Treat ROCmFPX measurements as environment-specific evidence, not HaloFPX results [S36-06].

