# Upstream Monitoring Process

> **Wiki status:** Proposed · **Evidence state:** D0 — design only · **Last reviewed:** 2026-07-17  
> **Machine-validation status:** Not run. Missing measurements are `INSUFFICIENT_EVIDENCE`, never an implicit pass.


Machine-readable sources and queries are in [config/upstream-watch.yaml](../config/upstream-watch.yaml); the human-readable query inventory is in [Upstream Query Catalog](Upstream-Query-Catalog.md); freshness budgets are in [config/freshness-policy.yaml](../config/freshness-policy.yaml). The executable collectors are [tools/upstream_watch.py](../tools/upstream_watch.py) and [tools/check_upstream_freshness.py](../tools/check_upstream_freshness.py).

## Watched surfaces

1. `ggml-org/llama.cpp`: RPC, server, cache, tokenizer/model support, HIP/ROCm, Vulkan, benchmark, security, releases.
2. AMD ROCm: `ROCm/ROCm`, `ROCm/TheRock`, `ROCm/rocm-systems`, release/support matrices, Ryzen limitations, profiling, telemetry, product security.
3. Linux: amdgpu/KFD, USB4/Thunderbolt, `thunderbolt-net`, stable kernel fixes and regressions.
4. Mesa/LLVM: RADV/AMD GPU and `gfx1151` compiler/backend work when Vulkan or toolchain paths are in the SUT.
5. Engine/framework adapters: vLLM, PyTorch, or other selected runtime.
6. OEM: BIOS/AGESA, USB4 firmware, thermal/power firmware, and product advisories for the exact node model.

## Event workflow

`collect → normalize → deduplicate → classify → map to affected experiments → triage → canary → baseline decision → close`.

Each event stores source ID, immutable commit/tag/advisory ID, publication and discovery times, affected SUT layers, severity, confidence, owner, required canaries, decision, and evidence links.

## Triage severity

| Severity | Trigger | Required action |
|---|---|---|
| P0 | Exploited/critical security, data corruption, kernel/GPU reset, RPC protocol break | Same-day isolation/rollback analysis; block releases |
| P1 | Strix Halo/gfx1151, RPC, USB4, cache, model-load, correctness, or major performance fix/regression | Triage within 1 business day; targeted canary within 3 days |
| P2 | Relevant release, dependency, documentation, or telemetry change | Weekly review and next scheduled canary |
| P3 | Low-confidence/community report | Track; promote only with reproduction or upstream confirmation |

## Change-to-canary map

| Change surface | Minimum canaries |
|---|---|
| RPC protocol/device placement | EXP-001, 008, 009, 016, 017, 018 |
| Prompt/KV cache | EXP-004, 011, 014, correctness C2/C3 |
| HIP/ROCm/LLVM/AMDGPU | EXP-005, 006, 007, 012, 013, 019 |
| USB4/Thunderbolt/kernel | EXP-002, 007, 016, 017, 019 |
| Model/tokenizer support | EXP-005, 006, 014, 015 |
| Server scheduling/batching | EXP-007, 008, 009, 011 |
| Telemetry/profiler | Collector overhead check plus EXP-012/013 |

GitHub scheduled workflows use cron and may be delayed or disabled under documented conditions; freshness should therefore be verified from the ledger, not assumed from workflow configuration alone. [[SRC-022]](../references/Sources.md#src-022)
