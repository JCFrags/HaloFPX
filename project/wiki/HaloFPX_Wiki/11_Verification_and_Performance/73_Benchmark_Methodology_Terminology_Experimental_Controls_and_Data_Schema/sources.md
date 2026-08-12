---
section_id: "73"
title: "Benchmark Methodology Sources"
status: "draft"
last_verified: "2026-07-17"
applies_to:
  repositories:
    - "ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689"
    - "vllm-project/vllm@9354f222042986addf20709e5274fc26e0d09745"
    - "mlcommons/inference_policies@c547732b539cb3a14cc5680597714c8c1df4cad0"
  software_versions: ["NVIDIA NIM LLM Benchmarking 1.0.0", "ROCm/rocm-systems@27b4e4dd4438e205c3c9163efe4084b890bbb08e", "JSON Schema Draft 2020-12"]
  hardware_revisions: []
related_sections: ["02", "05", "27", "36", "73", "74", "75", "76", "77", "79", "81"]
---

# Sources

Access date for all Internet sources: `2026-07-16`. No source below contains a HaloFPX measurement.

### S73-01 - llama.cpp `llama-bench`

- Publisher/repository: `ggml-org/llama.cpp`
- URL: <https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/tools/llama-bench/README.md>
- Revision: commit `788e07dc91d266ad3162a1ce9037665656269689`, observed repository head/pin on 2026-07-16
- Supports: `pp`/`tg`/`pg` scopes, repetition option/default, JSON/JSONL samples, parameters, explicit exclusion of tokenization and sampling.
- Limitations/conflicts: microbenchmark timing is not client-visible request latency; repository is fast moving.

### S73-02 - vLLM serving benchmark implementation

- Publisher/repository: `vllm-project/vllm`
- URL: <https://github.com/vllm-project/vllm/blob/9354f222042986addf20709e5274fc26e0d09745/vllm/benchmarks/serve.py>
- Revision: commit `9354f222042986addf20709e5274fc26e0d09745`, commit timestamp `2026-07-17T06:25:29Z`
- Supports: distinct TTFT, TPOT, ITL, E2EL, percentile and throughput fields; client-side E2EL boundary; bundled-token caveat.
- Limitations/conflicts: observed upstream commit is later than the local access date in UTC and is not a HaloFPX dependency pin; definitions can change.

### S73-03 - NVIDIA NIM LLM benchmarking metrics

- Publisher: NVIDIA
- URL: <https://docs.nvidia.com/nim/benchmarking/llm/1.0.0/metrics.html>
- Revision: NIM LLMs Benchmarking documentation `1.0.0`, page observed 2026-07-16
- Supports: TTFT boundary, terminal done/empty handling, ITL/TPOT formula and terminology conflict.
- Limitations/conflicts: NVIDIA tool convention is authoritative only for that tool; it is not a universal standard or AMD validation.

### S73-04 - MLPerf Inference benchmark rules

- Publisher/repository: MLCommons, `mlcommons/inference_policies`
- URL: <https://github.com/mlcommons/inference_policies/blob/c547732b539cb3a14cc5680597714c8c1df4cad0/inference_rules.adoc>
- Revision: commit `c547732b539cb3a14cc5680597714c8c1df4cad0`, `2026-07-07T14:01:12Z`
- Supports: SUT definition, fixed random seeds, latency boundary, tail-percentile/sample planning, fair/matched configuration principles.
- Limitations/conflicts: HaloFPX does not claim MLPerf compliance; MLPerf workload and cache restrictions are not automatically HaloFPX requirements.

### S73-05 - NIST confidence limits for a mean

- Publisher: NIST/SEMATECH Engineering Statistics Handbook
- URL: <https://www.itl.nist.gov/div898/handbook/eda/section3/eda352.htm>
- Revision/date: handbook web edition, page `1.3.5.2`, retrieved 2026-07-16
- Supports: interpretation of confidence intervals; Student-t mean interval when standard deviation is estimated.
- Limitations/conflicts: does not make arbitrary latency tails normal or independent; clustering/run structure still requires experiment-specific judgment.

