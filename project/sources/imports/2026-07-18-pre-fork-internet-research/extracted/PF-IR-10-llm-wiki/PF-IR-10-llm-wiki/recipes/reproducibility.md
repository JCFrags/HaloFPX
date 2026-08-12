# Deterministic generation contract

**Claim labels:** `SELF-GENERATED`, `VERIFIED-SOURCE`.

`recipes/generate_assets.py` is the sole generator for accepted binary and record fixtures. It uses Python standard-library operations only, fixed key ordering, LF newlines, little-endian packing, SplitMix64, and F32 values chosen from exact powers of two. No random device, locale-sensitive formatting, external model, tokenizer package, Jinja engine, or candidate binary is used.

Run from the corpus root:

```text
python3 recipes/generate_assets.py --root .
python3 recipes/verify_gguf.py fixtures/gguf/*.gguf fixtures/fork-specific/*.gguf fixtures/gguf/malformed/*.gguf
python3 recipes/verify_corpus.py --root .
```

Reproduction is accepted only when every generated path has the same SHA-256 as `qualification/generated-assets.json` and `MANIFEST.sha256` verifies.
