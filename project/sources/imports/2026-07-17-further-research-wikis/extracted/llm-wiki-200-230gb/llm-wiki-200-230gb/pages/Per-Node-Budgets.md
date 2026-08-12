# Exact Per-Node Memory Budgets

These tables expose the exact member-level arithmetic behind the fit summary. They use Q8_0 KV, one sequence, `ubatch=512`, equal planned split in quarter-GiB units, and the fixed reserves in `data/profiles.json`. The context is 128K, or the model native maximum when lower.

`Total = weights + KV + OS/services + runtime/graph + split-skew safety`. Negative margin means the configuration does not fit under the declared envelope.

## Qwen3-235B-A22B-Instruct-2507

Context: **131,072 tokens** · selected artifact: **UD-Q6_K_XL** · weight plan: **190 GiB**

| Profile | Member | Capacity | Weights | KV Q8 | OS | Runtime | Skew | Total | Margin | Fit |
|---|---|---|---|---|---|---|---|---|---|---|
| uma-256-single | host | 256.00 | 190.00 | 12.50 | 12.00 | 8.00 | 4.00 | 226.50 | +29.50 | FIT |
| rpc-2x128-uma | coordinator | 128.00 | 95.00 | 6.25 | 10.00 | 6.00 | 4.00 | 121.25 | +6.75 | FIT |
| rpc-2x128-uma | worker | 128.00 | 95.00 | 6.25 | 8.00 | 6.00 | 4.00 | 119.25 | +8.75 | FIT |
| local-3x96-gpu | gpu0 | 96.00 | 63.50 | 4.25 | 0.00 | 6.00 | 2.00 | 75.75 | +20.25 | FIT |
| local-3x96-gpu | gpu1 | 96.00 | 63.25 | 4.25 | 0.00 | 4.00 | 2.00 | 73.50 | +22.50 | FIT |
| local-3x96-gpu | gpu2 | 96.00 | 63.25 | 4.00 | 0.00 | 4.00 | 2.00 | 73.25 | +22.75 | FIT |
| local-4x64-gpu | gpu0 | 64.00 | 47.50 | 3.25 | 0.00 | 6.00 | 2.00 | 58.75 | +5.25 | FIT |
| local-4x64-gpu | gpu1 | 64.00 | 47.50 | 3.25 | 0.00 | 4.00 | 2.00 | 56.75 | +7.25 | FIT |
| local-4x64-gpu | gpu2 | 64.00 | 47.50 | 3.00 | 0.00 | 4.00 | 2.00 | 56.50 | +7.50 | FIT |
| local-4x64-gpu | gpu3 | 64.00 | 47.50 | 3.00 | 0.00 | 4.00 | 2.00 | 56.50 | +7.50 | FIT |

## Step-3.7-Flash

Context: **131,072 tokens** · selected artifact: **Q8_0** · weight plan: **196 GiB**

| Profile | Member | Capacity | Weights | KV Q8 | OS | Runtime | Skew | Total | Margin | Fit |
|---|---|---|---|---|---|---|---|---|---|---|
| uma-256-single | host | 256.00 | 196.00 | 3.50 | 12.00 | 8.00 | 4.00 | 223.50 | +32.50 | FIT |
| rpc-2x128-uma | coordinator | 128.00 | 98.00 | 1.75 | 10.00 | 6.00 | 4.00 | 119.75 | +8.25 | FIT |
| rpc-2x128-uma | worker | 128.00 | 98.00 | 1.75 | 8.00 | 6.00 | 4.00 | 117.75 | +10.25 | FIT |
| local-3x96-gpu | gpu0 | 96.00 | 65.50 | 1.25 | 0.00 | 6.00 | 2.00 | 74.75 | +21.25 | FIT |
| local-3x96-gpu | gpu1 | 96.00 | 65.25 | 1.25 | 0.00 | 4.00 | 2.00 | 72.50 | +23.50 | FIT |
| local-3x96-gpu | gpu2 | 96.00 | 65.25 | 1.00 | 0.00 | 4.00 | 2.00 | 72.25 | +23.75 | FIT |
| local-4x64-gpu | gpu0 | 64.00 | 49.00 | 1.00 | 0.00 | 6.00 | 2.00 | 58.00 | +6.00 | FIT |
| local-4x64-gpu | gpu1 | 64.00 | 49.00 | 1.00 | 0.00 | 4.00 | 2.00 | 56.00 | +8.00 | FIT |
| local-4x64-gpu | gpu2 | 64.00 | 49.00 | 0.75 | 0.00 | 4.00 | 2.00 | 55.75 | +8.25 | FIT |
| local-4x64-gpu | gpu3 | 64.00 | 49.00 | 0.75 | 0.00 | 4.00 | 2.00 | 55.75 | +8.25 | FIT |

