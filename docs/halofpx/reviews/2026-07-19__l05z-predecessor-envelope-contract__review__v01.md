# L05z predecessor-envelope contract independent adversarial review

**Result: ACCEPT the corrected contract and freeze it for implementation. No
contract-level blocker remains. This review makes no implementation,
qualification, durability, performance, or promotion claim.**

## Scope reviewed

The review covered only the proposed ADR-0026 step-4 contract recorded in
[`l05z-predecessor-envelope.md`](../l05z-predecessor-envelope.md), against the
accepted
[`0026-linux-registry-lab-initialization-discard-only.md`](../decisions/0026-linux-registry-lab-initialization-discard-only.md),
[`context-store-registry-lab-v1.cddl`](../contracts/context-store-registry-lab-v1.cddl),
the L05v predecessor-authentication boundary, the qualified L05y publication
pattern, and the target-native wire and protected-registry APIs. No L05z source
or test implementation existed for this review.

## Corrections required and incorporated

The frozen contract incorporates every adversarial correction:

1. **Three independent envelope checks.** Publication preserves the exact fd 4
   bytes, authenticates their protected-registry facts and launcher pins, and
   separately derives the registry-lab digest and exact lowercase digest name.
   It never re-encodes the object or substitutes the protected-registry carrier
   digest. Digest success alone cannot authenticate content.
2. **Locked secret and byte lifetime.** Original and readback envelope bytes
   and the placement-owned credential stay in `mlock`ed `secure_inputs`. The
   facts-only verifier is mandatory because the positive carrier verifier would
   copy the envelope into ordinary returned storage. Audit state contains no
   raw envelope or reusable authority. The existing target-native digest and
   authentication helpers retain their bounded, explicitly wiped cryptographic
   stack scratch; the contract does not falsely claim those helpers are
   zero-copy.
3. **Pinned identity, not same OFD.** A separately opened read-only marker,
   temporary, or destination is compared by pinned device/inode/mount and exact
   metadata/content identity. The contract makes no false same-open-file-
   description claim.
4. **Writable marker descriptor closure.** Every write-capable `root.marker`
   descriptor is closed successfully, without close retry, before the first
   step-4 mutation. Later access is read-only. The marker must remain byte-,
   authentication-, digest-, and inode-identical to L05y and must still select
   no `HEAD`.
5. **Exact path arithmetic.** Path length excludes the NUL. The final suffix is
   82 bytes, candidate length 4014 is admitted, 4015 is rejected, and 4097
   bytes of construction storage accommodate the maximum admitted path and its
   terminator.
6. **Narrow EINTR policy.** Only bounded `pread` and `pwrite` loops retry
   `EINTR`. Create, mode change, synchronization, rename, close, and every other
   syscall are not retried. Ambiguous publication is never adopted.
7. **Closed publication order.** The exact sequence is exclusive temporary
   create, fd-bound mode validation, bounded exact write/read/EOF/authentication,
   file `fsync`, immediate pinned-identity read-only validation, sole
   `RENAME_NOREPLACE`, destination `envelopes/` fsync, source `staging/` fsync,
   final pinned-identity/content validation, and unchanged-marker/exact-layout
   validation. Root fsync cannot substitute for the two renamed-directory
   synchronizations.
8. **Sticky failure authority.** Identical and unequal collisions, late
   completion, substitution, close failure, or any ambiguity cause whole-root
   discard. The contract admits no deduplication, adoption, per-entry unlink,
   truncation, repair, retry, or resumption.
9. **Explicit qualification matrices.** The contract names all seven
   exact-production mutation families, the 700-cell minimum two-node crash
   matrix, returned-fault occurrence classes, hostile/corruption/replay cases,
   fresh-media requirements, independent reconstruction, inherited controls,
   sanitizer limitations, isolation checks, and retained evidence.
10. **Default-off and provenance boundary.** L05z remains a distinct excluded
    extent with earlier entrypoints as controls. It adds no donor unit,
    dependency, installation, export, product, provider, runtime, or persistent
    authority edge.

## Accepted semantic boundary

The final layout contains the unchanged authenticated initializing
`root.marker`, the zero-length `writer.lock`, exactly one expected digest-named
regular object in `envelopes/`, empty `attempts/` and `staging/`, and no
`HEAD`. The predecessor envelope is therefore present but unreachable. Even a
complete step-4 execution returns discard-required and cannot be interpreted as
successful initialization.

The logical regular-byte accounting is exactly marker length plus predecessor
length under the inherited 16 MiB limit, with the inherited 256 MiB reserve.
The exact admitted name is
`e-<64-lowercase-registry-lab-digest-hex>.cbor`, where the digest domain is
`halofpx.registry-lab-registry-envelope.v1\0` over the exact fd 4 bytes.

Exact launcher-pinned old-but-authenticated bytes may pass in a fresh process.
That proves receipt-relative integrity only, not latestness or rollback
resistance. The envelope does not establish alignment between its opaque
authority-base commitment and newly generated root/store identities, rank or
world-size ownership, distributed authority, fallback, or inference state.

## Verdict and next gate

The corrected contract is sufficiently exact, bounded, failure-closed,
target-native, and reversible to open L05z implementation. Implementation must
remain a small extension of the existing initializer inputs, anchor, internal
audit seam, focused tests, and separately excluded controllers; it does not
require a wire-codec or protected-registry API change.

Promotion remains closed until a committed implementation and receipt prove
the complete focused, inherited, build, archive-isolation, fresh-media,
two-node crash, returned-fault, corruption, provenance, cleanup, and independent
final-review gates. This acceptance must not be cited as evidence that any L05z
code has been written or qualified.
