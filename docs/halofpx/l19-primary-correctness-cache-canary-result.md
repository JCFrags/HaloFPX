# L19 guarded primary correctness/cache canary result

Date: 2026-07-21

Base: `93c61eadd167285be448ef1e99b80f429fa4299a`

Outcome: **NOT PROMOTED — PRE-MUTATION REVIEW BLOCKER**

## Result

[VERIFIED] L19 stopped before the controller's first mutation. The exact pinned
artifact on nimo-2 remained 159,873,097,824 bytes with SHA-256
`96506ada918e60ca9a9cfde8a5437790e4453401a6a3e236e3f55e7bac3aaea6`.
The controller's read-only dry run admitted the exact current production
authority: nimo-1 coordinator PID 2144857 on 8081 serving HTTP 200 and the
standard UD-Q6 model, and nimo-2 worker PID 1305879 on 50052; both had
`NRestarts=0`.

The independent adversarial review found that the inherited runner cannot
satisfy L19's literal single-material-load boundary. Its capture, cold,
restore, missing-object, plan-mismatch, and runtime-off modes each start a new
process, and model initialization occurs before mode dispatch. Executing the
requested sequence would therefore material-load the 159.9 GB artifact six
times. The controller also remains deliberately L16-bound for its protected-key
paths, disposable cleanup allowlist, and maintenance-child identity; changing
only the child to L19 would fail key consumption after shutdown and weaken
rollback cleanup coverage.

Because these were pre-mutation stop gates, no L19 binary was built, no channel
key was provisioned, no production unit was stopped or restarted, no disposable
listener was opened, no primary weight was allocated, and no cache state or
inference result exists. The retained focused controller/runner/readiness suite
passed 52/52. This is a terminal L19 NOT PROMOTED result, not evidence against
the model, L18 allocation plan, or worker-local protocol.

## Residual boundary

Resolving the blocker would require new Project Lead authority for a different
execution contract: either a true one-load multi-phase coordinator lifecycle
that remains valid across worker restart, or explicit permission for the
multiple material loads inherent in the current isolated-process canary. It
would also require a fully L19-bound controller/child identity and earlier
failure-path evidence capture. This milestone does not open that work and does
not open L20.
