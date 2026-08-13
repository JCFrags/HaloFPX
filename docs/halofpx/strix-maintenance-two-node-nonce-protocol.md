# Offline two-node maintenance nonce protocol

Status: **deterministic offline model only; target execution and authorization
remain hard-disabled**

[`halofpx_strix_nonce_protocol.py`](../../scripts/halofpx_strix_nonce_protocol.py)
models the paired nonce-consumption gate required by
[ADR-0057](decisions/0057-offline-strix-maintenance-admission-controller.md).
It is a safety specification for later design work, not a deployable node
agent. [ADR-0066](decisions/0066-offline-two-node-maintenance-nonce-protocol.md)
owns the complete decision and nonclaim boundary.

## What the model checks

Both synthetic roles must evaluate the same canonical authorization binding:

| Binding component | Closed representation |
|---|---|
| authorization | SHA-256 of the exact upstream authorization bytes |
| replay identity | 256-bit nonce, positive consecutive epoch, 256-bit attempt ID |
| time | canonical UTC not-before/expiry, positive and at most eight hours |
| source | canonical repository, exact commit, tree digest, executable digest |
| experiment | exact plan and policy digests |
| safety incident | exact incident digest |

Each in-memory fake node keeps a persistent epoch high-water, burned-nonce set,
and quarantined transaction identity across simulated attempts. `PREPARED`,
`COMMITTED`, and `ABORTED` records are canonical, purpose-hashed, append-only,
sequence-checked, and hash-chained. The event stream separately binds every
request, response, role, record digest, and phase order.

Both fake roles must enter an attempt with identical epoch high-water,
burned-nonce set, and quarantine identity. A mismatch is rejected before the
attempt begins, and the verifier classifies a transcript with divergent
prestates as `IN_DOUBT`. This is a closed simulator precondition, not a
reconciliation algorithm for real divergent node state.

The burned commitment is derived from the nonce alone. The authorization
digest remains in the full binding, transaction, records, and certificate, but
rotating authorization bytes does not permit the same nonce to be consumed
again.

Replay rejection depends on that persistent fake state. The stdin verifier is
stateless and cannot tell that a byte-identical old transcript has been shown
twice. This is safe only because every verifier result is non-authorizing. A
future real gate requires protected current high-water state on both nodes.

Prepare burns the nonce before acknowledgement. Both matching prepare
acknowledgements must exist before commit. Abort is allowed only before commit.
A valid two-record pair still produces only `SIMULATED_COMPLETE`; it never
produces maintenance authorization.

Every ordinary deterministic scenario reserves its complete synthetic clock
interval through verification before the attempt begins. In v1, the success
fixture ends 13 seconds after its first request. A schedule whose verification
would be at or beyond the half-open expiry boundary is rejected before either
fake is reset or mutated. The separate `late-window` fixture requires a
12-second window, places both fake commits inside it, reaches pair verification
at expiry, then reports `IN_DOUBT`, writes pair quarantine, and emits no
certificate. These schedules describe the fake only; they are not production
timeouts or real-time guarantees.

Each named fault scenario has one closed event sequence, node-ledger phase
sequence, binding-presence shape, terminal status/reason, and certificate
presence. Every durable record must be named by exactly one matching `ACK` or
`LOST` response, and every such response must name exactly one durable record.
The verification timestamp must follow the final event. Relabeling a scenario,
prepending a rejected exchange, orphaning a record, or backdating verification
cannot yield `SIMULATED_COMPLETE` or downgrade uncertainty to `REFUSED`.
The verifier additionally requires the exact semantic issue set declared for
that scenario. Any extra ledger, event, state, binding, terminal, or quarantine
integrity issue turns a nominal refusal into `IN_DOUBT` and requires matching
pair quarantine.

## Interpreting verifier output

The output has three classifications:

| Classification | Meaning | Action authority |
|---|---|---|
| `SIMULATED_COMPLETE` | the closed fake pair is internally coherent | none |
| `REFUSED` | a definite pre-commit rejection or exact abort is represented | none |
| `IN_DOUBT` | a response was lost or a commit may exist without complete trustworthy pair evidence | quarantine; no retry/abort/fallback authority |

Every result fixes `authorized`, `simulation_qualified`,
`target_execution_qualified`, and `distributed_atomicity_proven` to false.
`IN_DOUBT` writes the same transaction quarantine into both fake states and is
sticky in v1: every later attempt refuses. The tool has no quarantine-clear or
reconciliation command and no single-node fallback. This shared-memory harness
bookkeeping does not prove that an unreachable real node received or persisted
any quarantine marker.

## Local-only examples

Generate and verify a deterministic success-shaped fake transcript through
standard input/output only:

```powershell
python -B scripts/halofpx_strix_nonce_protocol.py simulate --scenario success |
  python -B scripts/halofpx_strix_nonce_protocol.py verify
```

The result is `SIMULATED_COMPLETE` and still has `authorized=false`.

Exercise a lost commit response:

```powershell
python -B scripts/halofpx_strix_nonce_protocol.py simulate --scenario lost-commit-response |
  python -B scripts/halofpx_strix_nonce_protocol.py verify
```

The verifier returns `IN_DOUBT` and exits nonzero. Supported deterministic
faults also cover one-node prepare, split brain, expiry, stale epoch, replay,
lost prepare/abort responses, phase reorder, durable-record corruption, and a
success-shaped exchange whose verification crosses the authorization expiry.

Run the focused qualification:

```powershell
python -B -m unittest tests.test_halofpx_strix_nonce_protocol -v
```

These commands use fixture digests and an in-memory fake. They do not read a
target path, construct SSH, inspect a service, consume an owner authorization,
arm a watchdog, run a model, or contact either Strix Halo machine.

## Promotion boundary

The model intentionally cannot solve the distributed commit uncertainty that
it demonstrates. Real promotion still requires authenticated node agents,
protected crash-durable anti-rollback storage, trusted wall and monotonic time,
an explicit crash/partition fault model and reconciliation/consensus choice,
cryptographic owner authorization, exact source/executable binding,
independent recovery watchdogs, live HMM admission, complete adapter evidence,
and paired terminal recovery receipts. The existing issue-#41 `REFUSE` state
remains unchanged.
