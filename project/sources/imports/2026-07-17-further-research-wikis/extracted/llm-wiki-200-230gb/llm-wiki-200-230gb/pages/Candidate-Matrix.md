# Candidate Matrix

| Candidate | Class | Total B | Active B | Selected | GB | Effective bits/param | Native ctx | llama.cpp | ROCmFPX | License |
|---|---|---|---|---|---|---|---|---|---|---|
| Qwen3-235B-A22B-Instruct-2507 | MoE | 235 | 22 | UD-Q6_K_XL | 201.91 | 6.87 | 262,144 | Yes | Yes | Apache-2.0 |
| Step-3.7-Flash | MoE + hybrid attention | 198 | 11 | Q8_0 | 209.00 | 8.44 | 262,144 | Yes | Yes | Apache-2.0 |
| MiMo-V2-Flash | MoE + hybrid attention | 309 | 15 | Q5_K_M | 219.20 | 5.68 | 262,144 | Yes | Yes | MIT (model weights; verify code components separately) |
| GLM-4.7 | MoE | 358 | 32 | Q4_K_M | 218.52 | 4.88 | 202,752 | Yes | Yes | MIT |
| Llama-3.1-Nemotron-Ultra-253B-v1 | Dense NAS-pruned | 253 | 253 | Q6_K | 207.88 | 6.57 | 131,072 | Yes | Yes | NVIDIA Open Model License + Llama 3.1 terms |
| DeepSeek-R1-0528 | MoE + MLA | 671 | 37 | UD-IQ2_M | 229.00 | 2.73 | 163,840 | Yes | Yes | MIT |
| Llama-3.1-Tulu-3-405B | Dense | 405 | 405 | IQ4_XS | 216.57 | 4.28 | 131,072 | Yes | Yes | Llama 3.1 Community License |
| MiniMax-M3 | MoE + MiniMax Sparse Attention | 428 | 23 | UD-IQ4_XS | 208.00 | 3.89 | 1,048,576 | Not production-ready at snapshot: quant card points to a PR and says MSA falls back to dense attention. | Yes | MiniMax Community License |
| Kimi-K2-Thinking | MoE | 1000 | 32 | i1-IQ1_M community artifact | 228.00 | 1.82 | 262,144 | Architecture family is supported, but the exact community conversion is not qualified here. | Architecture-family support expected; exact artifact not qualified. | Modified MIT / model-specific terms — verify upstream |

Effective bits/parameter are artifact-level storage ratios, not nominal quant labels; metadata and protected/mixed tensors are included.
