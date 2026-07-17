# LLM Wiki maintenance contract

This repository follows the LLM Wiki pattern: raw sources are immutable, synthesis lives under `wiki/`, and this file is the operating schema for an agent maintaining the knowledge base.

## Authority order

1. Explicit human instructions.
2. Normative protocol requirements in the current wiki.
3. Machine-readable contracts in `protocol/`, reconciled with the wiki when inconsistencies are found.
4. Source notes in `raw/processed/source-catalog.md`.
5. Inferences, which must be labeled as such.

## Required workflow

Before editing, read `wiki/index.md`, this file, and the pages directly related to the task. For source ingestion, add an immutable source note or catalog entry, update all affected synthesis pages, update `wiki/index.md`, and append an entry to `log.md`.

Every substantive page must have YAML frontmatter with `title`, `tags`, `created`, `updated`, `status`, `sources`, and `related`. Use source IDs from the catalog. Use double-bracket wiki links for internal concepts and ordinary Markdown links for repository files or external sources.

## Protocol discipline

- Preserve the global-commit invariant: both expected ranks at one logical boundary, under one generation, epoch, and topology fingerprint.
- Never describe a prepared rank manifest as committed state.
- Never treat transport connection state as an epoch or leadership proof.
- Never infer single-node continuation from cache availability alone. State the model/weight and cache completeness preconditions explicitly.
- Distinguish process durability, host-failure durability, and independently durable storage.
- Treat Bloom filters and inventory summaries as hints; content digests remain authoritative.
- Fail closed on malformed input, stale epoch, topology mismatch, missing ranks, corruption, or ambiguous authority.

## Change hygiene

Record design reversals in `wiki/Decision-Log.md`. Put unresolved decisions in `wiki/Open-Questions.md`. Keep examples non-secret and synthetic. Do not add real prompts, tokens, tenant identifiers, credentials, raw cache tensors, or storage URLs. Run `python3 scripts/lint-wiki.py` before declaring the wiki healthy. For protocol or formal-model changes, also run `TLA2TOOLS_JAR=/path/to/tla2tools.jar python3 scripts/deep-validate.py` and record the result.
