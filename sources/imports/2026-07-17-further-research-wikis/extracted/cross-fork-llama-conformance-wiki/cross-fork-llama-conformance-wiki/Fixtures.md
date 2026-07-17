# Fixtures

## Manifest

[`fixtures/manifest.json`](fixtures/manifest.json) contains 161 logical fixture IDs referenced by the matrix. Each entry declares:

- kind and logical ID;
- whether it is included, downloaded, generated, operator-supplied, or a recipe;
- local recipe/input path;
- SHA-256 when the included artifact or external object is resolved;
- source and licensing notes.

The YAML mirror is [`fixtures/manifest.yaml`](fixtures/manifest.yaml).

## Model policy

Large models are not bundled. [`fixtures/models/model-manifest.json`](fixtures/models/model-manifest.json) includes one concrete upstream test pin:

| Model ID | Use | SHA-256 |
|---|---|---|
| `model.tiny-stories15m-q4_0` | smoke, server, state, thread-safety | `66967fbece6dbe97886593fdbb73589584927e29119ec31f08090732d1861739` |

That digest is sourced from the upstream CMake test fixture at the research snapshot. Other models remain unresolved because architecture, license, MTP availability, long-context configuration, and hardware needs must be selected by the program owners. A run cannot use an unresolved model ID.

Create a local lock:

```bash
python3 scripts/build-fixture-manifest.py   --model model.tiny-stories15m-q4_0=/models/stories15M-q4_0.gguf   --output reports/locks/models.json
```

## Included input fixtures

| Directory | Contents |
|---|---|
| `fixtures/tokenizer/` | Unicode, whitespace, special-token, and invalid API inputs |
| `fixtures/chat/` | Messages, tools, an authored Jinja template, and malformed templates |
| `fixtures/prompts/` | Core prompts and token-counted long-context specifications |
| `fixtures/sampling/` | Synthetic probability inputs and a reproducible seed schedule |
| `fixtures/determinism/` | Controls that define a deterministic lane |
| `fixtures/api/` | Native/OpenAI/Responses/Anthropic request corpus and normalization rules |
| `fixtures/quant/` | Symbolic boundary-shape and fallback matrices |
| `fixtures/cache/` | Conversation scenarios, page transitions, corruption and storage failure recipes |
| `fixtures/gguf/` | Byte-level mutation recipes and a safe mutator |
| `fixtures/rpc/` | Loopback-only topology and failure cases |
| `fixtures/failure/` | Global fault-injection safety and normalized error classes |
| `fixtures/harness/` | Watchdog policy |

## Expected-output policy

Input fixtures and structural expectations are version-controlled. Model-dependent outputs are not prefilled.

- Token vectors are promoted per vocabulary/model digest.
- Rendered output for the suite-authored standalone Jinja template may be exact because the fixture itself defines the computation.
- Generated model text, logits, embeddings, dequantized vectors, quality metrics, stochastic frequencies, and performance values are reference artifacts.
- Native upstream tests retain their own source-level expected constants; the outer suite records native pass/fail and source/binary digests rather than transcribing them.

## GGUF negative fixtures

Use `fixtures/gguf/mutate.py` only for the fixed header, truncation, and explicit byte flips. Use upstream `tests/test-gguf.cpp` for structure-aware malformed KV and tensor descriptors. Guessing variable-length offsets creates invalid tests that can accidentally exercise the wrong parser branch.

## Long-context fixtures

`generate-long-context.py` targets token counts through the tested `/tokenize` endpoint. It does not assume a character-to-token ratio. If a whole-unit corpus cannot hit an exact count, extend the generator with a vocabulary-specific suffix search and promote the resulting text and token IDs.

`generate-passkey.py` inserts a deterministic nonce. The exact nonce is the oracle; fluent surrounding text is not.

## Fixture change control

A fixture change invalidates references that list its old digest. Preserve old fixture versions while approved references depend on them. Never replace an external object at the same logical ID without changing its manifest digest and reference lineage.
