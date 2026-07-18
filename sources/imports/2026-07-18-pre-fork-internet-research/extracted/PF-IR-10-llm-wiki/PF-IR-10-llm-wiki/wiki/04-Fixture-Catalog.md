# Fixture catalog

The normative catalog is `manifests/fixtures.jsonl`. It contains 144 logical records. Locators are one of:

- whole immutable file;
- JSONL line plus record SHA-256;
- GGUF byte range plus range SHA-256.

## Core GGUF assets

| File | Purpose | Status |
|---|---|---|
| `pfir10-tiny-tensor-v3.gguf` | GGUF v3 header, metadata, alignment, one 2×2 F32 tensor | structural |
| `pfir10-byte-bpe-chat-v3.gguf` | 263-token byte BPE, special IDs, default/named templates | structural/tokenizer |
| `pfir10-tiny-llama-f32-v3.gguf` | one-layer, 8-dim, 263-vocab F32 model | `QUALIFICATION-REQUIRED` |
| `rocmfpx-turbo4-type106-probe.gguf` | custom type-ID 106 dispatch/header probe | `QUALIFICATION-REQUIRED`; not a numerical encoding claim |

## Coverage map

| Area | Representative paths |
|---|---|
| Tokenizer/special tokens | `fixtures/tokenizer/` |
| Default/named templates | `fixtures/chat/`; GGUF metadata byte ranges |
| Structured output/grammar | `fixtures/structured/` |
| Tool calls/API requests | `fixtures/api/requests/`; `fixtures/api/expected/` |
| Streaming traces | `fixtures/api/streaming/` |
| Malformed boundaries | `fixtures/gguf/malformed/`; `fixtures/malformed/` |
| Sampler/RNG/grammar state | `fixtures/sampler/`; grammar-state JSONL |
| Save/restore/recurrent/speculative | `fixtures/state/` |
| ROCmFPX-specific | `fixtures/fork-specific/` |

## Important distinction

Static API expected responses and SSE traces are protocol fixtures. They are not evidence that the generated random-weight model will autonomously produce those semantic answers. Live model-backed tool-call generation remains `open`.
