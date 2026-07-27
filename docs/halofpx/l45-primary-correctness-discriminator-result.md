# L45 — primary correctness discriminator

Result: **NOT PROMOTED — PRE-MUTATION INTEGRATION BLOCKER**

Base: `5f69d5cdaf8eb51283dd750c1fd8ca869fcf4d66`

L45 stopped before its controller could mutate production. Source inspection
proved that the accepted L44 mutable-session API is not wired into the real
distributed-state canary or its primary replay runner. Those paths contain no
session begin, structural role registration/exclusion, census commit, result
capture, or abort calls.

With `HALOFPX_RPC_MUTABLE_AUTH=1`, the RPC graph-compute path requires an
active committed L44 session and therefore refuses the current primary caller.
Leaving the feature disabled would omit authority that L45 explicitly
required. A manifest-only change cannot close this gap: roles must be
registered at the source call sites that create the replay's mutable inputs,
then bound to L42 scheduler admission and retained as authenticated evidence.

No primary artifact was read, hashed, mapped, allocated, or loaded. Production
was not stopped, restarted, or otherwise mutated. Read-only reconciliation
confirmed nimo-2 worker PID `1535639` on port `50052` and nimo-1 coordinator
PID `2356329` on port `8081`, HTTP `200`, with both `NRestarts=0`. The
worktree remained clean at the stop.

L45 establishes no correctness, performance, or cache-promotion claim.
