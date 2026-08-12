---
title: "References"
tags: ["references", "provenance"]
created: 2026-07-17
updated: 2026-07-17
status: design-proposal
sources: ["WIKI-01"]
related: ["Executive-Summary"]
---

# References

The authoritative source notes and URLs are in `raw/processed/source-catalog.md`. Source IDs in page frontmatter refer to that catalog.

## Research interpretation

Public systems such as vLLM, LMCache, Mooncake, and NVIDIA Dynamo demonstrate useful patterns—disaggregated prefill/decode, asynchronous KV movement, persistent multi-tier caches, leases, shared stores, and content-oriented transfer—but they do not by themselves specify HaloKV’s two-rank global checkpoint semantics. The coordinated commit, topology fingerprint, epoch fence, and single-node feasibility rules in this wiki are design synthesis and should be validated against the chosen execution engine.

The formal-method and fuzzing recommendations use official tool documentation and primary project material where available. Filesystem publication notes rely on the Linux `fsync(2)` and `rename(2)` contracts and must be revalidated for each actual filesystem or object store.

## Source groups

- `WIKI-*`: LLM Wiki and repository conventions.
- `KV-*`: LLM KV-cache transfer, persistence, and disaggregation systems.
- `COORD-*`: consensus, quorum, fencing, and conditional-update foundations.
- `RPC-*`: gRPC and Protocol Buffers reliability/security guidance.
- `STORAGE-*`: filesystem durability, hashing, and crash-consistency testing.
- `FORMAL-*`: TLA+/TLC, Apalache, P, and Alloy.
- `FUZZ-*`: coverage-guided, structured, stateful, and crash fuzzing.
- `SEC-*`: security-specific source notes.
