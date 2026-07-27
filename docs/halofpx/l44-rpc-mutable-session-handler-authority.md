# L44 — RPC mutable session and handler authority

Result: **PASS**

Base: `aba0f78d07c824c3bcdbcb5ffbdc26e174cda3bf`

L44 corrects the two terminal L43 review blockers without changing L40 graph
authority, L42 scheduler authority, or cache semantics.

## Accepted implementation

Mutable roles and explicit exclusions are owned by a value-identified admitted
session. Each operation revalidates session generation, connection epoch,
attempt nonce, graph UID, execution sequence, and RPC socket/allocation
authority. Registrations exist only inside the private session. Explicit abort,
RPC-buffer destruction, and backend destruction wipe keys/nonces and remove
all registrations. Two simultaneous connections were admitted and foreign,
cross-connection, stale, closed, and buffer-destroyed handles were refused.

Role authority remains structural and caller supplied. Names, tensor sizes,
and `GGML_TENSOR_FLAG_INPUT` are not accepted classification authority.
Unknown graph leaves refuse.

The real RPC `SET_TENSOR` handler was exercised with malformed/tampered
authority, duplicate/out-of-order mutation sequence, out-of-bounds range, and
wrong-view inputs. Each returned the exact bounded refusal receipt (`status=0`)
and did not advance the admitted mutation transcript. A deliberately omitted
unmutated reconstructed leaf was accepted only into the test commit, then
refused by the real graph-prepare handler with authenticated status `3`; the
execute handler was not entered and the output sentinel remained unchanged.

The positive disposable session exercised ordinary SET, a real SET_HASH
miss/fallback followed by cache hit, compute/recompute, an unflagged mutable
input, a nested/strided view, deterministic output, and controlled mutation
sensitivity. Client/server receipts cover the material actually applied after
synchronized server readback. The census and mutation roots remain bound to
the L40 graph execution identity and L42 scheduler admission.

## Feature-off and safety

The code remains Linux-only compile gated and runtime default off. The
compile-off RPC server built successfully. With the feature compiled but
runtime disabled, ordinary HELLO/device-memory RPC succeeded while every L44
admission API remained inert; no mutable command was sent.

No primary artifact was accessed. Production was never mutated. Read-only
closeout evidence records the exact system units active with original PIDs,
ports `50052`/`8081`, HTTP `200`, and `NRestarts=0`. Disposable ports
`51148`-`51150` were closed and no disposable RPC process remained.

## Evidence

Raw evidence and deterministic hashes are under
`docs/halofpx/evidence/l44-raw/`. The independent adversarial review is
`docs/halofpx/reviews/2026-07-26__l44-rpc-mutable-session-handler-authority__review__v01.md`.

L44 is a reusable default-off mutable/update authority layer only. It does not
authorize a primary run, production cache enablement, performance claims, or
L45.
