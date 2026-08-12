---
type: research-follow-up
status: proposed
priority: P0
target:
  - wiki/HaloFPX_Wiki/07_Distributed_Runtime/39_Coordinator_Rank_Worker_Session_and_Persistent_Graph_Architecture
  - wiki/HaloFPX_Wiki/07_Distributed_Runtime/45_Persistent_Rank_Protocol_Command_Rings_and_Graph_Reuse
  - wiki/HaloFPX_Wiki/07_Distributed_Runtime/48_Distributed_Correctness_Determinism_Fault_Recovery_and_Degraded_Mode
  - wiki/HaloFPX_Wiki/08_Fabric_and_Transport/53_Message_Framing_Credits_Flow_Control_Integrity_and_Security
  - wiki/HaloFPX_Wiki/09_HaloKV_Persistent_Cache/63_Durability_Modes_Atomic_Commit_Crash_Recovery_and_Corruption_Handling
created: 2026-07-16
last_researched: 2026-07-17
risk: low
approval_required: human
---

# Distributed protocol model-checking strategy

## Outcome

**[RECOMMENDATION]** Build one small TLA+ specification and use the TLC model checker as the P0 authority for exhaustive finite-state safety and liveness checking. Use native TLA+ for the cross-cutting state/action relation and temporal properties; use PlusCal only for coordinator/rank process loops where it materially improves reviewability, retaining and reviewing its generated TLA+ translation. Pin the command-line `tla2tools.jar` from stable TLA+ Tools release `v1.7.4`, tag commit `5a47802b5c391f59ecdd44117981f4ff8c0656ba`, and record a project SHA-256 for the downloaded artifact. Do not make the unmaintained Toolbox GUI or the `v1.8.0` prerelease a CI dependency.

After the TLC model is stable, make it acceptable to Apalache `v0.57.0` and use Apalache as a second, independently implemented checker for bounded safety and selected inductive invariants. Do not make Apalache the first or only checker, and do not treat a bounded Apalache success as a liveness proof.

The first model should be a protocol abstraction, not an implementation transcription. It must make message loss, duplication, reordering, crash, restart, stale epochs, partial durability, cancellation races, and retry decisions nondeterministic while replacing tensors, payload bytes, crypto, GPU work, and filesystem details with finite tokens and state labels.

This follow-up changes no wiki authority and creates no claim that the future implementation conforms to the model.

## Why TLA+/PlusCal with TLC is the practical P0

**[VERIFIED]** TLA+ is state based and intended for concurrent and distributed systems. PlusCal is an algorithm language translated to TLA+, and its primary debugging path is TLC. TLC is an explicit-state checker that checks safety and liveness properties [MC-01, MC-02].

**[VERIFIED]** Stable TLA+ Tools `v1.7.4` specifically fixed a liveness-checking unsoundness involving multiple workers. Its CLI is a Java archive and the upstream project documents Java 11 or newer [MC-03, MC-04].

**[INFERENCE]** This matches HaloFPX better than a proof-first tool because the immediate need is to expose counterexample traces in a proposed two-rank protocol, not prove an unbounded parameterized family or generate implementation code. PlusCal is readable enough to review beside the proposed coordinator/rank state machines, while TLA+ permits explicit temporal properties and fairness assumptions.

## Tool comparison

