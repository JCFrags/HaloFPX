# L14 disposable RPC application readiness independent review

Date: 2026-07-21

Scope: read-only adversarial review of the CAPS wire/server/client, bounded
probe, primary-runner integration, focused fixtures, exact build, both real
disposable attempts, production invariants, cleanup, and evidence archive.

Verdict: **ACCEPT L14 AS QUALIFIED — NO P1/P2 FINDING REMAINS**

## Findings and corrections

The initial review found three blocking integration issues:

1. applying positive CAPS admission to the deliberate runtime-off worker made
   the later runtime-off control unreachable;
2. the channel loader read before bounding and did not reject symlinks or
   unprotected files; and
3. the runner neither pinned the remote probe nor retained successful JSON.

Commit `432d2e9` resolved these with an exact HELLO-plus-CAPS-rejection
application check for explicit runtime-off mode, bounded protected file loading,
probe SHA pinning, and per-worker/final-summary readiness evidence.

The first real network run then exposed a missed P1: the Python helper assigned
CAPS ordinal 18, while exact C++ enum authority assigns CAPS 17 and CAPTURE 18.
Commit `e6b01a8` corrected the helper and added a test. Final commit `b688680`
compile-anchors `RPC_CMD_HALOFPX_STATE_CAPS == 17`, closing the remaining P2
maintenance gap in which a Python-only literal test could drift with C++.

## Incident boundary

The retained pre-fix journal has 58 zero-length CAPTURE requests, each rejected
at request-shape validation. It has no stored, ready, apply, abort, object, or
live-mutation operation. No contemporaneous failed-run root inventory was
sealed; the no-object/no-live-mutation conclusion is instead supported by the
exact rejections and source proof that rejection precedes object lookup/write
and live mutation. The failed probe and unit were terminated, and their port,
root, and key were removed.

This fail-closed incident must remain visible, but it does not invalidate the
corrected readiness qualification because it produced no admission or state.

## Corrected qualification

The corrected real worker used ROCm on isolated nimo-1 port 50178. The admitted
run used the `e6b01a8` probe and the `0dcc8365...` worker binary originally built
from the unchanged C++ at `95240b5`. PID 2111756 returned the complete exact
CAPS tuple in one attempt after 1.225 ms. Its journal contains zero state
operations and its root remained empty. Final `b688680` added the compile-only
ordinal assertion and reproduced the same binary SHA; all 30 focused tests pass.

The review independently reconciled final source `b688680`, `rpc-server`
SHA-256 `0dcc8365...`, probe SHA-256 `f2db27e...`, exact protocols/sizes/mask/
limits/rank/world/key generation/channel binding, and the secret-safe probe
output.

Production retained its original PIDs, start timestamps, exact commands,
listeners, `NRestarts=0`, standard UD-Q6 model, and HTTP 200. Both transient
units are unloaded/inactive/dead with `MainPID=0`; port 50178, roots, keys,
clones, and builds are absent.

The 36,931,349-byte v2 evidence archive includes explicit post-seal clone/build
cleanup receipts and passes its zstd and SHA-256 checks at
`0898c392cef58a43073a7827ee3f92e78b8130d8a8c4295b484a39f1c788c45f`.
L14 therefore admits the disposable readiness gate only. It does not authorize
a production transition, inference, primary-model retry, or cache enablement.
