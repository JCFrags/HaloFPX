---
section_id: "09"
title: "Numbered Functional Requirements"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["HaloFPX project", "ggml-org/llama.cpp", "charlie12345/ROCmFPX", "fewtarius/CachyLLama"]
  software_versions: ["commits in sources.md"]
  hardware_revisions: ["two-node Strix Halo target"]
related_sections: ["29", "38", "48", "53", "60", "69", "71", "74"]
---

# Numbered functional requirements

Unless marked optional, each item is **[RECOMMENDATION]** for v1 and remains draft until ratified.

## API and clients

- **FR-API-001** Serve authenticated streaming and non-streaming `/v1/chat/completions` for an explicitly tested subset of the pinned upstream behavior.
- **FR-API-002** Return a stable model ID, active execution mode, request/correlation ID, usage, finish reason, and structured error without leaking secrets.
- **FR-API-003** Preserve supported tool-call/JSON semantics for admitted model/template combinations; reject unsupported combinations before generation.
- **FR-API-004** Support cancellation, bounded request size, timeouts, and backpressure; overload must return an explicit retriable status.

## Models and execution modes

- **FR-MOD-001** Admit only cataloged model files verified by cryptographic hash, architecture, tokenizer, template, quantization, license, and tested backend.
- **FR-MOD-002** Report node/rank placement and reject plans exceeding measured memory or compatibility limits.
- **FR-EXE-001** Provide a correct single-node mode with the peer absent.
- **FR-EXE-002** Provide replication as the two-node independent-request baseline.
- **FR-EXE-003** Gate remote speculation, tensor parallel, pipeline parallel, and MoE hybrid modes by per-profile compatibility and acceptance evidence.
- **FR-EXE-004** Fence rank/session epochs and propagate cancellation/errors so stale rank work cannot publish a response.

## Caching and state

- **FR-CAC-001** Key persistent state by all compatibility-critical inputs, including model and template hashes, state/schema version, mode/shard plan, and rank owner.
- **FR-CAC-002** Verify header, length, checksum/authentication metadata, and compatibility before restore; corruption or mismatch must miss/recompute or fail explicitly.
- **FR-CAC-003** Publish cache objects atomically and recover safely from crash, partial write, full disk, eviction, and restart.
- **FR-CAC-004** Isolate cache/log state by authenticated service identity when multi-user mode is enabled; support bounded retention and deletion.
- **FR-CAC-005** Expose hit/miss/reject/restore/eviction metrics without exposing prompt content.

## Recovery and degradation

- **FR-REC-001** Detect node, rank, transport, and cache-store failure and expose degraded/unavailable state.
- **FR-REC-002** Complete, retry from a clean boundary, or fail a request explicitly; never silently continue with incomplete distributed state.
- **FR-REC-003** Support documented recovery to one link and/or single-node service where the model fits and semantics permit.
- **FR-REC-004** Provide graceful shutdown, clean restart, and rollback to the previous compatible build/config.

## Observability and administration

- **FR-OBS-001** Provide public-or-restricted readiness consistent with threat model, plus protected metrics for TTFT, queue, prompt/decode time, tokens, mode, rank/link health, cache, and errors.
- **FR-OBS-002** Emit structured logs with timestamps, request IDs, build/config/model IDs, and error cause; redact credentials and prompt content by default.
- **FR-ADM-001** Support declarative configuration validation, dry-run/profile inspection, model list/load policy, cache inspect/evict, and status commands.
- **FR-ADM-002** Record the effective configuration and prevent unknown options or incompatible combinations from silently taking effect.

## Installation, compatibility, and security

- **FR-INS-001** Build/install from pinned sources on each supported OS matrix and verify hashes, licenses, service identity, directories, and rollback.
- **FR-COM-001** Publish a versioned matrix for hardware revision, firmware, OS/kernel, drivers, backend, model, quantization, context, mode, API subset, and cache/wire schema.
- **FR-SEC-001** Bind to loopback by default; non-loopback exposure requires authentication, authorized admin routes, and documented transport protection.
- **FR-SEC-002** Run with least practical privilege; model/cache/config paths must resist traversal, symlink confusion, and unauthorized modification.
- **FR-SEC-003** Avoid required cloud egress during normal inference and make update/model-ingest network activity explicit and auditable.

## Verified inputs, not product proof

- **[VERIFIED]** Pinned upstream `llama-server` documents chat APIs, health, metrics, authentication keys, SSL inputs, parallel decoding, caching, and slot operations [S09-01].
- **[VERIFIED]** CachyLLama documents SSD cache and per-user isolation primitives [S09-02].
- **[VERIFIED]** ROCmFPX documents experimental AMD formats/backends and qualified local measurements [S09-03].
- **[OPEN]** None of those facts proves the combined requirements above.

