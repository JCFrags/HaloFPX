---
section_id: "73"
title: "Benchmark Design Implications for HaloFPX"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: ["HaloFPX integration repository (not yet frozen)"]
  software_versions: ["HaloFPX benchmark record schema 1.0.0", "jsonschema 4.26.0"]
  hardware_revisions: ["dual-Strix-Halo target"]
related_sections: ["38", "40", "41", "42", "43", "44", "52", "55", "57", "58", "65", "74", "75", "76", "77", "79", "81"]
---

# Benchmark design implications for HaloFPX

## One harness, multiple scopes

**[RECOMMENDATION]** HaloFPX should not force every measurement into one scalar score. Preserve four scopes:

1. engine microbenchmarks for prompt/decode kernels;
2. client-observed request benchmarks for user experience;
3. distributed-operation benchmarks for collectives and transport;
4. cache/storage benchmarks for lookup, restore, writeback, and endurance.

**[INFERENCE]** This separation follows the verified tool mismatch: `llama-bench` deliberately excludes tokenization/sampling while serving tools include wider paths. A combined dashboard may link scopes, but must not compare them as equivalent. [S73-01][S73-02][S73-03]

## Required clocks and event boundaries

**[RECOMMENDATION]** Each node should record a monotonic raw timestamp for local durations and an RFC 3339 UTC timestamp for correlation. Cross-node collective/event analysis requires a clock-sync receipt (offset, uncertainty, source, before/after samples). Never subtract unsynchronized wall clocks to calculate latency. [S73-08]

**[RECOMMENDATION]** Define request boundaries at the load generator and server separately. This permits decomposition into client transport/queueing/compute/streaming without silently changing TTFT.

## Distributed mode comparisons

**[RECOMMENDATION]** Replication, remote speculation, tensor parallelism, pipeline parallelism, and MoE-aware modes must use the same prompt trace and output-token policy. Each run must state:

- coordinator and rank ownership;
- rank-to-device and layer/expert mapping;
- link-to-flow mapping and fallback state;
- collective/message sizes and synchronization boundaries;
- whether both nodes' power is included;
- single-node fallback behavior.

**[INFERENCE]** System generation throughput can improve while individual-request TTFT or p99 worsens. Mode selection therefore needs a vector of goodput, TTFT, ITL/TPOT, request latency, failures, energy, and quality/correctness, not peak tokens/s alone.

## Cache experiments

**[RECOMMENDATION]** Cache experiments must classify every eligible lookup as `exact_hit`, `partial_hit`, `miss_absent`, `miss_incompatible`, `miss_corrupt`, or `error`. A corrupted/incompatible record is never a hit. Record restored tokens/bytes and recomputed tokens so request hit rate and token hit rate cannot be conflated.

**[RECOMMENDATION]** Distinguish:

- warm process and DRAM-resident state;
- Linux page-cache warm storage;
- NVMe-resident but page-cache cold storage;
- no reusable cache record.

**[INFERENCE]** Without this tier proof, a claimed SSD restore benefit may actually be a DRAM or page-cache hit. This dependency is resolved experimentally by sections 65 and 77.

## Speculative decoding

**[VERIFIED]** Speculative decoding verifies candidate tokens against the target distribution and can accept multiple draft tokens per target evaluation. Acceptance behavior depends on the target/draft pair and workload. [S73-10]

**[RECOMMENDATION]** Record proposed, accepted, rejected, and target-only token counts per verification step. Publish acceptance rate together with accepted tokens per target step, target evaluations per output token, TTFT, ITL, and network bytes. Acceptance rate alone can conceal expensive draft work or transport.

## Randomization and paired analysis

**[RECOMMENDATION]** Use a paired block design: within each block, run the same prompt/arrival trace for A and B, randomize whether A or B runs first, and reset to the declared cache/thermal state between conditions. Store `block_id`, `pair_id`, and `order_index`.

**[INFERENCE]** Pairing reduces noise from prompt mix and machine drift, while randomized order limits systematic advantage from heating, cooling, page-cache accumulation, or background activity.

## Schema governance

**[RECOMMENDATION]** Version the executable schema independently of the runtime. Additive fields may remain within a compatible minor revision; renamed units, changed denominators, event-boundary changes, or percentile-method changes require a major schema revision. Preserve the schema, validator, derivation code SHA, and input hashes beside each summary. [S73-07][S73-11]

**[RECOMMENDATION]** Derived data must be reproducible from raw records. If privacy prevents preserving prompt text, preserve a prompt-set manifest, content hashes, token IDs where permitted, length bins, and access-controlled source routing; never substitute an undocumented prompt sample.

## Release and decision implications

**[RECOMMENDATION]** Section 81 should reject a performance claim when any of these is missing: exact build/model identity, comparable controls, raw artifact, failures, sample counts, confidence method, or schema validation. A result can be informative yet remain non-gating.

**[OPEN]** Practical thresholds for regression, acceptable variance, thermal steady state, tail sample size, and power accuracy depend on measurements from sections 74-79 and must not be frozen here.
