---
section_id: "73"
title: "Benchmark Facts, Definitions, and Constraints"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories:
    - "ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689"
    - "vllm-project/vllm@9354f222042986addf20709e5274fc26e0d09745"
  software_versions: ["NVIDIA NIM LLM Benchmarking 1.0.0", "JSON Schema Draft 2020-12", "ROCm/rocm-systems@27b4e4dd4438e205c3c9163efe4084b890bbb08e"]
  hardware_revisions: ["HaloFPX dual-Strix-Halo target; exact revisions unresolved"]
related_sections: ["05", "18", "22", "27", "38", "55", "57", "65", "74", "75", "76", "77", "79"]
---

# Benchmark facts, definitions, and constraints

## Canonical HaloFPX terminology

**[RECOMMENDATION]** These definitions are normative for HaloFPX reports. Use nanoseconds in raw duration fields, seconds or milliseconds only in labeled derived fields, and tokens as produced by the recorded tokenizer identity.

| Metric | HaloFPX definition | Population and unit | Required qualifiers |
|---|---|---|---|
| time to first token (TTFT) | `first_output_observed_ns - request_start_ns` | per successful streaming request; ns | client or server observation point; whether tokenization, queueing, transport, detokenization are included |
| prompt throughput | `sum(input_tokens_processed) / measured_prompt_phase_s` | system or rank; token/s | cold/warm cache; batch/concurrency; whether tokenization is excluded |
| generation throughput | `sum(output_tokens) / steady_state_wall_s` | system-wide good output token/s | include/exclude first token explicitly; successful-only and offered-load denominators both reported |
| inter-token latency (ITL) | differences between consecutive token-observation timestamps | per token interval; ns | never infer from chunks unless each chunk carries token timestamps |
| time per output token (TPOT) | `(request_latency - TTFT)/(output_tokens-1)` for `output_tokens > 1` | per request; ns/token | derived proxy; not relabeled as observed ITL |
| request latency / E2EL | `final_response_observed_ns - request_start_ns` | per completed request; ns | client/server boundary, streaming completion rule, timeout policy |
| speculative acceptance rate | `accepted_draft_tokens / proposed_draft_tokens` | aggregate and per step; ratio | exclude target-only tokens; also report accepted tokens per target evaluation step |
| exact cache request hit rate | requests whose requested reusable state is wholly satisfied / eligible lookups | request ratio | eligibility and exact/fuzzy/partial class |
| cache token hit rate | reusable prompt tokens restored / eligible reusable prompt tokens requested | token ratio | rank, tier, source (DRAM/page cache/NVMe), validated/rejected state |
| collective latency | completion minus issue time for a named collective | operation; ns | collective, payload bytes, dtype, ranks, link mapping, synchronization boundary |
| utilization | busy interval divided by observation interval | per device/resource; ratio | sensor definition, sample period, aggregation; do not equate GPU-busy with arithmetic occupancy |
| power | sensor-reported instantaneous/averaged power and integrated energy | W and J | measurement boundary, sensor/source, cadence, calibration, missing samples |
| tail percentile | empirical quantile of a named population, normally p90/p95/p99 | same unit as population | quantile algorithm, sample count, warmup/failure policy, confidence interval |

**[VERIFIED]** Pinned `llama-bench` offers prompt-processing (`pp`), text-generation (`tg`), and combined (`pg`) tests, defaults to five repetitions, emits individual nanosecond samples in JSON/JSONL, and explicitly excludes tokenization and sampling. It is a kernel/runtime microbenchmark input, not a substitute for client-visible serving latency. [S73-01]

**[VERIFIED]** vLLM's pinned serving benchmark distinguishes TTFT, TPOT, ITL, and E2EL. Its source defines E2EL at the client from sending the request through the complete response and notes that multiple output tokens can be bundled by some backends. [S73-02]

**[VERIFIED]** NVIDIA NIM LLM Benchmarking 1.0.0 defines TTFT including tokenization and detokenization and defines its average ITL/TPOT as `(E2E - TTFT)/(output tokens - 1)`, excluding the terminal done/empty signal. This differs from the raw event-interval definition above; reports must identify the formula rather than relying on the label. [S73-03]

## Population, percentiles, and uncertainty

**[VERIFIED]** MLPerf Inference treats the SUT as the complete measured hardware and software set, fixes random seeds, defines latency from scheduled query issue to response, and uses explicit tail-latency/sample-count rules. Its rules demonstrate why a tail percentile without a population and sufficient observations is weak evidence. HaloFPX is not claiming MLPerf compliance. [S73-04]

