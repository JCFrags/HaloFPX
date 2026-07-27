# L52 evidence-directory ordering result

Status: **NOT PROMOTED**
Date: 2026-07-27
Base: `1746c15c9688cb068751ab40619bb0637cff1b3a`

## Scope and outcome

L52 reconstructed the independently pre-runtime-approved L51 disposable
stories15M candidate and corrected only the controller-owned evidence namespace
ordering and device-receipt publication authority. The focused correction
passed review and the single authorized controller execution was attempted.

The execution passed the exact ROCm device gate, atomic receipt publication,
worker HELLO/HFXCAP2 readiness, and placement admission. It then failed during
the capture coordinator's model warmup before prompt chunks, capture, restore,
or composed-result verification. The exact unit result was
`ExecMainCode=1`, `ExecMainStatus=4`, `Result=exit-code`; its journal records
`llama_decode: failed to decode, ret = -3`. L52 is therefore terminal
**NOT PROMOTED**. No retry was performed.

This result does not classify the decode failure as a cache defect and does not
establish stories lifecycle correctness or primary preflight readiness.
The reconstructed runtime/controller candidate was removed from the terminal
worktree as required for a NOT PROMOTED closeout; only reviewed documentation
and immutable evidence remain.

## Evidence-ordering correction

The execution's closed L48 fixture manifest owned the exact nimo-2 evidence directory,
temporary/final receipt names, owner, and modes. Before any disposable child or
device publication, the controller:

1. exclusively creates the local child evidence directory and remote
   `/var/tmp/halofpx-l48-evidence`;
2. verifies normalized paths, cleanup ownership, directory type, owner,
   mode `0700`, emptiness, and absence of a preexisting namespace;
3. admits key provisioning and child execution only after that directory
   authority exists.

The device receipt is copied only to
`.device-admission.pending`, opened without following links, checked for exact
regular-file type, owner-only mode, size, digest, and stable inode identity,
fsynced, and published with `renameat2(RENAME_NOREPLACE)`. The directory is
fsynced and the final file is reopened and revalidated. The controller
independently requires the final file and digest and the absence of the
temporary name.

All post-admission operations are inside the controller cleanup/reconciliation
scope. Focused tests covered success, publish-before-create, collision,
tamper, wrong mode, partial cleanup, structured fish-safe argv, and the
affected manifest/result bindings. The final focused suite passed 58/58.

## Single execution facts

- Device receipt: `ROCm0`, backend `ROCm`, `gfx1151`.
- Worker binary SHA-256:
  `7a8fb0496486cc12746ec31f7ed3eb32ba6d9b450948bb2fc39dd7014e194b10`.
- Receipt bytes/SHA-256: `631` /
  `86329338612449190502fc477e4b011b7de69b009f8e942a689a5d255e01267a`.
- Publication: no-replace atomic publication PASS; file and directory fsync
  recorded.
- Capture worker readiness: admitted on `10.44.0.1:50248`, RPC protocol
  `4.0.1`, state protocol `1.0`, rank `1`, world `2`, all five state commands.
- Placement: exact `RPC0,ROCm0`, layer split, tensor split `1,1`; 32 repeating
  layers on RPC and 30 on ROCm, with the output device ROCm0.
- Capture coordinator unit:
  `halofpx-l48-canary-capture.service`, PID `1697527`,
  `ExecMainStatus=4`.
- Earliest retained runtime failure: warmup `llama_decode` returned `-3`.
- Prompt chunks completed: zero.
- Capture/restore token equality: not exercised.
- L40/L42/L44 composed records: not produced.
- Legacy state GET/SET acceptance: not reached.

## Production and cleanup

Production was never stopped or restarted. The before and after snapshots are
byte-identical with SHA-256
`511f05d2b638277e19ae7af44573eb9e244d829458f24cdefc46e3f88ffd6ded`:

- nimo-2 `minimax-m27-rpc-worker.service`: active/running,
  PID `1535639`, listener `50052`, `NRestarts=0`;
- nimo-1 `minimax-m27-q6-server.service`: active/running,
  PID `2356329`, listener `8081`, HTTP `200`, `NRestarts=0`.

Execution cleanup removed all manifest-owned source, build, state, evidence,
rendezvous, key, and unit namespaces on both hosts. Controller evidence and
the adjacent read-only reconciliation verified their absence.
The adjacent read-only cleanup reconciliation records all five disposable
units as `not-found/inactive/dead` with `MainPID=0` and port `50248` absent on
both hosts.
No primary artifact was accessed.

## Evidence identity

The retained evidence is under `docs/halofpx/evidence/l52-raw/`.
`SHA256SUMS` has SHA-256
`8ca25ed998ffd2629c8621813fef0d4e0d61db795c9aec983fd236ef9ea1598b`.
The rejected pre-runtime admissions remain preserved externally under their
unique `HaloFPX-L52-runtime-01` through `-04` identities; none launched the
stories child.
