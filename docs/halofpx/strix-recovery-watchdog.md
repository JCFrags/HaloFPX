# Offline independent two-node recovery-watchdog model

Status: **offline fake qualification only; target execution is literal false**

This artifact models the recovery actor required by ADR-0057 without adding an
operations path. [`halofpx_strix_recovery_watchdog.py`](../../scripts/halofpx_strix_recovery_watchdog.py)
contains only two in-memory fake nodes. It cannot execute a command, open a
target connection, manage a service, install a watchdog, or reach `nimo-1` or
`nimo-2`. Its public CLI only verifies an already-created offline evidence
pair.

## Fixed topology and authority boundary

The topology is closed:

- `nimo-2` is the RPC worker and owns
  `minimax-m27-rpc-worker.service`;
- `nimo-1` is the coordinator and owns
  `minimax-m27-q6-server.service`; and
- worker recovery and application readiness must be durably proven before any
  coordinator recovery action.

The Python API accepts a `PreverifiedAuthority` object with exact before
identities, a canonical disposable allowlist, monotonic lease durations,
source and executable digests, a nonce digest, and an opaque external
verification-receipt digest. `authority_sha256` binds all those fields.

“Preverified” is an explicit assumption, not authentication. The module does
not verify a signature or measure the checkout/executable. A future physical
implementation must receive the object only after the independent owner-
signature, source binding, trusted-time, and atomic two-node nonce gates in
ADR-0057. The verifier also requires the expected authority digest from an
external caller so a self-consistent replacement authority is not silently
accepted.

## Recovery-only state machine

Both local watchdogs retain and cross-bind their arms before the fake
controller can change service state. Controller loss is injected before the
first stop, after the coordinator stop, after both stops, or during the
disposable experiment. Every trigger sets experiment continuation to false.

Each node then evaluates these phases against a strictly advancing local
monotonic clock and a bounded recovery deadline:

1. reconcile the armed `boot_id` and monotonic epoch;
2. require a complete scan of disposable identities and reject anything
   outside the exact local allowlist;
3. attempt exact cleanup, then require an independent complete absence scan;
4. reconcile a complete elevated HMM/KFD/render owner census;
5. on the coordinator, validate and hash the exact worker-ready receipt;
6. preserve only the exact armed service identity, or start a missing service;
7. independently validate the service postcondition and reject stale or
   restart-drifted identity;
8. require listener and application readiness;
9. require the exact final protected HMM owner; and
10. reconcile boot identity plus global-OOM, OOM-kill, AMDGPU-fault, GPU-reset,
    and KFD-fault counters.

The worker completes the entire sequence first. If its receipt is unavailable
or invalid, the coordinator stops at its peer-receipt phase and all later
coordinator phases are retained as skipped. There is no single-node fallback.

A cleanup or start call can take effect even when its response is lost. The
model records that ambiguity as a transaction error and proceeds only through
an independent observed postcondition. This can produce
`recovery_complete=true` with `status=failure`: production is restored, but
the transaction is not promoted to success. Start-response loss is injected
independently on either node in both closed outcomes: after-effect evidence can
recover through a fresh-identity postcondition, while no-effect evidence finds
the service still absent and refuses. If the worker start had no effect, no
worker-ready receipt exists and the coordinator start remains skipped.

## Identity, ownership, and deadline rules

If a protected service was never observed absent, its final identity must be
byte-for-byte equal to the arm identity. After observed absence, PID and
invocation ID must differ, process-start and active-enter monotonic values must
increase, and boot ID plus restart count must remain exact. A reappearing old
identity refuses.

A boot-ID change invalidates the local monotonic epoch. An accepted,
preserved, or lost-response action may not be recorded at or after the
recovery deadline. Once any phase fails, every later recovery phase on that
node is evidence-only and must be `skipped`.

HMM ownership is closed-world: the census must be complete, elevated, error-
free, and contain exactly the current protected service owner at final
readiness. An active unit without listener and application readiness is not
ready. Any global-OOM, OOM-kill, AMDGPU-fault, GPU-reset, or KFD-fault counter
delta refuses recovery.

## Exact cleanup boundary

Disposable identities are literal `(host, kind, value)` tuples. Kinds are
limited to:

- units under the `halofpx-watchdog-*.service` namespace, excluding protected
  production units;
- canonical non-protected ports from 1024 through 65535; and
- canonical absolute paths under `/var/tmp/halofpx-watchdog-*`.

Wildcards, traversal, non-canonical paths or ports, protected ports, duplicates,
unknown identities, cross-host identities, and incomplete scans refuse. The
post-cleanup scan must prove that the local disposable set is empty.

## Paired terminal custody

Each fake node directory retains:

- identical `authority.json` bytes;
- its `arm.json` and `peer-arm.json`;
- a canonical ordered `event-*.json` state-machine trace;
- an optional `ready-receipt.json`;
- `local-terminal.json`, `LOCAL-SHA256SUMS.json`, and
  `LOCAL-FINALIZED.json`; and
- an identical `pair-ack.json` plus node-local `PAIR-FINALIZED.json`.

The pair acknowledgement binds both terminal hashes and both local-marker
hashes. Verification requires the exact two-node tree, exact file inventory,
canonical duplicate-free JSON, every content digest, both arm cross-bindings,
strict event order, monotonic/deadline semantics, worker receipt ordering, and
an exact semantic replay which cross-binds service observation, preservation or
start actuation, independent postcondition, final identity, and readiness.
Partial, corrupt, divergent, unlisted, or semantically impossible records
refuse. Tests also rewrite semantic fields and rebuild every unsigned hash to
prove that validation is not merely checksum comparison.

Verify an offline pair only when the expected authority digest is held outside
the bundle:

```powershell
python -B scripts/halofpx_strix_recovery_watchdog.py verify-pair `
  C:\absolute\path\to\offline-watchdog-evidence `
  --expected-authority-sha256 <64-lowercase-hex-digest>
```

The generator is intentionally a Python-only fake test seam rather than a
public CLI. The CLI cannot create evidence or execute recovery.

On Windows, the receipt records `file-sync-only` because Python exposes no
portable directory-fsync primitive. On POSIX, the model syncs each file and
its parent directory. Neither mode proves power-loss-safe atomic publication
across physical nodes, and node outputs are not authenticated.

## Qualification

Run the offline adversarial suite with:

```powershell
python -B -m unittest tests.test_halofpx_strix_recovery_watchdog -v
```

The suite is deterministic and contacts no target. It covers controller loss
and controller deadline at all four mutation cuts, recovery deadline at every
phase on both nodes, peer loss, reboot, cleanup and start response ambiguity,
both-role start ambiguity with after-effect and no-effect outcomes, readiness,
stale and restart-drifted identities, HMM and kernel reconciliation, authority
and allowlist rejection, and terminal custody corruption.

## What remains open

This model does not satisfy the real independent-watchdog gate. The project
still requires owner-approved signed authority, measured source/executable
binding, trusted node clocks, replay-proof atomic two-node consumption, real
systemd and HMM/KFD/render adapters, installed boot-persistent node-local
watchdogs tested under controller and network loss, complete adapter evidence,
a real minimal two-rank recovery probe, and authenticated power-loss-safe
paired terminal reconciliation. Issue #41 continues to prohibit target
mutation until those cumulative gates are reviewed and accepted.

See [ADR-0065](decisions/0065-offline-independent-two-node-recovery-watchdog.md)
for the decision and non-authority boundary.
