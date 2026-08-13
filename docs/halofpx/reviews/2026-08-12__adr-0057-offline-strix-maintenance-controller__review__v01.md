# ADR-0057 offline Strix maintenance controller — independent review v01

Review date: 2026-08-12

Scope: pre-publication branch diff against canonical base
`9bfccf25d43af0c446df591035e9cdac0b74d6c0`, including the controller, fake
tests, exact offline example pair, ADR-0057, routing documentation, and CI.

Boundary: read-only source review and hosted fake execution. Neither physical
target was contacted. No SSH or real process Runner exists in the reviewed
controller.

## Findings resolved in this slice

1. The recovered exact-owner census originally recorded failure but still
   reached the minimal inference probe. The probe is now gated on successful
   recovered-census admission, with an exact negative test.
2. The terminal originally equated both services being started with completed
   issue-#41 recovery. It now exposes `services_ready`,
   `recovery_census_complete`, and `recovery_probe_complete` separately;
   `recovery_complete` requires all mandatory proof gates.
3. Census identity lookup was keyed only by PID, which can collide across two
   hosts. Frozen owner authority is now keyed by `(host, pid)`, with a
   same-numeric-PID cross-host test.
4. The public validator could parse a `dual-strix-maintenance` scope even
   though authentication is not implemented. Public validation now admits
   only `offline-domain-simulation`; target execution remains hard-disabled.
5. Foreign-owner injection is now tested at pre-stop, between-stop, post-stop,
   and recovered-census boundaries.
6. Event persistence originally participated in cleanup/recovery admission.
   Event-write failure is now retained as a custody error without revoking a
   completed safety action or blocking later worker-first recovery.

## Intentionally retained promotion blockers

- GitHub comment/login/account fields are self-asserted metadata, not a
  cryptographically verified owner signature.
- The declared commit is cross-bound between policy and authorization but is
  not bound to the checked-out or executable source.
- `--now-utc` is fixture input, not trusted node time or a monotonic deadline.
- Replay custody is one local write-once evidence root, not an atomic nonce
  consumption receipt from both nodes.
- `offline_fake=True` is a test convention, not a security boundary.
- The sparse adapter receipt proves ordering, not the complete immutable
  PR-#51 evidence tree.
- There is no independent, out-of-band watchdog capable of worker-first
  recovery after controller/control-PC/network loss.

ADR-0057 records every item above as a mandatory future promotion gate. None
may be inferred from a successful local validation or fake run.

## Qualification observed

- `python -B -m unittest tests.test_halofpx_strix_maintenance -v`: 30/30 pass,
  including a construction trap around the merged PR-#51 SSH Runner.
- combined Strix core, CachyOS adapter, and maintenance suite: 71/71 pass.
- tracked offline authorization/policy validation at historical fixture time:
  pass, with `target_execution_enabled=false` and offline scope.
- source inspection: literal target gate false; no SSH, subprocess, socket,
  HTTP client, or real Runner in the controller.
- documentation graph, canonical wiki manifest/structure, and PR-#44 incident
  validator plus its 19 negative contracts: pass.

## Conclusion

Accept for offline domain qualification and documentation of the future
maintenance contract only. Reject any interpretation as owner authorization,
target execution qualification, issue-#41 closure, production recovery
evidence, or a performance result.

The final exact-tree re-review reported no P0, P1, or P2 findings within that
offline-only scope. It bound the reviewed controller source to SHA-256
`816c6a1fa511b3b9be5bbaba7a83783dad17415fd6422e3e1d26fc47c71931ee`
and the test source to SHA-256
`baafc8a4e874666463cde2205b7779e3efa5164d8629a1e995bde8e237085f5e`.