### S73-06 - Nonparametric bootstrap

- Publisher: Bradley Efron, *The Annals of Statistics*
- URL: <https://doi.org/10.1214/aos/1176344552>
- Revision/date: "Bootstrap Methods: Another Look at the Jackknife", volume 7 issue 1, 1979
- Supports: resampling framework behind proposed bootstrap intervals.
- Limitations/conflicts: the paper does not select HaloFPX's resampling unit, algorithm, repetitions, or coverage target.

### S73-07 - JSON Schema Draft 2020-12

- Publisher: JSON Schema project / specification authors
- URL: <https://json-schema.org/draft/2020-12>
- Revision/date: Draft 2020-12, published `2022-06-16`
- Supports: versioned machine-validatable JSON schema vocabulary.
- Limitations/conflicts: it validates structure, not benchmark semantics, provenance truth, or statistical correctness.

### S73-08 - RFC 3339 timestamps

- Publisher: IETF / RFC Editor
- URL: <https://www.rfc-editor.org/info/rfc3339>
- Revision/date: RFC 3339, July 2002; updated in limited respects by RFC 9557 (April 2024)
- Supports: interoperable Internet date-time representation and UTC offsets.
- Limitations/conflicts: wall time is not a monotonic duration clock and synchronization accuracy is out of scope.

### S73-09 - AMD SMI CLI documentation source

- Publisher/repository: AMD, `ROCm/rocm-systems`
- URL: <https://github.com/ROCm/rocm-systems/blob/27b4e4dd4438e205c3c9163efe4084b890bbb08e/projects/amdsmi/docs/how-to/amdsmi-cli-tool.md>
- Revision/date: commit `27b4e4dd4438e205c3c9163efe4084b890bbb08e`, commit timestamp `2026-07-17T02:03:47Z`
- Supports: JSON/CSV output and monitoring of power, temperature, graphics/memory utilization and clocks.
- Limitations/conflicts: observed upstream commit is later than the local access date in UTC; exact fields, units and device support must be verified on the installed version; package telemetry is not calibrated wall power.

### S73-10 - Speculative decoding

- Publisher: Yaniv Leviathan, Matan Kalman, Yossi Matias; ICML/PMLR
- URL: <https://proceedings.mlr.press/v202/leviathan23a.html>
- Revision/date: "Fast Inference from Transformers via Speculative Decoding", PMLR 202, published 2023
- Supports: target verification of draft candidates and acceptance of multiple tokens per target-model evaluation while preserving the target distribution.
- Limitations/conflicts: speedups and acceptance behavior are workload/model/hardware specific; none are imported as HaloFPX results.

### S73-11 - HaloFPX benchmark record schema validation

- Local artifacts: [schema](data/benchmark_record.schema.json), [validator](scripts/validate_benchmark_records.py), [valid fixture](data/fixtures/valid-request.json), [invalid fixture](data/fixtures/invalid-request.json), and [validation evidence](evidence/schema-validation-result.json).
- Revision: schema `1.0.0`; Python 3.14.4; `jsonschema` 4.26.0; executed 2026-07-17.
- Integrity: schema SHA-256 `e3981e18bea145881d68560e5a14f5d3ce5167f1568f672eeaaabb422537640b`; validator SHA-256 `6c7310449e8013bb3b96c4779b9620c942645dc04797aa8bc671e3c0d8e1a145`.
- Supports: Draft 2020-12 structural coverage for seven record families, deterministic semantic checks, valid-fixture acceptance, invalid-fixture rejection, and preserved command/result evidence.
- Limitations/conflicts: fixture validation does not establish target-harness integration, metric correctness, sample sufficiency, approved confidence methods, performance thresholds, or release eligibility.

## Local authority used for governance

The project rules in `../../../../AGENTS.md`, the routing pointer in `../../../../references/agent-harness.md`, and canonical `C:\Users\britt\Documents\Agent_Harness\AGENTS.md` plus `guide/architecture.md` governed evidence promotion and closeout. They are not technical benchmark sources and therefore do not receive S73 claim IDs.
