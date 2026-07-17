---
title: Assumptions and evidence ledger
status: auditable ledger
---

# Assumptions and evidence ledger

## Platform facts

| Item | Value | Label | Qualification |
|---|---:|---|---|
| Native USB4 ports | 2 × 40 Gb/s | SOURCED FACT | AMD SoC specification; OEM exposure and controller topology still require inspection. |
| Maximum system memory | 128 GB | SOURCED FACT | Not all memory is available to model weights/KV/workspace. |
| Memory interface | 256-bit LPDDR5x, up to LPDDR5x-8000 | SOURCED FACT | Does not determine model runtime performance by itself. |
| Linux host-to-host interface | One virtual Ethernet interface per port via `thunderbolt-net` | SOURCED FACT | Kernel/runtime version and system firmware remain configuration variables. |
| Linux direct stream path | `thunderbolt-stream` / USB4STREAM | SOURCED FACT | Requires an application/runtime integration; it is not automatically used by inference engines. |
| Windows host-to-host path | USB4NET network adapter | SOURCED FACT | Behavior of two simultaneous links must be measured. |
| Dual-port nominal arithmetic ceiling | 80 Gb/s = 10 GB/s | CALCULATED LOWER BOUND INPUT | Before protocol, software, copies, contention, and latency; not a throughput claim. |

Primary links: [AMD Ryzen AI Max+ 395](https://www.amd.com/en/products/processors/laptop/ryzen/ai-300-series/amd-ryzen-ai-max-plus-395.html), [Linux USB4/Thunderbolt](https://docs.kernel.org/admin-guide/thunderbolt.html), [Windows USB4 interdomain connections](https://learn.microsoft.com/en-us/windows-hardware/design/component-guidelines/usb4-interdomain-connections).

## Modeling assumptions

| Symbol | Repository default | Label | Why it is not hidden |
|---|---:|---|---|
| \(b_a\) | 2 bytes | SCENARIO ASSUMPTION | BF16/FP16 transmitted activation example. Change for FP32 or activation compression. |
| \(b_{kv}\) | 2 bytes | SCENARIO ASSUMPTION | BF16/FP16 KV example. Change for quantized KV. |
| \(b_t\) | 4 bytes | SCENARIO ASSUMPTION | Fixed-width token-ID transport example. |
| \(b_p\) | 2 bytes | SCENARIO ASSUMPTION | Probability/logit transport example. |
| Prompt example \(N\) | 4096 tokens | SCENARIO ASSUMPTION | Used only to instantiate byte formulas. |
| Decode example \(Q\) | 1 sequence | SCENARIO ASSUMPTION | Used only to show per-step volumes. |
| MoE remote fraction \(\rho\) | 0.5 in sensitivity rows | SCENARIO ASSUMPTION | Never presented as routing behavior. |
| USB4 payload floors | 5 GB/s per link; 10 GB/s dual | CALCULATED LOWER BOUND | Decimal conversion of nominal 40/80 Gb/s, ignoring every overhead and latency term. |

## Unset measured inputs

The repository intentionally provides no default values for:

\[
B_1,B_2,\ell_1,\ell_2,m_{AR},\eta_{TP},M_{usable},T_{stage},T_{draft},T_{verify},a,\rho_l.
\]

Their measurement procedures are in the [benchmark plan](benchmarking/benchmark-plan.md). Their blank structured records are in `data/measurement_template.csv` and `data/assumptions.csv`.

## Model architecture evidence

- **Llama 3.1 8B and 405B.** Official Meta SKU definitions supply hidden width, layer count, attention heads, and KV heads. [Meta `sku_list.py`](https://github.com/meta-llama/llama-models/blob/main/models/sku_list.py).
- **Mixtral 8x7B.** Official Mistral configuration supplies dimensions, 8 experts, and top-2 routing; the primary paper reports approximately 47B total and 13B active parameters. [Official config](https://huggingface.co/mistralai/Mixtral-8x7B-Instruct-v0.1/blob/main/config.json), [Mixtral paper](https://arxiv.org/abs/2401.04088).
- **Qwen3-30B-A3B-Instruct-2507.** Official Qwen configuration supplies 48 layers, hidden size 2048, 4 KV heads, explicit head dimension 128, 128 experts, and top-8 routing. [Official config](https://huggingface.co/Qwen/Qwen3-30B-A3B-Instruct-2507/blob/main/config.json).

## Runtime evidence boundary

AMD has published a four-node Ryzen AI Max+ example using llama.cpp RPC over 5 Gb/s Ethernet, establishing that distributed llama.cpp RPC is an implementation avenue on the platform. It does not establish dual-USB4 performance, two-node optimality, or support for every placement in this wiki. [AMD technical article](https://www.amd.com/en/developer/resources/technical-articles/2026/how-to-run-a-one-trillion-parameter-llm-locally-an-amd.html).
