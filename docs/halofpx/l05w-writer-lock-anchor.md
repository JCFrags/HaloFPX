# L05w discard-only writer-lock anchor

L05w is the first filesystem-mutating implementation slice under M63-01b and
ADR-0026. It carries the L05v authenticated sealed-input state through an
anchored Btrfs parent/root/fixture admission, then creates and qualifies only
`writer.lock` in a fresh empty candidate root. The feature is Linux-only,
default-off, excluded from normal builds, and every invocation that reaches
the mutation latch is permanently classified as
`initialization_discard_required`.

## Authority and ordering

The L05v transport and authentication sequence remains the compatibility
control. A file-private non-copyable session retains the locked credential and
predecessor state after final fd3/fd4 authentication and revalidation. Both
descriptors are closed before the first parent, root, or fixture syscall. No
credential, predecessor bytes, secret, descriptor, absence proof, or reusable
mutation authority escapes the initializer.

The root operation requires exact launcher-pinned canonical paths, devices,
inodes, mount IDs, owners, modes, Btrfs filesystem UUIDs, subvolume UUIDs, and
fixture-lock identity. It uses anchored `openat2` calls with
`RESOLVE_BENEATH | RESOLVE_NO_SYMLINKS | RESOLVE_NO_MAGICLINKS |
RESOLVE_NO_XDEV`, requires a fresh empty candidate root, obtains the fixture
whole-file OFD lock, revalidates every identity, and obtains nonzero 256-bit
root and 128-bit store identifiers from complete `getrandom` loops before the
mutation latch.

## Admitted mutation and failure behavior

After the sticky discard latch, the only admitted namespace mutation is an
exclusive `openat2` create of `writer.lock` with mode `0600`. The initializer
then applies fd-based `fchmod(0600)`, validates the new inode, calls
`fsync(writer.lock)`, acquires its whole-file OFD write lock, and reopens and
revalidates the parent, root, fixture, fixture lock, and writer lock. The root
must contain exactly that one empty regular file.

Cleanup closes non-lock descriptors, wipes/unlocks/unmaps sealed state,
explicitly unlocks and closes writer then fixture locks, releases the root
guard, and restores the signal mask. Any failure after the latch remains
discard-required. A root that was already nonempty is
`preexisting_root_discard_required` with no new mutation. Pre-latch failures
clear positive path, storage, reserve, generated-ID, and mutation audit facts
while preserving only ordering, counts, and cleanup evidence.

L05w does not sync the root directory and therefore makes no durable namespace
visibility, completed initialization, adoption, repair, retry, persistence,
cache-hit, or restore claim. Qualification controllers deactivate each whole
verified loopback medium as authority rather than unlinking or repairing
entries; the detached image is retained only as immutable evidence.

## Rollback

Source rollback removes or reverts the L05w anchor include, API, tests, ptrace
controller, and CMake routing. All affected targets are default-off and
`EXCLUDE_FROM_ALL`, and no product, service, deployment, or persistent server
state changed, so no runtime rollback is required. Any qualification root that
crossed the mutation latch remains discard-only and must never be adopted or
repaired.

## Isolation and qualification boundary

The initializer archive remains exactly two objects with exactly three
callable definitions. The implementation does not link or call L05s
`qualify_once()`, and it has no product, server, provider, cache, inference,
HIP, Vulkan, RPC, WebUI, install, or export edge. General Linux registry
mutation remains a fatal configuration gate.

An external test-only ptrace controller is `EXCLUDE_FROM_ALL` and is not a
CTest or product target. It drives the exact production qualification binary
and kills the exec'd clean child at real syscall entry and exit stops without
adding production crash hooks.

Exact hashes, run counts, evidence-bundle identities, cleanup receipts, and
nonclaims are pinned in
`evidence/l05w-writer-lock-anchor-receipt.json`. The independent review is
`reviews/2026-07-19__l05w-writer-lock-anchor__review__v01.md`.
