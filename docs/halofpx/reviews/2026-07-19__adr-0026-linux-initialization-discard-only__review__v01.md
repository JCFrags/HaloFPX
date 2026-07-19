# ADR-0026 Linux initialization/discard-only review v01

- Date: 2026-07-19
- Scope: ADR-0026 plus the initializer-only CDDL layout amendment
- Verdict: **ACCEPT** for L05t/M63-01b implementation; promotion remains closed

## Outcome

The accepted contract admits one default-off, Linux-only, excluded
administrative initializer on a new disposable Btrfs root. It does not admit
normal reopen, recovery, quarantine, compare-and-advance, provider linkage,
cache restore, inference, or the general Linux mutation gate.

The initializer cannot consume L05s as a hidden handoff. It owns a reviewed
behavioral port of the primitive algorithms, uses a separate fixture OFD lock
before root mutation, creates and holds the permanent root lock as the first
root mutation, and returns no reusable authority. Any post-latch error, death,
or missing acknowledgement makes the whole exact disposable filesystem
discard-only, including a complete-looking tree.

## Findings and repairs

The first independent source and fault-contract passes found material gaps:

- fd 4 incorrectly inherited the credential memfd identity;
- the predecessor was authenticated but not independently launcher-pinned;
- the registry-lab digest required for the immutable name was private;
- L05s could not return the credential, root dirfd, or lock state implied by a
  direct handoff;
- the permanent writer lock was not explicitly acquired and synchronized;
- the marker replacement had no admitted staging namespace;
- the initializing marker's absent HEAD field was described as a zero digest
  rather than CBOR null;
- the initial publications exposed partial final names;
- nonempty-root and discard classifications conflicted;
- the proposed production crash hook could not honestly prove the exact final
  binary; and
- the CDDL's normal HEAD-derived filename rule contradicted publication of the
  initial predecessor before HEAD exists.

The final contract closes each issue. It freezes distinct fd 3/fd 4 names and
alias rules; exact predecessor receipt fields and continuity; a narrow no-I/O
lab-digest API with an independent vector; direct parsing into locked storage;
no direct L05s link; fixture and permanent OFD locks; CBOR null initializing
state; four exact initialization-only staging names; no-replace publication and
source/destination directory synchronization; a distinct preexisting-root
discard result; external ptrace process-kill qualification of exact binaries;
separately hashed deterministic fault builds; and the narrow initial-filename
exception after fd-4 authentication and pinned digest comparison.

## Review dimensions

- Correctness: exact one-shot sequence, identity binding, write/readback/sync,
  atomic rename, final reopen, and cleanup ordering are closed.
- Security: secrets remain locked and wiped, paths are anchored, both writer
  fences are held, all visible creates are no-replace, and unsupported behavior
  has no weaker fallback.
- Failure safety: every post-latch outcome is discard-only; no repair or
  adoption path exists.
- Provenance: the design is target-native and requires no donor code, GPL code,
  P3 admission, or new dependency.
- Rollback/performance: the target remains default-off, excluded, and absent
  from product graphs, so the known-good services and feature-off control are
  unchanged.
- Freshness/clarity: the CDDL comment and ADR now agree on normal versus initial
  filename derivation and the transient namespace.

The final independent source/API/build-gate re-review returned **ACCEPT** with
no remaining blocker. This verdict authorizes implementation only; it is not a
filesystem, durability, cache, inference, or performance qualification.
