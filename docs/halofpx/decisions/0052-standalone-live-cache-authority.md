# ADR-0052: standalone live-derived cache authority

- Status: accepted for the issue-#33 implementation slice
- Date: 2026-08-12
- Scope: default-off standalone authority construction only

## Decision

HaloFPX will replace opaque operator compatibility digests with one immutable
authority derived from exact live model, runtime, request, and topology facts.
The first implementation slice remains outside every product target under
`HALOFPX_CONTEXT_STORE_LIVE_AUTHORITY=OFF` by default.

The standalone builder derives these ADR-0003 component inputs itself:

- `[0]` ordered model/shard roles, ordinals, exact byte lengths, and SHA-256 of
  the supplied exact bytes;
- `[1]` typed GGUF metadata, including nested arrays, plus tensor names,
  dimensions, types, shard/file order, offsets, and encoded lengths;
- `[2]` exact tokenizer artifacts, token bytes and score bits, merge order,
  special-token roles, and typed policy;
- `[3]` the one resolved effective template, renderer ID, and exact renderer
  binary identity;
- `[6]` exact canonical source-tree inventory/content bytes, exact dirty-tree
  patch/untracked bytes when applicable, executable/library bytes, toolchain,
  typed build options, and state ABI;
  and
- `[14]` the fixed ordered two-rank roster, stable endpoint/device identities,
  distinct global plan, ownership, and placement facts, and a nonzero stable
  topology-configuration epoch.

Each derived component uses the exact ADR-0003 component hash domain and its
closed deterministic-CBOR submanifest. The existing closed 16-component
builder remains the sole compatibility-root constructor.

The other ten components enter this slice as exact deterministic-CBOR
preimages, not component digests. A separately trusted validator capability
must parse the complete item, enforce the exact component-index CDDL and closed
registered-ID registry, and reject trailing/noncanonical bytes. Missing,
duplicated, required-component, non-map, indefinite top-level map, oversized,
unvalidated, or byte-identical supplemental preimages are refused.

## Refusal and lifecycle boundary

Any missing, placeholder-marked, mutable, invalidly ordered, duplicated, ambiguous, or
placeholder-equal required fact returns a typed refusal and a completely zero
authority. The builder never returns a partial compatibility root.

Registered IDs inside the six typed families are exact caller-supplied schema
data in this slice. The product adapter must validate each one against its
deployment profile's closed registry and derive immutable source/build
identity; an operator assertion such as `source_is_immutable=true` is not
sufficient product authority.

ADR-0003's typed tensor record now binds the model-artifact semantic order in
field `5`, alongside the exact file-relative offset in field `3`. A tensor
assignment to a different shard therefore changes component `[1]` without
overloading or truncating the exact offset.

A future product adapter must own one immutable authority snapshot for the
model/context lifetime and use that same snapshot for lookup and publication.
Model reload, sleep/wake reload, context recreation, or effective plan change
must close the store, discard the old snapshot, derive a new one, and reopen.
Operator component digests may remain only in explicitly gated canary tests;
they cannot override or combine with product authority.

Persistent topology identity binds stable authenticated worker/device identity
and topology configuration. RPC connection epochs, allocation epochs, remote
pointers, and other restart-ephemeral facts belong to live stage/commit
validation and must not poison disk compatibility across a valid restart.

## Why product wiring is deferred

The current loader is the last trustworthy point that owns the actual opened
GGUF shards and typed GGUF contexts. The retained public metadata map drops
arrays and stringifies other values. The current server initializes its store
before effective chat-template objects are finalized, and a normal allocated
run does not yet retain the complete final ownership/placement plan needed by
component `[14]`.

The adapter must therefore first:

1. freeze ordered opened-shard byte receipts and typed GGUF/tokenizer/tensor
   facts while the loader contexts are alive;
2. resolve the actual template and renderer before store initialization;
3. freeze resolved context/KV/backend semantics after automatic fallbacks;
4. retain distinct stable global-plan, rank-ownership, and rank-placement facts;
5. compose request-resolved components `[4]` and `[10]` through `[13]` for each
   reusable canonical prefix; and
6. rebuild authority on every model/context lifetime transition.

Until those prerequisites are implemented, linking this builder into
`server-context` would create false authority and is prohibited.

## Compatibility and reference boundary

The existing world-one exact-key canary, its command line, and its bytes remain
unchanged. This decision creates no cache reader, writer, lookup, publication,
restore, RPC command, filesystem access, target execution, or performance
claim.

Pinned `fewtarius/CachyLlama` commit
`6be745998f568e379ea197fcf827baec73ff9940` remains the saved-cache behavioral
reference. No donor cache format, trust rule, or source was copied into this
authority.
