# Glossary

**AEAD** — Authenticated encryption with associated data; provides confidentiality and integrity for ciphertext plus selected metadata.

**Atomic visibility** — Readers see either the old complete name/object or the new complete name/object, not an intermediate byte sequence. Rename can provide this on one filesystem.

**Durable commit** — After acknowledged success, data and the directory/manifest reference survive the required crash/power-loss model. Requires checked synchronization in addition to atomic visibility.

**Cache key** — Semantic address derived from namespace, compatibility fingerprint, prompt root and reuse boundary.

**CATALOG_ENTRY_VALID** — Offline result meaning a keyed-authenticated manifest and immutable object are internally consistent, but the validator lacks one or more authorized current-request bindings; not eligible for engine import.

**Compatibility fingerprint** — Cryptographic digest of every model/runtime/input-interpretation field required for byte-compatible state reuse.

**Content-addressed object** — Immutable object named by a cryptographic digest of its complete bytes.

**FNV-1a** — Fast non-cryptographic hash used by CachyLLama for token, conversation, user and compatibility identities. It is not a checksum or adversarial collision defense.

**Generation** — Monotonic manifest revision used for compare-and-swap and rollback detection.

**Hot/warm/cold** — CachyLLama tiers: active RAM, recent RAM, and SSD-only checkpoint.

**HIT_VERIFIED** — Integrated cache result emitted only after authorization, all outer validation, and complete transactional engine import.

**IMPORT_CANDIDATE_VALID** — Offline result for a catalog entry whose namespace, compatibility digest, cache key, prompt root, and engine family all match caller-supplied current-request bindings; still not a hit.

**KV cache** — Key/value attention state produced during prefill/decoding. Hybrid models may also require recurrent, convolutional, indexer or sliding-window state.

**LP64** — ABI where `long` and pointers are 64-bit; conventional x86-64 alignment yields the explicit legacy offsets used by this Wiki.

**Manifest** — Small semantic pointer from an authorized cache key to one immutable object digest and generation.

**MISS_RECOMPUTE** — Required result for absent, invalid, incompatible, unsupported, corrupt, undecryptable or engine-rejected cache state. Recompute starts from trusted current inputs.

**Partial reuse** — Reusing a consecutive prefix of independently verified complete chunks. It does not mean accepting a truncated object or partially decoded recurrent state.

**Prompt root** — Parent-chained cryptographic digest of exact token and semantic non-token inputs through a reuse boundary.

**Quarantine** — Restricted, non-reusable storage for invalid objects retained temporarily for diagnosis.

**State ABI** — Versioned contract between serialized bytes and the engine state importer, including layout and semantics beyond a model architecture name.

**TBW** — Terabytes written rating used as one endurance-budget input. The report's lifetime estimates are scenario models, not vendor warranties.

**WAF** — Write amplification factor, NAND bytes programmed divided by host bytes written.
