# Qwen3-0.6B pure ROCmFPX portable-fixture evidence

Tracking: [issue #43](https://github.com/JCFrags/HaloFPX/issues/43)

## Result

**[VERIFIED]** The immutable source, license/provenance record, and derived
artifact identities are exact. **[MEASURED]** The off-target WSL2 run converted
the source into pure Q3, Q6, and Q8 ROCmFPX artifacts and passed a bounded
pinned-b77 CPU load/tensor-check/four-token smoke. A Q3 repeat was recorded as
identical after validation; its duplicate payload was then removed.

This evidence does not establish model quality, target backend correctness, or
inference performance. No target machine was contacted. HIP, Vulkan, single-
node Strix Halo, dual-node distribution, quality gates, and matched performance
measurements all remain **[OPEN]**.

## Authority chain

1. [`source-metadata.json`](source-metadata.json) records the exact source,
   pinned distribution metadata, license declaration, and retained captures.
2. [`build-receipt.txt`](build-receipt.txt) records exact source commits,
   build configuration, pinned-b77 conversion failure, and the narrowly
   pinned compatible producer.
3. [`derived-manifest.json`](derived-manifest.json) records exact output bytes
   and GGUF tensor census.
4. [`smoke-summary.json`](smoke-summary.json) records the pinned-b77 CPU
   consumer and bounded acceptance outcome.
5. [`publication-preflight.json`](publication-preflight.json) records the
   repository/permission observation, official host-limit sources, exact asset
   sizes, and deliberate no-upload state.
6. The portable contract and executable procedure are in
   [`../../fixtures/qwen3-0.6b-rocmfpx/`](../../fixtures/qwen3-0.6b-rocmfpx/)
   and [`scripts/materialize-rocmfpx-fixture.py`](../../../../scripts/materialize-rocmfpx-fixture.py).

## External retained evidence

The canonical workstation artifact root during this off-target run was:

```text
C:\Users\britt\Documents\HaloFPX_ROCmFPX_Fixture_Artifacts_20260812
```

That directory retains source bytes, derived GGUFs, build/quantization/smoke
logs, dynamic Hub API captures, and failed partial headers. It is workstation
evidence, not a portable path or Git authority. The registry and recipe can
rebuild its necessary contents under any caller-selected external root.

The large files are deliberately absent from Git and have not been uploaded as
release assets. [GitHub blocks ordinary Git files larger than 100 MiB](https://docs.github.com/en/repositories/working-with-files/managing-large-files/about-large-files-on-github).
Each candidate is below [GitHub's 2 GiB per-release-asset limit](https://docs.github.com/en/repositories/releasing-projects-on-github/about-releases),
and the checked repository identity reported sufficient permissions on
2026-08-12. Limits, permissions, license/provenance accompaniment, and the
immutable release manifest must still be rechecked immediately before any
publication.

## Warnings retained without promotion

- Exact `main` at `b77f2bce...` builds the CPU tools on Linux/WSL but its
  quantizer rejects this external GGUF at the loader-owned source-offset gate.
  The parent of the enforcing change is pinned only as the artifact producer.
- The loader warns that token `128247` (`</s>`) looks control-like but is not
  control-typed and overrides it during the smoke. The smoke still exits zero;
  this warning is not a tokenizer-quality acceptance result.
- Quantization and smoke elapsed times are intentionally excluded from claims.
  Quantizations ran concurrently and all tests were off target.
