# Validation artifacts

This directory records the reproducible checks executed for the research package on **2026-07-17**.

- `build-report.json` is the machine-readable summary, including tool versions, input hashes, checked invariants, state counts, and explicit scope limitations.
- `deep-validation-output.txt` is the complete aggregate run of `scripts/deep-validate.py`.
- `sany-output.txt` records the standalone TLA+ parser and semantic-analysis run.
- `tlc-output.txt` records the standalone exhaustive TLC run for `formal/tla/HaloKV.cfg`.

The `tla2tools.jar` binary is intentionally not bundled. Its SHA-256 digest and tool revision are recorded in `build-report.json`; obtain a trusted TLA+ tools release independently before reproducing the run.

A passing finite model is evidence about the modeled control-state abstraction, not proof of a production implementation, storage backend, GPU runtime, transport stack, or cryptographic implementation. The P file remains an uncompiled research starter, as stated in the report.
