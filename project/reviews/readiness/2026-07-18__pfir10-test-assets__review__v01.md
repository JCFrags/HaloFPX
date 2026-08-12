---
type: test-asset-review
status: partial-promotion
date: 2026-07-18
decision: OPEN-TEST-01
---

# PF-IR-10 test-asset qualification

## Verdict

Promote 52 exact CC0-1.0 static/structural fixtures as hash-pinned references.
Defer seven candidate-execution fixtures until the local implementation exists
and an isolated candidate-binary qualification is authorized. No candidate
binary was executed during this review.

The accepted machine-readable list is
`sources/test-assets/2026-07-18__pfir10-static-assets__accepted-manifest.json`.
Files remain in immutable intake and must be materialized later only after hash
verification.

## Qualification record

The imported verifier was run only in a disposable copy. Its detailed checks
passed for 27 applicability rows, 27 boundary references, zero third-party
packages, excluded-payload absence, 144 fixture-manifest rows, 59 generated
hashes, 4 valid and 6 malformed GGUF structures, 6 grammar cases, 21 JSON files,
94 JSONL records, 4 license evidence records, 7 schema cases, 18 source ranges,
5 SSE cases, and 18 tokenizer cases.

Four `adapter-executable` failures are Windows portability false positives:
`os.access(path, os.X_OK)` reports ordinary files, including a README, as
executable. The one reproduction mismatch is also isolated: the generator writes
`fixtures\\gguf\\...` on Windows while the preserved canonical JSON uses
`fixtures/gguf/...`. Source hash
`3C5046A766A6B62EF7120038F5998EC220000C1AB9D9C19B1497C110272D8256`
became regenerated hash
`75804B08CF46D2E21DBE6FBFDD257136B63B528A8EF7BDE5B4A78A93F1F527F6`.
These inherited defects are not silently patched in raw intake.

`OPEN-TEST-01` is closed for static reference assets and remains open for the
seven deferred candidate-execution assets.
