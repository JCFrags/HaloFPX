---
section_id: "06"
title: "Project Charter, Vision, and Intended Outcomes"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["charlie12345/ROCmFPX", "fewtarius/CachyLLama", "fewtarius/llama-ai", "ggml-org/llama.cpp"]
  software_versions: ["ROCmFPX a5605a7", "CachyLLama 6be7459", "llama-ai 1017f3d", "llama.cpp 788e07d"]
  hardware_revisions: ["two matched AMD Strix Halo systems; exact BOM open"]
related_sections: ["07", "08", "09", "10", "18", "38", "49", "68"]
---

# Project charter

**[ASSUMPTION]** HaloFPX is a local-first inference product for two matched AMD Strix Halo machines joined by two host-to-host USB4 links. Its vision is to make the pair more useful than either node alone while preserving a correct, usable single-node mode.

**[RECOMMENDATION]** Interpret “fastest practical” as the best measured configuration for a declared workload, model, quality floor, power envelope, and reliability requirement—not as an unconditional benchmark claim.

## Problem statement

Local agent workloads repeatedly evaluate long, mostly stable prompts, need predictable privacy and availability, and may exceed the capacity or latency envelope of one APU. Existing source projects expose useful AMD quantization, serving, prompt-cache, and telemetry primitives, but do not establish a production-quality two-node architecture for this exact topology ([sources](sources.md)).

## Intended outcome

Deliver an evidence-backed stack that can select among single-node serving, replication, remote speculation, tensor parallelism, pipeline parallelism, and MoE-aware hybrid execution; persist rank-local reusable state safely; and expose an authenticated, observable client API.

The charter is operationalized by [success definitions](design_implications.md), bounded by [Section 08](../08_Scope_Non_Goals_Boundaries_and_External_Dependencies/README.md), and tested by [Section 09](../09_Functional_Requirements_SLOs_and_Acceptance_Criteria/README.md).

## Research split

- Internet/source work completed: current repository heads, declared features, server interfaces, and local benchmark provenance were reviewed.
- Machine work required: exact BOM and software inventory, matched single-node baselines, link characterization, two-node mode tests, quality comparisons, and fault injection.
- Decisions contingent on measurements: default backend, default execution mode, cache policy, supported model envelope, and numeric SLOs.

