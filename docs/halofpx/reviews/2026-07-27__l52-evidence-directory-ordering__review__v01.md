# L52 evidence-directory ordering terminal review

Date: 2026-07-27
Review type: independent adversarial source, controller, and evidence review
Verdict: **ACCEPT — terminal NOT PROMOTED**

The reviewer first withheld runtime approval for two material gaps: incomplete
cleanup ownership after directory admission and a publication rename that
could overwrite a racing destination. The corrected candidate placed all
post-admission work inside cleanup/reconciliation authority and used one remote
publication helper with `O_NOFOLLOW`, type/mode/size/digest/inode checks, file
fsync, `renameat2(RENAME_NOREPLACE)`, directory fsync, final reopen and
revalidation, followed by independent controller verification.

A second review withheld approval when the reconstructed controller and child
double-serialized remote argv for fish. The accepted boundary passed structured
argv from the child and performed per-element quoting exactly once in the
bounded SSH runner. The focused suite passed 58/58 and end-to-end controller
preflight passed before the sole runtime execution.

For terminal closeout, the reviewer independently verified:

- all 20 `SHA256SUMS` entries and the checksum-list hash
  `8ca25ed998ffd2629c8621813fef0d4e0d61db795c9aec983fd236ef9ea1598b`;
- directory-before-publication ordering and authenticated, no-replace,
  fsynced receipt publication;
- exact ROCm0/ROCm/gfx1151 device admission, HELLO/HFXCAP2 authority, and
  RPC0/ROCm0 placement;
- the capture coordinator's exact warmup failure boundary:
  `ExecMainCode=1`, `ExecMainStatus=4`, `Result=exit-code`, decode return `-3`;
- byte-identical production snapshots with exact PIDs, commands, listeners,
  `NRestarts=0`, and HTTP 200;
- all five disposable units absent/inactive/dead with `MainPID=0`, port 50248
  absent on both hosts, and all manifest-owned paths and keys removed;
- no retry and no primary artifact access; and
- removal of the rejected runtime/controller/test candidate from the terminal
  worktree.

Receipt:

> ACCEPT — terminal L52 NOT PROMOTED is evidence-supported; hashes,
> ordering/atomicity, gate results, failure boundary, production equality, and
> complete cleanup reconcile, with no retry or primary access.
