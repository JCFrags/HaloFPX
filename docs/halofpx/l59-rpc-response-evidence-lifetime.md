# L59 RPC response evidence lifetime

**Status:** `[MEASURED] NOT PROMOTED`

L59 retained the L58 response-boundary instrumentation and corrected failure
evidence lifetime without changing RPC, model, or cache semantics. The worker
and client streams are now harvested from the canary's `finally` path after the
canary is quiescent and before worker, key, remote-root, or controller cleanup.
Each stream has an explicit `present`, `missing`, or `error` result. Available
closed prefixes are authenticated; two missing streams, a live writer, an
invalid copy, or an invalid verifier result refuse.

The copy path uses the bounded L25 transport primitive, including process-group
termination and typed timeout evidence. Remote input uses no-follow,
owner/mode/type/size bounds and durable staging. Local publication is atomic
no-replace. POSIX retains file and directory `fsync`. Windows uses binary file
`fsync`, atomic no-replace publication, and reopen/type/size/digest
revalidation; evidence identifies that mechanism and does not claim Windows
directory `fsync`.

## Qualification

The focused source suite passed 63 tests and 11 subtests; one POSIX-only
process-group assertion was skipped on Windows and its Linux behavior was
covered by the retained L25 primitive. Independent pre-runtime review accepted
the lifetime and Windows durability correction.

A no-model transient unit
`halofpx-l59-partial-evidence.service`, invocation
`685a294d8d084c6cbc5d91f7d0d5bb94`, PID `2514260`, deliberately exited
status 19 after fsyncing an authenticated 455-byte server prefix. The harvester
retained SHA-256
`3b151b37d39368d212e5996cdcb249c5e345b644e454f7fafeaacae1fb28509f`;
the verifier accepted the prefix and recorded the absent client separately.
Wrong type, mode, owner/access, symlink, and oversize cases refused.

## Deferred discriminator outcome

Neither controller attempt reached stories15M model loading or the first armed
chunk:

1. `l59-raw` stopped after the device receipt when the original Windows
   directory-fd `fsync` attempted to open the child evidence directory and
   received `PermissionError`. This admission failure is immutable.
2. After the authorized durability correction, `l59-raw-replacement` refused
   `systemd-run` because `halofpx-l50-device-gate.service` was still loaded at
   the instant of admission. The fail-closed child cleanup then reconciled it
   to `not-found/inactive`, with port 50249 absent.

The replacement was the only authorized replacement execution. No RPC
response-boundary discriminator result exists, and L59 makes no model/cache
inference. The narrow remaining controller defect is transient-unit
name/lifecycle reconciliation before reuse; correcting or rerunning it requires
new authority.

Production was read-only and byte-identical across both controller snapshots:
nimo-2 worker PID 1535639 on port 50052 and nimo-1 coordinator PID 2356329 on
port 8081 with HTTP 200, both `NRestarts=0`. Disposable ports, keys, units, and
manifest-owned roots were absent at closeout.
