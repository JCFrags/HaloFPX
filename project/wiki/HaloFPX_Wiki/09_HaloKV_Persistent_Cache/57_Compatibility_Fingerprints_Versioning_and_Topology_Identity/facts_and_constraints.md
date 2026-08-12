---
section_id: "57"
title: "Fingerprint facts and compatibility constraints"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: ["charlie12345/ROCmFPX", "fewtarius/CachyLLama", "ggml-org/llama.cpp"]
  software_versions: ["CachyLLama@6be745998f568e379ea197fcf827baec73ff9940", "llama.cpp@788e07dc91d266ad3162a1ce9037665656269689", "ROCmFPX@a5605a72768c6562241b248e268e33dc92787394"]
  hardware_revisions: ["dual gfx1151 Strix Halo; exact hosts pending inventory"]
related_sections: ["13", "15", "32", "39", "43", "48", "56", "58", "61", "63"]
---

<a id="s57-facts"></a>
# Fingerprint facts and compatibility constraints

| Fact or constraint | Consequence for HaloKV |
|---|---|
| **[VERIFIED]** CachyLLama v3 records hold a `uint64_t compat_hash`; the producer computes FNV-1a over `llama_model_desc()` and K/V type integers [S57-01][S57-02]. | Reject this value as the sole trust gate. It omits exact weights, tokenizer/template, resolved runtime values, topology, state ABI, and an actually appended commit. |
| **[VERIFIED]** The pinned system-cache source accepts a template hint but explicitly leaves it unused [S57-02]. | Hash the effective template bytes after every override; do not infer template identity from a model directory. |
| **[VERIFIED]** GGUF is typed metadata plus named, shaped, typed tensors; llama.cpp maps architecture, tokenizer, chat-template, and multiple RoPE keys explicitly [S57-03][S57-04]. | Preserve key name, GGUF value type, value bytes, array order, tensor name/shape/type/offset, and shard order. A display summary is insufficient. |
| **[VERIFIED]** The pinned llama.cpp sequence-state file has its own magic and version (`LLAMA_STATE_SEQ_VERSION 2`) [S57-05]. | State schema/version and producer ABI belong in the manifest; successful deserialization is not proof of semantic equivalence. |
| **[VERIFIED]** ROCmFPX adds fork-local numeric types, layouts, recipes, and backend implementations [S57-06]. | Fingerprint numeric type ID plus symbolic name, block-layout/recipe revision, per-tensor type map, and backend implementation/build. |
| **[VERIFIED]** RFC 8949 deterministic CBOR constrains shortest encodings, definite lengths, and map-key ordering; general CBOR otherwise permits multiple encodings [S57-07]. | Define one exact canonical byte representation and reject duplicate keys, indefinite lengths, non-minimal encodings, NaNs, and unknown critical fields. |
| **[VERIFIED]** FIPS 180-4 specifies SHA-256 as a secure hash algorithm producing message digests [S57-08]. | Use a full 256-bit root and per-object digests. Do not truncate acceptance digests to 64 bits. |

## Complete compatibility input set

**[RECOMMENDATION]** The acceptance manifest must contain these typed fields; each component also gets its own digest for diagnostics.

| Component | Required identity |
|---|---|
| Envelope | schema ID; major/minor; canonicalization profile; hash algorithm; domain string; critical-field list |
| Model bytes | ordered shard count; each shard length and SHA-256; whole logical model digest; adapters/projectors and their order |
| GGUF/model | GGUF version; typed metadata digest; tensor inventory digest; architecture; exact dimensions; model-data alignment; split metadata |
| Tokenization | tokenizer model/pre-tokenizer; exact token byte table, scores, merges, types, special-token IDs/flags; external sidecar digests |
| Prompt rendering | effective chat-template bytes and template-engine/version; all runtime overrides and special-token policy |
| Position/state math | resolved RoPE type, dimension sections/count, base, scale/scaling type/factor, original context, YaRN/LongRoPE values; context length and shift policy |
| Numeric layout | per-tensor GGML/ROCmFPX type ID/name/shape; quant block-layout and recipe revision; K and V cache types; recurrent/state element types |
| Runtime/ABI | HaloKV object/state schema; llama sequence-state magic/version/flags; backend protocol/ABI; backend library and kernel/shader digests; build options/toolchain target |
| Software | repository full commit IDs; submodule/dependency lock digests; dirty flag plus deterministic patch/untracked-input digest; binary/library SHA-256 |
| Topology | execution mode; plan schema/digest; rank count and logical IDs; replica/collective groups; tensor/layer/expert half-open shard ranges and owner; pipeline order; transport protocol |

**[INFERENCE]** Hardware serial numbers, hostnames, paths, timestamps, cache location, and performance knobs should be recorded in an operational provenance manifest but should invalidate cache only when a demonstrated semantic/ABI dependency promotes them into the compatibility manifest. Over-fingerprinting harmless identity destroys reuse; under-fingerprinting correctness inputs permits false acceptance.

## Security and collision boundary

- **[RECOMMENDATION]** Hash the bytes that are actually loaded, not a remote model name or expected checksum.
- **[RECOMMENDATION]** Use `SHA-256("halofpx.compat.v1\0" || deterministic_cbor)` for the root, and distinct domains for component and object digests. Store the canonical manifest beside the root.
- **[RECOMMENDATION]** Validate object length and digest before deserialization, then exact manifest equality and semantic invariants. If equal roots accompany unequal canonical manifests, treat this as a fatal integrity/canonicalizer defect; quarantine both.
- **[VERIFIED]** A digest detects changes but does not authenticate who wrote them [S57-08]. **[RECOMMENDATION]** If cache writers are not equally trusted, add authenticated storage/signatures; do not describe SHA-256 alone as authorization.
