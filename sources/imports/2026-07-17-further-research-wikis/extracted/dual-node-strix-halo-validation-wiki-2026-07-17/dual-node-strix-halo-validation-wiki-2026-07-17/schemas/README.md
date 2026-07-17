# Raw-Data Schemas

> **Wiki status:** Proposed · **Evidence state:** D0 — design only · **Last reviewed:** 2026-07-17  
> **Machine-validation status:** Not run. Missing measurements are `INSUFFICIENT_EVIDENCE`, never an implicit pass.



The schemas use JSON Schema 2020-12. Raw record streams are newline-delimited JSON (`.jsonl`), with one independently valid object per line. Manifests, summaries, and decisions are ordinary JSON documents.

| Schema | Record granularity | Canonical location |
|---|---|---|
| `run-manifest.schema.json` | One run | `raw-data/<run-id>/manifest.json` |
| `request-trace.schema.json` | One request | `requests.jsonl` |
| `token-event.schema.json` | One stream event/token | `tokens.jsonl` |
| `telemetry-sample.schema.json` | One node/sample time | `telemetry/<node>.jsonl` |
| `fault-event.schema.json` | One injection | `faults.jsonl` |
| `correctness-record.schema.json` | One test/request judgment | `correctness.jsonl` |
| `summary.schema.json` | One candidate/stage aggregate | `summary.json` |
| `release-decision.schema.json` | One signed decision | `release-decision.json` |
| `upstream-event.schema.json` | One normalized upstream event | `upstream/events.jsonl` |
| `baseline-record.schema.json` | One immutable promoted baseline | `baselines/<id>.json` |

## Data rules

1. Durations use monotonic clocks. UTC timestamps support correlation and audit only.
2. Raw token text may contain sensitive data; store token IDs or a salted/controlled hash where retention policy forbids plaintext.
3. Never overwrite a raw file. Corrections are new files plus an adjudication record.
4. Every file is named in `manifest.json` with byte count and SHA-256.
5. Missing fields are represented explicitly as `null` only where the schema permits; zero is never a missing-value sentinel.
6. Derived metrics retain input run IDs, code commit, and calculation timestamp.
