# L13R executable retry prerequisites independent review

Date: 2026-07-21

Reviewed commit: `0fd867f118776a7313bc8119ecfb9bb32c781b20`

Verdict: **ACCEPT PREREQUISITE A AND PREREQUISITE B — NO P1/P2 REMAINS**

## Prerequisite A

The first evidence return was rejected because the retained output did not
prove `count > n_batch`. The corrected exact-commit rerun retains the invocation
and runtime measurement: 1,129 tokens, boundary 1,128, `n_batch=512`, three
chunks, maximum chunk 512. Capture and cold both execute three chunks. Restore
uses the immutable worker-local object after a worker PID change. Capture,
restore, and cold token and decoded hashes match exactly. The object SHA matches
the capture identifier, and the bounded capture/store and ready/apply windows
contain zero GET/SET state payload operations.

Prerequisite A is accepted for the disposable canary boundary.

## Prerequisite B

The first source return was rejected for permissive inactive-state and command
matching, unvalidated recovery snapshots, and overwriteable preflight evidence.
The accepted controller now requires exact inactive/dead/MainPID-zero and closed
8081 before worker stop, exact ExecStart and complete normalized argv, fixed
host/unit/model/listener bindings, schema- and identity-validated recovery
snapshots, and create-once evidence.

All 15 focused tests pass, including deactivating coordinator rollback without
worker stop, command and ExecStart drift, tampered snapshot refusal before
mutation, evidence overwrite refusal, and abnormal maintenance-command
worker-first rollback. A fresh independent read-only dry-run passed against the
live production identities and made no mutation.

Prerequisite B is accepted. Any primary retry remains limited to the reviewed
controller and the existing Project Lead authorization.
