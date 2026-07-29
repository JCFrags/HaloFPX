# L94 independent terminal review

Disposition: **ACCEPT NOT PROMOTED; no retry**.

The independent reviewer found no P1, security defect, or accepted invalid
state. The retained default-off source is safe because both newly exposed
controller defects fail closed:

1. The restore-canary `systemd-run` completed successfully, but the controller
   searched only stdout for the InvocationID. Operation sequence 331 retained
   rc=0, empty stdout, and stderr containing InvocationID
   `ca734d620f8c42e7805e8c6ac23bc9fb`. The terminal refusal was therefore
   `restore canary launch InvocationID is unavailable`.
2. Finally cleanup compared raw `ps -o cgroup=` output directly with systemd's
   normalized `ControlGroup`. The runtime reported
   `shared listener alternate owner identity mismatch`; this remained
   fail-closed and bounded external cleanup succeeded.

The review credits the exact journal-cursor correction, authenticated
residency-A token `21549` / suffix `alpha`, four retained authenticated
4200-byte server authorities, and fresh restore-worker admission. It does not
credit a residency-B result, restored token, state equality, or cache
correctness conclusion.

Terminal cleanup and recovered production evidence were accepted:

- coordinator PID `2947160`, InvocationID
  `23808765f78d4d6eaf506052ac91aab4`, NRestarts `0`, unique listener `8081`,
  HTTP 200;
- worker PID `2124976`, InvocationID
  `9622a846b8ed45838afc5657f64c5bdd`, NRestarts `0`, unique listener `50052`;
- unchanged production unit/argv/configuration/executable identities, no
  observed continuing OOM/restart/fault, and all L94 disposable resources
  absent.
