# ADR-0057: offline Strix maintenance admission controller

Status: accepted for offline domain qualification only. Real target execution,
maintenance authorization, issue-#41 closure, and performance measurement are
not accepted.

Date: 2026-08-12

## Context

The HMM/global-OOM incident retained by PR #44 proved that ordinary available
memory and process RSS are insufficient target-safety signals while production
owns large KFD/render/HMM allocations. Issue #41 therefore blocks every target
build, quantization, disposable inference, and benchmark until an isolated
maintenance window proves exact custody and real distributed recovery.

PR #51 added a read-only-first CachyOS A/B adapter draft, but its target gate is
literal false and it deliberately refuses before constructing its SSH Runner.
It cannot solve maintenance custody by itself. A complete failure model is
needed before any later proposal can join authorization, production shutdown,
the disposable adapter domain, and production recovery.

## Decision

Add a separate Python controller that models the complete transaction through
an explicit offline-fake protocol only. Keep `TARGET_EXECUTION_ENABLED = False`,
define or construct no target Runner or SSH path, and make the public `execute`
command refuse before policy loading. Reuse the merged PR-#51 module only for
its frozen policy/receipt definitions; its separate target gate remains false.
The public `validate` command may check only the closed offline
policy/authorization relationships.

The domain model freezes issue #41, the PR-#44 incident manifest, exact PR-#51
plan/policy bytes and schedule row, a write-once evidence root, protected
before identities, disposable ports/units, and a non-performance two-rank
recovery probe. Every input is closed-schema JSON; raw authorization bytes are
SHA-256-bound by policy. Duplicate fields, expired or oversized windows,
changed digests, identity drift, incomplete censuses, foreign owners, and
uncertain state fail closed.

Shutdown is coordinator-first and recovery is worker-first. Every stop return
or exception is followed by an independent active/absent production snapshot
that closes host, unit ActiveState/SubState, main PID, listener PIDs, cgroup
path/existence/membership, and identity, plus an elevated closed-world
GPU-owner census.
The adapter cannot run until both production ranks are proven absent. Cleanup
and a reconciled census gate recovery. Worker readiness gates coordinator
start. Exact recovered ownership gates the modeled minimal two-rank inference
contract. The terminal may call recovery complete only if both that census and
contract pass and a final authoritative production snapshot exactly matches
the validated recovered identities. Every role once proven absent must remain
absent until an explicit fresh start returns greater process-start and systemd
active-enter monotonic identities; a stale pre-stop PID cannot be classified
as preserved recovery. Stop/start response loss remains a transaction error,
but a separately observed exact postcondition drives safe recovery. Cleanup
similarly admits recovery only through an independent disposable-absence
proof, not through the cleanup command response alone.

Events, original policy/authorization bytes, exact adapter plan/policy bytes,
adapter receipt, intent, terminal, and final hashes are retained under a
never-preexisting evidence root. No result from this controller is a
performance result. A mandatory forward event write failure aborts the body
before its next mutation and enters cleanup and recovery. Recovery evidence
failures remain nonblocking so they cannot prevent worker-first recovery.
`FAILED.json` is distinct from success finalization.
Offline success is accepted only by the strict closed-tree `verify-bundle`
path, which requires a coherent final `COMMITTED.json`, terminal,
`SHA256SUMS`, exact inventory and hashes, empty errors, and every recovery
gate. It also replays the event sequence and sparse adapter receipt semantics
from the retained frozen inputs. Marker existence alone is never authority.
Pre-publication failure makes a best-effort terminal-failure rewrite and
publishes no success marker. A rename error is treated as a possible lost
response, and a post-rename sync failure is similarly ambiguous: both first
attempt to withdraw the marker and never rewrite bytes which an indeterminate
surviving marker may already bind. On Windows, directory sync is unavailable,
so this offline artifact does not claim crash durability. This still is not
atomic two-node real-target terminalization.

## Explicit non-authority and future promotion gates

The v1 GitHub comment reference is metadata, not authentication. Its login,
account ID, URL, and node ID are self-asserted fields. The declared repository
commit is cross-bound between files but is not checked against the current
checkout. The supplied timestamp is fixture input rather than trusted node
time. Nonce custody is local to one evidence directory. `offline_fake=True` is
a test convention, not a security boundary. The sparse adapter receipt proves
ordering only. The fake process has no out-of-band recovery actor;
consequently, disposable residue blocks controller-driven restart, while a
lost cleanup response may proceed only after the independent absence proof.

These are mandatory, cumulative gates for any later real-target proposal:

- a cryptographically verified repository-owner signature over the complete
  exact authorization receipt;
- binding of that signed source identity to the reviewed checkout and
  executable;
- trusted node clocks plus a monotonic deadline and fail-closed expiry during
  the transaction;
- atomic, replay-proof consumption of one authorization digest and nonce by
  both physical nodes before either production rank is stopped;
- an owner-approved independent two-node watchdog, armed before mutation,
  which survives controller or control-PC loss and restores worker then
  coordinator under bounded deadlines;
- fresh live closed-world HMM/KFD/render, systemd, PID, cgroup, listener,
  kernel/OOM, and capacity admission; and
- full validation and immutable custody of the complete PR-#51 adapter
  evidence tree; and
- atomic two-node terminal receipt reconciliation including cleanup, fresh
  identities, exact owner census, real minimal inference, and kernel deltas.

No field, comment, local validation result, fake receipt, or health endpoint
may substitute for those gates. Promotion requires a separate reviewed PR and
project-owner acceptance while issue #41 is resolved. The controller has no
single-node target fallback: ambiguity or loss of either rank refuses.

## Qualification and consequences

Deterministic fake-runner tests exercise the happy path and adversarial
ordering, ownership, identity, adapter, cleanup, recovery, actuator-lost-
response and postcondition reconciliation, active/absent terminal state,
mandatory custody, closed-tree commit verification and publication cuts,
probe, kernel, replay, input, example-pair, and hard-off cases. They do not
contact either target, execute a real unit, use SSH, prove owner authorization,
prove watchdog behavior, or measure model performance.

This decision makes the future safety contract reviewable without weakening
the current stop gate. It also exposes the controller-loss recovery problem
instead of hiding it behind best-effort cleanup.

## Relationship and rollback

PR #44 remains incident evidence authority, issue #41 remains the P0 stop gate,
and PR #51 remains the blocked adapter authority. This ADR does not supersede
any of them.

Rollback removes the offline controller, exact example pair, fake tests,
documentation, CI entry, and this ADR. No target state, stored model format,
cache format, or production service is changed.
