# Quality Evidence

| Candidate | Evidence available | Selected-quant caveat |
|---|---|---|
| Qwen3-235B-A22B-Instruct-2507 | Official BF16/Instruct card reports MMLU-Pro 83.0 and GPQA 77.5; results are not directly comparable to other cards. | Moderate-low: high effective bit budget; candidate-specific independent quant benchmark not located. |
| Step-3.7-Flash | Official card supplies broad benchmark evidence; AesSedai reports Q8_0 PPL 1.894568 and KLD 0.005301 for its evaluation setup. | Low for Q8_0; direct quant-specific PPL/KLD evidence is available. |
| MiMo-V2-Flash | Official model card reports benchmark results for coding, reasoning, and agents; use only as upstream evidence. | Moderate: Q5_K_M is materially safer than 4-bit options, but no candidate-specific independent quant benchmark was located. |
| GLM-4.7 | Official card reports SWE-bench Verified 73.8, multilingual 66.7, TerminalBench 2.0 41, and HLE 42.8. | Moderate-high versus Q5/Q6/Q8 candidates because the chosen artifact is Q4_K_M. |
| Llama-3.1-Nemotron-Ultra-253B-v1 | NVIDIA model card provides reasoning, chat, RAG, and tool-use evaluation; no selected-quant evaluation was located. | Moderate-low: Q6_K gives high weight fidelity. |
| DeepSeek-R1-0528 | Official BF16 card reports MMLU-Pro 85.0, GPQA 81.0, AIME 2025 87.5, and SWE-bench Verified 57.6. | High: effective storage is only about 2.73 bits/parameter and no direct selected-quant evaluation was found. |
| Llama-3.1-Tulu-3-405B | Upstream Tulu card supplies instruction-following and preference-training evidence; quant card provides qualitative guidance only. | Moderate-high: IQ4_XS is an aggressive dense-model quant; card claims similar quality to Q4_K_S but lacks candidate-specific independent evaluation. |
| MiniMax-M3 | Official card reports coding, cowork, reasoning, and long-context results; deployment maturity remains the gating factor. | Moderate-high: IQ4_XS; no selected-quant independent evaluation located. |
| Kimi-K2-Thinking | No acceptable selected-quant quality evidence located. | Very high: about 1.82 effective bits/parameter; the community card labels the quant as suitable only for the mostly desperate. |

## Evidence hierarchy used

1. **Direct selected-quant evaluation on a disclosed dataset/harness.** Step Q8_0 has a published PPL/KLD result.
2. **Official upstream model benchmarks.** Useful for capability orientation, but not proof of the GGUF quant.
3. **Quantizer qualitative guidance or generic quant studies.** Supporting evidence only.
4. **Anecdotes.** Excluded from ranking unless reproducible details are available.

## Why official benchmark numbers are not normalized

The cards use different harness versions, prompts, sampling parameters, tool scaffolds, maximum generation lengths, and benchmark dates. This package records representative official evidence but does not compute a cross-model intelligence average.

## Required local quality suite

Create a fixed, versioned set of prompts and expected validators. At minimum:

- 100–500 representative production prompts;
- exact structured-output schemas;
- executable code tests;
- long-context retrieval needles at multiple positions;
- multilingual and safety-policy cases relevant to the deployment;
- comparison against a high-bit or upstream reference;
- repeatability runs with the production sampling configuration.

Record task score, malformed-output rate, refusal/over-refusal rate, latency, tokens/s, peak memory, and any special-token leakage.
