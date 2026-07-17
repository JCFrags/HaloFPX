---
title: Worked-example method
status: calculated examples
---

# Worked-example method

The examples instantiate communication and KV formulas with official architecture fields. They do not predict runtime performance.

## Common declared scenario

| Input | Value | Label |
|---|---:|---|
| Prefill token count \(N\) | 4096 | SCENARIO ASSUMPTION |
| Decode active sequences \(Q\) | 1 | SCENARIO ASSUMPTION |
| Activation element bytes \(b_a\) | 2 | SCENARIO ASSUMPTION (BF16/FP16 transport) |
| KV element bytes \(b_{kv}\) | 2 | SCENARIO ASSUMPTION (BF16/FP16 KV) |
| Token ID bytes \(b_t\) | 4 | SCENARIO ASSUMPTION |
| MoE remote fraction \(\rho\) | 0.5 where shown | SCENARIO ASSUMPTION for sensitivity only |
| Nominal link floor | 40 Gb/s = 5 GB/s per link | SOURCED FACT + CALCULATED conversion |
| Nominal dual floor | 10 GB/s | CALCULATED arithmetic ceiling, only after an impossible ideal aggregation |

The nominal payload time is:

\[
T_{floor}=V/(5\times10^9)
\]

for one link or \(V/(10\times10^9)\) for two. It excludes latency, protocol, copies, reassembly, queueing, and runtime overhead. It is an impossible-to-beat lower bound, not an estimate of observed transfer time.

## Included formulas

- Boundary activation per token: \(hb_a\).
- Whole-model KV per token: \(2LH_{kv}db_{kv}\).
- TP per-rank forward bytes: \(2LNhb_a\) prefill; \(2LQhb_a\) decode.
- Contiguous split: \(Nhb_a\) prefill; \(Qhb_a\) decode plus token feedback.
- MoE expert service: \(L_{MoE}Nk\rho(2hb_a)\) in the simplified uniform case, before metadata.
- Ideal Q4 weights: \(0.5P\) bytes.

All machine-readable calculations are in [`data/worked_examples.json`](../../data/worked_examples.json) and [`data/worked_examples.csv`](../../data/worked_examples.csv). Regenerate them with:

```bash
python tools/cost_model.py --demo \
  --output data/worked_examples.json \
  --csv-output data/worked_examples.csv
```

## Cross-model calculated summary

| Model | Boundary / token | KV / token | TP sent / rank, decode Q=1 | TP sent / rank, prefill N=4096 | One-cut prefill N=4096 |
|---|---:|---:|---:|---:|---:|
| Llama 3.1 8B | 8 KiB | 128 KiB | 512 KiB | 2 GiB | 32 MiB |
| Llama 3.1 405B | 32 KiB | 504 KiB | 7.875 MiB | 31.5 GiB | 128 MiB |
| Mixtral 8x7B | 8 KiB | 128 KiB | 512 KiB | 2 GiB | 32 MiB |
| Qwen3-30B-A3B | 4 KiB | 96 KiB | 384 KiB | 1.5 GiB | 16 MiB |

**CALCULATED.** TP's per-rank payload is \(2L\) times the one-cut payload for the same forward tokens. The synchronization structures remain different and must be measured.
