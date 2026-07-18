# Qualification records

These files distinguish internal package validation from candidate execution.

| File | Meaning |
|---|---|
| `generated-assets.json` | Generator hash and every generated fixture file hash |
| `gguf-structural-self-check.json` | Independent parser results for valid/malformed GGUFs |
| `tokenizer-reference-check.json` | Self-generated tokenizer vector check |
| `special-token-reference-check.json` | Special-token vector check |
| `json-schema-reference-check.json` | Schema fixture check |
| `sse-reference-check.json` | Streaming fixture check |
| `COMPARATOR-SELF-CHECK.json` | Comparator CLI smoke tests on self-authored data |
| `SELF-CHECK.json` | Integrated clean-regeneration and manifest-locator check |
| `EXECUTION-STATUS.json` | Explicit record that candidate execution did not occur |
| `PACKAGE-STATUS.json` | Deterministic archive settings, written during packaging |

A green self-check proves internal consistency of PF-IR-10. It does not prove that any candidate loads or accepts an asset.
