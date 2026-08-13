# ADR-0065: offline independent two-node recovery-watchdog model

Status: accepted for offline state-machine qualification only. Installation,
arming, service control, target access, issue-#41 closure, and real watchdog
qualification are not accepted.

Date: 2026-08-13

## Context

ADR-0057 models an issue-#41 maintenance transaction but explicitly lacks an
out-of-band actor that survives loss of the controller, control PC, or control
network. It makes a real independent two-node watchdog a cumulative promotion
gate. The worker must be recovered and ready before the coordinator, and
uncertain protected HMM/KFD/render ownership cannot be treated as safe.

A watchdog design also has failure modes which an ordinary controller happy
path does not expose: the controller may disappear before or after its first
stop; the peer receipt may be unreachable; a node may reboot and invalidate
its monotonic epoch; cleanup or start may take effect while its response is
lost; an active unit may not be application-ready; an old protected identity
may reappear; and one terminal file may be partial, corrupt, or inconsistent
with its peer.

## Decision

Add a separate deterministic Python state-machine model over two built-in
in-memory fake nodes. Keep `TARGET_EXECUTION_ENABLED = False` and
`OFFLINE_FAKE_ONLY = True`. The module has no injectable Runner, subprocess,
socket, SSH, systemd, target command, or target-execution subcommand. Its only
CLI operation verifies an offline evidence pair against an externally expected
authority digest.

The model accepts one closed `PreverifiedAuthority`. It includes a transaction
ID, nonce digest, source commit, executable digest, external verification-
receipt digest, maintenance and recovery durations, exact protected before
identities, and a canonical exact disposable allowlist. A SHA-256 digest binds
that complete payload. The model treats the external signature verification as
an opaque precondition; it does not implement or claim authentication.

Both node-local watchdogs must retain their own arm and the digest of the
peer's arm before any fake controller stop. The controller may then disappear
at any of four closed points: before the first stop, after the coordinator
stop, after both stops, or while the disposable experiment is present. A
deadline trigger and a controller-loss trigger both enter the same
recovery-only path. No trigger can continue or restart the experiment.

Every local recovery phase checks a strictly advancing synthetic monotonic
clock against its own recovery deadline. A boot-ID change invalidates the
armed monotonic epoch. The worker runs the complete local recovery sequence
first: boot reconciliation, complete disposable scan, cleanup limited to the
exact local allowlist, independent absence observation, closed-world HMM/KFD/
render reconciliation, exact service preservation or recovery, listener and
application readiness, final exact owner census, and kernel counter
reconciliation. Only then may it retain a worker-ready receipt.

The coordinator runs the same local gates, but it must consume and hash-bind
the exact canonical worker-ready bytes before its service-recovery action.
Missing, corrupt, stale, or unreachable worker readiness refuses coordinator
recovery. There is no single-node maintenance fallback.

An identity which was never observed absent must remain byte-for-byte equal to
the armed protected identity. After observed absence, the final identity must
have a different PID and invocation ID, greater process-start and active-enter
monotonic identities, the same boot ID, and the exact armed restart count.
Boot drift, restart-count drift, stale identity reappearance, foreign or
incomplete HMM ownership, and global-OOM/OOM-kill/AMDGPU/reset/KFD counter
changes refuse readiness.

An actuator response is not a postcondition. A lost cleanup response remains
a terminal error, but an independent complete absence observation may permit
production recovery. A lost production-start response also remains an error,
but an independent exact fresh identity plus readiness, HMM, and kernel proof
may establish recovery. Consequently `recovery_complete` and terminal
`status` are separate: production can be recovered while the transaction is
still a retained failure.

Each fake node publishes an append-only local event sequence, local terminal,
closed file manifest, and finalization marker. A paired acknowledgement binds
both terminal digests and both local-marker digests, is written identically to
both node directories, and is followed by one node-local pair-finalization
marker. Verification requires the exact two-directory and exact file
inventory, canonical JSON with duplicate-key rejection, every digest, arm
cross-binding, monotonic ordering and deadlines, failure/skipped ordering,
worker-before-coordinator receipt ordering, recovery semantics, and an
externally supplied expected authority digest.

## Safety invariants

- Controller or controller-network loss always selects production recovery;
  `experiment_continuation_allowed` is literal false in every terminal and
  paired result.
- No fake mutation is admitted until both node arms cross-bind the same
  authority.
- Cleanup may remove only exact allowlisted unit, port, and path identities;
  wildcard, protected, non-canonical, cross-host, unknown, or incompletely
  scanned identities refuse.
- Worker readiness bytes gate every coordinator recovery action.
- A failed phase causes every later recovery phase on that node to be retained
  as skipped; no action is accepted at or after the recovery deadline.
- Recovery readiness requires exact service, readiness, HMM owner, boot, and
  kernel reconciliation. A health route or active-unit flag alone is not
  sufficient.
- Existing evidence roots are never overwritten. Local and paired terminal
  records are separately hash-bound, and partial, corrupt, divergent, or
  semantically impossible records refuse verification.

## Explicit non-authority and remaining promotion gates

The authority hash is integrity binding, not a signature. The external
verification-receipt digest does not prove that verification happened. The
fake source and executable digests are supplied fields, not measurements of
the current checkout. Fake monotonic clocks are deterministic inputs, not
trusted node clocks. In-memory nodes do not prove process, kernel, network,
service-manager, reboot, or hardware behavior. Node terminal files are not
cryptographically authenticated. On Windows, only file sync is available;
the model does not claim directory or power-loss durability. On POSIX, file
and parent-directory sync still does not prove atomic publication across two
physical nodes.

Therefore ADR-0057's cumulative real-target gates remain open, including:

- owner-signature verification over the complete authorization;
- binding the authorized source and executable to reviewed measured bytes;
- trusted per-node time and replay-proof atomic two-node nonce consumption;
- an owner-approved watchdog implementation installed and boot-tested on both
  physical nodes, independent of controller and ordinary control networking;
- exact real systemd/PID/cgroup/listener and elevated HMM/KFD/render adapters;
- complete immutable adapter evidence plus a real minimal two-rank recovery
  probe; and
- authenticated, power-loss-safe, atomically reconciled two-node terminal
  custody.

Issue #41 remains the hard target stop gate. This model is not a Runner, an
installation artifact, a maintenance authorization, a target test, or a
performance result.

## Qualification and consequences

The offline suite covers all four controller-loss points, deadline firing at
each point, deadline expiry at every worker and coordinator phase, peer loss,
boot drift, cleanup and start lost responses, cleanup residue, active-but-not-
ready worker state, stale identities, restart drift for preserved and fresh
services, incomplete and foreign HMM ownership, kernel deltas, monotonic
regression, strict allowlists, authority mismatch, worker-receipt tampering,
and partial, corrupt, divergent, extra, and semantically rewritten custody.
Semantic tamper cases rebuild all unsigned hashes before verification and
still refuse.

This closes a reviewable offline design gap without weakening the target stop
gate. It also makes recovery-complete-but-transaction-failed outcomes explicit
instead of collapsing ambiguous actuator replies into success.

## Relationship and rollback

ADR-0057 remains the maintenance-domain authority and issue #41 remains the P0
target stop. ADR-0065 qualifies only the independent recovery state machine
that ADR-0057 named as a future gate; it does not satisfy the real gate.

Rollback removes the offline watchdog module, tests, guide, CI entry, routing
links, and this ADR. No target state, service, model format, cache format, or
benchmark evidence is changed.
