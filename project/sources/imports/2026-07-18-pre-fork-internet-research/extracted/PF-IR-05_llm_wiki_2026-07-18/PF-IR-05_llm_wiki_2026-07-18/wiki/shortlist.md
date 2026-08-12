# Immutable Phase 2 shortlist

Access date: **2026-07-18**  
Machine validation: **NOT_RUN**

## Evidence labels

- `[PUBLISHER]` — publisher-controlled model repository or license.
- `[CONVERTER]` — third-party GGUF/quantization repository.
- `[STATIC-SOURCE]` — source-code presence at exact runtime commit.
- `[LOCAL-VALIDATION]` — evidence produced on the target machine; all such claims are `NOT_RUN` here.
- `[INFERENCE]` — explicitly reasoned conclusion, not a direct source statement.
- `[UNAVAILABLE]` — requested exact field could not be captured.
- `[DECISION]` — shortlist routing; not final model selection.

## Decision table

| Rank | Candidate | Selected quant | Stored size evidence | Exact artifact identity | License gate | Decision |
|---:|---|---|---|---|---|---|
| 1 | [DeepSeek-V3.1](models/deepseek-v3.1.md) | `UD-IQ2_XXS` | 226 GB (Hugging Face UI, rounded) | PARTIAL | MIT | **ADVANCE_TO_LOCAL_PREFLIGHT** |
| 2 | [GLM-4.7](models/glm-4.7.md) | `Q4_K_M` | 216 GB-class | 216,455,572,576 B / 216.455573 GB / 201.589961 GiB | MIT | **ADVANCE_TO_LOCAL_PREFLIGHT** |
| 3 | [Qwen3.5-397B-A17B](models/qwen3.5-397b-a17b.md) | `UD-Q4_K_S` | 228 GB (Hugging Face UI, rounded) | PARTIAL_5_OF_6_EXACT | Apache-2.0 | **ADVANCE_TO_LOCAL_PREFLIGHT** |
| 4 | [NVIDIA Nemotron 3 Ultra 550B-A55B](models/nemotron-3-ultra.md) | `UD-IQ3_XXS` | 223 GB (Hugging Face UI, rounded) | NAMES_AND_ROUNDED_SIZES_ONLY | OpenMDW-1.1 | **ADVANCE_TO_LOCAL_PREFLIGHT** |
| 5 | [DeepSeek-R1](models/deepseek-r1.md) | `Q2_K_XS` | 221 GB-class | 221,253,686,944 B / 221.253687 GB / 206.058553 GiB | MIT | **ADVANCE_TO_LOCAL_PREFLIGHT** |
| 6 | [MiniMax-M2.7](models/minimax-m2.7.md) | `UD-Q6_K_XL` | 207 GB (Hugging Face UI, rounded) | NAMES_AND_PARTIAL_ROUNDED_SIZES_ONLY | Custom non-commercial license; commercial use requires prior written authorization and attribution; prohibited-use appendix. | **HOLD_LICENSE_AND_CONVERSION_PREFLIGHT** |

## Interpretation

The five `ADVANCE_TO_LOCAL_PREFLIGHT` candidates are immutable artifact candidates, not claims of fit, loadability, execution correctness, throughput, quality, or business suitability. MiniMax-M2.7 is held because its custom license is materially restrictive and its selected artifact manifest is incomplete.

Exact selected-shard totals are available only for GLM-4.7 Q4_K_M and DeepSeek-R1 Q2_K_XS. All rounded totals remain display observations.
