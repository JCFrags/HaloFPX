# Fixtures, models, tokenizers, templates, schemas, and corpora

## Models and quantized GGUFs

ROCmFPX fixture scripts refer to maintainer-local Qwen and DeepSeek paths and generate derived GGUFs. They do not record publisher revision, source license, file hash, quantizer identity, or permission. Current publisher pages can narrow candidate families but cannot prove the local bytes.

- Current Qwen3-0.6B page: Apache-2.0. Local `Q4_K_M` producer/revision remains unknown.
- Current DeepSeek-V4-Flash page: MIT, but current model size conflicts with the local `180B` path.
- No model or quantized fixture is approved for promotion by this dossier.

## Tokenizers and generated vocabulary fixtures

Tokenizer tests and conversion scripts consume external tokenizer repositories and generate `ggml-vocab-*.gguf`. Required record:

`publisher repo → exact revision → source files/hashes → source license/notice → generator blob/version/command → output hash → proposed release path`.

## Chat templates

The extraction script pulls `chat_template` from mutable `main` `tokenizer_config.json` files. Checked-in Jinja output must be treated as publisher-specific expression until the exact source revision and terms are captured. Tencent Hy3 is specifically version-sensitive because current Hy3 and an earlier preview used different terms.

## Schemas and grammars

Implementation ideas, protocol facts, field names required for interoperability, and independently authored tests should be recorded separately from copied source/schema/prose expression. Any copied JSON Schema, metaschema, tutorial, or fixture needs exact source and license evidence.

## Project Gutenberg corpus

The benchmark script downloads ebook 1184 at runtime. Project Gutenberg's public-domain and trademark/terms framework is jurisdiction-sensitive. Reference the fixture or independently reacquire and hash the exact edition; do not promote an unidentified cache.
