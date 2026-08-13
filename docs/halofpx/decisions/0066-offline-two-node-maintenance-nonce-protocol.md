# ADR-0066: offline two-node maintenance nonce protocol

Status: proposed offline deterministic model. Its focused tests establish only
the synthetic contract described below. Real node-side consumption,
maintenance authorization, distributed atomicity, issue-#41 closure, target
execution, and performance are not accepted.

Date: 2026-08-13

## Context

ADR-0057 keeps the Strix maintenance controller target-disabled and lists
atomic, replay-proof consumption of one owner authorization on both physical
nodes as a mandatory promotion gate. A single control-PC marker cannot meet
that gate. It can be replayed, it does not prove node custody, and a lost
response can hide a one-node state transition.

A real protocol cannot be designed honestly without first separating a
definite refusal from an outcome whose durable state is unknown. In
particular, two participants and an unreliable coordinator cannot guarantee
both atomic completion and nonblocking progress after a crash, partition, or
lost response. An offline test must expose that uncertainty rather than
describe a two-phase exchange as real distributed atomicity.

## Decision

Add a pure Python, in-memory state-machine simulator and closed transcript
verifier. The source has literal `TARGET_EXECUTION_ENABLED = False`; imports no
filesystem, process, socket, HTTP, or SSH implementation; accepts only the
exact `FakeDurableNode` class; and exposes only `simulate` plus stdin `verify`
commands. There is no target Runner, host, service, path, actuator, or
single-node fallback.

One immutable binding owns all of these values:

- SHA-256 of the exact upstream authorization bytes;
- one 256-bit authorization nonce and one 256-bit attempt identity;
- canonical UTC not-before and expiry seconds in a positive window no longer
  than eight hours;
- canonical repository URL, exact 40-hex commit, source-tree SHA-256, and
  controller-executable SHA-256;
- exact plan, policy, and incident SHA-256 values; and
- one positive, consecutive protected epoch.

Canonical JSON and purpose-separated SHA-256 domains derive a binding digest,
transaction identity, and nonce-only commitment. The nonce commitment does not
include the authorization digest, so changing authorization bytes cannot make
reuse of the same nonce look new. Every node record repeats the binding digest,
transaction identity, authorization digest, nonce commitment, epoch, logical
role, synthetic node identity, timestamp, sequence, and prior record digest.
Both logical roles must own byte-identical bindings before the pair can
complete.

The fake persistent state contains an epoch high-water, a sorted set of burned
nonce commitments, and either a zero or exact quarantined transaction identity.
A simulation attempt is admitted only when both fake roles begin with exactly
the same three persistent values. Asymmetric epochs, burned-nonce sets, or
quarantine identities are rejected before either fake starts an attempt; the
verifier also treats divergent transcript prestates as `IN_DOUBT`. The model
does not invent a coordinator-side repair for divergent prestates.
A valid `PREPARED` record advances the epoch and burns the nonce before its
acknowledgement. Abort does not unburn it. Every `IN_DOUBT` result writes the
same transaction quarantine on both fakes, and any later attempt refuses.
There is deliberately no clear operation. This chooses safety over
availability: a one-node prepare can wedge the pair until a separately
designed reconciler resolves the divergent durable state.

The only admitted node histories are:

```text
empty -> PREPARED -> COMMITTED
empty -> ABORTED
empty -> PREPARED -> ABORTED
```

`COMMITTED -> ABORTED`, repeated phases, record overwrite, nonconsecutive
epochs, reused nonce commitments, inactive-window prepare/commit, and changed
bindings refuse. Commit requests are admitted only after acknowledged matching
prepares from both roles. A pair certificate is valid only when both exact
commit records, both acknowledgements, the complete hash-chained event stream,
the active window, and the complete binding agree.

Before any ordinary deterministic scenario begins, the simulator reserves its
whole synthetic timeline from the first request through the serialized
verification timestamp. The success path spans 13 one-second clock offsets in
v1. If either half-open window endpoint is inactive, the simulator rejects the
schedule before resetting or mutating either fake and emits no transcript or
certificate. The separate `late-window` fault requires at least a 12-second
authorization window, deliberately writes two in-window fake commits, reaches
pair verification at expiry, and serializes verification one second later. It
is always `IN_DOUBT`, pair-quarantined, and certificate-free. These fixture
schedules are test-model invariants, not real protocol deadlines, timing
guarantees, or clock-trust claims.

The verifier admits exactly one event, ledger-phase, node-binding-presence,
terminal-status, terminal-reason, and certificate-presence grammar for each
named scenario. Every durable fake record has exactly one matching `ACK` or
`LOST` response and every such response names exactly one record. Orphan,
duplicate, or cross-phase record links are `IN_DOUBT`; self-labeling them as a
refusal cannot clear the required pair quarantine. The verification timestamp
must be strictly later than the final hash-chained event. Each scenario also
has an exact allowed semantic-issue set. A nominal refusal remains `REFUSED`
only when its evidence matches that set exactly; an unexpected ledger, event,
state, binding, terminal, or quarantine contradiction becomes sticky
`IN_DOUBT` and requires the exact transaction quarantine on both fake roles.