| Tool | Primary capability | Fit for this P0 | Practical concerns | Decision |
|---|---|---|---|---|
| TLA+/PlusCal + TLC `v1.7.4` | Explicit-state safety, deadlock, and liveness checking with counterexample traces | Best match for epochs, crash/restart, messages, cancellation, commit visibility, and degraded-mode temporal behavior | Finite model required; state explosion demands small bounds and symmetry; specification/implementation conformance remains separate | **Primary** |
| Apalache `v0.57.0` | SMT-backed bounded safety and inductive-invariant checking for typed TLA+; temporal checking is available but bounded/incomplete in relevant modes | Reuses the TLA+ state/action definitions and may handle arithmetic constraints differently from TLC | Requires compatible TLA+ subset/type annotations and more memory; bounded success means only “no counterexample within bound” | **Secondary safety checker after TLC** |
| Spin/Promela `6.5.2` | Mature on-the-fly protocol checker with buffered channels, assertions, deadlock/non-progress checks, and LTL | Technically strong for credits and lossy/reordered messages | Would create a second semantic model in Promela and require C compiler/Unix-like tooling; duplicate-spec drift outweighs benefit for P0 | **Do not adopt initially** |
| Ivy (`microsoft/ivy` HEAD `9f3c7ecc0b2383129fdd0953e10890d98d09a82d`, observed 2026-07-17) | Automated inductive-invariant verification in a decidable fragment; protocol implementation/testing support | Attractive later if HaloFPX needs parameterized proofs or generated/testable interfaces | Higher proof/modeling burden and a specialized ecosystem; implementation glue becomes part of the trusted boundary | **Revisit after protocol stabilizes** |
| Alloy `6.2.0` | SAT-based bounded relational model finding with mutable state and temporal logic | Good for static relationships such as manifest/shard consistency and compatibility graphs | Operational queues, fairness, credits, and retry progress are less direct; bounded scopes do not establish unbounded correctness | **Optional focused model, not the protocol authority** |

**[RECOMMENDATION]** Keep one authoritative behavioral specification. Do not maintain “equivalent” TLA+ and Promela models. Cross-checking with Apalache is valuable because it consumes substantially the same TLA+ model instead of introducing a separately translated protocol.

## Cross-section issue to resolve before implementation

Section 45 distinguishes `cluster_epoch`, per-ring `ring_epoch`, and per-session `session_epoch`. Section 53's proposed 64-byte frame has a single field named `session_epoch` but describes it as the active global epoch. **[INFERENCE]** That naming can conceal stale-message acceptance when a session is cancelled without a topology reset, or when a ring resets without a cluster change.

**[RECOMMENDATION]** The model must use three distinct scopes:

- `clusterEpoch`: coordinator/worker boot identity, topology, model plan, communicator, or leadership generation;
- `ringEpoch[rank,lane]`: command/completion-ring reset generation;
- `sessionEpoch[session]`: cancellation, rewind, restore, or migration generation.

Before wire implementation, section 53 must decide which scopes are carried in each frame or are unambiguously bound by the authenticated connection/ring. Model configurations must deliberately vary one epoch while keeping the others constant.

## Bounded first model

### Purpose and limits

Model only the correctness boundary from accepted request through command execution, visible-token commit, cancellation/failure, checkpoint publication, recovery, and degraded admission. Do not model logits, tensor arithmetic, cryptographic algorithms, byte serialization, real time, HIP graphs, or storage firmware.

The model proves properties only of its abstraction and chosen finite instances. Implementation assertions, trace refinement, property-based tests, and section-80 fault injection remain required.

### Initial constants

| Constant | P0 bound | Reason |
|---|---:|---|
| coordinator boot identities | 2 (`old`, `new`) with one authority lease per cluster epoch | Allow process overlap and delayed old messages during restart without designing consensus |
| ranks | 2 | Exact intended coupled topology |
| rails | 2 for safety; 1 for reduced liveness runs | Exercise reordering/failure across rails without exploding every liveness run |
| sessions | 1 initially, then 2 in isolation suite | One exposes protocol races; two checks cross-session fencing |
| command sequences | `0..2` per ring epoch | Enough for original, duplicate, gap/reorder, and post-reset command |
| ring slots | 2 | Exercise empty, partial, and full/backpressure states |
| credit capacity | 2 abstract units per rail plus 1 reserved control unit | Exercise exhaustion, duplicate credit, release, and control progress |
| checkpoint generations | `0..2` | Previous, prepared candidate, and later committed generation |
| crashes | at most one coordinator crash and one rank crash per scenario | Ensures finite recovery traces; repeated crashes belong in a later sweep |
| network queues | at most 3 abstract records per rail/direction | Exercise loss, duplication, delay, and cross-rail reorder |

Bounds are configuration, not protocol constants. Run separate suites rather than one Cartesian product: epoch/replay, credits, cancellation, checkpoint publication, and degraded recovery.

### State variables

