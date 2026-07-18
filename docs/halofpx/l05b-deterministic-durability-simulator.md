# L05b deterministic durability simulator

Status: accepted after independent adversarial review. This is an offline
high-level fault-harness slice toward M63-01, not M63-01 or L05 completion.

## Boundary and state

`context_store_publication_simulator` implements the L05a injected backend in
memory. It is compiled only into the existing `EXCLUDE_FROM_ALL` publication
target and its offline tests. It accepts no path, handle, byte buffer, key,
provider, server option, asynchronous completion, or OS primitive and performs
no filesystem I/O.

The simulator is constructed from exact predecessor/next anchor fixtures and a
positive object count bounded to 128. For each synthetic object and the
manifest it tracks separate temporary-live, written, verified, file-durable,
published-live, and published-durable states. The protected anchor has separate
live and durable values. Object/manifest collision equality is prequalified
synthetic fixture metadata; this slice does not compare real bytes and makes no
encoder, hashing, or filesystem equality claim.

The predecessor chain is an explicit prevalidated control fixture at
construction. The simulator does not encode its historical manifests or
objects. Invalidating that control makes anchor reads fail and recovery miss for
both old and next anchors. This is a one-bit chain-validity abstraction, not
revalidation evidence for real historical bytes.

The operation trace is bounded to exactly twice the one-attempt call maximum:

```text
2 * (1 read + 6 * object_count + 6 manifest + 2 anchor)
```

That is 1,554 entries at 128 objects. A call beyond the bound returns a storage
error without extending the trace. Every retained entry records operation,
object index or no-index, before/after edge, result, and whether a configured
fault fired.

## Fault and crash projection

One named failpoint can fire before or after any of the 21 operations in the
two-object test plan. A before fault has no operation effect. An after fault
applies the deterministic state change and then returns the configured failure,
including the ambiguous anchor-replacement and post-sync cases.

Admitted high-level injected results are no-space, quota exhaustion, reserve
exhaustion, read-only, I/O error, interruption, generic storage error, and sync
error. These are coordinator outcome simulations, not errno, short-write,
capacity-accounting, or syscall-conformance claims.

Crash projection independently selects whether unsynchronized namespace
changes and an unsynchronized anchor survive. Temporary state is never
hit-authority and is retained only as a garbage count. Synchronized namespace
bindings and anchor state always survive. The four deterministic combinations
cover old/new and mixed persistence extremes without claiming a device model.

Recovery first requires the prevalidated predecessor-chain control, then reads
only the exact post-crash protected anchor. If it selects the predecessor,
recovery returns the old generation regardless of unreachable new material. If
it selects the next anchor, the exact simulated manifest and every
simulated object must remain published and non-conflicting; otherwise recovery
returns a miss. It never enumerates for a newer generation or falls back from an
invalid selected next generation. Unreachable temporaries and finals published
by the failed attempt are counted and never request-deleted.

## Qualification matrix

The main fault matrix runs every combination of:

- 21 named operations;
- before and after edges;
- 8 high-level resource/I/O/sync results; and
- 4 namespace/anchor crash projections.

All 1,344 runs must be unacknowledged and recover only the exact expected old or
new generation. Separate tests cover a fully synchronized success under all
four crash projections; exact semantic trace determinism across
repeated runs; equal and unequal object/manifest collision fixtures; retained
temporary and unreachable-final counts; selected manifest/object removal or
conflict as a miss with no fallback; predecessor-chain invalidation;
manifest-to-anchor digest mismatch; sequential stale
retry before and after crash; invalid configuration; the 128-object/777-call
maximum; and stable operation/recovery names.

The static contract includes simulator sources in the existing product-link,
filesystem/API, provider, and donor scans. No CachyLLama or GPL llama-ai code or
documentation entered this slice. The direct-cherry-pick roster remains empty.

## Gates still closed

This simulator does not contain canonical object or manifest bytes, independent
encoder vectors, real authentication, file identity, capacity arithmetic,
short/partial I/O, corruption within bytes, quarantine records, cleanup,
observability, quota enforcement, or reserve policy. Those remain subsequent
offline slices and M63-02/resource-control work.

The synchronous after-effect model is not proof of late asynchronous
completion, cancellation, reused-slot fencing, concurrent stale attempts,
authority transfer, coordinator failover, cross-process ownership, or locks.
The current backend cannot safely claim those behaviors because it carries no
attempt identity or expected-predecessor compare-and-swap argument.

There is still no no-follow/no-replace path implementation, concrete OS writer,
process-kill restart, reboot, directory-sync proof, filesystem/kernel/SSD
qualification, power-loss evidence, provider/server hook, persistent option, or
canary. Persistent writes and every durability-mode label remain closed.

Rollback is source-only: remove the simulator files/test and revert the small
enum/CMake/static-contract additions. No node, model, runtime, cache root, or
deployment is changed.
