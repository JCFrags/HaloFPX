# L90 controller prerequisite gates

The L90 prerequisite is a closed disposable-only reconciliation executed
before evidence-directory staging, channel-key preparation, or production
shutdown. It accepts exact absence or an exact loaded transient identity with
MainPID 0, no cgroup, process reference, or relevant listener. Only that
ownerless stale identity may be stopped and unloaded. Active ownership,
unknown or near identity, query ambiguity, listener/process references, and
failed absence postconditions refuse.

The exact path set is separately constrained by literal path, resolved
identity, type, owner, mode, non-mount status, no process reference,
no-symlink absence proof, no-glob removal, and an exact absence postcondition.

Focused no-host qualification: 62 passed with 11 subtests. Coverage includes
stale active/exited MainPID 0, genuine active ownership, process and listener
ownership, unknown identity, failed post-stop query, idempotent absence,
ambiguous stat failure, wrong type/owner/mode, symlink, mount, referenced
path, removal failure, and source ordering before production shutdown.

Independent review verdict: **PASS**. P1 none; P2 none. The reviewer confirmed
the prior ambiguity and postcondition findings are closed, response semantics
are unchanged, and one primary transition is safe under the existing
no-retry/recovery bounds.

Controller SHA256:
`89254efe4872a426f7b6193b1bd453f7b2b442446bd886d9a559beb9336ef742`.
