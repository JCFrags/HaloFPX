# Program Charter

> **Wiki status:** Proposed · **Evidence state:** D0 — design only · **Last reviewed:** 2026-07-17  
> **Machine-validation status:** Not run. Missing measurements are `INSUFFICIENT_EVIDENCE`, never an implicit pass.


## Objective

Move the target from integration to stable dual-node operation with evidence that supports five claims:

1. **Functional:** the intended model, tokenizer, context, API, cache mode, and partition execute correctly.
2. **Performance:** latency, throughput, resource, power, and thermal behavior meet declared SLOs and do not regress beyond policy.
3. **Resilience:** bounded component and link faults are detected, fail explicitly, and recover without silent corruption.
4. **Reproducibility:** every claim can be reconstructed from immutable raw data and full provenance.
5. **Maintainability:** upstream changes are detected within a defined freshness budget and mapped to targeted canaries.

## In scope

- Two matched or documented Strix Halo-class nodes.
- Dedicated USB4/Thunderbolt host-to-host transport, normally USB4NET/IP.
- A coordinator and remote worker; default adapter is `llama.cpp` RPC.
- Single-request, concurrent serving, long-context, and soak workloads.
- Cold start, page-cache warm, model-resident warm, and exact-prefix cache-hit states.
- Linux OS, kernel, firmware, ROCm/HIP and/or Vulkan stack, engine, model, tokenizer, storage, and link.

## Out of scope unless explicitly added

- Training correctness.
- More than two inference nodes.
- Internet-facing RPC exposure.
- Security certification or multi-tenant isolation.
- Claims about models, contexts, operating systems, or transports not present in the SUT manifest.

## Non-negotiable rules

- No dual-node performance claim without Node A and Node B baselines.
- No speedup claim for a workload that cannot run on one node.
- No release pass with missing SLOs, missing raw data, or incomplete provenance.
- No output-quality waiver for silent corruption, malformed UTF-8, schema violations, or wrong model/tokenizer identity.
- No stable release while the normal operating path produces kernel oopses, GPU resets, thermal throttling, unbounded hangs, or unexplained link renegotiation.

## Roles

| Role | Accountability |
|---|---|
| Benchmark owner | Matrix, workload integrity, run order, and raw evidence |
| Platform owner | BIOS, firmware, kernel, power, thermal, storage, and USB4 |
| Runtime owner | Engine build, partitioning, cache semantics, and server instrumentation |
| Correctness owner | Canaries, quality suite, drift adjudication, and output review |
| Release authority | Gate decision, waivers, claim language, and rollback |
| Upstream steward | Watch freshness, triage, canary triggers, and source provenance |
