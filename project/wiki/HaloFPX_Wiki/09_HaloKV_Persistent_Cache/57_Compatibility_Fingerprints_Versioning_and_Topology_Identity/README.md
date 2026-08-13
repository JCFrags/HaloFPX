---
section_id: "57"
title: "Compatibility Fingerprints, Versioning, and Topology Identity"
status: "needs-machine-validation"
last_verified: "2026-08-12"
applies_to:
  repositories: ["charlie12345/ROCmFPX", "fewtarius/CachyLLama", "ggml-org/llama.cpp"]
  software_versions: ["fingerprint schema candidate v1"]
  hardware_revisions: ["nimo-1 and nimo-2 Nimo Direct MME3L, Ryzen AI MAX+ 395/gfx1151; physical identity observed, exact per-run compatibility manifests pending"]
related_sections: ["03", "11", "13", "15", "39", "43", "47", "48", "56", "58", "59", "61", "63", "65"]
---

<a id="s57-overview"></a>
# Compatibility fingerprints, versioning, and topology identity

**[RECOMMENDATION]** HaloKV must accept persisted inference state only after validating both (1) a versioned, canonical compatibility manifest and (2) strong digests of every stored object. The manifest root is `SHA-256(domain || deterministic-CBOR(manifest))`; it is not a 64-bit shortcut, filename, model description, or software commit alone.

The manifest covers exact model/shard bytes, typed GGUF metadata, tokenizer and effective chat template, resolved architecture and RoPE parameters, per-tensor quantization, K/V types, backend/state ABIs, topology and shard ownership, and dirty-aware software builds. A mismatch is a cache miss/recomputation unless corruption or an impossible same-digest/different-manifest condition requires quarantine and operator attention.

**[VERIFIED]** The pinned CachyLLama implementation uses a 64-bit FNV-1a value over `llama_model_desc()` plus K/V cache type integers; its comments/logs overstate build-commit coverage, and its system-template hint is unused [S57-01][S57-02]. It is therefore evidence for a porting gap, not the HaloKV acceptance design.

## Retrieval map

- [Facts and constraints](facts_and_constraints.md#s57-facts)
- [Manifest and migration design](design_implications.md#s57-design)
- [Validation procedures](procedures_and_checks.md#s57-procedures)
- [Open questions](open_questions.md#s57-open)
- [Pinned primary sources](sources.md#s57-sources)

## Research split

- **Internet/source-code research complete now:** pinned cache comparison, llama.cpp GGUF/state identity surfaces, ROCmFPX format surface, deterministic encoding, and SHA-256 basis.
- **Machine work required:** inventory the effective manifests on both hosts; mutation, upgrade/rollback, corruption, collision-simulation, rank-remap, and exact-continuation tests.
- **Contingent decisions:** final manifest schema, backend ABI boundary, which state changes are migratable, device-specific invalidators, and whether a faster secondary content hash is justified.

No machine performance or compatibility result is claimed.
