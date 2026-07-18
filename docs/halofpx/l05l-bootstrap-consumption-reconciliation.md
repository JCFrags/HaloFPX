# L05l bootstrap-consumption ambiguity reconciliation

Status: accepted offline after independent adversarial review.

L05l adds a target-owned reconciliation seam for the one unsafe ambiguity left
by L05k: an exact registry compare-and-advance whose completion could not be
classified. The seam remains excluded, synchronous, memory-only, and detached
from the server. It opens no filesystem, anchor, cache, persistence, or node
path.

The backend retains the exact uncertain L05k operation and admits one fresh
root-scoped reconciliation attempt. Its sole primitive atomically fences the
original attempt and observes the protected-registry head. A read followed by a
fence is forbidden. The response must echo the root, both attempts, and both
operation commitments before a bounded exact-byte witness can be classified.

An exact successor yields a move-only recovered proof. An exact predecessor is
terminal and explicitly grants no retry authority. Any other present head is a
conflict. Absence, unreadability, malformed or contradictory responses,
exceptions, incomplete fences, uncertainty, and late-completion risk produce no
proof and sticky-quarantine the backend root. Replays and materially different
plans cannot enter reconciliation.

Independent review first returned REVISE because the tests lacked an
independently implemented commitment vector, a two-fresh-attempt concurrency
case, reentrant-lock evidence, and complete binding/outcome coverage. The final
suite adds a standard-library Python vector with nine mutations, a separately
written C++ serializer checked at every backend callback, an exactly-one-call
race, a reentrant quarantine check, every non-present outcome, confirmation and
binding mutations, hostile witness shapes, and reachable terminal quarantine.
The final independent verdict is ACCEPT.

The clean Windows CPU/WebUI-off Release control passed 29/29 HaloFPX CTests,
seven focused inherited tests, and 600/600 repeated authority, successor-golden,
and independent-reconciliation-golden processes. Exact hashes are retained in
`evidence/l05l-bootstrap-consumption-reconciliation-repeat-receipt.json`.

Still closed: a concrete protected registry, durable or restart-surviving
fencing, cross-process coordination, key custody, durable bootstrap-material
proof, anchor inspection or creation, bootstrap execution, cache persistence,
server integration, provider behavior, and node qualification. ADR-0015 keeps
the required order explicit: recovered consumption proof, then exact durable
objects and manifest proof, then one atomic protected-anchor operation.
