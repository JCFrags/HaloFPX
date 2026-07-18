# ADR-0004: publication and failure

- Status: accepted for L02
- Date: 2026-07-17

## Decision

Only an authenticated immutable manifest selected by a matching protected
publication anchor creates hit eligibility. One writer owns each
publication root unless a later proven coordination protocol replaces this
rule. Persistent writing remains disabled at L02.

The required publication order is:

1. create unique staging objects under the fixed root without following links;
2. write bounded bytes and verify exact lengths and SHA-256;
3. unconditionally synchronize every complete object before visibility;
4. publish immutable objects without replacement on the same filesystem and
   synchronize their parent directories where required;
5. write, authenticate, and re-read a complete manifest naming one exact object
   and rank set;
6. synchronize and publish the unique immutable manifest without replacement,
   then synchronize its directory;
7. atomically replace and synchronize the protected external publication anchor
   with its exact lineage, epoch, generation, manifest, and predecessor digest;
8. only then allow lookup or acknowledge the generation's durability.

A final destination is never overwritten by rename. Implementations require an
atomic create-if-absent/no-replace primitive or an equivalently proven protocol.
On object `EEXIST`, safe-open without links and verify exact canonical bytes,
length, type, domain, and digest before reuse. Equal-name/unequal-content is a
fatal publication error and quarantine event. Manifest names include store,
lineage, policy/key generation, and generation; on `EEXIST` an identical
authenticated idempotent retry may continue, but ambiguity never replaces it.

A crash before step 7 leaves the old anchor and old selected generation valid;
new material is unreachable. Anchor replacement is the linearization point. A
failed new write cannot advance the anchor or destroy an older committed
generation. Startup considers committed manifest names only, requires the exact
anchor-selected manifest and predecessor lineage, validates the entire
referenced set, never mixes generations, and treats staging/torn tails as
unreachable garbage. Garbage is
quarantined or retained for bounded administrative cleanup, not silently
deleted by a request.

Corruption, truncation, malformed length, wrong domain/version, duplicate or
unexpected field, missing component, replay, hostile path, and partial I/O are
misses. ENOSPC, quota/reserve exhaustion, read-only filesystems, EIO, late
completion, cancellation, or synchronization failure abort publication and
leave inference on the cold path.

The first writer is gated on a reviewed TLA+/TLC model of prepare, durability,
publication, crash, recovery, corruption, stale generation, abandonment, and
coordinator authority; filesystem-specific crash/power-loss qualification;
reserve/quota/eviction; observability; rollback; and administrative controls.

## Replay boundary

The replay lineage key is `(store_uuid, namespace_id, policy_epoch,
checkpoint_lineage_id)`. A namespace may contain many independent live
lineages; advancing one never invalidates another. Within one lineage the
protected anchor outside the cache root records key generation, selected
generation, manifest digest, and predecessor digest. The writer serializes the
read-anchor, predecessor check, manifest publication, and anchor replacement.
A normal next generation is exactly old plus one and names the old selected
manifest as predecessor. Policy/key epoch changes require an authenticated
administrative transition to a new lineage/namespace.

Older generations may remain retained for forensic or explicit offline
rollback but cannot become a server hit through implicit fallback. A failed
publication leaves the prior anchor selected. A missing, rolled-back, skipped,
forked, or inconsistent anchor/lineage disables only that lineage and forces
cold recomputation. These pre/post-anchor crash states and multiple independent
lineages are mandatory in the TLA+ model. This protects against ordinary
cache-root rollback and stale writers; it does not claim to survive compromise
of both the cache and protected service state.
