# 200–230 GB Large-Model Selection and Capacity Wiki

> **Research snapshot:** 2026-07-17  
> **llama.cpp:** `6bdd77f13cf11b264b4231d320afc404f48d576e`  
> **ROCmFPX:** `61f2f2d7bc4955e9bca821095ef69125837133b5`

This wiki selects runnable large-model representations whose **chosen artifact** is approximately **200–230 decimal GB**, then budgets the rest of the runtime explicitly: KV cache, graph/work buffers, operating-system memory, split skew, multimodal sidecars, and validation margin.

## Decision summary

| Rank | Candidate | Selected representation | Artifact GB | Plan GiB | Type | Active B | Disposition |
|---|---|---|---|---|---|---|---|
| 1 | [Qwen3-235B-A22B-Instruct-2507](candidates/qwen3-235b-a22b-2507.md) | UD-Q6_K_XL | 201.91 | 190 | MoE | 22.0 | shortlist |
| 2 | [Step-3.7-Flash](candidates/step-3.7-flash.md) | Q8_0 | 209.0 | 196 | MoE + hybrid attention | 11.0 | shortlist |
| 3 | [MiMo-V2-Flash](candidates/mimo-v2-flash.md) | Q5_K_M | 219.2 | 206 | MoE + hybrid attention | 15.0 | shortlist |
| 4 | [GLM-4.7](candidates/glm-4.7.md) | Q4_K_M | 218.52 | 205 | MoE | 32.0 | shortlist |
| 5 | [Llama-3.1-Nemotron-Ultra-253B-v1](candidates/nemotron-ultra-253b.md) | Q6_K | 207.88 | 195 | Dense NAS-pruned | 253.0 | shortlist |
| 6 | [DeepSeek-R1-0528](candidates/deepseek-r1-0528.md) | UD-IQ2_M | 229.0 | 215 | MoE + MLA | 37.0 | conditional |
| 7 | [Llama-3.1-Tulu-3-405B](candidates/tulu3-405b.md) | IQ4_XS | 216.57 | 203 | Dense | 405.0 | conditional |
| 8 | [MiniMax-M3](candidates/minimax-m3.md) | UD-IQ4_XS | 208.0 | 195 | MoE + MiniMax Sparse Attention | 23.0 | experimental |
| 9 | [Kimi-K2-Thinking](candidates/kimi-k2-thinking.md) | i1-IQ1_M community artifact | 228.0 | 214 | MoE | 32.0 | screened-out |

## Recommended starting points

1. **Qwen3-235B-A22B-Instruct-2507 UD-Q6_K_XL** is the default deployment candidate: mature dual-runtime support, Apache-2.0, 22B active parameters, and a high effective bit budget.
2. **Step-3.7-Flash Q8_0** is the fidelity-first sparse candidate: approximately 209 GB, only about 11B active parameters, and direct quant-specific PPL/KLD evidence. Pin the runtime because Step support is newer.
3. **MiMo-V2-Flash Q5_K_M** is the long-context capacity candidate: its hybrid attention produces much slower KV growth than full-attention models.
4. **GLM-4.7 Q4_K_M** has strong official coding/agent evidence, but the selected 4-bit artifact and full-attention KV growth increase deployment risk.
5. **Nemotron Ultra 253B Q6_K** is the dense high-bit option; it is compute-heavy even though the artifact fits the target band.

## Non-negotiable caveats

- **A 210 GB file is not a 210 GB deployment.** The tables use a conservative weight allocation plus KV and fixed planning reserves.
- **MoE reduces active compute, not resident weights.** All experts remain resident unless a runtime explicitly implements expert paging/offload.
- **Official benchmark tables are not cross-model leaderboards.** Prompting, harnesses, dates, and tool scaffolds differ.
- **Strix Halo target-size performance is open.** ROCmFPX publishes local Strix Halo results for much smaller 27B/35B examples; this package does not extrapolate them to 200–230 GB artifacts.
- **RPC is a capacity mechanism, not a performance guarantee.** llama.cpp describes the RPC backend as proof-of-concept and without authentication or encryption.

Start with the [Ranked Shortlist](pages/Ranked-Shortlist.md), then read [Capacity Planning](pages/Capacity-Planning.md) and the candidate card for the selected model.
