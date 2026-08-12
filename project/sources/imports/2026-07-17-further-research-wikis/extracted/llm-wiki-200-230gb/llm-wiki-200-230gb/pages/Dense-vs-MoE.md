# Dense vs MoE

## Resident memory

For inference in llama.cpp-style runtimes, **all model weights normally remain resident** whether the model is dense or sparse. MoE routing reduces the matrix multiplications performed per token; it does not reduce the weight artifact that must be loaded. Expert paging is not assumed in any capacity table.

## Active compute

| Candidate | Class | Resident B | Active B/token | Resident ÷ active |
|---|---|---|---|---|
| Qwen3-235B-A22B-Instruct-2507 | MoE | 235 | 22 | 10.7× |
| Step-3.7-Flash | MoE + hybrid attention | 198 | 11 | 18.0× |
| MiMo-V2-Flash | MoE + hybrid attention | 309 | 15 | 20.6× |
| GLM-4.7 | MoE | 358 | 32 | 11.2× |
| Llama-3.1-Nemotron-Ultra-253B-v1 | Dense NAS-pruned | 253 | 253 | 1.0× |
| DeepSeek-R1-0528 | MoE + MLA | 671 | 37 | 18.1× |
| Llama-3.1-Tulu-3-405B | Dense | 405 | 405 | 1.0× |
| MiniMax-M3 | MoE + MiniMax Sparse Attention | 428 | 23 | 18.6× |
| Kimi-K2-Thinking | MoE | 1000 | 32 | 31.2× |

Dense candidates provide a useful architecture-control baseline and avoid router/expert implementation risk, but their decode cost scales with every layer's full FFN. The 405B dense control has roughly 18× the active parameter count of Qwen3-235B-A22B even though both occupy the same storage band after quantization.

## Expert activation details

- Qwen: 128 routed experts, 8 active.
- Step: 288 routed experts, 8 active plus a shared expert on MoE layers.
- MiMo: model-card/config topology indicates sparse routing with approximately 15B active parameters.
- GLM: 160 routed experts, 8 active plus one shared expert; first three blocks are dense.
- DeepSeek R1: 256 routed experts, 8 active plus shared expert; first three blocks are dense.
- MiniMax M3: 128 experts, 4 active plus shared expert; support remains experimental.

## Operational consequence

Sparse models are the logical first choice when decode throughput or power is constrained, but the only safe conclusion before measurement is **lower active compute**, not a tokens/second prediction. Kernel quality, memory bandwidth, router implementation, quant type, and interconnect still dominate realized performance.
