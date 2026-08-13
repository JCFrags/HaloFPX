# L10e default-off exact longest-prefix selector

Status: **implemented and locally selector-unit-qualified as a non-product
seam; fresh-process selector, target, and distributed restore remain open**

This issue-32 slice turns the existing bounded authenticated exact-key catalog
into a deterministic longest-prefix lookup seam while the caller holds
catalog-mutation custody. It uses the pinned
[`fewtarius/CachyLLama@6be745998f568e379ea197fcf827baec73ff9940`](https://github.com/fewtarius/CachyLLama/tree/6be745998f568e379ea197fcf827baec73ff9940)
as a behavioral reference for reusable prefixes and suffix prefill. It does not
copy CachyLlama's storage/index implementation, native-endian FNV authority,
fuzzy lookup, or system-boundary heuristic. The canonical Wiki's
[retain/redesign/reject map](../../project/wiki/HaloFPX_Wiki/09_HaloKV_Persistent_Cache/56_CachyLLama_Cache_Semantics_and_Porting_Map/design_implications.md)
remains the porting authority.

The accepted standalone boundary is recorded in
[ADR-0051](decisions/0051-default-off-exact-longest-prefix-selector.md). It
narrowly succeeds ADR-0038's prefix-matching exclusion without admitting a
server or distributed restore path.

## Implemented authority

**[VERIFIED]** The Linux-only
`HALOFPX_CONTEXT_STORE_LONGEST_PREFIX_CANARY` option defaults to `OFF`, requires
the exact-key catalog and complete common/tools/server build graph, is checked
before test traversal, builds an `EXCLUDE_FROM_ALL` library only when explicitly
enabled, and has no `llama-server`, `server-context`, runtime-mode, or CLI edge.
The source contract permits the exact isolated target declaration, focused
test link, and the exact separately default-off ADR-0054 world-1 product block.
Any other direct or intermediate product reference fails qualification. It
also rejects weak selector mechanisms.

**[VERIFIED]** The caller supplies the complete canonical token-ID sequence and
at most eight complete semantic checkpoint boundaries in strictly increasing
order. The selector never parses prompt text and never invents a chat, role, or
system boundary. Zero, out-of-range, duplicate, descending, or over-limit
boundaries fail closed before catalog I/O.

For every boundary, longest first, the selector:

1. takes the exact canonical prefix `tokens[0:boundary]`;
2. derives the existing L10b HMAC exact-session identifier over that byte-exact
   token sequence, scope, compatibility root, admitted profile, plan/rank
   digests, and topology;
3. forms the unchanged L10d catalog identity with the explicit policy epoch;
4. calls the existing authenticated `restore_exact` path with the same exact
   token prefix and target-only world-1/rank-0 transformer profile; and
5. repeats exact token, identity, and profile equality before returning a hit.

The inherited catalog locks each `restore_exact` probe separately. Therefore,
the caller must prevent publish or any other catalog mutation for the whole
selector call; concurrent read-only restores are allowed. Without that
quiescent-mutation boundary, a publication could interleave between probes and
the result would not represent one atomic catalog snapshot. Product ADR-0054
owns the separate coordinator needed to enforce that boundary.

An accepted result reports matched-token count, restored-token count, suffix
offset/count, candidates examined, a fixed-cardinality fallback reason, the
last catalog/authority status, and validation time. The caller can prefill the
untouched request suffix starting at `residual_token_offset`; the selector
itself does not mutate a live context or publish a checkpoint.

## Failure and tie behavior

**[VERIFIED]** A missing longer entry or one under a different authenticated
authority is not eligible, so the next shorter exact boundary may be tried.
Two entries at the same token count remain distinct by their authenticated
exact-session authority; the exact authority wins independent of catalog scan
order. Supplying the same boundary twice is ambiguous input and is rejected.

**[VERIFIED]** Once an exact authenticated catalog record selects a child,
corruption or incompatibility is terminal for the lookup. It returns an empty
snapshot and a typed cold-recompute reason; it never falls through to a shorter
checkpoint. The inherited catalog also treats malformed or ambiguous records
anywhere in its fixed layout as corruption. Restore performs no repair or
rewrite.

**[OPEN]** ADR-0054 adds a separate fail-closed world-1 server shell, but no
trusted live-authority provider can produce a positive product hit. Two-node
product wiring still depends on issue [#26](https://github.com/JCFrags/HaloFPX/issues/26),
because one owner must restore both rank-local state partitions atomically or
choose ordinary recompute/single-node fallback. The merged
[test-only coordinator contract](two-rank-cache-coordinator-contract.md) owns
fake-provider capture/stage/commit ordering and failure semantics, but it has
no real rank-local cache object, RPC, filesystem, or server adapter. This L10e
library remains world size 1, rank 0, ordinary transformer memory, target-only
state, and greedy-memoryless sampling. Recurrent, hybrid, draft, MTP,
multimodal, adapter, grammar, tool, sampler, and distributed state are rejected.

**[OPEN]** This slice does not complete issue #32's model-backed qualification.
The inherited exact-key test remains the fresh-process control, and the
inherited catalog test covers catalog tampering, child corruption, and
read-only failure. Neither executes this selector across fresh processes.
Fresh-process prefix selection, deterministic model output equality, a pinned
model-fixture hash, canonical-token/timing capture, and retained raw model-run
evidence remain required before the issue-level CPU gate can close.

## Local selector-unit qualification

**[VERIFIED] 2026-08-12 local-only:** A Release CPU build on Ubuntu 26.04 WSL2
(`6.18.33.2-microsoft-standard-WSL2`, GCC 15.2.0, CMake 4.2.3) compiled the
selector with every predecessor cache gate enabled and the repository-required
RPC authority support present. The selector, inherited authenticated-catalog,
and static-contract CTest set passed 3/3. A separate default-off configure
contained no selector target, and the static non-product contract passed
against that source tree. The portable correctness receipt is
[retained here](evidence/l10e-default-off-exact-longest-prefix-selector-receipt.json).

The functional test covers three available prefixes plus a missing full
boundary, longest selection, exact suffix offset/count, full hit, no-boundary
cold path, empty request rejection, token mismatch, same-length entries under
different authenticated authorities, duplicate/out-of-range boundaries, a
different longer authenticated plan authority followed by a shorter valid hit,
exact policy-epoch mismatch, incompatible compatibility authority, wrong
topology, an exact selected child-token incompatibility, same-size child-object
corruption, truncated child-object corruption, terminal no-fallthrough, empty
returned state, and an unchanged path/file-content inventory after corrupt
lookup. The different plan authority is not a generation or policy-epoch
ordering claim. The inventory comparison is not a metadata or timestamp claim.

This is correctness evidence only. No model, GPU, CachyOS host, Strix Halo
target, end-to-end prompt, NVMe timing, or speed claim is included. Target
performance work must measure selector validation, state read/decode/install,
matched/restored tokens, suffix prefill, TTFT, prompt rate, and exact output
parity through issue [#18](https://github.com/JCFrags/HaloFPX/issues/18).

## Rollback

Remove the L10e option, selector library, focused tests/contracts, and this
record together with ADR-0054's exact gated product link. The prior L10d
catalog format is unchanged, so rollback requires no cache migration and
leaves feature-off server behavior intact.
