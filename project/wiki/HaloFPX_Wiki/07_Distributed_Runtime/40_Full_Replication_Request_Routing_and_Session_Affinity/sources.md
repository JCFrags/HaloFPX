---
section_id: "40"
title: "Replication Sources"
status: "draft"
last_verified: "2026-07-16"
applies_to:
  repositories: ["vLLM", "llama.cpp", "CachyLlama"]
  software_versions: []
  hardware_revisions: []
related_sections: ["38", "39", "46"]
---

# Sources

| ID | Source/revision | Supports | Limitation |
|---|---|---|---|
| S40-01 | [vLLM data-parallel deployment](https://github.com/vllm-project/vllm/blob/9354f222042986addf20709e5274fc26e0d09745/docs/serving/data_parallel_deployment.md), commit `9354f22`, accessed 2026-07-16 | independent KV, routing telemetry and deployment modes | CUDA-oriented; current LB not cache-aware |
| S40-02 | [llama.cpp server README](https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/tools/server/README.md) and [developer guide](https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/tools/server/README-dev.md), commit `788e07d`, accessed 2026-07-16 | slots, batching, cache reuse/save/restore | not a two-node router; nondeterminism warning |
| S40-03 | [CachyLlama](https://github.com/fewtarius/CachyLlama/tree/6be745998f568e379ea197fcf827baec73ff9940), especially `common/kv-ssd-cache.*` and `tools/server/server-context-ssd-cache.cpp`, commit `6be7459`, 2026-07-08; accessed 2026-07-16 | SSD cache code presence | portability/correctness/performance unverified |
| S40-04 | [Orca](https://www.usenix.org/conference/osdi22/presentation/yu), OSDI 2022 | iterative request/scheduling behavior | different runtime/hardware |
| S40-05 | Local Agent Harness `guide/architecture.md`, read 2026-07-16 | explicit authority and evidence routing | governance only |