- Authority: `coordAlive[boot]`, `coordState[boot]`, `coordAuthority`, `clusterEpoch`, `topology`, `mode` (`Dual`, `Degraded`, `Unavailable`).
- Ranks: `rankBoot[r]`, `rankState[r]`, `readyEpoch[r]`, `lastAccepted[r]`, `lastCompleted[r]`.
- Rings: `ringEpoch[r]`, `head[r]`, `tail[r]`, `slots[r]`, terminal replay cache.
- Sessions: `sessionEpoch[s]`, `sessionState[s]`, `cancelledAt[s]`, `committedIteration[s]`, `visibleTokens[s]`.
- Commands: identity tuple `(clusterEpoch, ringEpoch, sessionEpoch, seq, request, iteration)`, status, and abstract side-effect count.
- Transport: per-rail record/message queues, `recordSeq`, applied credit-message IDs, `availableCredit`, `dataInFlight`, `bytesHeld`, `releasedPendingCredit`, `creditInFlight`, control reserve, link state.
- Checkpoints: `generation`, predecessor, durability mode, required ranks, compatibility/topology fingerprint, per-rank object state (`Absent`, `Prepared`, `Durable`, `Corrupt`), manifest state (`Absent`, `Written`, `Durable`, `Published`, `Invalid`), and client acknowledgement state.
- Recovery: selected committed generation, recompute boundary, degraded reason, and admitted topology epoch.

### Actions

1. `StartCoordinator`, `StartRank`, `Hello`, `ValidatePlan`, `BecomeReady`.
2. `IssueCommand`, `EnqueueRecord`, `DropRecord`, `DuplicateRecord`, `DeliverRecord`, `ReorderRecords`.
3. `AcceptCommand`, `ExecuteCommand`, `PublishCompletion`, `CommitVisibleToken`.
4. `ReleaseBuffer`, `SendCredit`, `ApplyCredit`, `DuplicateCredit`.
5. `CancelSession`, `ExpireDeadline`, `DiscardLateCompletion`.
6. `CrashRank`, `RestartRank`, `CrashCoordinator`, `RestartCoordinator`, `FaultRing`, `ResetRing`, `AdvanceClusterEpoch`.
7. `PrepareRankCheckpoint`, `MakeRankDurable`, `CorruptObject`, `WriteManifest`, `MakeManifestDurable`, `PublishManifest`, `AbandonGeneration`.
8. `RecoverCheckpoint`, `RejectCheckpoint`, `RecomputeFromCommittedTokens`, `EnterDegraded`, `RefuseDegraded`.
9. `RetryUncommittedWork` only after an explicit decision and in a new epoch; no automatic replay action crosses an epoch.

Every action must be small enough that a counterexample identifies one protocol decision. “Reconnect and recover” must not be one atomic action.

`DropRecord` must not silently refund DATA credit. Once a sender reserves receiver capacity, the model keeps that unit in `dataInFlight` across loss and same-identity retransmission until the receiver releases it, or an epoch-reset action explicitly abandons and reinitializes the old channel state. Otherwise the model could “prove” conservation by granting capacity the real receiver has not released.

## Safety properties

| ID | Property | Required assertion |
|---|---|---|
| MC-S01 | Epoch isolation | A record/command/completion whose cluster, ring, or session epoch is stale cannot mutate current rank/session state or visible output. |
| MC-S02 | At-most-once side effects | A command identity executes at most once; a duplicate returns/reuses a terminal result or faults, never launches a second side effect. |
| MC-S03 | Output commit agreement | A visible token/iteration commits only after every required current rank has a matching successful completion for the same plan, epochs, request, session, and iteration. |
| MC-S04 | Cancellation fencing | After `sessionEpoch` advances, no completion from the old epoch can become visible; it may only release resources. |
| MC-S05 | Ring boundedness | `0 <= tail-head <= depth`; an unconsumed slot or referenced buffer generation is never overwritten/reused. |
| MC-S06 | Credit conservation | For each rail/channel/epoch, `0 <= availableCredit <= capacity` and `availableCredit + dataInFlight + bytesHeld + releasedPendingCredit + creditInFlight = capacity`; a credit ID is applied at most once. Lost/retransmitted credit cannot create or destroy capacity. |
| MC-S07 | Control reserve | Bulk work never consumes the reserved control capacity required for credit, cancel, reset, or close progress. |
| MC-S08 | Collective/order agreement | Coupled ranks never execute different plan/iteration/collective ordinals as one successful iteration; mismatch faults the topology. |
| MC-S09 | Atomic checkpoint visibility/durability | A manifest becomes logically `Published` only when every required rank object is valid and prepared for the same generation/fingerprint. `turn-durable`/`strict` acknowledgement additionally requires all promised objects and the manifest to be durable. After crash, every mode recovers only a complete durable valid manifest; `performance` mode may instead lose the checkpoint and recompute. |
| MC-S10 | No mixed-generation recovery | Recovery never combines rank objects from different generations, topology fingerprints, or compatibility IDs. Corruption yields rejection/miss. |
| MC-S11 | Safe retry | Retried work was not visibly committed, and retry uses a fresh applicable epoch or an identical cached terminal response; no ambiguous same-epoch re-execution. |
| MC-S12 | Safe degraded entry | `Degraded` readiness uses a fresh cluster epoch and begins only from a valid published checkpoint or deterministic recomputation from committed tokens; partial distributed KV is never reused. |
| MC-S13 | Authority fencing | No two coordinator boot identities can commit output or publish a manifest in the same cluster epoch. |
| MC-S14 | Monotonic visibility | Visible token iteration and published checkpoint generation never move backward within one session lineage. |

