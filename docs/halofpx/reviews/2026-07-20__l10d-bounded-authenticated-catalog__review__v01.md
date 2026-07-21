# L10d bounded authenticated catalog independent review

Date: 2026-07-20

Verdict: **ACCEPT for the narrow default-off bounded catalog canary**

Production persistence: **closed**
Blocking code findings after final focused re-review: **none**
Blocking evidence findings: **none**

## Scope reviewed

I reviewed the complete uncommitted L10d diff against `AGENTS.md`, ADR-0038,
the L09 generation-one lifecycle and storage contract, the L10a anchor-first
selection contract, the L10b exact-session authority, the L10c operational
request boundary, and the canonical Wiki requirements for authenticated
private scope, bounded untrusted parsing, duplicate rejection, immutable
data-before-index publication, quota/reserve admission, corruption as a miss,
and cold fallback.

The final retained nimo-2 bundle
`/var/tmp/halofpx-l10d-evidence-20260720-v2.tar.zst` has SHA-256
`a9336a4425b2bf2458573ffad4677adcc6fba9ca810cd9e93e812682e2250303`.
Its source hashes exactly match the final implementation and tests reviewed here.
The bundle records 8/8 focused ON tests, 1/1 feature-surface OFF test, two
independent restart hits, exact cold/restored continuations, a nonmutating
capacity refusal, and an active zero-restart production worker. Those are
valid positive results. The focused adversarial additions exercise and close
the authority defects identified during the initial review below.

The change is target-native. I found no donor implementation, GPL `llama-ai`
code, CachyLLama transplant, new dependency, WebUI change, remote, production
deployment mutation, or reference-clone change in the L10d diff.

## Findings and final resolutions

### RESOLVED — authenticated records now bind the catalog root identity

ADR-0038 requires every final slot record to bind the derived catalog identity
as well as store UUID, capacity, ordinal, and both inspected child-root
identities. The implementation inspects and retains `catalog_identity`, but the
`record` structure and its 416-byte canonical body contain no catalog-root
device, inode, mount, UID, filesystem, or mode fields. `expected()` therefore
checks only UUID, capacity, ordinal, child roots, manifest, producer, topology,
and rank facts.

Consequently, authenticated reservation/final bytes can be transplanted into
another owner-controlled catalog root while reusing the same store UUID, key,
slot roots, and configuration. The alternate root accepts the record even
though ADR-0038 says directory placement and filenames are not authority and
the catalog identity is part of the authenticated domain.

Required correction:

- add the inspected catalog-root identity, or a collision-resistant canonical
  digest over it, to reservation and final record bodies;
- authenticate and compare it on every read before considering a position
  occupied or selectable; and
- add an adversarial test that copies otherwise valid records into a distinct
  catalog root with the same store/configuration and proves a safe non-hit.

Focused re-review: the canonical authenticated body now serializes the full
inspected catalog-root identity and `expected()` compares it before selection.
The new transplant test copies valid records to an alternate catalog root with
the same key, store, slots, and configuration and receives `miss_corrupt` with
an empty snapshot. This finding is closed.

### RESOLVED — duplicate and ambiguous pending/final combinations are rejected

Both `publish()` and `restore_exact()` read four fixed names per position, but
validate each present record independently. They do not reject coexistence of
`reserve.v1` with `reserve-pending.v1`, `final.v1` with
`final-pending.v1`, or other authenticated duplicate evidence. In
`restore_exact()`, a valid final plus coherent reservation can still select and
hit even when either pending duplicate is also present.

This contradicts ADR-0038's rejection of duplicate, ambiguous, conflicting, or
unexpected catalog evidence and the canonical format rule that unexpected
extra data cannot become part of a trusted hit. An attacker under the
service-UID threat boundary can reproduce the already authenticated bytes; an
HMAC check alone cannot distinguish that duplicate.

Required correction:

