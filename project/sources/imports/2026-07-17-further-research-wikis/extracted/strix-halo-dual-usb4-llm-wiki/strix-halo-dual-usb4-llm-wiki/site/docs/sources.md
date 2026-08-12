---
title: Primary sources
status: source ledger
---

# Primary sources

Accessed for this repository on **2026-07-17**. Sources are grouped by the claim they support. Calculations and decision rules are original synthesis in this repository.

## Platform and interconnect

1. **AMD — Ryzen AI Max+ 395 product specification.** Relevant fields: former codename Strix Halo, maximum memory, memory interface/speed, integrated graphics configuration, and two native USB4 40 Gb/s ports.  
   <https://www.amd.com/en/products/processors/laptop/ryzen/ai-300-series/amd-ryzen-ai-max-plus-395.html>

2. **AMD — Trillion-Parameter LLM on an AMD Ryzen AI Max+ Cluster.** A published four-system llama.cpp RPC implementation example; used only as evidence that a distributed RPC avenue exists on Ryzen AI Max+ systems.  
   <https://www.amd.com/en/developer/resources/technical-articles/2026/how-to-run-a-one-trillion-parameter-llm-locally-an-amd.html>

3. **Linux kernel documentation — USB4 and Thunderbolt.** Relevant sections: host-to-host `thunderbolt-net`, one virtual Ethernet interface per connected port, and `thunderbolt-stream`.  
   <https://docs.kernel.org/admin-guide/thunderbolt.html>

4. **Microsoft Learn — USB4 interdomain connections.** Relevant field: USB4NET network adapter behavior between two PCs.  
   <https://learn.microsoft.com/en-us/windows-hardware/design/component-guidelines/usb4-interdomain-connections>

5. **Microsoft Learn — USB4 design details and general requirements.** Used for USB4 architecture/bandwidth-sharing context; no effective payload value is inferred.  
   <https://learn.microsoft.com/en-us/windows-hardware/design/component-guidelines/usb4-design-details-and-general-requirements>

## Parallel execution methods

6. **Shoeybi et al. — “Megatron-LM: Training Multi-Billion Parameter Language Models Using Model Parallelism.”** Relevant result: the described tensor-parallel transformer layer uses two all-reduces in the forward path. The wiki adapts the communication graph to inference; it does not transfer the paper's measured performance.  
   <https://arxiv.org/abs/1909.08053>

7. **Huang et al. — “GPipe: Efficient Training of Giant Neural Networks using Pipeline Parallelism.”** Background for microbatch pipeline scheduling and bubbles. The wiki derives an inference-specific two-stage service model.  
   <https://arxiv.org/abs/1811.06965>

8. **Rajbhandari et al. — “DeepSpeed-MoE: Advancing Mixture-of-Experts Inference and Training to Power Next-Generation AI Scale.”** Background for expert parallelism and token dispatch/combine communication.  
   <https://proceedings.mlr.press/v162/rajbhandari22a.html>

9. **Leviathan, Kalman, and Matias — “Fast Inference from Transformers via Speculative Decoding.”** Primary exact speculative-decoding method and distribution-preservation basis.  
   <https://proceedings.mlr.press/v202/leviathan23a.html>

10. **“Delay-Adaptive Speculation Control for Low-Latency Edge-Cloud LLM Inference.”** Recent primary research on latency-aware remote speculative control; cited as related work, not as a performance proxy.  
    <https://arxiv.org/abs/2606.20591>

11. **“PicoSpec: A Pipelined Collaborative Speculative Decoding Framework for Efficient Edge-Cloud LLM Inference.”** Recent primary research on pipelined collaborative speculation; cited as related work, not as evidence for the target systems.  
    <https://arxiv.org/abs/2603.19133>

## Model architecture records

12. **Meta Llama model repository — `models/sku_list.py`.** Official architecture fields used for Llama 3.1 8B and 405B examples.  
    <https://github.com/meta-llama/llama-models/blob/main/models/sku_list.py>

13. **Mistral AI — Mixtral-8x7B-Instruct-v0.1 `config.json`.** Official hidden size, layers, attention/KV heads, expert count, top-k, vocabulary, and context fields.  
    <https://huggingface.co/mistralai/Mixtral-8x7B-Instruct-v0.1/blob/main/config.json>

14. **Jiang et al. — “Mixtral of Experts.”** Primary paper used for rounded total and active parameter counts and architecture context.  
    <https://arxiv.org/abs/2401.04088>

15. **Qwen — Qwen3-30B-A3B-Instruct-2507 `config.json`.** Official hidden size, layers, attention/KV heads, explicit head dimension, expert count, top-k, vocabulary, and context fields.  
    <https://huggingface.co/Qwen/Qwen3-30B-A3B-Instruct-2507/blob/main/config.json>

## Source-use policy

- A **SOURCED FACT** must be traceable to a source above or an equally authoritative replacement.
- A **CALCULATED** value must name its formula and inputs.
- A **SCENARIO ASSUMPTION** must not be presented as observed behavior.
- A **MEASURED INPUT REQUIRED** remains blank until produced by the target systems.
- A source's benchmark result is not imported into the Strix Halo/USB4 decision model unless the hardware, topology, runtime, and workload match and the transfer is explicitly justified.
