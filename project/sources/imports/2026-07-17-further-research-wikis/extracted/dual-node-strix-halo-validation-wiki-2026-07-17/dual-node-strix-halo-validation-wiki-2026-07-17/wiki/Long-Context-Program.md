# Long-Context Program

> **Wiki status:** Proposed · **Evidence state:** D0 — design only · **Last reviewed:** 2026-07-17  
> **Machine-validation status:** Not run. Missing measurements are `INSUFFICIENT_EVIDENCE`, never an implicit pass.


## Sweep

Tokenize prompts before the run and test exact lengths at powers of two from 1K through the lesser of model-declared limit and configured maximum: `1K, 2K, 4K, 8K, 16K, 32K, 64K, 128K, ...`. Add the production p50/p95 lengths even when they are not powers of two.

For each length, run:

- Prefill-only or one-token completion to isolate context ingestion.
- Fixed 128-token decode to expose growing-KV decode cost.
- Multi-turn append workload with 10%, 25%, and 50% new suffixes.
- Retrieval/needle canaries at 10%, 50%, and 90% of the context position.
- Cache-on/cache-off variants where prefix reuse is part of production.

## Required outputs

- Client and server TTFT, prefill tokens/sec, decode ITL/TPOT, E2E latency.
- Per-node memory/KV allocation, minimum headroom, swap, faults, PSI.
- USB4 bytes and one-second utilization; disk reads/writes.
- Output correctness and structured-output validity.
- Failure mode at and beyond the supported boundary.

## Stable-context declaration

The **maximum validated context** is the largest length that passes correctness, latency, memory, thermal, and 30-minute sustained-repeat gates with at least `max(8 GiB, 10% of physical memory)` available on each node and no swap-in. A larger length that merely completes once is not stable.

## Regression model

Store prefill latency per 1K input tokens and decode ITL versus context length. Flag non-monotonic discontinuities, slope changes above 15% from the prior baseline, and context-specific link saturation. Do not reduce the test to a single maximum-length point.