- define and enforce the exact allowed state machine for each position;
- reject every impossible or ambiguous combination before identity matching or
  child restore, including authenticated duplicates; and
- add table tests for copied reservation/final pending records, final without
  reservation, mismatched reservation/final, and multiple authenticated
  representations of one phase. Every case must produce a safe non-hit with an
  empty returned snapshot and no writeback.

Focused re-review: both publish and restore reject a canonical final coexisting
with either pending record and a canonical reservation coexisting with its
pending duplicate before identity matching or child restore. The copied
authenticated final-pending test returns `miss_corrupt` with an empty snapshot.
This finding is closed for the admitted state machine.

### RESOLVED — bounded catalog layout validation and accounting are present

L09 treats persistent roots as untrusted input: it allow-list scans entries,
rejects unknown names, wrong types, symlinks, duplicate inodes, and identity
drift, and accounts existing logical and allocated bytes. L10d opens and
inspects the catalog directory and its lock, then reads only the four expected
paths per configured slot. An unexpected regular file, directory, symlink,
device, hard link, or extra slot record name is ignored. Existing catalog
records and unexpected files are not included in a catalog-root logical or
allocated-byte accounting snapshot.

This makes the milestone documentation's claim that unexpected catalog state
fails closed inaccurate and leaves the configured global quota incomplete. A
fixed bounded selector is appropriate, but a bounded allow-list validation pass
is still required; enumeration must validate layout, never confer hit
authority.

Required correction:

- add a bounded catalog-root allow-list inspection under the held writer lock,
  with contained/no-follow opens and the inherited owner, mode, mount, type,
  link, duplicate-inode, and root-identity checks;
- accept only `writer.lock` and the exact per-capacity protocol names in states
  admitted by the position state machine;
- account existing catalog files in the global logical projection without
  treating scan order or names as selection authority; and
- test representative unknown-file, unexpected-slot, hostile-link/type, and
  duplicate-inode cases. Existing child hits must not be accepted while the
  catalog authority is uncertain.

Focused re-review: a bounded allow-list scan now runs under the lifetime writer
lock before open completion and before every publish/restore. It validates the
root's fixed entry bound, names, regular-file type, owner, mode, link count,
device, mount, sizes, duplicate inodes, checked logical bytes, checked allocated
bytes, and quota. Unknown-file and hard-link cases return safe corrupt misses.
Enumeration remains validation only; selection still reads and authenticates
the fixed records. This finding is closed.

### RESOLVED — reserve admission jointly retains final-record headroom

The amended `reserve_available()` correctly rounds one record to the observed
filesystem fragment and uses checked `reserve + allocation` arithmetic. The
catalog's quota partition also reserves two fragment-rounded records per slot.
That fixes the narrow logical-versus-allocated defect, but not ADR-0038's full
invariant that every child retain the filesystem reserve **plus the remaining
final-record upper bound until catalog visibility**.

Before reservation publication the catalog checks headroom for only one record,
not the reservation and later final record. The child publisher then performs
its own L09 admission against `reserve + child_remaining_upper_bound`; it does
not know about the catalog final record. When catalog and child roots share a
mount, child publication may legally consume the surplus down to the configured
reserve. The post-child catalog check can then fail for lack of one fragment,
leaving a durable reservation and selected child anchor without the final
catalog authority. That is a safe non-hit, but it contradicts the accepted
admission contract and unnecessarily strands capacity under a condition the
initial admission was required to reject.

Required correction:

- ensure initial admission and child publication jointly retain the allocation
  upper bound of the reservation and remaining final record, grouped correctly
  with child artifacts when roots share a mount;
- pass the remaining catalog headroom into the child budget or perform an
  equivalent combined mount-aware admission under the held authorities;
- add one boundary test where free space is between the logical record size and
  the complete combined upper bound. Publication must refuse before the
  reservation and child anchor rather than strand the position after child
  publication.