## MiMo-V2-Flash

Context: **131,072 tokens** · selected artifact: **Q5_K_M** · weight plan: **206 GiB**

| Profile | Member | Capacity | Weights | KV Q8 | OS | Runtime | Skew | Total | Margin | Fit |
|---|---|---|---|---|---|---|---|---|---|---|
| uma-256-single | host | 256.00 | 206.00 | 1.75 | 12.00 | 8.00 | 4.00 | 231.75 | +24.25 | FIT |
| rpc-2x128-uma | coordinator | 128.00 | 103.00 | 1.00 | 10.00 | 6.00 | 4.00 | 124.00 | +4.00 | FIT |
| rpc-2x128-uma | worker | 128.00 | 103.00 | 0.75 | 8.00 | 6.00 | 4.00 | 121.75 | +6.25 | FIT |
| local-3x96-gpu | gpu0 | 96.00 | 68.75 | 0.75 | 0.00 | 6.00 | 2.00 | 77.50 | +18.50 | FIT |
| local-3x96-gpu | gpu1 | 96.00 | 68.75 | 0.50 | 0.00 | 4.00 | 2.00 | 75.25 | +20.75 | FIT |
| local-3x96-gpu | gpu2 | 96.00 | 68.50 | 0.50 | 0.00 | 4.00 | 2.00 | 75.00 | +21.00 | FIT |
| local-4x64-gpu | gpu0 | 64.00 | 51.50 | 0.50 | 0.00 | 6.00 | 2.00 | 60.00 | +4.00 | FIT |
| local-4x64-gpu | gpu1 | 64.00 | 51.50 | 0.50 | 0.00 | 4.00 | 2.00 | 58.00 | +6.00 | FIT |
| local-4x64-gpu | gpu2 | 64.00 | 51.50 | 0.50 | 0.00 | 4.00 | 2.00 | 58.00 | +6.00 | FIT |
| local-4x64-gpu | gpu3 | 64.00 | 51.50 | 0.25 | 0.00 | 4.00 | 2.00 | 57.75 | +6.25 | FIT |

## GLM-4.7

Context: **131,072 tokens** · selected artifact: **Q4_K_M** · weight plan: **205 GiB**

