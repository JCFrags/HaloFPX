# Cross-Fork llama.cpp Conformance Wiki

**Research snapshot:** 2026-07-17 · **Suite version:** 0.1.0 · **Matrix:** 175 cases · **Failure injection:** 55 cases

This folder is a design and runnable scaffold for comparing:

- upstream `ggml-org/llama.cpp`;
- `charlie12345/ROCmFPX`;
- the requested `llama-ai/CachyLlama` identity, represented provisionally by the public candidate `fewtarius/CachyLLama`;
- a future integration fork that must be pinned before execution.

Start at **[Home](Home.md)**. The complete matrix is available as [Markdown](Test-Matrix.md), [CSV](matrix/test-matrix.csv), [JSON](matrix/test-matrix.json), and [YAML](matrix/test-matrix.yaml).

> [!IMPORTANT]
> No model-specific numeric result or threshold is fabricated. Exact structural or token equality is declared where appropriate. Logit, embedding, quantized-kernel, quality, performance, and distributional gates remain uncalibrated until promoted from pinned reference runs.

## What is included

- GitHub-Wiki-compatible `Home.md`, `_Sidebar.md`, and `_Footer.md`;
- a 175-case cross-fork test matrix with applicability and CI tiers;
- pinned source inventory and an upstream reuse map;
- included input fixtures, failure recipes, model-lock policy, and schemas;
- a standard-library Python comparator harness;
- reference-recording and tolerance-calibration tools that cannot auto-approve;
- CI workflow examples for CPU, GPU, cache, RPC, statistical, and failure lanes;
- suite-integrity checks and a local runbook.

## Fast validation

```bash
python3 scripts/verify-suite.py
python3 scripts/verify-package-manifest.py
./scripts/run-harness-tests.sh
```

The test wrapper imports the harness directly from `harness/src`; only `pytest` must be available. An editable install remains optional for using the `llama-conformance` console command:

```bash
python3 -m pip install --no-build-isolation -e ./harness
```

The suite is a conformance specification and harness scaffold. It does not bundle large model binaries or claim that unresolved fork/model identities are release-ready.
