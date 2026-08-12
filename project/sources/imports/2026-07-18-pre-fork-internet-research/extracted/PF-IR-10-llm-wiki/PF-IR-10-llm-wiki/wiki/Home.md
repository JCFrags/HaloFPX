# PF-IR-10 LLM Wiki

> **P0 corpus proposal**  
> `SELF-GENERATED` · `VERIFIED-SOURCE` · `QUALIFICATION-REQUIRED`

PF-IR-10 supplies a minimal, redistributable semantic-conformance corpus without bundling third-party model weights. The generated assets are byte-stable and the self-check reproduces them in a clean directory.

## Snapshot

| Item | Result |
|---|---:|
| Logical fixture records | 144 |
| Applicability proposals | 27 |
| Structurally valid GGUFs | 4 |
| Deterministically malformed GGUFs | 6 |
| Streaming trace scenarios | 5 |
| Candidate binaries executed | 0 |
| Exact candidate identities pinned | 3 |
| Unresolved candidate identities | 1 (`HaloFPX`) |

## What is actually bundled

- A 2×2 F32 GGUF tensor/container probe.
- A vocab-only byte-BPE GGUF with default, `strict`, and `tools` named chat templates.
- A one-layer F32 Llama-shaped GGUF generated from exact integer-defined bytes; candidate loading remains unqualified.
- Six malformed GGUF boundary derivatives.
- Tokenizer, special-token, chat, tool/schema, grammar, SSE, malformed JSON/UTF-8, sampler, RNG, save/restore, recurrent, speculative, and fork-specific records.
- Comparator profiles and executable, candidate-independent reference validators.
- Exact MIT/CC0 license evidence, pinned source/commit locators, and selected literal upstream source-range captures with local hashes.

## What is not bundled

- `ggml-org/vocabs` binaries.
- `stories15M-q4_0.gguf` or its big-endian derivative.
- Publisher MTP, EAGLE, DFlash, or external draft-model weights.
- Any candidate build or output.

See [Decision](01-Decision.md) and [Licensing](03-Licensing.md).
