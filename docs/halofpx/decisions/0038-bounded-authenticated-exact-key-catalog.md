# ADR-0038: bounded authenticated exact-key catalog

Status: accepted for one default-off L10d laboratory milestone. Production
persistence remains closed.

## Context

L10c can automatically reuse one exact prompt, but its scope-dedicated
`anchor.v1` admits only one generation-one lineage. A different exact prompt
therefore computes cold and cannot become reusable. L10d must admit multiple
independent exact prompts without weakening the L09 lifecycle, L10a
anchor-first validation, or L10b exact-session and compatibility authority.

## Decision

Add a separately compiled, Linux-only multi-entry mode that requires every
earlier context-store gate and remains runtime opt-in. A configured capacity is
fixed at startup, greater than one, and no greater than a small implementation
limit. Omission of the new compile gate preserves the L10c surface and links no
catalog implementation.

Each catalog position owns one pre-provisioned data root and one pre-provisioned
anchor root. Within that position, the existing L09 generation-one authority
is unchanged: one immutable exact-session lineage, one authenticated
`anchor.v1`, one selected manifest, synchronized data before visibility,
corruption as a miss, and no replacement. Positions never share a mutable
generation or checkpoint lineage.

The catalog is a fixed, bounded set of predetermined positions, not an
unbounded directory index. Lookup reads at most the configured number of exact
slot paths. Filenames, directory enumeration order, directory presence, and
manifest names confer no authority. A hit requires an authenticated final slot
record whose canonical body binds:

- format domain and version;
- store UUID, derived catalog identity, and configured catalog capacity;
- exact slot ordinal and the inspected identities of both slot roots;
- private scope namespace, exact-session/checkpoint-lineage identifier, and
  closed compatibility root; and
- generation one with a null predecessor, the selected manifest digest, and
  the fixed producer, topology, rank-ownership, and rank-placement authority.

The slot record uses a catalog-purpose HMAC key derived from the owner key and
store UUID. Exact length, canonical encoding, registered algorithm, and tag are
mandatory. Unknown, duplicate, malformed, unauthenticated, differently scoped,
wrong-capacity, wrong-root, wrong-version, or conflicting records make lookup a
safe miss. The target exact identity must still pass the existing authenticated
anchor, manifest, compatibility, object, token, codec, and live-state checks.

Publication holds lifetime top-level writer authority and a process-local
non-reentrant operation guard. It first performs a bounded authenticated scan.
An existing exact record is never replaced. For an unused position it publishes
an immutable authenticated reservation before invoking the existing
generation-one publisher. The reservation binds the same identity and root
facts but is not hit authority. After the child anchor is durable, a separately
authenticated final record is synchronized, published without replacement,
and followed by catalog-directory synchronization. Only that final record makes
the position selectable.

A crash or failure after reservation can strand one position but cannot create
a hit, reuse it for a different identity, invalidate an already authenticated
position, or authorize deletion. Valid reservation/finalizing evidence without
a final record counts against capacity and remains offline-retirement evidence.
Unexpected or unauthenticated catalog evidence fails closed.

Reinsertion of identical authenticated bytes into the identical fixed position
is idempotent: this bounded milestone has no external monotonic catalog head
with which to distinguish it from restoration of the same immutable authority.
Catalog identity, ordinal, root identities, exact session, scope,
compatibility, topology, and selected manifest prevent cross-catalog or
cross-position replay. Detecting deletion and later reinsertion of identical
same-position bytes would require a separately reviewed monotonic authority and
is outside L10d.

Capacity is admission authority. When every position is finalized or reserved,
a new exact key receives a distinct capacity-exhausted result. The server must
not arm writeback, creates no new catalog or child publication, and continues
ordinary cold inference. Existing authenticated positions remain readable.
There is no implicit fallback from a corrupt matching position to a different
position.

The configured logical quota is conservatively partitioned across positions
after reserving the maximum authenticated catalog-record charge. Every child
retains the full filesystem reserve plus the remaining final-record upper
bound until catalog visibility. Catalog publication rechecks available space
before its own visibility. This may underutilize configured storage but cannot
multiply the global quota by the entry count.

The existing request boundary remains: authenticated native nonstreaming
completion, exact canonical tokens, private principal scope, closed
compatibility, target-only greedy-memoryless state, no client-supplied cache
identity, and cold fallback on every unsupported or failed path. Responses and
ordinary logs disclose no catalog position, key, digest, principal, path, or
hit/miss fact.

## Qualification boundary

L10d requires only focused evidence for the newly opened risk:

- two different exact keys publish, survive process restart, and hit
  independently with exact continuations;
- a changed key, wrong private scope, corrupt child state, and catalog tamper
  miss without accepting state;
- capacity exhaustion creates no reservation, final record, manifest, object,
  or anchor and preserves both prior hits;
- feature-off help, linkage, and inherited L10 smoke remain unchanged; and
- one independent adversarial review against the canonical Wiki and this ADR.

Disposable Linux roots are required. The known-good deployment is not stopped
or modified.

## Exclusions and rollback

L10d does not admit online deletion, eviction, slot reuse, generation
advancement or replacement, prefix matching, shared/cross-principal reuse,
administrator APIs, production enablement, distributed restore, broad soak,
or final primary-model G9/G10 claims. Whole disposable roots are retired only
offline.

Rollback is disabling the runtime mode, compiling without the L10d gate, or
reverting one coherent milestone commit. L10c and explicit-handle modes retain
their existing one-entry behavior.

The implementation is target-native. No donor implementation, GPL llama-ai
code, CachyLLama transplant, new dependency, WebUI, remote, or reference-clone
change is authorized.
