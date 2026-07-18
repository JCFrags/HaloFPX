# L05m offline bootstrap-material preparation seam

Status: accepted as a permanently synthetic offline contract after independent
adversarial review.

L05m closes the semantic gap between consumed bootstrap authority and the
future protected-anchor gate without claiming real storage durability. The new
target is excluded from the product, consumes an L05k or L05l proof as its
first action, owns an immutable bounded in-memory source set, and exposes no
filesystem, protected-anchor storage, server, provider, or persistence API.

Distinct fixed registry and material roots, exact policy/epoch/limit bindings,
the exact digest-identified manifest, all ordered descriptors, exact source
frames, and all provenance fields enter five independently recomputed
commitments. Every source and positive readback passes the complete L04c-
equivalent frame checks. Positive results require complete frame and manifest
readbacks plus an exact terminal-close confirmation.

The returned proof is permanently synthetic and non-convertible. It can never
be promoted into a concrete durability or production-anchor proof. Attempt
history is fixed at 512 without eviction; capacity, replay, fresh concurrency,
last-slot races, post-positive exceptions, and uncertainty quarantine all fail
closed.

Independent review found and closed aggregate-limit underflow, moved-from
anchor ownership, missing independent commitment recomputation, incomplete
provenance access, an ineffective CMake isolation check, post-backend exception
misclassification, and substantial adversarial-coverage gaps. Final review
returned ACCEPT only after the exact manifest, source/readback structural,
concurrency/history, response mutation, and 28-boundary failure matrices were
present and green.

The clean Windows CPU/WebUI-off Release control passed 31/31 HaloFPX CTests,
seven focused inherited tests, and 600/600 repeated authority, static-contract,
and independent-golden processes. Exact hashes are retained in
`evidence/l05m-bootstrap-material-repeat-receipt.json`.

Still closed: any concrete object source or filesystem backend, protected
registry durability/latestness, restart/cross-process fencing, real no-replace
and synchronization semantics, protected-anchor inspection/create, bootstrap
execution, cache admission, persistent writes, server/provider integration,
node use, and performance qualification.
