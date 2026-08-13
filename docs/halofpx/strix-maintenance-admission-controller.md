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
2. Stop the coordinator, prove its unit, PID, cgroup, listener, and GPU
   ownership absent, then repeat the exact census while the worker remains.
3. Stop and prove the worker absent, then require a complete empty census on
   both nodes.
4. Hand the exact frozen schedule entry to the PR-#51 adapter fake, reject any
   receipt that claims execution qualification or performance, then clean the
   disposable domain.
5. Reconcile GPU ownership before recovery. Recover the worker first and prove
   its fresh identity and RPC readiness; only then recover the coordinator and
   prove its fresh identity and health.
6. Require the recovered exact-owner census before invoking the modeled
   two-rank inference contract. A failed census cannot reach that fake seam.
7. Compare post-transaction OOM, fault, and reset counters with the baseline,
   retain every event, and write a terminal receipt plus `SHA256SUMS`.

An event-write or sync failure is retained as a custody error when the
remaining evidence root is writable, but it does not undo a successfully
validated cleanup/start/readiness action or stop worker-first recovery. Failure
to write the terminal receipt itself occurs only after recovery attempts and
still remains a mandatory atomic-terminalization problem for a future real
controller.

Rank ownership is explicit: `nimo-2` owns the RPC worker and must be ready
first; `nimo-1` owns the coordinator and may start only after worker readiness.
There is no single-node maintenance fallback in this controller. A partial or
ambiguous dual-node state fails the transaction.

The terminal differentiates `services_ready` from
`recovery_census_complete`, `recovery_probe_complete`, and
`recovery_complete`. Merely starting both services is not issue-#41 recovery;
`services_ready` means the relevant preserved or restarted identities passed
their modeled readiness contracts. `recovery_complete` additionally requires
the exact-owner census and two-rank inference-contract proof.

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
   production rank. A one-node or replayed receipt must refuse.
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
drift, absence proof, adapter and cleanup failures, worker-first recovery,
stale identities, recovered-census gating, mandatory probe semantics,
kernel-counter deltas, injected custody-write failure, replayed evidence roots,
closed input schemas, changed PR-#51 bytes, the exact tracked example pair, the
non-fake seam, and the public CLI's hard-off gate. They prove only the domain
contract.
