# Generation and qualification

## Deterministic generation

`recipes/generate_assets.py` uses no third-party packages or candidate code. It defines GGUF v3 bytes, metadata ordering, tensor values, malformed mutations, JSON canonicalization, and all record ordering.

The tensor palette uses exact binary fractions, and SplitMix64 supplies integer-defined selection. This replaces the non-portable `std::hash` plus `std::normal_distribution` pattern observed in upstream synthetic model generation.

## Independent self-check

`recipes/verify_gguf.py` parses the emitted GGUFs without importing candidate code. `recipes/verify_corpus.py --reproduce` regenerates assets into a clean directory, verifies all fixture locators/hashes, checks malformed error classes, validates normalized SSE, confirms excluded hashes are absent, and enforces that all HaloFPX cells remain open.

Current result: `qualification/SELF-CHECK.json` reports `match: true`.

## Candidate boundary

No candidate binary, library, server, model converter, or helper download script was run. `adapters/` is inert evidence and lacks executable permission. See `qualification/EXECUTION-STATUS.json` and the isolated qualification runbook.
