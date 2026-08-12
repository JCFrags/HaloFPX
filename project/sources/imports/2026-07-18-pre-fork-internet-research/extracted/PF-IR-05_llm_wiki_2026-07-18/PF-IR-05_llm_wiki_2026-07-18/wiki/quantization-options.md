# Converter quantization option snapshots

Access date: **2026-07-18**  
Evidence label: **`[CONVERTER]`**  
Local validation: **`NOT_RUN`**

> Sizes below are rounded converter UI observations. They are not exact artifact identity. Only the selected GLM-4.7 and DeepSeek-R1 rows have complete exact selected-shard byte/hash manifests in this package.

## DeepSeek-V3.1

Immutable converter snapshot: `unsloth/DeepSeek-V3.1-GGUF@f69aacf1ee1e4f66737daebc7e71fa527dfda1bd`

| Quantization | Display size | Selected | Exact selected bytes |
|---|---:|:---:|---:|
| `UD-IQ1_S` | 192 GB |  |  |
| `UD-TQ1_0` | 170 GB |  |  |
| `UD-IQ1_M` | 207 GB |  |  |
| `UD-IQ2_XXS` | 226 GB | **YES** | `UNAVAILABLE` |
| `Q2_K` | 246 GB |  |  |
| `UD-IQ2_M` | 236 GB |  |  |
| `Q2_K_L` | 246 GB |  |  |
| `UD-Q2_K_XL` | 256 GB |  |  |
| `UD-IQ3_XXS` | 280 GB |  |  |
| `Q3_K_S` | 290 GB |  |  |
| `Q3_K_M` | 320 GB |  |  |
| `UD-Q3_K_XL` | 300 GB |  |  |
| `IQ4_XS` | 358 GB |  |  |
| `Q4_K_S` | 381 GB |  |  |
| `IQ4_NL` | 379 GB |  |  |
| `Q4_0` | 380 GB |  |  |
| `Q4_1` | 421 GB |  |  |
| `Q4_K_M` | 405 GB |  |  |
| `UD-Q4_K_XL` | 387 GB |  |  |
| `Q5_K_S` | 463 GB |  |  |
| `Q5_K_M` | 476 GB |  |  |
| `UD-Q5_K_XL` | 485 GB |  |  |
| `Q6_K` | 551 GB |  |  |
| `UD-Q6_K_XL` | 574 GB |  |  |
| `Q8_0` | 713 GB |  |  |
| `UD-Q8_K_XL` | 781 GB |  |  |
| `BF16` | 1.34 TB |  |  |

## GLM-4.7

Immutable converter snapshot: `unsloth/GLM-4.7-GGUF@a030db4a327b2dece3f0e5c4ff82c5f4feab4f26`

| Quantization | Display size | Selected | Exact selected bytes |
|---|---:|:---:|---:|
| `UD-IQ1_S` | 97.2 GB |  |  |
| `UD-TQ1_0` | 84.5 GB |  |  |
| `UD-IQ1_M` | 108 GB |  |  |
| `UD-IQ2_XXS` | 116 GB |  |  |
| `Q2_K` | 131 GB |  |  |
| `UD-IQ2_M` | 122 GB |  |  |
| `Q2_K_L` | 131 GB |  |  |
| `UD-Q2_K_XL` | 135 GB |  |  |
| `UD-IQ3_XXS` | 145 GB |  |  |
| `Q3_K_S` | 155 GB |  |  |
| `Q3_K_M` | 171 GB |  |  |
| `UD-Q3_K_XL` | 159 GB |  |  |
| `IQ4_XS` | 192 GB |  |  |
| `Q4_K_S` | 204 GB |  |  |
| `IQ4_NL` | 202 GB |  |  |
| `Q4_0` | 203 GB |  |  |
| `Q4_1` | 225 GB |  |  |
| `Q4_K_M` | 216 GB | **YES** | 216,455,572,576 |
| `UD-Q4_K_XL` | 205 GB |  |  |
| `Q5_K_S` | 247 GB |  |  |
| `Q5_K_M` | 254 GB |  |  |
| `UD-Q5_K_XL` | 254 GB |  |  |
| `Q6_K` | 294 GB |  |  |
| `UD-Q6_K_XL` | 301 GB |  |  |
| `Q8_0` | 381 GB |  |  |
| `UD-Q8_K_XL` | 395 GB |  |  |
| `BF16` | 717 GB |  |  |

## Qwen3.5-397B-A17B

Immutable converter snapshot: `unsloth/Qwen3.5-397B-A17B-GGUF@0b7386368898883d58d445525ba8fa45e3f7289f`

| Quantization | Display size | Selected | Exact selected bytes |
|---|---:|:---:|---:|
| `UD-IQ1_M` | 107 GB |  |  |
| `UD-IQ2_XXS` | 115 GB |  |  |
| `UD-IQ2_M` | 123 GB |  |  |
| `UD-IQ3_XXS` | 140 GB |  |  |
| `Q3_K_S` | 164 GB |  |  |
| `UD-IQ3_S` | 146 GB |  |  |
| `UD-Q3_K_S` | 164 GB |  |  |
| `Q3_K_M` | 177 GB |  |  |
| `UD-Q3_K_M` | 177 GB |  |  |
| `UD-Q3_K_XL` | 179 GB |  |  |
| `UD-IQ4_XS` | 190 GB |  |  |
| `Q4_K_S` | 228 GB |  |  |
| `UD-Q4_K_S` | 228 GB | **YES** | `UNAVAILABLE` |
| `MXFP4_MOE` | 237 GB |  |  |
| `UD-IQ4_NL` | 194 GB |  |  |
| `Q4_K_M` | 244 GB |  |  |
| `UD-Q4_K_M` | 244 GB |  |  |
| `UD-Q4_K_XL` | 245 GB |  |  |
| `Q5_K_S` | 277 GB |  |  |
| `UD-Q5_K_S` | 277 GB |  |  |
| `Q5_K_M` | 294 GB |  |  |
| `UD-Q5_K_M` | 294 GB |  |  |
| `UD-Q5_K_XL` | 295 GB |  |  |
| `Q6_K` | 327 GB |  |  |
| `UD-Q6_K` | 327 GB |  |  |
| `UD-Q6_K_XL` | 362 GB |  |  |
| `Q8_0` | 422 GB |  |  |
| `UD-Q8_K_XL` | 428 GB |  |  |
| `BF16` | 793 GB |  |  |

