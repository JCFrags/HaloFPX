---
section_id: "63"
title: "Atomic commit and recovery design"
status: "needs-machine-validation"
last_verified: "2026-07-17"
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
6. Choose newest complete valid generation; otherwise miss/recompute.

**[RECOMMENDATION]** Stale/mixed-generation rank shards are never combined. Rebuild index data from valid manifests, not directory guesses.

## Protocol-model gate

**[RECOMMENDATION]** Before implementation approval, specify the mode-aware checkpoint state machine in TLA+ and exhaustively check finite configurations with TLC from pinned TLA+ Tools `v1.7.4`. The abstraction must make rank prepare/durability, manifest write/durability/publication, corruption, crash/restart, stale generations, coordinator authority, rejection, recomputation, and abandonment separate nondeterministic actions. Retain the `.tla`, `.cfg`, exact `tla2tools.jar` SHA-256, command, bounds, state/depth/runtime results, deliberate broken variants, and counterexample traces [S63-06].

Required safety includes: a published manifest names one complete generation/fingerprint; recovery never mixes generations; corruption/incomplete references are rejected; performance mode may lose the checkpoint and recompute after failure; and turn-durable/strict acknowledgement requires every object and manifest promised by that mode to be durable. Required conditional liveness includes: a fully durable prepared generation is eventually published or explicitly abandoned, assuming the coordinator and storage actions eventually run and storage does not fail permanently. Fairness assumptions and finite bounds must be explicit.

**[OPEN]** No TLA+/TLC model has been created or executed for HaloKV. A future bounded pass would validate only the chosen abstraction/configuration, not implementation conformance, filesystem behavior, or the machine durability promise; Section 80 fault injection and M63-03 remain mandatory.
