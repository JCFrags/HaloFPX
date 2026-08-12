---
section_id: "77"
title: "HaloKV Benchmark Design Implications"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["fewtarius/CachyLLama@6be745998f568e379ea197fcf827baec73ff9940"]
  software_versions: []
  hardware_revisions: ["exact NVMe devices pending"]
related_sections: ["56", "57", "58", "59", "60", "61", "62", "63", "64", "65", "73", "76"]
---

# Design Implications

## Factorized benchmark matrix

**[RECOMMENDATION]** Stage, then cross only decision-relevant factors:

- object: system prefix, same-conversation continuation, attention KV, recurrent state, MTP/draft/speculative state, sampling/RNG where applicable;
- hit shape: exact, partial at controlled boundaries, incompatible, corrupt, absent;
- residency: hot RAM, warm RAM, OS page-cache warm, device cold;
- context/prefix length and dirty tail length;
- page/segment size candidates and compression/encoding if implemented;
- durability: volatile, async, data sync, full transaction/manifest mode;
- ranks: one, two parallel healthy, one missing/slow/corrupt;
- concurrency and queue depth;
- free-space/fragmentation/GC states;
- idle, concurrent decode, and controlled NVMe contention.

Every cell freezes model/build/state ABI/topology fingerprints and records logical bytes, physical host-write deltas, I/O calls/bytes, CPU, memory, power, thermals, and latency distributions.

## Correctness-first acceptance

**[RECOMMENDATION]** Restore tests compare the resumed run against a no-cache oracle at token/logit/state checkpoints defined by Section 78. Rejection and recomputation is the required response to corruption, incompatible identity, missing rank, or incomplete commit.

**[RECOMMENDATION]** System-prefix and continuation traces include adversarial near matches, template changes, tokenizer/model changes, user/tenant boundaries, contexts beyond stored match windows, and cross-rank topology changes.

## Writeback and garbage collection

Measure dirty-tail serialization, bytes logically changed, bytes committed, sync latency, batching delay, foreground stall, queue depth, and completed generation/manifest publication. GC measurements distinguish scan, live-copy, metadata update, deletion/reclaim, foreground interference, and free-space recovered.

**[INFERENCE]** Large pages may improve sequential I/O and metadata cost but increase dirty-tail rewrite amplification; small pages may reduce rewritten bytes while increasing lookup/metadata/I/O overhead. Only a matched sweep can choose.

## Hit-rate traces

**[RECOMMENDATION]** Use synthetic traces for controlled reuse-distance/prefix distributions and sanitized replay traces for realism. Partition trace fitting and held-out evaluation by user/workload; preserve ordering and think time. Report oracle upper bound, policy hit rate, token-weighted savings, eviction causes, and cold-start transient.

## Endurance budget

**[RECOMMENDATION]** Convert isolated SMART host-write deltas into host bytes per logical cache byte and projected daily/yearly host writes under declared workload volumes. Compare with the exact drive's vendor TBW/DWPD warranty and observed Percentage Used trend, with margin. Do not extrapolate from a short run without uncertainty and workload sensitivity.

**[OPEN]** The release endurance budget, retention horizon, spare margin, and whether consumer-grade drive warranty is acceptable require product decisions.