Final focused re-review: the pre-reservation check now retains two
fragment-rounded catalog allocations. If either child root shares the catalog
mount, the child receives a checked reserve increase of one rounded final-record
allocation, so its inherited mount-aware admission cannot consume that
headroom. If both child roots are on distinct mounts, child publication cannot
consume the catalog mount and the catalog's pre-reservation check retains the
headroom independently. The final visibility check retains one rounded
allocation. Overflow paths reject startup or publication. This satisfies the
ADR-0038 invariant and closes the remaining code finding.

## Resolved evidence finding

### RESOLVED — final corrected evidence is retained immutably

The superseded v1 bundle has SHA-256
`28fbad0dca6286dfa6ebdd3ac8c0efbc4fa7a4b7be1a077ca1aaa243a5bedaf9`
and records catalog implementation/test hashes
`720f31b4afccaddf92decb70a197cdfc1694e46897309dc154fa551c56bd0ed4`
and `5df029c60b0d4aebba8451cf0fedc07d72ddb8f088cf64bf2dd458b10355e469`.
The finally reviewed files instead hash to
`f0cb77613443c02414398c2eebfbbe9f1c2fba8b53edb1e80dc7edb6c55c2ebc`
and `0418032b2fc5b5bce96aa58a74fe60ed24626bc65336ef1cd03d30941e1e8096`.
The bundle timestamp is 00:12 PDT; the corrected nimo-2 source was synchronized
at 00:23 PDT.

The live nimo-2 build does match the finally reviewed hashes, and its 00:24 PDT
unit, feature-off, and inherited selection tests pass. The coordinating report
also states that the two-entry restart/capacity process proof was rerun with
the same valid result. Those facts support the code disposition, but the
immutable evidence named by the receipt does not contain them.

Final focused re-review: the new immutable v2 bundle has SHA-256
`de744c808ca0ea0408831f7452b96e86931401674a0954ff506c6c1591572909`
and the receipt points to it. It records the exact final catalog/test hashes
`f0cb77613443c02414398c2eebfbbe9f1c2fba8b53edb1e80dc7edb6c55c2ebc`
and `0418032b2fc5b5bce96aa58a74fe60ed24626bc65336ef1cd03d30941e1e8096`,
the final server hash, build log, 8/8 focused test log, raw process workspace,
two independent restart hits, nonmutating capacity refusal, and active
zero-restart production-worker state. The earlier bundle remains separately
named and was not relabeled. This evidence finding is closed.

## Positive findings retained after correction

The overall milestone boundary is otherwise sound and appropriately narrow:

- the compile gate defaults OFF, is Linux-only, requires L10c, and alone links
  and exposes the catalog runtime mode;
- capacity is fixed at startup between two and eight positions and lookup reads
  only those positions;
- exact-session identity, private authenticated scope, closed compatibility,
  canonical tokens, admitted profile, and existing anchor/manifest/object/live
  state validation remain authoritative;
- each position composes an independent unchanged generation-one child with
  descriptor-pinned roots and no replacement;
- data and the authenticated child anchor become durable before final catalog
  visibility, and record publication uses write, data sync, no-replace rename,
  and directory sync;
- a full catalog returns `capacity_exhausted`, does not arm server writeback,
  leaves the tested tree unchanged, and preserves prior valid hits;
- failed or unsupported restoration remains cold inference, no client supplies
  cache identity, and responses do not expose catalog facts; and
- the retained process proof establishes that two distinct exact prompts can
  survive restart and hit independently in the nonadversarial case.

## Promotion conditions and boundary

All four implementation P1s and the evidence mismatch are closed. L10d is
accepted for the narrowly specified default-off bounded authenticated catalog
milestone. No additional implementation change or expanded test universe is
required before this coherent milestone commit.

This verdict does not request deletion, eviction, generation replacement,
prefix matching, shared reuse, distributed restore, broad soak, production
enablement, or final G9/G10 work. Those gates remain closed.
