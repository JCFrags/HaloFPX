# Offline Strix maintenance admission controller

Status: **offline domain qualification only; target execution is hard-disabled**

This slice models the issue-#41 maintenance transaction needed before the
blocked CachyOS A/B adapter can ever touch the two production Strix Halo
machines. It is a safety and continuity artifact, not an operations tool and
not a performance result.

[`scripts/halofpx_strix_maintenance.py`](../../scripts/halofpx_strix_maintenance.py)
contains a literal `TARGET_EXECUTION_ENABLED = False`, defines and constructs
no SSH or target Runner, and makes the public `execute` command refuse before
loading a policy. It imports the already-merged PR-#51 module only for frozen
policy/receipt definitions; that adapter's separate target gate also remains
false.
The only executable transaction seam is `execute_offline_domain`, which admits
an object whose `offline_fake` attribute is literally `True`. The repository's
deterministic fake tests use that seam; no target machine was contacted while
implementing or qualifying it.

## Authority and exact example pair

The controller binds closed JSON inputs to:

- issue #41 and one exact repository-owner GitHub comment reference;
- an at-most-eight-hour UTC window, nonce, canonical repository URL, and
  40-hex repository identity;
- the immutable PR-#44 incident-manifest bytes;
- the exact PR-#51 example plan and CachyOS policy bytes plus one schedule row;
- a resolved write-once evidence root and non-production unit/port namespace;
- exact before-state identities for the protected coordinator and worker; and
- a five-prompt-token, one-generated-token, world-size-2 recovery probe that
  cannot be classified as a performance result.

The tracked pair is:

- [`halofpx-strix-maintenance-authorization.example.json`](../../scripts/halofpx-strix-maintenance-authorization.example.json)
- [`halofpx-strix-maintenance-policy.example.json`](../../scripts/halofpx-strix-maintenance-policy.example.json)

It is deliberately historical, synthetic, Windows-control-host data with
`execution_scope=offline-domain-simulation`. Its exact approval statement says
that it authorizes no access to or mutation of `nimo-1` or `nimo-2`. The policy
hashes the authorization bytes, both files repeat the same authority and
repository identity, and tests keep the pair digest-bound. The referenced
issue comment is recovery provenance; it is **not** owner authorization for a
maintenance window. The synthetic PIDs, hashes, and paths are not target
observations and must never be promoted as such.

On a Windows clone, validate the exact tracked bytes at their historical time:

```powershell
python -B scripts/halofpx_strix_maintenance.py `
  --repository-root . `
  --policy scripts/halofpx-strix-maintenance-policy.example.json `
  --authorization scripts/halofpx-strix-maintenance-authorization.example.json `
  --now-utc 2026-08-13T07:00:00Z `
  validate
```

`validate` checks closed schemas and digest relationships. It does not
authenticate the GitHub comment body, verify an owner signature, consume a
nonce on either target, or bind the declared commit to the currently checked
out source tree. The supplied `--now-utc` is fixture time, not a trusted clock.
Those omissions are intentional promotion blockers, not features to work
around. Likewise, `offline_fake=True` is a deterministic testing convention,
not a security boundary into which a side-effectful object may be injected.

## Offline transaction model

The fake-only transaction is ordered as follows:

1. Create a write-once intent receipt, then snapshot exact production
   identities, both kernel/OOM baselines, and a complete elevated KFD/render
   owner census.
2. Stop the coordinator, then independently snapshot active/absent unit, PID,
   cgroup, listener, and identity state. Exact host/unit ActiveState/SubState
   and cgroup existence/membership are required before repeating the exact
   census while the worker remains.
3. Stop the worker and take the same independent postcondition snapshot, then
   require a complete empty census on both nodes.
4. Hand the exact frozen schedule entry to the PR-#51 adapter fake, reject any
   receipt that claims execution qualification or performance, then clean the
   disposable domain. Cleanup command success is insufficient; an independent
   unit/port/path absence proof gates recovery.
5. Reconcile GPU ownership before recovery. Recover the worker first and prove
   its fresh identity and RPC readiness; only then recover the coordinator and
   prove its fresh identity and health.
6. Require the recovered exact-owner census before invoking the modeled
   two-rank inference contract. A failed census cannot reach that fake seam.
7. Compare post-transaction OOM, fault, and reset counters with the baseline,
   take one final active/absent service snapshot, retain every event plus the
   exact adapter plan/policy/receipt bytes, and write a terminal receipt plus
   `SHA256SUMS`. Successful and failed finalization use distinct
   `COMMITTED.json` and `FAILED.json` markers.

A mandatory forward event-write or sync failure aborts the maintenance body
before its next mutation and enters cleanup/recovery. Evidence failure during
cleanup or recovery does not undo an already validated safety action or stop
worker-first recovery. A provisional terminal or marker existence is not
accepted as success. The executable `verify-bundle` path requires a canonical
`COMMITTED.json` binding the terminal and `SHA256SUMS`, exact regular-file
inventory with no staging or extra paths, every manifest digest, empty terminal
errors, every recovery gate, the exact event sequence, and the sparse adapter
receipt semantics recomputed from retained frozen inputs. Pre-publication
failures make only a best-effort terminal-failure rewrite. A rename exception
is treated as a possible lost response. If rename may have occurred, or a
post-rename directory sync fails, the controller first attempts marker
withdrawal and never mutates bytes an indeterminate surviving marker may bind.
Windows supplies no directory-fsync primitive here, so this offline fake does
not claim crash durability. Atomic two-node terminal reconciliation remains a
mandatory future real-controller gate.

Rank ownership is explicit: `nimo-2` owns the RPC worker and must be ready
first; `nimo-1` owns the coordinator and may start only after worker readiness.
There is no single-node maintenance fallback in this controller. A partial or
ambiguous dual-node state fails the transaction.

The terminal differentiates `services_ready` from
`recovery_census_complete`, `recovery_probe_complete`,
`final_observation_matches_recovery`, and
`recovery_complete`. Merely starting both services is not issue-#41 recovery;
`services_ready` means the relevant preserved or restarted identities passed
their modeled readiness contracts. `recovery_complete` additionally requires
the exact-owner census, two-rank inference-contract proof, and final
authoritative active/absent snapshot equality. Every proven-absent role must
cross an explicit fresh start; a reappearing old PID or backward
process/systemd monotonic identity refuses. A stop/start response error does
not prove no effect: the
independent postcondition decides whether a role is preserved, absent, or
freshly active, while the error still makes the overall transaction fail.

Verify a successful fake bundle through the sole local acceptance seam:

```powershell
python -B scripts/halofpx_strix_maintenance.py `
  --evidence-root C:\absolute\path\to\offline-evidence `
  verify-bundle
