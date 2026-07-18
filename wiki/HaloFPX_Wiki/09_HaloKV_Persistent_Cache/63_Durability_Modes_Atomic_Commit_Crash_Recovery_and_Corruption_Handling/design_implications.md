---
section_id: "63"
title: "Atomic commit and recovery design"
status: "needs-machine-validation"
last_verified: "2026-07-18"
applies_to:
  repositories: []
  software_versions: []
  hardware_revisions: ["dual Strix Halo"]
related_sections: ["58", "59", "61"]
---

# Design implications

## Two-phase generation

**[RECOMMENDATION]** Each rank writes immutable shard temp files, validates lengths/digests, syncs them per mode, renames to content-addressed final names, and reports prepared descriptors. The coordinator writes a complete manifest temp, syncs, renames it to the committed generation, and syncs its directory. Only that manifest makes shards reachable.

Manifest fields: format/version; generation and predecessor; compatibility/topology fingerprint; session/token-prefix identity; required streams/ranks; each object length and strong digest; durability mode; timestamps; commit state; producer commits.

**[RECOMMENDATION]** The coordinator never edits a committed manifest. A newer generation supersedes it. Rank timeout or partial prepare leaves unreachable objects for GC.

## Recovery

1. Enumerate committed manifest names only.
2. Parse with size/depth limits.
3. Validate schema/fingerprint/generation.
4. Validate every required object's path, size, and digest.
5. Quarantine bad manifests/objects with reason; do not delete automatically.
6. Validate only the exact protected anchor-selected identity. Directory enumeration must never select a newer or older generation; any missing, corrupt, replayed, cross-lineage, or incompatible selected chain is a miss/recompute [S63-07].

**[RECOMMENDATION]** Stale/mixed-generation rank shards are never combined. Rebuild index data from valid manifests, not directory guesses.

## Protocol-model gate

**[RECOMMENDATION]** Before implementation approval, specify the mode-aware checkpoint state machine in TLA+ and exhaustively check finite configurations with TLC from pinned TLA+ Tools `v1.7.4`. The abstraction must make rank prepare/durability, manifest write/durability/publication, corruption, crash/restart, stale generations, coordinator authority, rejection, recomputation, and abandonment separate nondeterministic actions. Retain the `.tla`, `.cfg`, exact `tla2tools.jar` SHA-256, command, bounds, state/depth/runtime results, deliberate broken variants, and counterexample traces [S63-06].

Required safety includes: a published manifest names one complete generation/fingerprint; recovery never mixes generations; corruption/incomplete references are rejected; performance mode may lose the checkpoint and recompute after failure; and turn-durable/strict acknowledgement requires every object and manifest promised by that mode to be durable. Required conditional liveness includes: a fully durable prepared generation is eventually published or explicitly abandoned, assuming the coordinator and storage actions eventually run and storage does not fail permanently. Fairness assumptions and finite bounds must be explicit.

**[MEASURED]** P63-00 now has a target-owned TLA+ model checked against the exact final source SHA-256 `320d294949624469a5c636fc510300f6f558845094139402a6845d765b1c38fe`. The promoted TLC matrix matched 17/17 expected outcomes across 44,539,476 generated and 5,968,128 distinct states; Apalache independently typechecked the model and bounded `Safety` through length 5 [S63-07]. This validates only the reviewed finite abstraction. Implementation conformance, filesystem behavior, power-loss behavior, and the machine durability promise remain open and require the disabled writer/fault harness, Section 80 fault injection, and M63-01..03.

**[VERIFIED]** The first C++ conformance slice at `b8123fe5` remains excluded from all product targets and accepts no paths or bytes. It orders bounded injected operations, binds the verified canonical manifest digest to the next exact anchor, rejects authority transfer, serializes coordinators sharing one in-process root fence, and never acknowledges an ambiguous anchor outcome [S63-08]. This is evidence for the coordinator kernel only. M63-01 still requires the deterministic durable-filesystem simulator, named crash/fault matrix, stale-attempt behavior, and retained recovery evidence.

**[VERIFIED]** Commit `4366e493` adds a deterministic in-memory live/durable projection and accepted high-level crash matrix [S63-09]. It directly distinguishes unsynchronized and directory-synchronized object/manifest bindings, requires a prevalidated predecessor-chain control, rejects invalid selected next state without fallback, and retains unreachable attempt material. Because it carries no canonical bytes, capacity arithmetic, asynchronous attempt identity, OS handles, or real persistence, it is protocol-harness evidence rather than filesystem durability or M63-01 completion.

**[VERIFIED]** Commit `3ae385d2` closes the final-anchor stale-coordinator gap in the excluded seam [S63-10]. Each request carries a nonzero 256-bit attempt identity, and final replacement receives that identity plus the exact full predecessor and next anchor. A typed stale result is safe only when no replacement occurred; an injected stale result after simulated linearization is converted to ambiguity. Tests interleave distinct root fences and independently mutate store, namespace, lineage, policy epoch, key generation, authority epoch, generation, manifest digest, and predecessor digest between read and CAS. This is exact final-CAS evidence only, not authenticated attempt registration or asynchronous per-operation fencing.

**[VERIFIED]** Commit `d85ee807` makes attempt lifetime explicit across the excluded synchronous coordinator and simulator [S63-11]. Begin binds the full predecessor, next identity, and object count; every later operation requires the same ID. Definite pre-anchor failure abandons and retains partial material as garbage. Begin, abandonment, CAS, sync, or durable-close ambiguity fences the root, reports whether fencing was confirmed, and blocks fresh IDs. Successful anchor sync is no longer acknowledged until durable close. A bounded 128-ID in-memory history rejects replay across intervening attempts. Persistent history, real asynchronous callbacks, and reconciliation remain unimplemented.

**[VERIFIED]** Commit `8537a830` freezes a target-owned authenticated protected-anchor envelope and bounded offline codec [S63-12]. The closed deterministic-CBOR body binds version, 16-byte store UUID, namespace, policy epoch, lineage, selected manifest-key generation, writer-authority epoch, selected generation/digest, and nullable predecessor; separate authentication metadata selects an anchor-purpose HMAC key. The complete canonical authenticated envelope is the future CAS unit. An independent encoder reproduces the exact 229-byte golden envelope, tag, and domain-separated digest. Rejected inputs expose no parsed carrier, and the only positive result remains `authenticated_unadmitted`. This is not a rollback-proof storage service: protection depends on external anchor/key/authority state, and no runtime or persistent writer is linked.

**[VERIFIED]** Commit `29cd9581` removes the provisional publication-anchor representation and admits only the exact owned carrier created by successful anchor encoding or verification [S63-13]. Ordinary transitions are checked before backend access: immutable domain and authority fields must match, generation must advance by exactly one, the next predecessor must equal the selected predecessor digest, and a private domain-separated commitment proves both carriers derive from the same effective anchor authority even when their declared key tuple is identical. Reads and final CAS compare every canonical envelope byte. Absent protected state returns `bootstrap_required`; no synthetic generation-one write is allowed. This is still an excluded in-memory seam without an administrative bootstrap protocol, protected key registry, concrete backend, or persistent writer.
