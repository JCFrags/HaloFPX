# L31 corrected primary restore confirmation — independent review

Date: 2026-07-23

Verdict: **PASS — TERMINAL NOT PROMOTED**

## Scope

The review independently checked the L31 runner/controller integration,
authenticated component analysis, immutable raw evidence, result and receipt,
production recovery, cleanup, and the terminal interpretation boundary.

## Findings

No material finding remains.

- The frozen-hash analyzer is wired into the actual L31 fresh-residency path
  before suffix comparison and fails closed on hash, exit, schema, phase,
  mismatch, or authentication failure.
- Each phase independently reconstructs to 64 components, 2,454,528 bytes,
  all type 8, 32 kind-1 plus 32 kind-2, and aggregate
  `014a1024f13225a3f7bd7bba6be43dce1106a0354d68b5043f284263cce19bc9`.
- Capture and stage Merkle roots are
  `0fc1f297514f5b7a38f9db179e80df0bb9b5ac6ae3865858d518669ef8a79884`;
  apply is
  `aac062cc10c560807b80d06bcf49ea79deb563f89d6924b7d933d278a66fe429`.
  Capture/stage ranges are exactly `[38656*n,38656*n+38352)`, with 304-byte
  gaps. Apply ranges and the 63 differing ordinals 1–63 match raw records.
  No phase has overlaps; each has one buffer group.
- The authenticated report has zero component identity/content mismatches.
  Its interpretation is correctly limited: equal content with different
  normalized topology does not establish a root cause.
- Coordinator control/local/manifest receipts are identical. Capture token
  21549 and restored token 9283, their token/text hashes, and the terminal
  first-token mismatch match raw evidence.
- Capture and restore state windows each contain zero legacy `GET_TENSOR` and
  zero `SET_TENSOR`. Both residencies record the 80,950,550,528-byte worker
  allocation.
- Transport streams contain exactly 625 controller and 229 child records,
  with no timeout.
- Production recovery is worker-first and reconciles exact system units,
  cgroups, PIDs, commands, listeners, HTTP 200, and `NRestarts=0`. Cleanup is
  complete.
- The immutable raw tree independently recomputes to 41 files, 2,002,777
  bytes, SHA-256
  `f8cbc28dace26dbbf25747b38316ba02b77679e49db808c9014aa2215f223de8`.
- The focused suite passes 88/88.

## Conclusion

Accept L31 as a coherent terminal NOT PROMOTED correctness result. Do not
infer a semantic root cause, retry the primary run, enable the cache in
production, claim performance, or open L32 under this milestone.
