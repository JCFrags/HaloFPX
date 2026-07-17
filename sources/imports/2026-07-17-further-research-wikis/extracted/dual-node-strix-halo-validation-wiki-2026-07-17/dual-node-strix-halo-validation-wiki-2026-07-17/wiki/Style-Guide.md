# LLM Wiki Style Guide

> **Wiki status:** Proposed · **Evidence state:** D0 — design only · **Last reviewed:** 2026-07-17  
> **Machine-validation status:** Not run. Missing measurements are `INSUFFICIENT_EVIDENCE`, never an implicit pass.


Every normative page uses five elements:

- A status and evidence banner.
- A decision or claim boundary near the top.
- Stable identifiers for metrics, experiments, gates, and sources.
- Tables for executable requirements; prose for rationale and caveats.
- Relative links to source and evidence records.

## Evidence labels

| Label | Meaning |
|---|---|
| `D0` | Design, protocol, or proposed threshold |
| `S0` | Synthetic parser/tool result; no machine evidence |
| `M1` | Measured on one pinned node |
| `M2` | Measured on the dual-node SUT |
| `R1` | Reproduced in an independent run block |
| `STABLE` | Signed decision after all mandatory gates |

Never use `PASS` for a missing measurement. Use `INSUFFICIENT_EVIDENCE`.
