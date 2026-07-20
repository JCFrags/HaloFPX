# ADR-0036: generation-one lifecycle guard and redacted inspection

Status: accepted for default-off L09 implementation after independent
adversarial review. This does not advance production-persistence authority.

L09 may make the existing Linux-only, default-off `full-v1-rw-canary`
operationally bounded and inspectable. It does not authorize production
persistence, automatic discovery, shared scope, generation advancement,
multi-writer publication, online deletion, or distributed reuse. All four
existing compile gates and the explicit runtime mode remain required.

The full-v1 canary must reject startup unless quota is positive, reserve is
nonnegative, and `max_entries` equals exactly one. A root remains dedicated to
one authenticated private scope, one lineage, and generation one. The root
quota therefore acts as both global and per-scope quota for this milestone.

The authority acquires both exact zero-byte lockfiles in deterministic root-
identity order and retains both OFD locks for its lifetime. A typed internal
capability lets the materializer reuse the held data lock instead of acquiring
a different open-file-description lock. Layout scan, charging, pending and
material publication, the pre-anchor audit, anchor and terminal publication,
and pending cleanup all occur while both locks are held. These advisory locks
coordinate writers using this implementation under the same service-UID trust
boundary; any observed drift still fails closed.

Storage accounting admits only a recursive allow-list. The data root contains
the exact zero-byte `writer.lock` and exact owner-only `staging`, `objects`, and
`manifests` directories. The anchor root contains its exact zero-byte lock and
only the protocol records and stage names valid for the observed attempt state.
Every entry is opened with contained, no-follow semantics and revalidated.
Unknown names, subdirectories, symlinks, devices, disappearing or appearing
entries, duplicate inodes, wrong owner/mount/mode/link count, root identity
change, or checked-add overflow closes the writer and retains the material.

The observation records existing logical bytes as checked `st_size` sums and
existing unique allocated bytes as checked `st_blocks * 512` sums. Link count
one plus duplicate-inode rejection makes the observed allocation unique within
the roots. L09 enforces an explicitly named logical-byte canary quota; a final
physical-capacity quota remains open. Fixed directories and the exact zero-byte
locks add no logical charge. `max_entries == 1` means one private lineage and
selected generation, not one immutable object.

Publication admission uses an exact projected logical peak: currently charged bytes
plus every absent immutable frame, the manifest, authenticated pending record,
anchor, and authenticated terminal record. A stage-to-final rename does not
double-charge one inode, and exact already-equal files are subtracted. The
combined projection must not exceed quota. Checked `f_bavail * f_frsize`
available bytes are grouped by mount identity so roots on one filesystem are
not double-counted. Each distinct mount group is evaluated independently as
`available[m] >= reserve[m] + remaining_upper_bound[m]`; free bytes are never
pooled across mounts, and the full configured reserve applies to each distinct
mount. Initial admission requires reserve plus a conservative allocation upper
bound for all absent artifacts. Immediately before anchor
visibility, accounting is repeated and available space must still cover
reserve plus the anchor and terminal allocation upper bounds. Initial quota or
reserve failure occurs before `pending.v1`; late reserve loss cannot advance
`anchor.v1`. Reserve is an admission invariant, not a guarantee against an
uncoordinated external filesystem consumer.

Writes close for the process after successful publication, quarantine, quota
failure, or reserve failure. A pre-pending budget close is process-sticky only;
L09 introduces no durable budget-failure marker. A persisted anchor/terminal or
authenticated pending/aborted terminal closes writes during construction. A
pre-anchor failure with pending remains closed. Restart reconciles it to an
aborted closed state without an anchor only when durable authenticated terminal
publication succeeds. Continued reserve, storage, or synchronization failure
retains authenticated pending and keeps writes closed for a later restart;
malformed or uncertain evidence quarantines. Valid reads remain available for
a closed writer; quarantine forces a miss. Interrupted or uncertain material
is retained for reconciliation or offline whole-root retirement.

No online deletion means no eviction of immutable objects, manifests, anchor,
or terminal records. Existing protocol cleanup remains required: transient
stage files are cleaned by their owning operation when safe, and `pending.v1`
is removed only after a durable authenticated terminal record. L09 adds no
eviction, compaction, migration, or secure-erasure behavior.

The adapter exposes a fixed read-only observation to the existing server
controller and bounded operator logs. L09 opens no HTTP status route because
the current API-key layer has principal authentication but no distinct
administrator role; ordinary authenticated inference principals receive no
storage facts. The cardinality-bounded observation contains only typed state
through two total fixed enums. `lifecycle_state` maps every authority outcome:
`unavailable`, `ready`, `published`, `recovered_success`, `recovered_aborted`,
`interrupted`, `busy`, `invalid`, `unsupported`, `source_mismatch`, `conflict`,
`storage`, `synchronization`, `quota_exhausted`, `reserve_exhausted`,
`layout_rejected`, `accounting_overflow`, or `quarantined`. `last_close_reason`
is `none`, `published`, `recovered_success`, `recovered_aborted`,
`quota_exhausted`, `reserve_exhausted`, `layout_rejected`,
`accounting_overflow`, `storage`, `synchronization`, or `quarantined`.
The observation also contains saturating logical/allocated/available/quota/
reserve counters, whether accounting is valid, whether writes are closed, and
a zero-byte online eviction classification. It must not expose paths, UUIDs,
namespaces, principals, lineages, selected digests, tokens, key identifiers,
or key material.
No request receives cache facts. Internal logs use the same fixed fields and
remain rate/cardinality bounded. A future administrative route requires a
separately reviewed administrator-authority contract.

Online eviction is explicitly closed. The selected generation is pinned, and
material implicated in an interrupted attempt remains reconciliation evidence.
Without authenticated multi-generation reachability, predecessor history,
reference counts, or a garbage ledger, zero bytes are proven safe to delete.
The only admitted classifications are selected generation pinned,
reconciliation required, uncertain material retained, and no safe online
eviction.

Focused qualification is proportionate: invalid quota/max-entry configuration;
exact-budget publish, hit, second-write closure, restart hit and closure; quota
one unit below plus arithmetic overflow with no mutation; one injected
pre-anchor reserve-loss boundary with no anchor, followed by retained pending
while reserve is unavailable and aborted/closed after terminal headroom returns;
one representative unexpected-entry, hostile-link-or-mode, and lock-contention
table; observation allow-list/redaction; feature-off and inherited L08g/L08h-b/
L08i smoke; one representative nimo-1 process canary; and one independent
milestone review. Exhaustive disk, fault, concurrency, multi-node, retention,
and soak matrices remain recorded later gates unless a concrete defect or
high-risk hypothesis requires earlier expansion.