## Liveness properties and explicit assumptions

Liveness is conditional. Permanent partition, permanently failed rank, endless coordinator crash, or an environment that never schedules an enabled action cannot promise success.

Use weak fairness only for protocol actions that an implementation can continuously enable and schedule. Do not add fairness merely to silence a counterexample.

| ID | Conditional liveness property | Assumptions |
|---|---|---|
| MC-L01 | An accepted, noncancelled command eventually reaches one terminal outcome: committed success, explicit fault, or epoch-fenced abort. | Required ranks and control path eventually remain available; worker/delivery/completion actions are weakly fair. |
| MC-L02 | Released receive storage eventually restores usable credit exactly once. | Control channel eventually delivers; credit processing is weakly fair. |
| MC-L03 | A cancellation eventually yields `CANCELLED`, `TOO_LATE`, or epoch reset; the session does not remain indefinitely “cancelling.” | Coordinator/worker control loops eventually run. |
| MC-L04 | After a detected coupled-rank failure, the system eventually becomes `Unavailable` or reaches valid `Degraded` readiness; it never waits forever in an ambiguous dual mode. | Failure detector eventually reports and coordinator remains available long enough to decide. |
| MC-L05 | A fully durable prepared checkpoint is eventually published or explicitly abandoned; partial generations never block newer work forever. | Coordinator and storage actions eventually run; no permanent storage failure. |
| MC-L06 | Control traffic continues to make progress under sustained bulk load. | Scheduler honors the modeled bounded priority/fairness rule. |

Run liveness separately with reduced bounds and TLC single-worker mode until the pinned tool's liveness behavior is revalidated in project CI. Stable `v1.7.4` fixed a multi-worker liveness unsoundness; the project should keep a known violating negative model to ensure the checker detects it [MC-03].

## Required model configurations

1. `EpochReplaySafety`: two ranks/two rails, stale cluster/ring/session records, duplicate/gap/reorder, ring reset, worker/coordinator restart.
2. `CreditSafety`: two rails, full bulk window, duplicated/lost/reordered credit messages, reserved control capacity, cancellation while full.
3. `CancelRetrySafety`: cancellation/timeout before accept, during execute, after completion but before visible commit, and after commit.
4. `CheckpointAtomicity`: rank prepare/durability interleavings, coordinator crash at every manifest transition, corruption, missing rank, stale fingerprint.
5. `DegradedSafety`: rank loss before/after token commit and checkpoint publish; fallback by valid checkpoint, recomputation, or refusal.
6. `ProgressLiveness`: one rail/one session/smallest buffers with declared fairness and finite fault recovery.
7. `TwoSessionIsolation`: two sessions with cancellation/restore of one while the other progresses.

For every configuration retain: `.tla`, generated translation, `.cfg`, exact tool version/hash, command, state count, depth, runtime/memory, result, and counterexample trace. A property that has never failed should also have a deliberate mutation/negative model proving the checker and configuration can detect its violation.

## Traceability to current sections

