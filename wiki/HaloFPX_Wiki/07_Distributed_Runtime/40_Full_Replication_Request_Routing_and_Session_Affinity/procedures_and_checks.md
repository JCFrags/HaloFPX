---
section_id: "40"
title: "Replication Procedures and Checks"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["HaloFPX design candidate"]
  software_versions: []
  hardware_revisions: []
related_sections: ["46", "48", "55", "66"]
---

# Procedures and checks

## Source research completed

Pinned vLLM data-parallel routing documentation, `llama.cpp` server slot/cache behavior, CachyLlama cache code presence, and Orca scheduling. No routing weights or failover guarantees are inferred from them.

## `DR-40-E1`: router/load matrix

1. Load the exact same model/hash on both nodes; verify tokenizer, sampler ABI, API, context, and cache compatibility declarations.
2. Warm controlled session prefixes on node A, node B, both, or neither.
3. Sweep prompt/output buckets and admitted concurrency with balanced and deliberately skewed load.
4. Compare round-robin, least-request, token-work, and cache-aware predicted-finish policies.
5. Record per-request route reason, queue/running tokens, prefix matched/recomputed, TTFT/ITL/E2E p99, throughput, evictions, and fairness.

## `DR-40-E2`: failover and cache safety

Inject process kill, node loss, coordinator restart, stale heartbeat, delayed response, corrupted cache file, wrong model hash, and disk-full during checkpoint. Test before dispatch, during prefill, and after output streaming. Verify epoch fencing, no duplicate commits, bounded detection, cache miss/recompute on any invalid state, and explicit client outcome.

## `DR-40-E3`: model diversity/reload

Measure cold and warm load time, storage bandwidth contention, memory release, graph rebuild, cache eviction, and request impact for same-model replication versus different models. Preserve exact model/checksum provenance.

## Promotion gates

- Correctness/failure tests pass before latency claims.
- Cache transfer requires whole identity plus integrity validation.
- Capacity is reported per replica; do not sum context capacity as if one session could use it.
- Failover claims distinguish API availability, request retry, and exact session continuation.
