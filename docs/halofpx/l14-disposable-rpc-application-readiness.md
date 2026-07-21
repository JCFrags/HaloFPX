# L14 disposable RPC application readiness

Date: 2026-07-21

Authorized base commit: `aa3c2cf63e44305057a7b4715efb28f492a82d07`

Final implementation commit: `b688680e8b2027f095a8414c18995009fb433451`

Outcome: **PASS — APPLICATION-LEVEL CAPS READINESS QUALIFIED**

## Decision

[MEASURED] The disposable HaloFPX RPC worker is no longer admitted by
systemd-active, listener visibility, TCP connect, ready-banner text, or
listener/MainPID agreement alone. The primary runner now requires a bounded
application exchange: exact RPC HELLO followed by the exact runtime-bound
HaloFPX CAPS response. A successful probe is rechecked against unit and listener
identity and its secret-safe JSON is retained.

No production service was stopped or started. No primary model was opened, no
inference ran, and no worker-local object or live state was created. The known-
good nimo-1 coordinator and nimo-2 worker remained live throughout.

## Exact readiness contract

The server answers CAPS only when worker-local state is compiled and configured
at runtime. Feature-off workers complete exact RPC HELLO but reject CAPS and
cannot be admitted as HaloFPX-ready. The deliberate future runtime-off control
has a separate application proof: exact HELLO plus confirmed CAPS rejection; it
does not receive positive HaloFPX admission.

The 128-byte little-endian `HFXCAP2` response binds:

| Field | Required value |
|---|---|
| RPC protocol | 4.0.1 |
| state protocol | 1.0 |
| CAPS command ordinal | 17, compile-anchored |
| encoded CAPS bytes | 128 |
| command mask | 31: CAPS, CAPTURE, STAGE, COMMIT_APPLY, ABORT |
| maximum request / response | 1,048,576 / 256 bytes |
| maximum components | 4,096 |
| maximum component / object | 1,073,741,824 / 68,719,476,736 bytes |
| state timeout | 5,000 ms |
| logical rank / world | 1 / 2 |
| key generation | 7 |
| channel identity | exact 32-byte expected binding |
| reserved fields | all zero |

The channel file loader is bounded to 256 bytes, refuses symlinks, requires a
regular current-owner protected file on Linux, and parses canonical lowercase
hex. Output retains only the SHA-256 of the channel and connection-capability
bytes; it does not print the control key or raw channel binding.

The default limits are explicit: 120 seconds total, 2 seconds per attempt,
0.1-second initial backoff, 1-second maximum backoff, and at most 256 attempts.
Connect failure, early disconnect, and per-attempt timeout retry only within
those bounds. Malformed sizes, wrong protocol, and wrong capabilities are
terminal mismatches.

## Focused qualification

[MEASURED] Thirty focused tests passed at final source. The artificial
listener-first fixture publishes a TCP listener before application protocol
readiness, delays readiness for 0.18 seconds, and requires the probe to take at
least two attempts and wait at least 0.17 seconds before admission. Other tests
cover bounded timeout, early disconnect, feature-off non-admission, explicit
feature-off confirmation, malformed CAPS, wrong RPC version, wrong endpoint
protocol, wrong capability, exact command ordinal, protected integration, and
the existing production-transition fail-closed matrix.

[VERIFIED] Final Release configuration compiled with HIP, Vulkan, RPC, and
`GGML_RPC_HALOFPX_LOCAL_STATE` enabled. Exact identities:

| Artifact | SHA-256 |
|---|---|
| final source bundle | `e87636008e9593de7dc446989c76d215c359a7574d38f39074d7fa7c5978bbe7` |
| ROCm-enabled `rpc-server` | `0dcc836507076fcfeb62c330bb5669e205885f97d7e4ea9cf0cc55e48d719019` |
| compiled RPC local-state test | `db6db956db834659fb12a8bdbb284b0e501bb257d25fa0c72afdc51395cf44d8` |
| readiness probe | `f2db27e26567b33a4d4e69c5cb248cf61b63dfa3765aa218d09668225905c980` |

The admitted network run used the corrected probe from `e6b01a8`. Its worker
binary was originally built from the unchanged RPC implementation at
`95240b5`. After the run, final source `b688680` added only the compile-time
ordinal assertion and reproduced the exact same worker binary SHA-256. The
network-run identity and final compile-qualified source are therefore distinct
and explicit.

## Preserved pre-fix incident

[MEASURED] The first real network invocation exposed a probe defect before the
successful qualification. The Python probe used command ordinal 18 while the
current RPC enum assigns CAPS to 17 and CAPTURE to 18. Its 58 zero-length
requests were all rejected at CAPTURE request-shape validation. The probe never
received CAPS and was not admitted.

The failed journal contains no stored, ready, apply, abort, object, or live-
mutation operation. Preflight proves the root was absent before setup, but no
contemporaneous failed-run root inventory was sealed before cleanup. The no-
object/no-live-mutation conclusion instead follows from the 58 exact rejection
records plus source inspection showing empty-request rejection before object
lookup/write or live-state mutation. The probe was terminated and the first
disposable unit, root, key, and port were cleaned.

The correction at `e6b01a8448306cd60eacc558ac97964b8a999585` changed the
probe to ordinal 17 and pinned its new hash. Final commit `b688680` additionally
places a C++ `static_assert` on CAPS ordinal 17 so this source/wire mapping cannot
drift silently. The initial local 132-byte expectation-file construction error
is also retained; the protected loader rejected it before any connection.

## Corrected real disposable gate

[MEASURED] The corrected probe connected from nimo-2 to a real ROCm-enabled
disposable worker on nimo-1 port 50178. PID 2111756 returned the exact CAPS tuple
in one application attempt after 1.225 ms. The client closed after the exchange.
The worker journal contains zero `[halofpx-state]` operations, and the protected
worker root was still empty while the unit was live.

Final production authority was unchanged from preflight:

| Host / role | Unchanged authority |
|---|---|
| nimo-2 worker | PID 1275544, port 50052, start 03:42:11 PDT, `NRestarts=0` |
| nimo-1 coordinator | PID 2093167, standard UD-Q6 model, port 8081, start 03:42:12 PDT, HTTP 200, `NRestarts=0` |

Both L14 transient units ended unloaded/inactive/dead with `MainPID=0`. Port
50178, both roots, both protected expectation/key files, source clones, build
trees, and intermediate bundles were removed after evidence sealing. The
retained evidence archive and raw evidence root remain on nimo-2.

## Evidence and scope closure

The evidence archive is retained on nimo-2 at
`/var/tmp/halofpx-l14-readiness-evidence-20260721-v2.tar.zst`, size 36,931,349
bytes, SHA-256
`0898c392cef58a43073a7827ee3f92e78b8130d8a8c4295b484a39f1c788c45f`.
It contains preflight/final production snapshots, build configuration/logs,
source and binary hashes, focused tests, both worker journals, the rejected
precondition and ordinal incident, corrected CAPS JSON, live empty-root/unit
evidence, pre- and post-seal cleanup receipts, the exact source bundle, and a
per-file checksum manifest.

Independent adversarial review accepts L14 with no remaining P1/P2 finding.
This milestone admits only the disposable readiness gate. A primary-model retry,
production transition, cache enablement, inference, or another lane requires a
new Project Lead decision.