| Section | Model ownership | Actions/properties |
|---|---|---|
| 39 - coordinator/rank architecture | Authority, lifecycle, session state, output commit, startup/readiness | `Start*`, `Hello`, `BecomeReady`, `CommitVisibleToken`; MC-S03, S04, S13; MC-L01, L04 |
| 45 - persistent rank protocol | Cluster/ring/session epochs, command identity, bounded rings, replay cache, cancellation, reset | `Issue/Accept/Execute/Complete`, `FaultRing`, `ResetRing`; MC-S01, S02, S04, S05, S08, S11 |
| 48 - correctness/recovery/degraded mode | Failure detection, no partial success, recomputation boundary, degraded admission | `Crash*`, `AdvanceClusterEpoch`, `Recompute`, `Enter/RefuseDegraded`; MC-S03, S08, S12-S14; MC-L04 |
| 53 - framing/credits/retries | Loss/duplication/reordering, credit IDs/conservation, control reserve, cancel/ACK semantics, reconnect epoch | `Enqueue/Drop/Duplicate/Deliver`, `Send/ApplyCredit`; MC-S01, S06, S07, S11; MC-L02, L03, L06 |
| 63 - atomic durability | Rank-local prepared/durable objects, immutable manifest publication, corruption rejection, recovery | `PrepareRankCheckpoint` through `Recover/RejectCheckpoint`; MC-S09, S10, S12, S14; MC-L05 |

## Implementation and test mapping after the model finds a stable protocol

**[RECOMMENDATION]** Every model action must map to one implementation event/assertion and at least one test hook. Every safety property must map to an always-on debug assertion or invariant metric where practical. Every retained counterexample must become a deterministic simulator/property test, and relevant crash/order traces must become section-80 fault-injection cases.

Examples:

- MC-S01 -> reject counters keyed by stale epoch scope; tests vary one epoch at a time.
- MC-S02 -> execution-side-effect counter per command identity; duplicate delivery asserts no increment.
- MC-S06 -> checked 64-bit credit accounting assertion and saturation/duplicate-credit tests.
- MC-S09 -> manifest publish hook refuses any nondurable/missing rank descriptor.
- MC-S12 -> degraded admission records checkpoint generation or recompute boundary and new cluster epoch.

The mapping is refinement evidence, not a machine-checked proof that C++/HIP/filesystem code implements the TLA+ model.

## Delivery sequence and exit gates

1. Write a state/action TLA+ skeleton with no liveness and check type/syntax.
2. Add safety invariants one at a time; intentionally weaken each rule and retain at least one understandable TLC counterexample.
3. Split safety configurations to keep exhaustive state spaces reviewable; use symmetry sets for rank/rail IDs only when the property is actually symmetric.
4. Add crash/restart and checkpoint publication, then degraded entry.
5. Add liveness only after the safety model stabilizes; document every fairness clause in plain language.
6. Type-annotate the compatible subset and run Apalache bounded safety/inductive checks as a second engine.
7. Map actions/properties/counterexamples to implementation assertions and fault tests before accepting a protocol ADR.

P0 exits only when:

- TLC exhaustively passes all bounded safety configurations and the reduced liveness configuration;
- deliberate broken variants produce expected counterexamples;
- Apalache checks the agreed safety subset or a documented incompatibility is reviewed;
- epoch naming/scope and retry/commit boundaries are resolved across sections 39/45/48/53/63;
- every fairness assumption and abstraction limit is explicit;
- counterexample-derived protocol changes are proposed for human review, not silently applied to wiki pages.

## Sources

All web sources were accessed 2026-07-17.

