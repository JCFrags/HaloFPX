---
section_id: "57"
title: "Compatibility fingerprint primary sources"
status: "draft"
last_verified: "2026-07-17"
applies_to:
  repositories: ["charlie12345/ROCmFPX", "fewtarius/CachyLLama", "ggml-org/llama.cpp"]
  software_versions: ["RFC 8949", "FIPS 180-4"]
  hardware_revisions: []
related_sections: ["13", "15", "26", "32", "56", "61", "63"]
---

<a id="s57-sources"></a>
# Compatibility fingerprint primary sources

Access date: 2026-07-17. Repository URLs are pinned to full commits.

| ID | Primary source and revision | Claims supported | Limitations |
|---|---|---|---|
| S57-01 | fewtarius/CachyLLama [`server-context-page-manager.cpp`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/tools/server/server-context-page-manager.cpp), commit `6be745998f568e379ea197fcf827baec73ff9940` | Exact FNV-1a inputs: `llama_model_desc()` and K/V type integers; mismatch between comment and implementation regarding build commit. | Static inspection; description contents can change and are not a byte identity. |
| S57-02 | CachyLLama [`kv-ssd-cache.h`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/common/kv-ssd-cache.h), [`kv-ssd-cache.cpp`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/common/kv-ssd-cache.cpp), and [`kv-ssd-system-cache.cpp`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/common/kv-ssd-system-cache.cpp), same commit | v3 record/version fields, 64-bit compatibility checks/rejections, unused chat-template hint. | Does not establish collision resistance, completeness, or runtime safety. |
| S57-03 | ggml-org/llama.cpp [`ggml/src/gguf.cpp`](https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/ggml/src/gguf.cpp) and [`src/llama-model-loader.cpp`](https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/src/llama-model-loader.cpp), commit `788e07dc91d266ad3162a1ce9037665656269689` | Implemented typed GGUF parsing, versions, tensor metadata/shards and loader identity surface. | Implementation is a moving source contract, not a HaloKV format guarantee. |
| S57-04 | llama.cpp [`src/llama-arch.h`](https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/src/llama-arch.h), [`src/llama-arch.cpp`](https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/src/llama-arch.cpp), and [`src/llama-context.cpp`](https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/src/llama-context.cpp), same commit | Architecture, tokenizer/chat-template, RoPE metadata keys and resolved runtime overrides. | Key presence alone does not define the final HaloFPX invalidation set. |
| S57-05 | llama.cpp [`include/llama.h`](https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/include/llama.h) and [`src/llama-context.cpp`](https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/src/llama-context.cpp), same commit | Sequence-state magic/version 2, flags, save/load and serialization entry points. | No cross-commit/backend semantic compatibility promise is inferred. |
| S57-06 | charlie12345/ROCmFPX [`ggml/include/ggml.h`](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/include/ggml.h), [`ggml/rocmfpx`](https://github.com/charlie12345/ROCmFPX/tree/a5605a72768c6562241b248e268e33dc92787394/ggml/rocmfpx), and [`ggml/rocmfp4`](https://github.com/charlie12345/ROCmFPX/tree/a5605a72768c6562241b248e268e33dc92787394/ggml/rocmfp4), commit `a5605a72768c6562241b248e268e33dc92787394` | Fork-local type IDs, block layouts and numeric/backend surface requiring identity. | Static source does not prove backend equivalence. |
| S57-07 | IETF [RFC 8949, CBOR](https://www.rfc-editor.org/rfc/rfc8949.html), December 2020, especially sections 4.2 and 5.4 | Core deterministic encoding requirements and evolution considerations. | HaloFPX must still define its own profile, schema, duplicate-key and limit policy. |
| S57-08 | NIST [FIPS 180-4, Secure Hash Standard](https://doi.org/10.6028/NIST.FIPS.180-4), August 2015 | SHA-256 algorithm and digest purpose. | NIST announced revision intent; hashing alone is not writer authentication. |

## Source conflict retained

**[VERIFIED]** The CachyLLama page-manager comment says to include a build commit, but the following statements hash only the model description and K/V type integers [S57-01]. HaloFPX must test generated manifests from code, not trust comments or log labels.
