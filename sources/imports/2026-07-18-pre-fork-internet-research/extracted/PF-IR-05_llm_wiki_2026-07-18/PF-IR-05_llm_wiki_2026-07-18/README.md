# PF-IR-05 — Exact 200–230 GB model artifact shortlist and immutable identity

This folder is a self-contained LLM Wiki-style evidence package generated on 2026-07-18. Open `index.html` for the navigable view. Machine-readable identity, shard, source, claim, license, and cache-invalidation records are under `manifests/`.

## Decision result

Five candidates advance to local preflight: DeepSeek-V3.1, GLM-4.7, Qwen3.5-397B-A17B, NVIDIA Nemotron 3 Ultra, and DeepSeek-R1. MiniMax-M2.7 is held for license and conversion preflight.

Only GLM-4.7 Q4_K_M (`216,455,572,576` bytes) and DeepSeek-R1 Q2_K_XS (`221,253,686,944` bytes) have complete exact selected-shard byte/hash manifests in this capture. Other totals remain rounded observations with explicit missing fields.

## Non-claims

No model weight was downloaded. No candidate was loaded or executed. No ROCm/GPU test, fallback trace, long-context test, tool-use test, multimodal test, MTP/speculative test, throughput test, or quality evaluation was run. Final selection requires human workload judgment and target-machine qualification.

## Entry points

- `index.html` — LLM Wiki overview.
- `wiki/shortlist.md` — decision matrix.
- `wiki/quantization-options.md` / `manifests/quantization_options.json` — pinned converter option snapshots and rounded size classes.
- `manifests/model_identity.json` — immutable identity inputs.
- `manifests/cache_invalidation.json` — cache-key requirements.
- `manifests/source_to_claim.csv` — claim lineage.
- `validation/preflight_matrix.csv` — required local gates.
- `scripts/README.md` — fail-closed refresh and verification.
