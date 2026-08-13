# ADR-0051: default-off exact longest-prefix selector

Status: accepted for the standalone L10e selector-unit seam only. Product,
server, and distributed prefix restoration remain closed.

Date: 2026-08-12

## Context

ADR-0038 deliberately excluded prefix matching from the bounded authenticated
exact-key catalog. Issue #32 needs deterministic reuse of the longest eligible
canonical-token checkpoint without treating text, token count, directory
order, or a donor hash as cache authority. CachyLlama commit
`6be745998f568e379ea197fcf827baec73ff9940` is a behavior-only reference for
prefix reuse and suffix prefill; its FNV and fuzzy/system-boundary mechanisms
are not admitted.

The real dual-rank owner remains issue #26. Its isolated two-rank coordinator
contract does not yet restore two rank-local objects or connect RPC, storage,
the server, or this selector. A world-1 selector must therefore remain outside
the inference runtime until that composition is separately decided and
qualified.

## Decision

Add a Linux-only, literal-default-`OFF`, `EXCLUDE_FROM_ALL` selector library
around the unchanged ADR-0038 catalog. It has no server-context, llama-server,
CLI, or runtime-mode link. The gate requires the exact-key catalog and the
common, tools, and server build graph so root configuration cannot traverse
tests before defining the implementation target.

The caller provides the complete canonical request token IDs and no more than
eight complete semantic checkpoint boundaries in strictly increasing order.
The selector does not inspect prompt text or infer chat, role, system, or fuzzy
boundaries. Invalid, ambiguous, empty, descending, duplicate, over-limit, or
out-of-range boundary input fails closed before catalog I/O.

Candidates are evaluated longest to shortest. For each boundary the selector
derives the existing exact-session HMAC over the exact canonical token prefix
and all admitted scope, compatibility, profile, plan, rank, and topology
authority. It forms the unchanged catalog identity with the explicit policy
epoch and invokes `restore_exact`. Same-length entries remain distinct; only
the requested authenticated authority may win, independent of catalog scan or
publication order.

A missing candidate or a catalog entry under a different authenticated
authority may continue to the next shorter boundary. Once the exact catalog
record selects a child, child corruption or incompatibility is terminal for
the lookup: return an empty snapshot and cold-recompute reason, never fall
through to a shorter checkpoint. The selector repeats exact token, identity,
and profile equality at its own boundary before returning a hit. Restoration
does not repair or rewrite storage.

An accepted hit returns the snapshot plus matched/restored token counts,
untouched suffix offset/count, candidates examined, fixed-cardinality fallback
and source status, and selector validation time. It does not install state in a
live context, replay the suffix, publish a checkpoint, or claim avoided work.

This seam admits only world size 1, rank 0, ordinary target-only transformer
memory with greedy-memoryless sampling. Recurrent, hybrid, draft,
speculative/MTP, multimodal, adapter, grammar, tool, sampler, and distributed
state are rejected.

## Qualification boundary

The standalone CPU suite must prove longest selection, exact and partial hits,
suffix accounting, no candidates, token and policy mismatch, same-length
resolution for both authenticated authorities after publication, invalid boundaries,
different longer authority followed by a shorter exact hit, unsupported
profile/topology rejection, and terminal selected-child incompatibility,
same-size corruption, and truncation without rewrite. A source contract must
prove literal default-off, early dependency/Linux gates, an isolated target,
no product link, exact-session/catalog calls, and absence of fuzzy, prompt-text,
FNV, or string-based selection.

This does not close issue #32. Fresh-process selector execution, pinned-model
output equality, canonical-token/timing evidence, retained raw evidence, real
state install plus suffix prefill, and matched Strix Halo performance remain
open. A correctness-only WSL run is not target evidence.

## Relationship and rollback

This decision narrowly succeeds ADR-0038 only for a standalone selector-unit
seam. It does not change the catalog format, weaken any prior authority, or
reopen ADR-0038's product, shared-scope, replacement, eviction, or distributed
exclusions. Product composition requires a later decision after issue #26 has
one atomic two-rank owner with ordinary recompute and single-node fallback.

Rollback removes the option, selector target/source, focused tests/contracts,
L10e record, and this ADR. Because no product target links the library and no
stored format changed, rollback requires no cache migration.
