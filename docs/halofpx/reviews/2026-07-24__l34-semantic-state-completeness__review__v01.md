# L34 semantic-state completeness adversarial review

Date: 2026-07-24

Verdict: **PASS**

The reviewer audited the final source, corrected-source fixture evidence,
receipt, production evidence, and cleanup.

## Accepted findings

- The normal harness structurally decodes indices 0 through 1,127, captures
  the 1,128-token boundary, and replays final prompt token index 1,128 exactly
  once on both capture and restore before sampling.
- Semantic provenance is default-off, closed at both runner and binary
  boundaries, canonical, HMAC-SHA256 authenticated with channel authority,
  remotely verified, uniquely parsed, retained in `result.json`, and
  fail-closed.
- The corrected-source normal fixture run has one capture and one restore
  record, positions 1127 to 1128, 32,000 identical logits with SHA-256
  `f6d0fa35238815d19f10cb97a0af1c75349080fa431064a25a4969f8d9b177b1`,
  and argmax/sample token 4245.
- Corrected-source zero, twice, and restore-invalidation cases retain
  authenticated records and exhibit the documented refusal behavior.
- Focused tests pass 42/42 and recorded source identities match the receipt.
- Production remained unchanged and healthy; cleanup removed the disposable
  namespace and port.

The reviewer independently recomputed the raw evidence as 283 files,
33,457,287 bytes, canonical relative-path-plus-NUL-plus-content SHA-256
`fe3b62535d4d4083addea2cfa65a9f2a26e9118079b909841f4f8e7821bb7e1b`,
matching the receipt.

No source-backed primary root cause is claimed. The separately authorized
primary logits discriminator remains the smallest next experiment.