| ID | Primary source | Use and limitation |
|---|---|---|
| MC-01 | Leslie Lamport, [TLA+ high-level view](https://lamport.azurewebsites.net/tla/high-level-view.html) and [TLA+ tools](https://lamport.azurewebsites.net/tla/tools.html) | State-based distributed modeling; TLC safety/liveness capability. General tool description, not HaloFPX evidence. |
| MC-02 | Leslie Lamport, [PlusCal tutorial introduction](https://lamport.azurewebsites.net/tla/tutorial/intro.html), revised 2024-08-21 | PlusCal-to-TLA+ workflow and TLC use. |
| MC-03 | TLA+ Tools [stable `v1.7.4` release](https://github.com/tlaplus/tlaplus/releases/tag/v1.7.4), released 2024-08-05, tag commit `5a47802b5c391f59ecdd44117981f4ff8c0656ba` | Stable CLI artifact and liveness-unsoundness fix. Upstream provides SHA-1; project should additionally record SHA-256. |
| MC-04 | TLA+ Tools [repository README](https://github.com/tlaplus/tlaplus/blob/5a47802b5c391f59ecdd44117981f4ff8c0656ba/README.md) | `tla2tools.jar`, Java 11+, CLI usage. |
| MC-05 | Leslie Lamport, [Safety, Liveness, and Fairness](https://lamport.azurewebsites.net/tla/safety-liveness.pdf), 2019-05-26 | Precise distinction and fairness operators. |
| MC-06 | Newcombe et al., [How Amazon Web Services Uses Formal Methods](https://doi.org/10.1145/2699417), CACM 58(4), 2015 | Primary industrial case study showing design-level TLA+ use; not evidence for this design. |
| MC-07 | Apalache [`v0.57.0` release](https://github.com/apalache-mc/apalache/releases/tag/v0.57.0), released 2026-04-24, annotated tag object `54157d602bfe6da66faa5cf32b2a2e248856af47`, dereferenced commit `635865a13998751a2955bf76552a79bb43d69bc5`; [running guide](https://apalache-mc.org/docs/apalache/running.html) | Current release and bounded/inductive safety semantics. Bounded checking is explicitly incomplete. |
| MC-08 | Apalache [installation guide](https://apalache-mc.org/docs/apalache/installation/index.html) and [symbolic-checking principles](https://apalache-mc.org/docs/apalache/principles/) | JVM/package options, memory guidance, type annotations and symbolic constraints. |
| MC-09 | Spin [source repository](https://github.com/nimble-code/Spin/tree/090f74209025a53297990ec17deca1bd51cb92a5), commit `090f74209025a53297990ec17deca1bd51cb92a5`; [`version.h`](https://github.com/nimble-code/Spin/blob/090f74209025a53297990ec17deca1bd51cb92a5/Src/version.h) reports `6.5.2`, 2025-09-18; [official manual](https://spinroot.com/spin/Man/Manual.html) | Promela protocol/channel model, deadlock, safety and liveness. The older spinroot landing page still says 6.5.1, so the exact source commit is authoritative for version here. |
| MC-10 | Microsoft Research, [Ivy language](https://microsoft.github.io/ivy/language.html), [invariant tutorial](https://microsoft.github.io/ivy/examples/client_server_example.html), and [decidability guide](https://microsoft.github.io/ivy/decidability.html) | Inductive-invariant and decidable-fragment capabilities. |
| MC-11 | Alloy Tools, [official site](https://alloytools.org/) and [Alloy 6 language reference](https://alloytools.org/spec.html), release `6.2.0`, 2025-01-09 | Bounded relational/temporal model finding; official docs warn that failure to find an instance within scope is not global proof. |
| MC-12 | HaloFPX sections [39](../../wiki/HaloFPX_Wiki/07_Distributed_Runtime/39_Coordinator_Rank_Worker_Session_and_Persistent_Graph_Architecture/README.md), [45](../../wiki/HaloFPX_Wiki/07_Distributed_Runtime/45_Persistent_Rank_Protocol_Command_Rings_and_Graph_Reuse/README.md), [48](../../wiki/HaloFPX_Wiki/07_Distributed_Runtime/48_Distributed_Correctness_Determinism_Fault_Recovery_and_Degraded_Mode/README.md), [53](../../wiki/HaloFPX_Wiki/08_Fabric_and_Transport/53_Message_Framing_Credits_Flow_Control_Integrity_and_Security/README.md), and [63](../../wiki/HaloFPX_Wiki/09_HaloKV_Persistent_Cache/63_Durability_Modes_Atomic_Commit_Crash_Recovery_and_Corruption_Handling/README.md), researched 2026-07-16 | Project design inputs. They are drafts/needs-machine-validation, not verified implemented behavior. |

## Review decision requested

Approve a P0 implementation task to create the versioned TLA+/PlusCal model, configurations, CI runner, negative variants, and retained counterexample traces. Keep the model under an implementation/research artifact path chosen by project governance; do not promote its conclusions into the wiki until the model, assumptions, and protocol changes are independently reviewed.
