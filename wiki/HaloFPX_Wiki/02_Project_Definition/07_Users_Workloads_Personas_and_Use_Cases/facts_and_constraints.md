---
section_id: "07"
title: "Persona and Workload Matrix"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["ggml-org/llama.cpp", "fewtarius/CachyLLama", "fewtarius/llama-ai"]
  software_versions: ["commits in sources.md"]
  hardware_revisions: ["two-node Strix Halo target"]
related_sections: ["09", "32", "34", "36", "46", "69"]
---

# Persona and workload matrix

## Verified capabilities that shape use cases

- **[VERIFIED]** `llama-server` documents streaming chat/completions, Responses, embeddings, parallel decoding, continuous batching, function calling, speculative decoding, timings, metrics, and slot save/restore [S07-01].
- **[VERIFIED]** CachyLLama documents request-level user IDs, cache namespaces, slot affinity, concurrency caps, SSD-backed prefix reuse, and expert telemetry [S07-02].
- **[VERIFIED]** llama-ai describes offline AMD APU agent work in which stable system/tool content is repeatedly sent, but its stated prompt sizes and performance are workload-specific [S07-03].

## Candidate personas

| Persona | Primary work | Sensitivity | Durability/privacy | Status |
|---|---|---|---|---|
| Local operator | Install, upgrade, choose model/mode, diagnose | Startup and recovery | Full administrative access; local secrets | **[RECOMMENDATION]** mandatory |
| Interactive engineer | Chat, code, FIM, structured output | TTFT and inter-token latency | Conversation privacy; moderate durability | **[ASSUMPTION]** primary |
| Agent orchestrator | Many tool turns with stable prefix | Warm TTFT, correctness, cache hit rate | Durable state; strict tool-call fidelity | **[ASSUMPTION]** primary |
| Researcher | Model conversion, quality tests, benchmarks | Throughput/reproducibility | Immutable artifacts and provenance | **[RECOMMENDATION]** mandatory |
| LAN client | Shared service requests | Fairness, queueing, availability | Authentication and tenant isolation | **[OPEN]** deployment scope |

## Workload envelope matrix

| ID | Workload | Prompt/turn shape | Concurrency | Priority | Unresolved inputs |
|---|---|---|---|---|---|
| W-INT | Interactive chat | Short-to-growing history; streamed output | Low | TTFT, ITL | token percentiles; model mix |
| W-CODE | Coding/FIM | Repository context plus concise output | Low/medium | TTFT, correctness | language corpus; tool schema |
| W-AGENT | Tool-calling loop | Large stable system/tools prefix; many turns | Bursty | warm TTFT, cache durability | trace length; failure retries |
| W-LONG | Long context/RAG | Large retrieved context, prefix overlap varies | Low | capacity, prompt throughput | target context and retrieval sizes |
| W-BATCH | Evaluation/jobs | Many independent prompts | Configurable | aggregate throughput | queue size and completion window |
| W-MULTI | Shared service | Mixed workloads and users | Medium+ | fairness, p95/p99 | user count and isolation policy |
| W-MODEL | Experimentation | load/quantize/swap/benchmark | Serialized | reproducibility, startup | supported formats and storage budget |
| W-OFFLINE | Network-isolated use | Any above without cloud dependency | Any | availability/privacy | update and model-ingest process |

**[OPEN]** No observed prompt-length, turn-count, concurrency, or model-mix distribution is yet available. Values in upstream READMEs are not adopted as requirements.

