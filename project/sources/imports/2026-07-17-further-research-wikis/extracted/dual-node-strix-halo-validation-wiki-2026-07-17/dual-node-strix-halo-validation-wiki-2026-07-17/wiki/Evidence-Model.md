# Evidence Model and Claim Discipline

> **Wiki status:** Proposed · **Evidence state:** D0 — design only · **Last reviewed:** 2026-07-17  
> **Machine-validation status:** Not run. Missing measurements are `INSUFFICIENT_EVIDENCE`, never an implicit pass.


## Evidence classes

| Class | Minimum contents | Permitted claim |
|---|---|---|
| D0 | Reviewed method, schema, gate, and owner | “Designed” |
| S0 | Synthetic data with tool output | “Tool path exercised” |
| M1 | Single-node raw data, manifest, hashes, timestamps | “Measured on Node X” |
| M2 | Dual-node raw data plus link and both node telemetry | “Measured on the dual-node SUT” |
| R1 | Independent run block on a different day or fresh boot set | “Reproduced” |
| STABLE | Mandatory gates pass, decision signed, exceptions bounded | “Stable for the declared profile” |

## Claim packet

Every published number or release claim must resolve to:

`claim → summary metric → aggregation code/version → raw records → run manifest → model/data/software/hardware hashes`.

A screenshot alone is not evidence. A dashboard without immutable raw exports is not evidence. A README command that starts successfully is integration evidence, not stable-operation evidence.

## Missing and invalid data

- Missing required metric: `INSUFFICIENT_EVIDENCE`.
- Instrumentation failure: run is retained and marked `INVALID_INSTRUMENTATION`.
- Environmental contamination under a predeclared rule: `INVALID_ENVIRONMENT`, with telemetry proving the condition.
- Operator error: `INVALID_PROCEDURE`, with deviation record.
- Poor performance or a crash is a valid failing result, not an invalid run.