**[VERIFIED]** A confidence interval describes a procedure whose intervals contain the population parameter at the stated long-run rate; it is not the probability that a realized interval contains the parameter. NIST's mean interval uses the Student-t distribution when variance is estimated from a finite sample. [S73-05]

**[RECOMMENDATION]** Treat independent run as the resampling unit for run-level comparisons. Requests within a continuous-batching run share scheduler, thermal, cache, and load state and are not automatically independent replicates. Report:

- per-run estimates;
- across-run median and 95% confidence interval;
- raw request-level distributions;
- exact `n_runs`, `n_requests`, `n_token_intervals`, and failure counts.

**[RECOMMENDATION]** Use a two-sided 95% Student-t interval for an approximately symmetric across-run mean only when justified. Otherwise use a predeclared percentile bootstrap over independent runs and preserve the seed, resample count, and algorithm. Never compute a narrow request-level CI and present it as run-to-run reproducibility. [S73-06]

## Controls and matching constraints

| Control family | Must be identical for an A/B claim unless it is the independent variable |
|---|---|
| source/build | repository SHAs, dirty-tree patch hash, build type, compiler/linker, ROCm/Mesa/kernel/firmware, flags |
| model | byte hash for every shard/file, GGUF metadata, quantization recipe, tokenizer hash, chat-template hash, draft/MTP model hashes |
| runtime | backend, device placement, world size/rank map, tensor/pipeline split, batch/ubatch/context, KV types, flash attention, mmap/direct I/O |
| workload | prompt-set manifest hash, exact token IDs or canonical text, prompt/output length policy, arrival trace, concurrency, seed |
| sampling | temperature, top-k/top-p/min-p, penalties, grammar, stop conditions, seed, EOS handling |
| cache | cold/warm class, eligibility, namespace, compatibility fingerprint, DRAM/page-cache/NVMe state, preload procedure |
| host/environment | BIOS/firmware, clocks/governor, power profile, CPU/IRQ/cgroup/NUMA placement, free memory, ambient/inlet temperature |
| fabric | cable/port identity, routes, MTU, queue and transport settings, dual-link policy, peer state and error counters |

**[INFERENCE]** Since HaloFPX changes distribution mode, rank-local cache state, and dual-link transport, a run identity that omits any of those dimensions can collapse materially different systems into one comparison.

## Raw-data record families

**[VERIFIED]** Schema 1.0.0 implements these append-only record types with required common identity/timing fields and type-specific structural fields [S73-11]:

| `record_type` | Cardinality | Minimum purpose |
|---|---|---|
| `run_manifest` | one/run | immutable identities, control settings, declared hypotheses and metrics |
| `request` | one/request | timestamps, token counts, status/failure, cache/speculation summary |
| `token_event` | zero or more/output token | client observation timestamp, token index/id, chunk identity |
| `collective_event` | one/collective | rank, operation, bytes, issue/complete timestamps, link/transport |
| `cache_event` | one/lookup/restore/write | eligibility, hit class, tier, tokens/bytes, validation and error outcome |
| `telemetry_sample` | periodic/resource | monotonic and UTC timestamps, power/temp/utilization/clocks/errors |
| `run_summary` | derived, one/run | statistic definitions, counts, estimates, CIs, derivation version |

**[VERIFIED]** JSON Schema Draft 2020-12 supplies a versioned machine-validation vocabulary. RFC 3339 supplies interoperable wall-clock timestamps. Raw latency computation must use a monotonic clock; UTC timestamps are for cross-host correlation and provenance. [S73-07][S73-08]

**[VERIFIED]** The validator adds deterministic cross-field checks for timestamp order, restored tokens not exceeding eligible tokens, accepted draft tokens not exceeding proposed tokens, collective issue/completion order, and confidence-interval bound order. The valid fixture produced zero errors; the invalid fixture produced the expected invalid result [S73-11].

**[RECOMMENDATION]** Do not infer statistical adequacy from schema validity. The schema records methods, counts, estimates, and intervals but does not select or approve their values.

## Power and utilization constraints

**[VERIFIED]** AMD SMI documentation exposes JSON/CSV telemetry including power, temperature, graphics utilization, clocks, memory utilization, and VRAM. These readings are sensor/tool observations; they do not establish wall-plug power or computation occupancy. [S73-09]

**[RECOMMENDATION]** Report two power scopes separately: device/package telemetry and synchronized wall power. Integrate energy over the same measured window as useful work; report joules/request and joules/output-token only when the denominator includes a stated failure policy.

## Measurements absent

**[OPEN]** No sensor accuracy, clock skew, steady-state duration, warmup convergence, request distribution, cache hit ratio, collective latency, utilization, power, or percentile was measured for HaloFPX in this section.
