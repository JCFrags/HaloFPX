# Capacity Planning

## Exact planning convention

All table arithmetic is exact under these declared assumptions:

- publisher display size in decimal GB;
- `weight_plan_GiB = ceil(GB × 10^9 / 2^30) + 1`;
- Q8_0 KV cache, one sequence, `ubatch=512`, FlashAttention, `--swa-full` disabled;
- weight and KV budgets divided in quarter-GiB units across members;
- fixed reserves shown below include OS (where applicable), graph/runtime, layer-granularity skew, and safety;
- multimodal projector memory is separate unless explicitly added.

These are deterministic **capacity envelopes**, not measured allocator traces.

## Profile reserves

| Profile member | Capacity GiB | OS/services | Runtime/graph | Split/skew safety | Usable before weights/KV |
|---|---|---|---|---|---|
| uma-256-single:host | 256.0 | 12.0 | 8.0 | 4.0 | 232.0 |
| rpc-2x128-uma:coordinator | 128.0 | 10.0 | 6.0 | 4.0 | 108.0 |
| rpc-2x128-uma:worker | 128.0 | 8.0 | 6.0 | 4.0 | 110.0 |
| local-3x96-gpu:gpu0 | 96.0 | 0.0 | 6.0 | 2.0 | 88.0 |
| local-3x96-gpu:gpu1 | 96.0 | 0.0 | 4.0 | 2.0 | 90.0 |
| local-3x96-gpu:gpu2 | 96.0 | 0.0 | 4.0 | 2.0 | 90.0 |
| local-4x64-gpu:gpu0 | 64.0 | 0.0 | 6.0 | 2.0 | 56.0 |
| local-4x64-gpu:gpu1 | 64.0 | 0.0 | 4.0 | 2.0 | 58.0 |
| local-4x64-gpu:gpu2 | 64.0 | 0.0 | 4.0 | 2.0 | 58.0 |
| local-4x64-gpu:gpu3 | 64.0 | 0.0 | 4.0 | 2.0 | 58.0 |

## 128K-or-native qualification

For models whose native context is below 128K, the native context is used. The margin is the tightest member margin after equal planned split.

| Candidate | Weights plan GiB | Q8 KV plan GiB | 256 UMA | 2×128 RPC UMA | 3×96 local | 4×64 local |
|---|---|---|---|---|---|---|
| Qwen3-235B-A22B-Instruct-2507 | 190 | 12.50 | FIT (+29.50 GiB min) | FIT (+6.75 GiB min) | FIT (+20.25 GiB min) | FIT (+5.25 GiB min) |
| Step-3.7-Flash | 196 | 3.50 | FIT (+32.50 GiB min) | FIT (+8.25 GiB min) | FIT (+21.25 GiB min) | FIT (+6.00 GiB min) |
| MiMo-V2-Flash | 206 | 1.75 | FIT (+24.25 GiB min) | FIT (+4.00 GiB min) | FIT (+18.50 GiB min) | FIT (+4.00 GiB min) |
| GLM-4.7 | 205 | 24.50 | FIT (+2.50 GiB min) | NO (-6.75 GiB min) | FIT (+11.25 GiB min) | NO (-1.50 GiB min) |
| Llama-3.1-Nemotron-Ultra-253B-v1 | 195 | 33.50 | FIT (+3.50 GiB min) | NO (-6.25 GiB min) | FIT (+11.75 GiB min) | NO (-1.25 GiB min) |
| DeepSeek-R1-0528 | 215 | 4.75 | FIT (+12.25 GiB min) | NO (-2.00 GiB min) | FIT (+14.50 GiB min) | FIT (+1.00 GiB min) |
| Llama-3.1-Tulu-3-405B | 203 | 33.50 | NO (-4.50 GiB min) | NO (-10.25 GiB min) | FIT (+9.00 GiB min) | NO (-3.25 GiB min) |

## Strix Halo interpretation

A single 128 GiB-class Strix Halo node cannot hold any selected 200–230 GB artifact entirely. The two-node RPC profile shows only a memory envelope. It does not establish tokens/second, latency, prompt-fill scaling, inter-node transfer cost, or thermal behavior. Those remain open until the exact target model, quant, runtime commit, backend, context, and network are measured.

## Local multi-GPU interpretation

The 3×96 and 4×64 profiles aggregate enough accelerator memory for most candidates, but layer sizes are indivisible and embeddings/output tensors can skew allocation. The fixed skew reserve is a planning allowance, not proof. Use the startup memory breakdown and adjust `--tensor-split` before acceptance.

## Full detail

See [Exact Per-Node Budgets](Per-Node-Budgets.md) for the complete human-readable 128K/native member tables.

`data/capacity_budgets.csv` contains every profile member for 32K, 64K, 128K, and each model's native context. `data/capacity_summary.csv` provides fit and maximum utilization by model/profile/context.

## Vision sidecars

Step's projector is about 3.97 decimal GB. Reserve **5 GiB** in the deployment that owns the vision path. Text-only inference does not need that sidecar. MiniMax-M3 multimodal memory is not qualified here.
