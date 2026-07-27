# L57 parent/split graph identity correction

Status: **NOT PROMOTED**
Date: 2026-07-27
Base: `8af226d675d9ae287d5d2bddd849f9920507d9ba`

## Source result

The reviewed correction represents scheduler execution identity at two explicit
levels:

- the overall scheduler graph UID remains the parent execution authority;
- every actual scheduler split has an immutable scheduler-owned tuple of parent
  UID, execution sequence, split ordinal, backend ordinal, and split graph UID.

The complete ordered mapping is HMAC-bound into the prepared admission. Each
RPC backend receives only its exact scheduler-owned split subset and the common
mapping root. RPC compute/recompute refuses an unbound graph, and result
reconciliation consumes the authenticated receipt for the exact split UID,
ordinal, backend, parent, execution sequence, attempt, and mapping root.
Finalize, abort, and disarm remove the mapping and result authority.

The L56 refusal subreason remains available. Successful composed evidence now
records the parent UID, split ordinal/UID, backend, reconciliation status, and
an explicit RPC split count. The standalone verifier requires exact
cardinality, strict integer/range authority, scalar/list consistency, ordered
unique splits, and phase-neutral backend/ordinal/graph-digest equality.

Focused qualification passed:

- 56 Python controller/result/status tests, including strict missing,
  duplicate, reordered, stale-shaped, wrong-backend, malformed, and boolean
  authority refusal;
- the Linux scheduler authority test with both one- and two-split mappings;
- the RPC split binding/refusal fixture;
- exact worker, canary, placement, and RPC builds with L57 provenance;
- closed controller dry-run against the frozen manifest and staged binaries.

Independent pre-runtime review returned **GO** after one verifier-completeness
finding was corrected and re-reviewed.

## Sole stories15M execution

The one authorized controller-managed stories15M run passed the protected-key
provisioning, evidence publication, ROCm device admission, HFXCAP2 readiness,
and placement gates. The capture coordinator then failed during its first
execution:

- unit: `halofpx-l48-canary-capture.service`;
- invocation: `13c8604cb3c242008d1a94c738fcced2`;
- main PID: `1736662`;
- terminal systemd result: `core-dump`, `ExecMainCode=3`,
  `ExecMainStatus=6`;
- coordinator status record:
  `branch=scheduler_graph_compute_failed_authority_0`,
  execution sequence `1`, pending `1`, GGML status `-1`;
- immediately retained source location:
  `ggml-rpc.cpp:1851`, “Remote RPC server crashed or returned malformed
  response.”

The worker journal records an ordinary 144-node RPC graph execution before the
coordinator failure. The retained evidence does not contain a completed L40
parent/split receipt, L42 final transcript, L44 final census/update result, or
composed-result record for this execution. Therefore L57 makes no
parent/split runtime correctness claim and does not infer a deeper cause from
the abort/backtrace. No second prompt chunk, capture, restore, token, or repeat
occurred.

## Production and cleanup

Production was never stopped or reconfigured. The before/final snapshots are
byte-identical (SHA-256
`511f05d2b638277e19ae7af44573eb9e244d829458f24cdefc46e3f88ffd6ded`).
Closeout independently reconciled:

- nimo-2 system worker: PID `1535639`, exact system cgroup/command, port
  `50052`, `NRestarts=0`;
- nimo-1 system coordinator: PID `2356329`, exact standard UD-Q6 model
  command/system cgroup, port `8081`, HTTP `200`, `NRestarts=0`.

All disposable units were collected; ports `50248` and `50249`, source/build
trees, archives, roots, evidence publication path, and protected keys were
absent. The immutable raw evidence contains 21 files with canonical tree hash
`2867f6c8b402e3030855541b9686e9ba785ccff8dfc5661aa1140435ef8d4c14`.

L57 is terminal **NOT PROMOTED**. It does not make the primary path
preflight-ready and authorizes no retry or L58.
