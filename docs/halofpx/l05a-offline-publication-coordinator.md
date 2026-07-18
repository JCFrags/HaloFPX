# L05a offline publication coordinator slice

Status: accepted after independent adversarial review. This is the first
code-conformance slice after P63-00; it is not completion of
M63-01 and opens no persistence, filesystem, provider, server, or canary gate.

## Boundary

`halofpx-context-store-publication` is an `EXCLUDE_FROM_ALL` static target. It
is linked only by its offline unit test and has no concrete backend. The
coordinator owns no path, bytes, key, file handle, thread, queue, provider, or
runtime option. Its abstract backend is an injected protocol seam whose
implementations are untrusted until separately qualified.

The request supplies an exact expected predecessor, exact next anchor identity,
and a positive object count capped at the accepted v1 manifest bound of 128.
Before invoking the backend the coordinator requires:

- identical store, namespace, checkpoint lineage, policy epoch, and key
  generation;
- predecessor generation below `UINT64_MAX` and next generation exactly old
  plus one;
- the next predecessor digest exactly equal to the old manifest digest; and
- an unchanged writer-authority epoch. Authority transfer is outside this slice
  and cannot be self-authorized by a publication request.

The first backend result must reproduce the expected predecessor exactly,
including authority epoch, generation, manifest digest, and predecessor digest.
A mismatch is `stale_predecessor` and performs no publication step.

## Ordered protocol

For each object the coordinator calls stage, write, verify, file sync,
no-replace publication, and directory sync in order. Only then does it perform
the same six phases for the manifest. Manifest verification returns the digest
of the exact canonical authenticated bytes; the coordinator compares it to the
next anchor's manifest digest before file sync, publication, or anchor change.
Only then may atomic anchor replacement and anchor synchronization follow. The
result cannot report `published` or acknowledge durability before the final
anchor synchronization succeeds.

`already_equal` is admitted only for object and manifest no-replace publication,
where a future qualified backend must have verified exact equality. It is
rejected at reads, writes, verification, synchronization, and anchor operations.
An unequal object or manifest collision and a manifest/anchor digest mismatch
fail closed. Any failed or throwing anchor-replacement attempt is reported as
`anchor_visibility_uncertain`, as is a successful replacement whose sync fails;
none is acknowledged as durable.

An explicit noncopyable root-fence object permits only one synchronous call
across all coordinator instances that share that root authority. The embedding
code must own exactly one fence per configured publication root. Exceptions
cannot cross the boundary. This is an in-process conformance guard, not
cross-process locking, authority transfer, attempt fencing, or a filesystem
coordination protocol.

## Synthetic tests

The scripted backend verifies the exact 21-call trace for a two-object plan and
injects one failure at every call boundary. Tests require no premature
acknowledgement, no anchor replacement before its step, explicit uncertainty
after failed/thrown replacement and post-replacement sync failure,
object/manifest collision rejection, exact manifest-to-anchor digest binding,
verified-equal publication retries, rejection of `already_equal` at other
phases, every predecessor identity field, authority-escalation rejection,
lineage/generation/count checks, exception containment before and after the
anchor linearization point, deterministic exclusion across two coordinators
sharing one root fence, and stable status names.

The static contract rejects product linkage, concrete filesystem calls,
runtime-provider references, and donor names. No CachyLLama or GPL llama-ai
implementation or documentation entered this slice; the direct-cherry-pick
roster remains empty.

## Gates still closed

This seam does not prove exact bytes, streaming writes, short-I/O handling,
safe path traversal, no-follow/no-replace primitives, authentication, an anchor
wire format, directory synchronization, crash-old/crash-new recovery, late
completion fencing, cross-process ownership, ENOSPC/EDQUOT/EIO/read-only
handling, quota/reserve/eviction, observability, or device power-loss durability.

The next safe M63-01 work is a deterministic durable-filesystem simulator with
live and durable namespace state, named before/after failpoints, crash recovery,
unreachable-garbage retention, exact traces, and stale-attempt tests. A concrete
OS writer remains blocked on a frozen authenticated anchor format and update
authority, a manifest encoder with independent golden vectors, a streaming
object writer/verifier, production root and resource policy, and actual
filesystem qualification. Windows currently supplies no claimed equivalent to
POSIX directory `fsync`; this milestone makes no Windows durability claim.

Rollback is source-only: remove the excluded target, its tests, and this record.
No cache root, runtime configuration, node, model, or deployment can be affected.
