# Changelog

## 2026-07-17 — Initial research artifact

- Pinned CachyLLama, llama.cpp, LMCache, SGLang and vLLM source commits.
- Documented CachyLLama v3 checkpoint, index and v1 system-prompt native layouts.
- Separated observed behavior, derived filesystem/ABI consequences and HaloFPX recommendations.
- Added comparison and failure matrices, compatibility field list and endurance scenarios.
- Added immutable object schema, generationed HMAC-authenticated manifest schema, and key derivation.
- Added Mermaid and rendered Graphviz state diagrams.
- Added standard-library validators, deterministic fixtures, 18-case fault injection, unit tests and endurance calculator.
- Defined `MISS_RECOMPUTE` as the only result for invalid/unverifiable state.
- Reserved `HIT_VERIFIED` for successful transactional engine import; offline validation distinguishes authenticated-but-unbound `CATALOG_ENTRY_VALID` from fully request-bound `IMPORT_CANDIDATE_VALID`.