| Profile | Member | Capacity | Weights | KV Q8 | OS | Runtime | Skew | Total | Margin | Fit |
|---|---|---|---|---|---|---|---|---|---|---|
| uma-256-single | host | 256.00 | 205.00 | 24.50 | 12.00 | 8.00 | 4.00 | 253.50 | +2.50 | FIT |
| rpc-2x128-uma | coordinator | 128.00 | 102.50 | 12.25 | 10.00 | 6.00 | 4.00 | 134.75 | -6.75 | NO |
| rpc-2x128-uma | worker | 128.00 | 102.50 | 12.25 | 8.00 | 6.00 | 4.00 | 132.75 | -4.75 | NO |
| local-3x96-gpu | gpu0 | 96.00 | 68.50 | 8.25 | 0.00 | 6.00 | 2.00 | 84.75 | +11.25 | FIT |
| local-3x96-gpu | gpu1 | 96.00 | 68.25 | 8.25 | 0.00 | 4.00 | 2.00 | 82.50 | +13.50 | FIT |
| local-3x96-gpu | gpu2 | 96.00 | 68.25 | 8.00 | 0.00 | 4.00 | 2.00 | 82.25 | +13.75 | FIT |
| local-4x64-gpu | gpu0 | 64.00 | 51.25 | 6.25 | 0.00 | 6.00 | 2.00 | 65.50 | -1.50 | NO |
| local-4x64-gpu | gpu1 | 64.00 | 51.25 | 6.25 | 0.00 | 4.00 | 2.00 | 63.50 | +0.50 | FIT |
| local-4x64-gpu | gpu2 | 64.00 | 51.25 | 6.00 | 0.00 | 4.00 | 2.00 | 63.25 | +0.75 | FIT |
| local-4x64-gpu | gpu3 | 64.00 | 51.25 | 6.00 | 0.00 | 4.00 | 2.00 | 63.25 | +0.75 | FIT |

## Llama-3.1-Nemotron-Ultra-253B-v1

Context: **131,072 tokens** · selected artifact: **Q6_K** · weight plan: **195 GiB**

| Profile | Member | Capacity | Weights | KV Q8 | OS | Runtime | Skew | Total | Margin | Fit |
|---|---|---|---|---|---|---|---|---|---|---|
| uma-256-single | host | 256.00 | 195.00 | 33.50 | 12.00 | 8.00 | 4.00 | 252.50 | +3.50 | FIT |
| rpc-2x128-uma | coordinator | 128.00 | 97.50 | 16.75 | 10.00 | 6.00 | 4.00 | 134.25 | -6.25 | NO |
| rpc-2x128-uma | worker | 128.00 | 97.50 | 16.75 | 8.00 | 6.00 | 4.00 | 132.25 | -4.25 | NO |
| local-3x96-gpu | gpu0 | 96.00 | 65.00 | 11.25 | 0.00 | 6.00 | 2.00 | 84.25 | +11.75 | FIT |
| local-3x96-gpu | gpu1 | 96.00 | 65.00 | 11.25 | 0.00 | 4.00 | 2.00 | 82.25 | +13.75 | FIT |
| local-3x96-gpu | gpu2 | 96.00 | 65.00 | 11.00 | 0.00 | 4.00 | 2.00 | 82.00 | +14.00 | FIT |
| local-4x64-gpu | gpu0 | 64.00 | 48.75 | 8.50 | 0.00 | 6.00 | 2.00 | 65.25 | -1.25 | NO |
| local-4x64-gpu | gpu1 | 64.00 | 48.75 | 8.50 | 0.00 | 4.00 | 2.00 | 63.25 | +0.75 | FIT |
| local-4x64-gpu | gpu2 | 64.00 | 48.75 | 8.25 | 0.00 | 4.00 | 2.00 | 63.00 | +1.00 | FIT |
| local-4x64-gpu | gpu3 | 64.00 | 48.75 | 8.25 | 0.00 | 4.00 | 2.00 | 63.00 | +1.00 | FIT |

## DeepSeek-R1-0528

Context: **131,072 tokens** · selected artifact: **UD-IQ2_M** · weight plan: **215 GiB**

