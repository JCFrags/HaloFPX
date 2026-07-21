# ADR-0037: default-off exact-key operational cache canary

Status: accepted for narrow implementation; production persistence remains closed.

## Context

L08i and L08j prove explicit-handle full-v1 miss, publication, process restart,
authenticated hit, and corruption-as-miss through the server. L09 adds the
generation-one lifecycle budget, reserve, one-entry limit, sticky write
closure, and redacted observation. The remaining usability gap is that a
client must already possess the selected-manifest handle. A normal completion
cannot discover and reuse the one admitted checkpoint.

## Decision

Add a separately named Linux-only laboratory mode under all four existing
full-v1 compile gates and a new explicit runtime opt-in. Omission of either
gate preserves feature-off behavior and performs no context-store root access.

The canary is limited to one authenticated principal, one exact canonical
token sequence, one target-only greedy-memoryless completion profile, one
generation, and one entry. Authentication and canonical tokenization happen
before deriving cache identity. The exact-key preimage binds a versioned
target-owned domain, private principal scope, closed compatibility root,
token count and fixed-width token IDs, logical/output position, transformer
profile, topology/rank ownership, and every other admitted output-affecting
component. Request fields are inputs, never authority.

The fixed scope-dedicated authenticated `anchor.v1` is the only discovery
authority. There is no authoritative index, filename trust, directory scan for
fallback candidates, prefix matching, or shared reuse. After authenticating
the anchor, the implementation obtains its selected manifest and then applies
the existing manifest, compatibility, object, checkpoint-lineage, expected-
token, and full-state validation before live restore. Any missing, ambiguous,
unsupported, stale, corrupt, incompatible, differently scoped, or partially
decoded state is an opaque miss followed by ordinary cold computation.

On an exact authenticated hit, restore occurs before prompt evaluation. On a
clean not-found miss, the server may capture and publish the exact prompt state
once at the completed-prompt boundary before sampling. Cancellation, late
completion, publication failure, or storage failure never changes response
correctness. Corrupt, incompatible, or ambiguous state is never overwritten.
A later different exact key misses and cannot replace generation one.

One server controller thread remains the invocation owner, and the adapter
also supplies a local non-reentrant serialization guard. The L09 dual OFD
locks, logical quota, filesystem reserve and late reserve recheck,
`max_entries=1`, synchronized data-before-visibility order, sticky close,
quarantine, and zero-safe-online-eviction rules remain unchanged. Responses
and ordinary logs disclose no cache key, principal, path, digest, selected
manifest, or distinct hit/miss timing label.

## Qualification boundary

Promotion requires only the risk-proportionate set for this new boundary:

- feature-off help/contract and inherited context-store smoke;
- bounded empty, sole-selected, ambiguous/corrupt, and wrong-authority adapter
  behavior;
- one nimo-1 process sequence proving exact request miss, write, restart, hit,
  and byte-exact continuation;
- one wrong-principal or one-token-different cold recomputation without
  overwrite;
- one quota/reserve no-publication control; and
- one independent review against the canonical Wiki.

Broader filesystem, fault, concurrency, multi-key, prefix, distributed, and
soak matrices remain deferred until a corresponding boundary opens or a defect
creates a concrete risk hypothesis.

## Consequences and rollback

This canary makes a one-key generation-one checkpoint usable without a client
handle, but it is not a multi-prompt product cache and does not admit production
persistence. Multi-generation selection, physical-capacity policy, retention,
eviction, administrator authority, distributed ownership/recovery, and final
durability/performance qualification remain required.

Rollback is disabling the runtime mode, building without the new gate, or one
coherent source revert. Disposable canary roots are retired only offline as a
whole. Existing explicit-handle behavior remains available for diagnosis.

The implementation is target-native. No donor implementation, GPL llama-ai
code, CachyLLama transplant, new dependency, WebUI, remote, or reference-clone
change is authorized.
