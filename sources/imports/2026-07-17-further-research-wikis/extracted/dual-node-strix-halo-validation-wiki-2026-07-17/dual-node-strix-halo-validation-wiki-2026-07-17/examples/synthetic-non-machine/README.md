# Synthetic, Non-Machine Examples

> **Evidence state:** S0 — synthetic tool check only  
> **Machine-validation status:** Not run. These records are invented and must evaluate to `INSUFFICIENT_EVIDENCE` for G2–G4.

The files exercise schemas, aggregation, hashing, and gate logic. Numerically plausible values are deliberate test fixtures; they are not observations from Strix Halo, USB4, ROCm, llama.cpp, or any physical machine.

Expected invariant: `python tools/evaluate_gates.py examples/synthetic-non-machine/summary.json --stage G4` returns `INSUFFICIENT_EVIDENCE` because `evidence_origin=synthetic` and `evidence_level=S0`, even though the invented metric values look passing.
