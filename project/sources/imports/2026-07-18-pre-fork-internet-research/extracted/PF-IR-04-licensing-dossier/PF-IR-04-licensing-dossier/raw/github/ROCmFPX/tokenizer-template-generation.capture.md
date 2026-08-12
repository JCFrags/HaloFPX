# ROCmFPX tokenizer and chat-template generation paths

capture_method: GitHub/public-web connector capture
byte_fidelity: line-normalized authoritative connector capture
claim_label: PRIMARY-GENERATION-RECIPE
source_url: https://github.com/charlie12345/ROCmFPX/tree/61f2f2d7bc4955e9bca821095ef69125837133b5
repository: charlie12345/ROCmFPX
commit_or_revision: 61f2f2d7bc4955e9bca821095ef69125837133b5
repository_path: convert_hf_to_gguf_update.py; scripts/get_chat_template.py; tests/test-tokenizer-0.sh; models/templates
git_blob_sha1: n/a
access_date: 2026-07-18

---

- `tests/test-tokenizer-0.sh`, blob `7024b00afe341eb8d8411085c8a7db963e25a15c`, consumes external tokenizer directories and generated `ggml-vocab-*.gguf` files.
- `convert_hf_to_gguf_update.py`, blob `8d73b1f5546abf3b04bf3af85449513b941a5e4a`, retrieves many Hugging Face tokenizers and generates vocabulary fixtures without a release-time publisher revision record.
- `scripts/get_chat_template.py`, blob `b4827b317e1c934b359388a578b534f114bdc282`, fetches `tokenizer_config.json` from a repository's mutable `main` branch and extracts `chat_template`.
- `models/templates/README.md`, blob `3a649b8f4dbd9099e3a6ad49793b88f13530fa26`, documents checked-in templates; `models/templates/tencent-Hy3.jinja` is blob `f6185ccbfc00d25b4aaa7caeb26b3db6f4466d3b` and has no source/license header.

A generated container does not erase obligations attached to source tokenizer/template expression. Pin source revision, capture source license, hash the source and result, and preserve attribution.