Even then, the result is named `SIMULATED_COMPLETE`. The certificate and
verifier always return all of these values as false:

```text
authorized
simulation_qualified
target_execution_qualified
distributed_atomicity_proven
```

`pair_committed` means only that the closed fake transcript contains two valid
simulated commit records. It is not a capability and cannot be consumed by the
maintenance controller.

## Refusal and uncertainty

The verifier has three result classes:

- `SIMULATED_COMPLETE`: both closed fake journals and the pair transcript are
  internally coherent; every authority flag still remains false.
- `REFUSED`: a definite pre-commit failure exists, such as replay, inactive
  window, stale epoch, binding divergence, one-node prepare followed by exact
  abort, or a commit request that was definitely rejected before durable
  commit.
- `IN_DOUBT`: a response was lost, a commit was observed or may have been
  attempted without a complete pair, or post-commit evidence is corrupt,
  missing, reordered, spliced, or contradicts the terminal/certificate.

`IN_DOUBT` never falls back to `REFUSED` because a terminal claims it did. The
v1 model provides no quarantine-clear, retry, abort-after-commit, or
reconciliation operation. Those omissions prevent an ambiguous transition
from being silently reused.

## Explicit impossibility and nonclaim boundary

This model does not prove real distributed atomicity. A two-participant
prepare/commit protocol can block after a coordinator failure; a network
partition or lost response can leave one node committed and the other
prepared. No deterministic asynchronous protocol can infer the missing state
from silence. The simulator therefore treats every such boundary as
`IN_DOUBT` and chooses no recovery action.

In-memory `simulated_durable` records do not establish `fsync`, storage-stack
ordering, crash or power-loss survival, rollback resistance, authenticated
node identity, protected keys, trusted time, monotonic deadlines, mutually
authenticated transport, Byzantine resistance, consensus, watchdog behavior,
production recovery, or terminal receipt reconciliation. Canonical hashing
detects transcript changes only inside this deterministic model; it does not
authenticate who created the bytes.

The pair-quarantine marker is deterministic harness bookkeeping over two
in-memory fakes. It is not a protocol message, an atomic write, or evidence
that an unreachable physical node durably recorded quarantine. A real design
must make unresolved-state admission fail closed independently on each node.

The persistent fake nodes reject a second attempt that reuses the nonce. A
stateless verifier cannot distinguish a byte-for-byte replay of an old complete
transcript from the original transcript because it has no protected current
high-water source. That old transcript remains non-authorizing here. A real
gate must compare the candidate with current anti-rollback state owned by both
authenticated nodes; carrying a self-asserted `before` object is insufficient.

Untrusted transcript pre-scanning is iterative and bounded by depth and node
count. Exhausting either bound preserves possible-commit uncertainty and never
authorizes. Deep JSON parser recursion is converted to a closed non-authorizing
CLI result. These are parser-availability controls, not an authentication
mechanism.

A future real design still needs separately reviewed owner-signature
verification and exact executable binding; authenticated node agents; trusted
wall and monotonic time; protected anti-rollback storage with specified
durability; a declared crash/partition threat model; an owner-approved
blocking reconciliation or an appropriate quorum/consensus service; an
independent worker-first recovery watchdog; live closed-world HMM admission;
complete adapter evidence validation; and paired terminal reconciliation.
This ADR supplies none of those gates and issue #41 remains `REFUSE`.

## Qualification

The focused suite uses only synthetic bindings, clocks, nodes, records, and
faults. It covers success with permanent non-authorization; durable abort;
nonce burn and real fake-state replay; one-node and split-brain state; expired
window; stale epoch; lost prepare, commit, and abort responses; phase reorder;
record/event/certificate corruption; one-node commit; sticky pair quarantine;
later-attempt refusal; orphan prepare/abort/commit records; closed scenario
relabeling; impossible prepended exchanges; record/response bijection;
backdated verification; cross-authorization nonce reuse; cross-transaction
splice; post-window verification and exact expiry boundaries for every named
scenario; refusal state burn/high-water loss; asymmetric fake prestates; every
material binding component; malformed, deep, and duplicate JSON; unsafe
authority flags; exact-fake admission; and source scans that reject target/
filesystem/process/network imports or commands.

Continuous integration runs this suite beside the existing Strix harness,
CachyOS adapter, and maintenance-controller offline tests. Hosted execution is
not CachyOS, ROCm, `gfx1151`, target, or performance evidence.

## Relationship and rollback

ADR-0057 remains the maintenance transaction and promotion-gate authority.
The issue-#41 HMM/global-OOM receipt remains the target stop authority. This
model can later serve as a specification oracle for a separately reviewed
real protocol, but its transcript or certificate is never an input that
enables target work.

Rollback removes the simulator, focused tests, operations document, CI entry,
this ADR, and its decision-index entry. It changes no implementation runtime,
model, cache, target, production unit, or stored target state.
