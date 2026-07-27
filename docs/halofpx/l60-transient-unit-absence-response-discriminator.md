# L60 transient-unit absence and RPC response discriminator

**Status:** `[MEASURED] NOT PROMOTED`

L60 added a fixed prelaunch and post-cleanup absence guard to the closed
stories15M controller path. For every admitted transient unit, it binds the
exact user manager, host, unit, and listener port and requires
`not-found/inactive/dead/MainPID=0`, no fragment or control group, no matching
PID, no listener, and no unit-file registration. An ownerless inactive or
failed transient unit receives one bounded stop/reset-failed reconciliation;
active or ambiguously owned state and every authority-query failure refuse.
The guard runs immediately before each `systemd-run` through the L60 manifest
path and after cleanup.

The pre-runtime focused suite passed 26 tests after the final operational
corrections. Closeout verification of the affected L59/L60/controller surface
passed 76 tests and 11 subtests, with one retained POSIX-only assertion skipped
on Windows.
The real no-model qualification reconciled
`halofpx-l60-stale-guard.service` from loaded/failed/failed with a transient
fragment to exact not-found authority. The first preserved qualification
attempt remains evidence that the initial predicate rejected the real
ownerless failed state; the narrow correction admitted only that safe state.
Independent pre-runtime review accepted the corrected guard, explicitly
scoped to the L60 manifest-owned launch path.

## Deferred first-chunk discriminator

The sole stories15M runtime attempt loaded the disposable model and entered the
first armed 512-token execution. The RPC worker logged ordinary execution of a
144-node/193-tensor graph and subsequent reads. The coordinator then returned
scheduler status `-1`, `llama_decode` status `-3`, logged `Remote RPC server
crashed or returned malformed response`, and later aborted while freeing its
RPC buffer.

L60 cannot classify the response boundary. The mandatory response streams were
not retained:

- the client stream was absent;
- worker harvesting failed before collection because the controller asked
  nimo-1 to hash the nimo-2 source path
  `/var/tmp/halofpx-l48-source-nimo2/scripts/halofpx_rpc_response_harvest.py`.

The client process then exited by `SIGABRT`/core dump during teardown. Journal
text is useful directional evidence but is not a substitute for the missing
authenticated response-boundary records. Therefore L60 makes no claim about a
worker crash, response publication, truncation, opcode/size mismatch, socket
failure, or client validation. The smallest remaining prerequisite is to bind
the harvester executable path per host (and prove client prefix durability
before teardown) before any future discriminator. No correction or retry was
performed in L60.

The earlier rejected admission attempt is preserved separately. Production was
read-only and byte-identical before and after the runtime attempt: nimo-2
worker PID 1535639 on port 50052 and nimo-1 coordinator PID 2356329 on port
8081 with HTTP 200, both `NRestarts=0`. All eight recorded prelaunch/postcleanup
unit guards ended at exact absence, and the controller removed disposable
units, ports, roots, keys, source staging, and evidence staging.
