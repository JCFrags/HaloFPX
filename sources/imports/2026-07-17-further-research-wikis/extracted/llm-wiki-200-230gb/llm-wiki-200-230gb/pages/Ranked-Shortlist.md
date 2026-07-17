# Ranked Shortlist

The rank is a **deployment-fit ordering for this 200–230 GB requirement**, not a universal intelligence ranking. It emphasizes runtime maturity, licensing/provenance, quantization fidelity, memory headroom, active-compute efficiency, and direct quant evidence.

| Rank | Candidate | Score /100 | Representation | GB | Active B | License | Status |
|---|---|---|---|---|---|---|---|
| 1 | [Qwen3-235B-A22B-Instruct-2507](../candidates/qwen3-235b-a22b-2507.md) | 92 | UD-Q6_K_XL | 201.91 | 22 | Apache-2.0 | shortlist |
| 2 | [Step-3.7-Flash](../candidates/step-3.7-flash.md) | 90 | Q8_0 | 209.00 | 11 | Apache-2.0 | shortlist |
| 3 | [MiMo-V2-Flash](../candidates/mimo-v2-flash.md) | 86 | Q5_K_M | 219.20 | 15 | MIT (model weights; verify code components separately) | shortlist |
| 4 | [GLM-4.7](../candidates/glm-4.7.md) | 83 | Q4_K_M | 218.52 | 32 | MIT | shortlist |
| 5 | [Llama-3.1-Nemotron-Ultra-253B-v1](../candidates/nemotron-ultra-253b.md) | 76 | Q6_K | 207.88 | 253 | NVIDIA Open Model License + Llama 3.1 terms | shortlist |
| 6 | [DeepSeek-R1-0528](../candidates/deepseek-r1-0528.md) | 68 | UD-IQ2_M | 229.00 | 37 | MIT | conditional |
| 7 | [Llama-3.1-Tulu-3-405B](../candidates/tulu3-405b.md) | 66 | IQ4_XS | 216.57 | 405 | Llama 3.1 Community License | conditional |
| 8 | [MiniMax-M3](../candidates/minimax-m3.md) | 55 | UD-IQ4_XS | 208.00 | 23 | MiniMax Community License | experimental |
| 9 | [Kimi-K2-Thinking](../candidates/kimi-k2-thinking.md) | 35 | i1-IQ1_M community artifact | 228.00 | 32 | Modified MIT / model-specific terms — verify upstream | screened-out |

## Scoring rubric

| Component | Weight | Interpretation |
|---|---:|---|
| Runtime maturity | 30 | Architecture implemented in the pinned llama.cpp and/or ROCmFPX snapshot; exact model path qualified |
| License clarity | 15 | Clear commercial/research terms and source attribution |
| Quantization fidelity | 20 | Effective bit budget, quant family, and distance from extreme low-bit regimes |
| Memory headroom | 15 | Weight plan plus Q8 KV under plausible configurations |
| Active-compute efficiency | 10 | Sparse activation versus dense all-parameter execution |
| Provenance | 5 | Publisher/revision/path specificity and integrity metadata |
| Direct quant evidence | 5 | Candidate-specific PPL/KLD/benchmark evidence for the selected representation |

## Tiers

### Tier A — deploy first

- **Qwen3 UD-Q6_K_XL:** most balanced and least surprising.
- **Step Q8_0:** best representation fidelity; accept newer-runtime risk.
- **MiMo Q5_K_M:** best KV-cache behavior for long context.
- **GLM Q4_K_M:** strong upstream agent/coding evidence, but more quant and KV pressure.

### Tier B — workload-specific

- **Nemotron Q6_K:** dense, high-fidelity, high compute.
- **DeepSeek UD-IQ2_M:** reasoning-focused, but 2-bit quality must be proven on the intended workload.
- **Tulu 405B IQ4_XS:** mature dense control, but the worst active-compute cost in the qualified set.

### Tier C — research gates

- **MiniMax-M3 UD-IQ4_XS:** capacity fits, but mainline MSA/runtime maturity is not yet a stable baseline.
- **Kimi-K2-Thinking IQ1_M:** screened out; file size fits only because the representation is near 1.82 effective bits/parameter.
