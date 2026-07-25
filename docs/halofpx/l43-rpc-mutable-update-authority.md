# L43 — RPC mutable-input and update authority

Result: **NOT PROMOTED**

Base: `d0d74ff55d8b063ab73911ae95516512177c824d`

L43 attempted the remaining runtime-default-off mutable-input census and
authenticated RPC `SET_TENSOR` / `SET_TENSOR_HASH` authority layer on top of
the accepted L40 graph and L42 scheduler foundations. The candidate was
removed after independent review found that it did not meet the reusable-layer
acceptance boundary.

## What the rejected candidate established

The disposable RPC qualification exercised canonical little-endian HMAC
records, explicit structural roles and exclusions, authenticated server
readback, compute/recompute, an unflagged mutable input, a nested/strided view,
deterministic output, controlled mutation sensitivity, and a real
`SET_TENSOR_HASH` miss/fallback followed by a hit. Focused correction work also
made scheduler admission source-owned, made reconstructed-leaf census checking
bidirectional, and bound mutation view authority to the census view digest.

These results are retained only as rejected-candidate evidence. They are not
accepted protocol or implementation authority.

## Blocking review findings

Two material gaps remained:

1. The public role-registration API has no admitted-session identity. A
   process-global pointer registry therefore cannot safely serve multiple RPC
   sockets. Serial global cleanup is not a reusable substitute; the accepted
   design needs registrations owned by an explicit session handle, or a
   separately frozen single-session API with concurrency refusal proved at the
   API boundary.
2. Qualification did not drive malformed/tampered updates, duplicate or
   out-of-order mutation sequences, out-of-bounds ranges, wrong view authority,
   and an omitted reconstructed leaf through the real server handlers. Client
   refusal and record/helper self-tests do not independently prove those
   server-side gates.

Per the L43 decision, the rejected source and test candidate were removed.
L40 and L42 remain unchanged.

## Safety and closeout

No primary artifact was accessed and production was never mutated. Disposable
builds, workers, ports, roots, and cache files were removed. Final read-only
checks retained in `docs/halofpx/evidence/l43-raw/` show the exact system
production units active on ports `50052` and `8081`, coordinator HTTP `200`,
and `NRestarts=0`.

L43 does not authorize L44.
