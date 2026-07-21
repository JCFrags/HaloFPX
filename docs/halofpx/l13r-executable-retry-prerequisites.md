# L13R executable retry prerequisites

Date: 2026-07-21

Implementation commit: `0fd867f118776a7313bc8119ecfb9bb32c781b20`

Outcome: **PASS — BOTH PREREQUISITES ACCEPTED**

This milestone records the two mandatory gates from the Project Lead decision
"accept L13 safety stop and require executable retry guards." Production was
not mutated while these gates were implemented or qualified.

## Prerequisite A — bounded long-prefix decode

[MEASURED] The repository-tracked disposable invocation used the retained 15M
Q4_0 fixture, a 1,129-token prompt, saved boundary 1,128, `n_batch=512`, and
three prompt-decode chunks with a maximum chunk of 512. It exercised the actual
distributed state canary capture path, restarted the worker from PID 2069893
to PID 2069926, restored from the worker-local object, and ran a clean cold
recomputation.

| Evidence | Result |
|---|---|
| capture/cold prompt chunks | 3 / 3 |
| maximum prompt chunk | 512 |
| capture/restore/cold token suffix SHA-256 | `8833fa5b4ce8735bd0520e7eb2e969b26ae302b688c5f4f80c2fb58858af703f` |
| capture/restore/cold decoded suffix SHA-256 | `a46e483407ed242d4e4301ac39247072f283cf76abb2112a31ffa4a33f3a78c8` |
| worker state bytes/components | 2,598,912 / 4 |
| immutable worker object bytes | 2,599,880 |
| worker object SHA-256 / capture object ID | `fffeea6de555b6777d6594ea752474c9536e99308e4ceb0ab48afa4622dc5576` |
| capture/restore state-window GET/SET | 0 / 0 |

The successful runtime evidence closes the unqualified `bed36b7` long-prompt
chunking correction for this disposable canary boundary.

## Prerequisite B — executable transition authority

[VERIFIED] `scripts/halofpx-production-transition.py` is the sole authorized
production transition mechanism for this retry. It binds the coordinator to
nimo-1 / `minimax-m27-q6-server.service` / port 8081 and the worker to nimo-2 /
`minimax-m27-rpc-worker.service` / port 50052. Before mutation it validates
remote hostname, unit and exact ExecStart, full normalized process argv,
standard UD-Q6 model path, MainPID/listener PID, port, HTTP, start time, and
`NRestarts`, then publishes a create-once preflight snapshot.

Shutdown stops nimo-1 first and requires exact `inactive/dead/MainPID=0` plus a
closed port 8081 before worker stop is authorized. Recovery starts and verifies
nimo-2 and port 50052 first, then starts nimo-1, waits for HTTP 200, and
reconciles exact commands and unchanged `NRestarts`. Exceptions, SIGINT,
SIGTERM, and abnormal maintenance-command exits enter the idempotent worker-
first recovery trap after the first mutation.

[MEASURED] Fifteen focused tests passed. They include swapped hostname/unit,
partial preflight, unexpected listener PID, full command and ExecStart drift,
coordinator still active, deactivating with a closed listener, tampered recovery
snapshot, evidence overwrite, missing preflight, ordered success, and abnormal
child rollback. A fresh live `--dry-run` validated current production without
mutation: nimo-1 PID 2068256 / HTTP 200 and nimo-2 PID 1247685 / port 50052,
both with `NRestarts=0`.

## Review and admission

Independent adversarial re-review accepted A and B at exact commit `0fd867f`
with no remaining P1/P2 finding. This admits only one controller-managed L13
primary-model retry within the already authorized model, request, identity,
fault, recovery, and scope boundary. It does not admit manual service commands,
another retry after failure, production cache enablement, eviction, shared or
prefix reuse, cable faults, broad fault injection, or G9/G10 claims.

The prerequisite evidence bundle is retained on nimo-2 at
`/var/tmp/halofpx-l13-retry-prerequisites-20260721-v1.tar.zst`, SHA-256
`6427ba13860728934e673dc19b32da5a42e42a086482ab8f0cb1ad38dcca075a`.
