# KV Cache and Runtime Buffers

## Storage encodings

| Cache type | Exact storage used in formulas |
|---|---:|
| F16/BF16 | 2 bytes/scalar |
| Q8_0 | 34 bytes per 32 scalars = 1.0625 bytes/scalar |
| Q4_0 | 18 bytes per 32 scalars = 0.5625 bytes/scalar |

## General GQA formula

```text
KV_bytes = layers × context_tokens × KV_heads × (K_head_dim + V_head_dim) × bytes_per_scalar
```

MoE expert count does not change KV-cache growth. Attention topology does.

## Candidate formulas

- **Qwen:** `94 × T × 4 × (128+128)` scalars.
- **GLM:** `92 × T × 8 × (128+128)` scalars.
- **Tulu 405B:** `126 × T × 8 × (128+128)` scalars.
- **Nemotron upper bound:** same full-attention formula as the 126-layer Llama control; actual is lower where NAS blocks skip attention.
- **DeepSeek MLA:** `61 × T × (512+64)` scalars under the current absorption implementation.
- **MiMo hybrid:** `9 × T × 4 × (192+128) + 39 × S128 × 8 × (192+128)`, where `S128 = pad256(min(T, 128+512)) = 768` for the contexts in this package.
- **Step hybrid:** `12 × T × 8 × 256 + 33 × S512 × 8 × 256`, where `S512 = pad256(min(T, 512+512)) = 1024`.

## Q8_0 cache results

| Candidate | 32K GiB | 64K GiB | 128K GiB | Native-context GiB |
|---|---|---|---|---|
| Qwen3-235B-A22B-Instruct-2507 | 3.121 | 6.242 | 12.484 | 24.969 |
| Step-3.7-Flash | 0.865 | 1.662 | 3.256 | 6.443 |
| MiMo-V2-Flash | 0.449 | 0.823 | 1.570 | 3.064 |
| GLM-4.7 | 6.109 | 12.219 | 24.438 | 37.802 |
| Llama-3.1-Nemotron-Ultra-253B-v1 | 8.367 | 16.734 | 33.469 | 33.469 |
| DeepSeek-R1-0528 | 1.139 | 2.279 | 4.557 | 5.696 |
| Llama-3.1-Tulu-3-405B | 8.367 | 16.734 | 33.469 | 33.469 |
| MiniMax-M3 | unverified | unverified | unverified | measurement gate |
| Kimi-K2-Thinking | unverified | unverified | unverified | measurement gate |

## Buffers are not a universal formula

Graph/work buffers depend on backend, FlashAttention, batch/ubatch, output count, offload map, quant kernels, and graph reuse. This package therefore uses explicit fixed **planning reserves** rather than fabricating a model-independent buffer equation. Replace those reserves with measured startup logs before purchase approval.