| Profile | Member | Capacity | Weights | KV Q8 | OS | Runtime | Skew | Total | Margin | Fit |
|---|---|---|---|---|---|---|---|---|---|---|
| uma-256-single | host | 256.00 | 215.00 | 4.75 | 12.00 | 8.00 | 4.00 | 243.75 | +12.25 | FIT |
| rpc-2x128-uma | coordinator | 128.00 | 107.50 | 2.50 | 10.00 | 6.00 | 4.00 | 130.00 | -2.00 | NO |
| rpc-2x128-uma | worker | 128.00 | 107.50 | 2.25 | 8.00 | 6.00 | 4.00 | 127.75 | +0.25 | FIT |
| local-3x96-gpu | gpu0 | 96.00 | 71.75 | 1.75 | 0.00 | 6.00 | 2.00 | 81.50 | +14.50 | FIT |
| local-3x96-gpu | gpu1 | 96.00 | 71.75 | 1.50 | 0.00 | 4.00 | 2.00 | 79.25 | +16.75 | FIT |
| local-3x96-gpu | gpu2 | 96.00 | 71.50 | 1.50 | 0.00 | 4.00 | 2.00 | 79.00 | +17.00 | FIT |
| local-4x64-gpu | gpu0 | 64.00 | 53.75 | 1.25 | 0.00 | 6.00 | 2.00 | 63.00 | +1.00 | FIT |
| local-4x64-gpu | gpu1 | 64.00 | 53.75 | 1.25 | 0.00 | 4.00 | 2.00 | 61.00 | +3.00 | FIT |
| local-4x64-gpu | gpu2 | 64.00 | 53.75 | 1.25 | 0.00 | 4.00 | 2.00 | 61.00 | +3.00 | FIT |
| local-4x64-gpu | gpu3 | 64.00 | 53.75 | 1.00 | 0.00 | 4.00 | 2.00 | 60.75 | +3.25 | FIT |

## Llama-3.1-Tulu-3-405B

Context: **131,072 tokens** · selected artifact: **IQ4_XS** · weight plan: **203 GiB**

| Profile | Member | Capacity | Weights | KV Q8 | OS | Runtime | Skew | Total | Margin | Fit |
|---|---|---|---|---|---|---|---|---|---|---|
| uma-256-single | host | 256.00 | 203.00 | 33.50 | 12.00 | 8.00 | 4.00 | 260.50 | -4.50 | NO |
| rpc-2x128-uma | coordinator | 128.00 | 101.50 | 16.75 | 10.00 | 6.00 | 4.00 | 138.25 | -10.25 | NO |
| rpc-2x128-uma | worker | 128.00 | 101.50 | 16.75 | 8.00 | 6.00 | 4.00 | 136.25 | -8.25 | NO |
| local-3x96-gpu | gpu0 | 96.00 | 67.75 | 11.25 | 0.00 | 6.00 | 2.00 | 87.00 | +9.00 | FIT |
| local-3x96-gpu | gpu1 | 96.00 | 67.75 | 11.25 | 0.00 | 4.00 | 2.00 | 85.00 | +11.00 | FIT |
| local-3x96-gpu | gpu2 | 96.00 | 67.50 | 11.00 | 0.00 | 4.00 | 2.00 | 84.50 | +11.50 | FIT |
| local-4x64-gpu | gpu0 | 64.00 | 50.75 | 8.50 | 0.00 | 6.00 | 2.00 | 67.25 | -3.25 | NO |
| local-4x64-gpu | gpu1 | 64.00 | 50.75 | 8.50 | 0.00 | 4.00 | 2.00 | 65.25 | -1.25 | NO |
| local-4x64-gpu | gpu2 | 64.00 | 50.75 | 8.25 | 0.00 | 4.00 | 2.00 | 65.00 | -1.00 | NO |
| local-4x64-gpu | gpu3 | 64.00 | 50.75 | 8.25 | 0.00 | 4.00 | 2.00 | 65.00 | -1.00 | NO |

## Lower-context and native-context detail

Every 32K, 64K, 128K, and native-context row is available in [`data/capacity_budgets.csv`](../data/capacity_budgets.csv). The CSV is the authoritative human-auditable output of `scripts/calculate_capacity.py`.

Actual llama.cpp placement can differ because layers and non-layer tensors are indivisible. Replace the fixed skew reserve with measured startup allocation before final procurement.
