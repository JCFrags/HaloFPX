# HaloKV two-node persistent-cache protocol research wiki

> **Status:** Design proposal and research package, not an implementation audit or production certification. Research date: **2026-07-17**.

This repository specifies how persistent LLM KV-cache state should behave when one model execution is split across two logical ranks. It combines a GitHub-Wiki-compatible shell (`Home.md`, `_Sidebar.md`, `_Footer.md`) with the LLM Wiki pattern: immutable source notes under `raw/`, synthesized and interlinked pages under `wiki/`, an agent schema in `AGENTS.md`, a content index, and an append-only `log.md`.

No authoritative public implementation named **HaloKV** was identified in the reviewed sources. “HaloKV” is therefore used here as the working name for the proposed protocol.

## Decisive conclusions

1. A reusable checkpoint is a **global commit certificate plus an immutable manifest containing every expected rank at one coherent token boundary**. A lone rank manifest is never globally committed state.
2. A two-member control quorum has **zero failure tolerance**. Epoch issuance and commit publication should use an independent, strongly consistent authority, normally a three-voter service or another store with verified compare-and-swap and consistency semantics.
3. Rank-local pages should remain rank-local and content-addressed. Recovery should exchange inventories and fetch only missing or corrupt pages; it should not copy the surviving rank’s multi-gigabyte cache merely because its peer changed.
4. **Single-node continuation is not the default and is usually impossible.** It is possible only when the remaining or replacement node can execute the complete model and possesses, fetches, or reconstructs the complete logical KV state in a layout accepted by that one-node topology.
5. Topology mismatch, stale epochs, incomplete rank sets, failed integrity checks, and ambiguous commit outcomes all fail closed.

## Navigate

- Start at [Home](Home.md) or [`wiki/index.md`](wiki/index.md).
- Read the core design in [`wiki/Executive-Summary.md`](wiki/Executive-Summary.md) and [`wiki/Protocol-Overview.md`](wiki/Protocol-Overview.md).
- Inspect message contracts in [`protocol/halokv.proto`](protocol/halokv.proto) and [`protocol/json-schema/`](protocol/json-schema/).
- Inspect protocol diagrams in [`protocol/state-machines/`](protocol/state-machines/).
- Inspect the model-checking package in [`formal/tla/`](formal/tla/) and the research plan in [`wiki/Formal-Modeling.md`](wiki/Formal-Modeling.md).
- Inspect fault/security test campaigns in [`fuzz/`](fuzz/) and the CSV matrices in [`tables/`](tables/).
- Inspect recorded build evidence in [`VALIDATION.md`](VALIDATION.md) and [`validation/`](validation/).

## Repository status vocabulary

- **Normative**: a protocol requirement in this proposal.
- **Recommended**: the preferred implementation choice, with alternatives stated where material.
- **Illustrative**: example values, limits, or service objectives that require benchmarking.
- **Open**: requires an implementation-specific decision or experiment.

Run `python3 scripts/lint-wiki.py` to validate structure, JSON, CSV shape, frontmatter, and wiki-link targets. The packaged `MANIFEST.sha256` records hashes for all repository files except itself.
