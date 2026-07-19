# L05u sealed initializer-input transport

L05u is the first implementation slice under M63-01b. It adds a Linux-only,
process-one-shot transport for the two launcher-supplied inputs that a later
initializer will need. It does not authenticate the predecessor, open a
registry root, acquire `writer.lock`, mutate storage, or initialize anything.

## Admitted behavior

The excluded initializer archive may consume only inherited descriptors 3 and
4. They must be distinct, exact-name, zero-link tmpfs memfds with `FD_CLOEXEC`,
exact sizes, and exactly `F_SEAL_SEAL | F_SEAL_SHRINK | F_SEAL_GROW |
F_SEAL_WRITE`. No other descriptor may alias either object.

Before descriptor inspection the process blocks all blockable signals,
unshares its file-descriptor table with `CLONE_FILES`, and proves that it has
exactly one task. This makes descriptor cleanup private to the calling task and
rejects a multithreaded launch. Input bytes are read at offset zero into a
page-rounded anonymous mapping, whose C++ object lifetime is begun with
placement construction and whose pages must be locked. Cleanup wipes the full
mapping, unlocks and unmaps it, closes both inherited descriptors, and restores
the signal mask.

The credential transport admits only the existing bounded package shape and
the launcher-pinned key identifier and generation. The predecessor transport
admits only a bounded envelope whose existing L05t digest equals the
launcher-pinned digest. The result name is deliberately
`transport_validated_no_root_access`: the secret is not authenticated and the
predecessor bytes are not reusable authority.

## Isolation and failure behavior

The implementation opens only `/proc/self/task` and `/proc/self/fd`. It has no
root or fixture path, no public credential parser, no protected-registry
verifier, and no mutation syscall. The target remains Linux-only,
default-off, `STATIC EXCLUDE_FROM_ALL`, uninstalled, unexported, and linked only
to the registry wire lineage. Unsupported, malformed, aliased, stale,
unlocked, partially readable, or incompatible inputs fail without root access
or mutation. A second call in the same process is rejected.

## Promotion boundary

This milestone authorizes only the next no-root-access slice: authenticate the
credential and every launcher-pinned predecessor field in locked storage, then
discard all sensitive state. Root traversal, `writer.lock`, persistence,
publication, cache hits, restore, and inference integration remain closed.

Qualification and exact hashes are pinned in
`evidence/l05u-sealed-input-transport-receipt.json`; the independent review is
`reviews/2026-07-19__l05u-sealed-input-transport__review__v01.md`.
