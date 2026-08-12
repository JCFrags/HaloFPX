# Quantization Comparison

| Candidate | Selected representation | Artifact GB | Effective bits/parameter | Risk assessment |
|---|---|---|---|---|
| Qwen3-235B-A22B-Instruct-2507 | UD-Q6_K_XL | 201.91 | 6.874 | Moderate-low: high effective bit budget; candidate-specific independent quant benchmark not located. |
| Step-3.7-Flash | Q8_0 | 209.00 | 8.444 | Low for Q8_0; direct quant-specific PPL/KLD evidence is available. |
| MiMo-V2-Flash | Q5_K_M | 219.20 | 5.675 | Moderate: Q5_K_M is materially safer than 4-bit options, but no candidate-specific independent quant benchmark was located. |
| GLM-4.7 | Q4_K_M | 218.52 | 4.883 | Moderate-high versus Q5/Q6/Q8 candidates because the chosen artifact is Q4_K_M. |
| Llama-3.1-Nemotron-Ultra-253B-v1 | Q6_K | 207.88 | 6.573 | Moderate-low: Q6_K gives high weight fidelity. |
| DeepSeek-R1-0528 | UD-IQ2_M | 229.00 | 2.730 | High: effective storage is only about 2.73 bits/parameter and no direct selected-quant evaluation was found. |
| Llama-3.1-Tulu-3-405B | IQ4_XS | 216.57 | 4.278 | Moderate-high: IQ4_XS is an aggressive dense-model quant; card claims similar quality to Q4_K_S but lacks candidate-specific independent evaluation. |
| MiniMax-M3 | UD-IQ4_XS | 208.00 | 3.888 | Moderate-high: IQ4_XS; no selected-quant independent evaluation located. |
| Kimi-K2-Thinking | i1-IQ1_M community artifact | 228.00 | 1.824 | Very high: about 1.82 effective bits/parameter; the community card labels the quant as suitable only for the mostly desperate. |

## How to read the ratios

`artifact_effective_bits_per_parameter = artifact_GB × 8 / total_parameters_B`

This is not the nominal tensor encoding. It includes metadata, mixed tensor policies, unquantized scales, embeddings/output protection, and any architecture side data. It is useful for comparing how aggressively a whole artifact was compressed.

## Selected-representation interpretation

- **Step Q8_0 (8.44 effective bits/parameter):** near the expected 8.5-bit Q8_0 storage ratio and the safest quant choice in the band.
- **Qwen UD-Q6_K_XL (6.87):** high-bit mixed/dynamic quant, good default balance.
- **Nemotron Q6_K (6.57):** high fidelity, but dense compute.
- **MiMo Q5_K_M (5.68):** mid/high quant suitable for a production qualification pass.
- **GLM Q4_K_M (4.92):** common balanced 4-bit family; verify tools/JSON and long reasoning.
- **Tulu IQ4_XS (4.28):** lower dense-model storage cost, with architecture/backend tradeoffs.
- **DeepSeek UD-IQ2_M (2.73):** severe compression. Upstream BF16 quality cannot be assumed to survive.
- **Kimi i1-IQ1_M (1.82):** below the practical acceptance threshold for this shortlist.

## ROCmFPX formats

At the pinned ROCmFPX snapshot, the documented weight formats are ROCmFP3 (3.5 bpw), ROCmFP4 FAST (4.25), balanced ROCmFP4 (4.50), ROCmFP6 (6.5), and ROCmFP8 (8.25). Small Qwen comparisons in the project report ROCmFP4 artifacts around 12% smaller than matched Q4_K_M files. Those measurements are **not** evidence that the same quality or speed delta holds for the target models.

A released Step ROCmFPX Q3 QualityPlus recipe is 3.57 bpw over nine shards, with higher precision assigned to attention/shared/dense tensors. It is materially below the 200–230 GB band and is listed as an alternative, not the selected artifact.

## Acceptance test

For any Q4-or-lower candidate, run the exact workload suite against a BF16/F16 or higher-bit reference. Include:

- long-form reasoning completion and answer correctness;
- structured JSON/tool calls with schema validation;
- code generation plus execution tests;
- multilingual prompts if relevant;
- long-context retrieval at several document positions;
- repetition, truncation, and malformed-special-token checks.