## NVIDIA Nemotron 3 Ultra 550B-A55B

Immutable converter snapshot: `unsloth/NVIDIA-Nemotron-3-Ultra-550B-A55B-GGUF@05c80881664d32be9b26bab9d0360be00230ade4`

| Quantization | Display size | Selected | Exact selected bytes |
|---|---:|:---:|---:|
| `UD-IQ1_M` | 188 GB |  |  |
| `UD-IQ2_XXS` | 194 GB |  |  |
| `UD-IQ2_M` | 194 GB |  |  |
| `UD-Q2_K_XL` | 202 GB |  |  |
| `UD-IQ3_XXS` | 223 GB | **YES** | `UNAVAILABLE` |
| `UD-IQ3_S` | 251 GB |  |  |
| `UD-Q3_K_M` | 274 GB |  |  |
| `UD-Q3_K_XL` | 274 GB |  |  |
| `UD-IQ4_XS` | 286 GB |  |  |
| `UD-Q4_K_S` | 328 GB |  |  |
| `MXFP4_MOE` | 352 GB |  |  |
| `UD-IQ4_NL` | 294 GB |  |  |
| `UD-Q4_K_M` | 359 GB |  |  |
| `UD-Q4_K_XL` | 360 GB |  |  |
| `UD-Q5_K_S` | 392 GB |  |  |
| `UD-Q5_K_M` | 427 GB |  |  |
| `UD-Q5_K_XL` | 427 GB |  |  |
| `UD-Q6_K` | 461 GB |  |  |
| `UD-Q6_K_XL` | 523 GB |  |  |
| `Q8_0` | 584 GB |  |  |
| `UD-Q8_K_XL` | 595 GB |  |  |
| `BF16` | 1.1 TB |  |  |

## DeepSeek-R1

Immutable converter snapshot: `unsloth/DeepSeek-R1-GGUF@4dab99fdb3fea560e5a402b49c6ef11e025321a8`

| Quantization | Display size | Selected | Exact selected bytes |
|---|---:|:---:|---:|
| `UD-IQ1_S` | 140 GB |  |  |
| `UD-IQ1_M` | 169 GB |  |  |
| `UD-IQ2_XXS` | 196 GB |  |  |
| `Q2_K_XS` | 221 GB | **YES** | 221,253,686,944 |
| `Q2_K` | 244 GB |  |  |
| `Q2_K_L` | 244 GB |  |  |
| `UD-Q2_K_XL` | 227 GB |  |  |
| `Q3_K_M` | 319 GB |  |  |
| `Q4_K_M` | 404 GB |  |  |
| `Q5_K_M` | 475 GB |  |  |
| `Q6_K` | 551 GB |  |  |
| `Q8_0` | 713 GB |  |  |
| `BF16` | 1.34 TB |  |  |

## MiniMax-M2.7

Immutable converter snapshot: `unsloth/MiniMax-M2.7-GGUF@04ab68de18da5486aa9869e3bc72f0249ef51f89`

| Quantization | Display size | Selected | Exact selected bytes |
|---|---:|:---:|---:|
| `UD-IQ1_M` | 60.7 GB |  |  |
| `UD-IQ2_XXS` | 65.4 GB |  |  |
| `UD-IQ2_M` | 70.1 GB |  |  |
| `UD-Q2_K_XL` | 75.3 GB |  |  |
| `UD-IQ3_XXS` | 80.1 GB |  |  |
| `UD-IQ3_S` | 83.6 GB |  |  |
| `UD-Q3_K_S` | 93.6 GB |  |  |
| `UD-Q3_K_M` | 101 GB |  |  |
| `UD-Q3_K_XL` | 102 GB |  |  |
| `UD-IQ4_XS` | 108 GB |  |  |
| `UD-Q4_K_S` | 131 GB |  |  |
| `MXFP4_MOE` | 136 GB |  |  |
| `UD-IQ4_NL` | 111 GB |  |  |
| `UD-Q4_K_M` | 140 GB |  |  |
| `UD-Q4_K_XL` | 141 GB |  |  |
| `UD-Q5_K_S` | 159 GB |  |  |
| `UD-Q5_K_M` | 169 GB |  |  |
| `UD-Q5_K_XL` | 169 GB |  |  |
| `UD-Q6_K` | 188 GB |  |  |
| `UD-Q6_K_XL` | 207 GB | **YES** | `UNAVAILABLE` |
| `Q8_0` | 243 GB |  |  |
| `UD-Q8_K_XL` | 247 GB |  |  |
| `BF16` | 457 GB |  |  |

## Interpretation

The option tables establish conversion lineage and available size classes at the pinned converter snapshots. They do **not** establish that every option shares the same calibration corpus, imatrix generation procedure, tensor override rules, chat-template fix set, or runtime behavior. Selected-artifact imatrix/provenance fields are recorded separately in `manifests/artifacts/*.json`; missing corpus, commands, and hashes remain explicit.