```

This verifies only the closed offline fake artifact. It is not a signature,
two-node receipt, performance result, or target-execution qualification.

## Offline independent-watchdog qualification

[ADR-0065](decisions/0065-offline-independent-two-node-recovery-watchdog.md)
and the [recovery-watchdog guide](strix-recovery-watchdog.md) add a separate
offline-only state-machine model for the independent recovery actor named by
gate 5 below. It arms and cross-binds two built-in fake nodes before modeled
mutation, chooses recovery after controller loss at every closed mutation cut,
requires worker readiness before coordinator recovery, and retains paired
terminal custody. Its adversarial model includes peer loss, reboot, deadline
expiry at every phase, cleanup and start response ambiguity, stale identity,
HMM/restart/kernel drift, and corrupt terminal records.

This is design qualification only. It has no Runner, target transport,
service-manager adapter, installer, or target-execution command, and its target
gate is literal false. It does not satisfy gate 5 on either physical machine
or weaken any other cumulative gate.

## Mandatory gates before any real-target promotion

A future real controller must be a separate reviewed change. It must keep all
current fail-closed checks and add every gate below before the first target
mutation:

1. **Owner signature:** cryptographically verify the repository owner's
   signature over the exact authorization bytes, including source commit,
   before identities, window, nonce, policy/plan/incident digests, disposable
   namespace, and recovery request. Self-asserted GitHub login, numeric ID,
   URL, or node ID fields are not authentication.
2. **Source identity:** bind the signed commit to the exact reviewed checkout
   and executable source, not merely equality between two JSON files.
3. **Trusted time:** admit against trusted node clocks, retain a monotonic
   transaction deadline, and fail closed if the signed window expires before
   any remaining mutation.
4. **Atomic two-node consumption:** atomically consume the same signed
   authorization digest and nonce on both `nimo-1` and `nimo-2`, prove neither
   was consumed before, and commit the paired receipt before stopping either
   production rank. A one-node or replayed receipt must refuse. The
   [ADR-0066 offline model](strix-maintenance-two-node-nonce-protocol.md)
   distinguishes definite refusal from lost-response/partial-commit
   uncertainty, but supplies no real node agent, durability, consensus, or
   authorization and cannot satisfy this gate.
5. **Independent recovery watchdog:** install and arm an owner-approved,
   out-of-band watchdog on both nodes before the controller may stop
   production. It must survive controller/control-PC/network loss, enforce
   bounded deadlines, remove only the exact disposable domain, and restore and
   prove production worker-first then coordinator. Controller cleanup failure
   cannot be the only recovery path.
6. **Live closed-world admission:** obtain a fresh complete elevated owner
   census, protected systemd/PID/cgroup/listener identities, kernel counters,
   and adequate HMM headroom under the signed window. Any foreign or uncertain
   owner refuses.
7. **Complete adapter evidence:** validate the complete immutable PR-#51
   evidence tree, not the deliberately sparse offline handoff receipt.
8. **Atomic terminalization:** reconcile controller and watchdog receipts from
   both nodes, including cleanup, fresh identities, exact recovered ownership,
   the mandatory real two-rank probe, and kernel deltas. A health route alone
   is never recovery proof.

Issue #41 remains open until the project owner accepts evidence from a real,
isolated maintenance window. This offline slice does not close it and does not
authorize the PR-#51 target adapter.

## Hosted qualification

Run the proportional suite with:

```powershell
python -B -m unittest tests.test_halofpx_strix_maintenance -v
```

The fake-runner cases cover ordering, foreign/incomplete ownership, identity
drift, active/absent postconditions, adapter and cleanup failures, worker-first
recovery, stale/reappearing/backward identities, actuator errors after side
effects, final authoritative observation, recovered-census gating, exact
bounded probe semantics, kernel-counter deltas, mandatory forward and best-
effort recovery custody failures, marker publication cuts, terminal rewrite
failure, closed-tree verification, replayed evidence roots, closed input
schemas and scalar types, changed PR-#51 bytes, the exact tracked example pair,
the non-fake seam, and the public CLI's hard-off gate. They prove only the
domain contract.
