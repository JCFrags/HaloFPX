# ADR-0057 offline Strix maintenance controller — independent review v02

Review date: 2026-08-13

Scope: exact repaired implementation commit
`702523c7809d0ee6a2ef4a943b308b4bf98a2ae8`, whose branch is rebased over
canonical main commit `3d9a0c3cc52168f696d600099742c7caf964161f`.
The review covered the controller, deterministic fake tests, ADR-0057, and
the operations document. The earlier v01 review is preserved verbatim as the
historical review of its own exact source and test hashes.

Boundary: read-only source review and offline fake execution. Neither Strix
Halo target was contacted. No target service, model, build, inference, or
benchmark was run. The controller still defines no real Runner or target
transport, and `TARGET_EXECUTION_ENABLED` remains literal false.

## Findings resolved after v01

1. A role proven stopped can no longer reappear under its pre-stop identity
   and be accepted as preserved. Absence remains authoritative even when later
   custody or cross-role validation fails.
2. Every stop, start, and adapter-cleanup response is reconciled through an
   independent postcondition. Lost responses remain terminal errors, while
   observed effects still drive worker-first recovery.
3. Production snapshot v3 closes host, unit ActiveState/SubState, main PID,
   listener PIDs, cgroup path/existence/membership, and identity. Residual
   cgroup state is not accepted as absence.
4. Fresh recovery requires greater process-start and systemd monotonic
   identities and freezes executable, arguments, restart count, cgroup, and
   coordinator health authority.
5. Boolean values are rejected wherever exact integers are required, and the
   non-performance recovery probe is exactly five prompt tokens, one generated
   token, and world size two.
6. Mandatory forward evidence loss aborts before the next mutation. Recovery
   evidence loss remains nonblocking so restoration can continue.
7. `FAILED.json` and `COMMITTED.json` have distinct meanings. Rename and sync
   lost-response cuts withdraw an ambiguous success marker when possible and
   never rewrite bytes that a surviving marker may bind.
8. Success is accepted only by the closed-tree bundle verifier. It checks the
   exact inventory, canonical manifest and marker, all hashes, terminal gates,
   event semantics, final state, and retained authorization/policy inputs.
9. The bundle retains the exact adapter plan, adapter policy, and sparse
   receipt. The verifier reconstructs their semantic relationship and rejects
   a rehashed receipt that claims performance.
10. Retained policy paths must be canonical repository-relative paths, and the
    incident path must be the exact issue-#41 manifest path. A fully rehashed
    traversal rewrite is rejected.

## Qualification observed

- focused maintenance-controller suite: 57/57 pass;
- combined Strix core, CachyOS adapter, and maintenance suite: 109/109 pass;
- Python compilation and `git diff --check`: pass;
- canonical Wiki manifest and structure: 86/86 complete, with 5/5 validator
  tests passing;
- documentation validator: pass for 556 Markdown files, with zero broken
  internal links and zero authoritative orphans;
- PR-#44 incident bundle validator: pass, plus 19/19 negative contracts;
- portable ROCmFPX fixture suite: 12/12 pass; and
- fresh-PC recovery suite: 42/42 pass.

The final independent exact-scope review found no reachable offline P0, P1,
or P2 issue. It reported the worktree clean and bound these files:

- controller SHA-256:
  `289f566a2570e2dfe3c59cce044dc5de4733a1e482b752cb9596a82a3f93367e`;
- tests SHA-256:
  `f8d109afefb31332b6968bcd06f0871afb9440e5fe64748bf7b2cff5dd569fdb`;
- operations document SHA-256:
  `81d771ea275befee3383c48120444bd4e6821ff893729ee0d43eea7988783c6d`;
- ADR-0057 SHA-256:
  `d3ad6fb2a24d977975ab701856f24855b9154c375fba48cb66694add872381ad`;
- preserved v01 review SHA-256:
  `9fdbd9079819c82e13e723ac11f6724f58aac73643a7acb9f0741bc39f21e219`.

## Retained real-target blockers

Offline qualification does not promote the controller. A separate reviewed
real-target change still requires every cumulative gate in ADR-0057:

- a cryptographic repository-owner signature over the complete exact receipt;
- exact checkout and executable binding;
- trusted node clocks and an enforced monotonic transaction deadline;
- atomic replay-proof authorization and nonce consumption by both nodes;
- an independent node-local watchdog, armed before mutation, that restores the
  worker before the coordinator after controller, control-PC, or network loss;
- fresh live closed-world HMM/KFD/render, systemd, PID, cgroup, listener,
  kernel/OOM, and capacity admission;
- complete immutable validation of the PR-#51 evidence tree; and
- atomic reconciliation of both node receipts and the real recovery probe.

GitHub issue #41 remains open and P0-blocking. PR #44 and PR #51 remain the
incident and adapter authorities respectively; neither authorizes a real
maintenance window. A local `COMMITTED.json`, fake receipt, health response,
or this review cannot substitute for any retained gate.

## Conclusion

Accept the repaired commit for offline domain qualification and continuation
documentation only. Reject any interpretation as owner authorization, target
execution qualification, issue-#41 closure, production recovery evidence, or
a performance result.
