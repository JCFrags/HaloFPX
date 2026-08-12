---
title: "Formal modeling plan"
tags: ["formal-methods", "tla", "tlc", "apalache", "p"]
created: 2026-07-17
updated: 2026-07-17
status: design-proposal
sources: ["FORMAL-01", "FORMAL-02", "FORMAL-03", "FORMAL-04", "COORD-02"]
related: ["Protocol-State-Machines", "Fuzzing-and-Fault-Injection", "System-Model"]
---

# Formal modeling plan

## Tool selection

**TLA+/TLC is primary.** The protocol’s principal risks are interleavings among prepare, commit, cancel, epoch bump, delayed messages, crashes, reconnects, and corruption. TLC is well suited to an explicit finite model of these states and to checking inductive safety invariants and bounded liveness assumptions.

**Apalache is secondary.** Use it for typed bounded symbolic checks, larger integer ranges, and cross-checking selected safety properties. Its bounds and semantics must be documented; it does not replace TLC exploration or proof.

**P is implementation-near.** Model coordinator and ranks as communicating state machines, generate reproducible schedules, and use monitors for stale acceptance, partial commit, and corrupt read. Map production trace events to P events for conformance and use P traces as stateful-fuzzer seeds.

**Alloy is optional and static.** Use it for finite relational assertions: rank/shard coverage, no overlap, topology descriptor uniqueness, conversion-contract preconditions, and manifest graph reachability. It is not the primary temporal protocol model.

## Included model

`formal/tla/HaloKV.tla` is a small safety model with two ranks, bounded epochs, delayed prepare messages, durable/prepared sets, commit/cancel terminal states, crashes, topology faults, corruption, repair, and read acceptance. `HaloKV.cfg` checks the core invariants. The model is intentionally abstract: page counts, bytes, cryptography, and transport frames are represented by validated actions rather than detailed data structures.

## Recorded baseline result

The included `Ranks={r0,r1}`, `MaxEpoch=2` configuration was parsed by SANY and exhaustively explored by TLC build `2026.03.02.213938` on 2026-07-17. TLC generated 2,800,241 states, found 242,384 distinct states at maximum depth 34, emptied the queue, and reported no invariant violation. See [[Validation-Evidence]] and `validation/tlc-output.txt`. This result validates only the current abstraction and does not discharge the extension matrix or implementation refinement obligations below.

## Safety properties

| ID | Property | Model expression |
|---|---|---|
| F-1 | no partial global commit | committed implies `prepared = durable = Ranks` |
| F-2 | no commit after cancellation | committed implies `cancelled = FALSE` |
| F-3 | stale prepare is not accepted | prepared rank has accepted the current epoch |
| F-4 | topology-valid commit only | `Commit` is enabled only when all expected ranks are topology-compatible; later incompatibility makes reads reject |
| F-5 | corrupt state is not read | accepted read implies all required rank objects uncorrupt |
| F-6 | authority epoch is monotonic | each rank’s accepted epoch never exceeds authoritative epoch |
| F-7 | exact rank set | certificate rank set equals configured ranks |
| F-8 | terminal operation is immutable within epoch | no transition changes committed to aborted or vice versa |

Extend the model to multiple checkpoint sequences to check monotonic commit order, garbage-collection reachability, and fallback to prior valid checkpoints.

## Liveness properties and assumptions

Liveness must be conditional, not absolute. Candidate properties:

- If authority remains available, both ranks remain online and compatible, resources are eventually granted, and messages are fairly delivered, an open checkpoint eventually commits or aborts.
- A cancellation accepted before commit eventually stops rank work and reaches a terminal abort.
- A fenced stale instance cannot indefinitely prevent the current epoch from attaching.
- Under bounded corruption and an available independent copy or replay path, recovery eventually reaches ready or a terminal reset decision.

State environmental fairness separately from protocol obligations. Do not claim progress across a permanent partition, unavailable authority, full disk, or missing complete model.

## TLC exploration matrix

Run increasingly rich configurations:

1. Two ranks, epochs `0..2`, one checkpoint, no corruption; all message loss/reorder/duplication represented by nondeterministic delivery.
2. Add cancellation before/after each prepare and at the commit boundary.
3. Add rank crash/recover and stale messages retained across epoch bump.
4. Add topology mismatch and corruption before and after commit.
5. Add two checkpoint sequences and coordinator restart/ambiguous commit observation.
6. Add bounded credits/backpressure and verify no resource count becomes negative or unbounded.
7. Add old/new instance identities and reconnect attach barriers.

Use symmetry sets for ranks where shard-specific behavior is abstracted. Apply state constraints only after preserving counterexample-relevant states. Record TLC version, config, seed, workers, state count, distinct states, depth, and fingerprint in CI artifacts.

## Refinement plan

Map implementation events to abstract actions:

```text
checkpoint.begin.accepted       -> Begin
page.publish.durable(rank)      -> WriteRank(rank)
rank.prepared.received(rank)    -> DeliverPrepared(rank, epoch)
certificate.cas.succeeded       -> Commit
checkpoint.cancel.linearized    -> Cancel
session.epoch.persisted         -> BumpEpoch / AcceptEpoch
object.integrity.failed(rank)   -> Corrupt(rank)
checkpoint.materialize.result   -> TryRead
```

A conformance checker verifies action preconditions and invariants against recorded traces. Fields omitted by abstraction—digests, byte counts, deadlines—are validated by separate schema/property tests.

## Counterexample workflow

For every counterexample:

1. minimize the trace while preserving failure;
2. classify model bug, underspecified assumption, or protocol bug;
3. add the trace to `fuzz/traces/` as a deterministic regression;
4. update the state machine/schema if behavior changes;
5. record the decision and invariant in the wiki;
6. rerun TLC, P, and implementation regression suites.

## Exit criteria for design review

- Core invariants hold across the agreed bounded model matrix with no unexplained state constraints.
- Every liveness claim states environmental assumptions and has at least one adverse schedule test.
- Production trace vocabulary covers all state-changing transitions and linearization points.
- Model and schemas agree on terminal states, epoch rejection, rank completeness, and cancellation outcomes.
- Known deliberate limitations—especially single-node infeasibility—are represented as model guards, not comments only.
