# PF-IR-08 — RCCL/two-host collective and network-plugin support boundary

Offline LLM-Wiki evidence package generated on **2026-07-18**.

Open [`index.html`](index.html) for the styled wiki. Start with [`01-executive-decision.md`](01-executive-decision.md) for the decision boundary and [`10-local-experiment-matrix.md`](10-local-experiment-matrix.md) for the test plan.

## Result in one paragraph

RCCL is a defensible **gated experiment candidate** and stock Socket baseline, not established support for the target pair. Active source is RCCL 2.30.4 at monorepo head `801a9ca2ad8940ac7cd7d571163e003f3a3d6cab`; ROCm 7.2.x maps to RCCL 2.27.7 at `96a25b5fd6f73fba58c7d83eb57cf19a50230aa4`. Stock Socket is host-pointer-only and has no DMA-BUF registration callback, so this package makes no GPU-direct-over-USB4 claim. Upstream gfx1151 enablement and later Ethernet-switch testing are relevant, but only the exact two-host Ethernet-over-USB4 experiment can establish suitability.

## Integrity

Run:

```bash
python3 scripts/verify_hashes.py
```

The verifier checks every entry in `manifests/SHA256SUMS`. Source provenance is in `manifests/source_manifest.csv`.

## Preservation caveat

This is a targeted, commit-pinned research snapshot rather than a complete Git repository mirror. Every partial source or discussion snapshot is labeled as partial; no truncated record is represented as complete.

## Byte-exact preservation set

`evidence/raw/` contains eight complete upstream files whose recomputed Git blob identifiers exactly match the pinned upstream blob SHAs. The set includes the active RCCL 2.30.4 fault-tolerance document, active v12 network-plugin headers, active and stable version files, active and stable plugin top-level headers, and both licenses. See `manifests/raw_evidence_manifest.csv`.

Larger implementation files are retained as claim-scoped excerpts with explicit completeness labels; this package is not a full repository mirror.
