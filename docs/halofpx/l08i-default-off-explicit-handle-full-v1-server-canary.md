# L08i default-off explicit-handle full-v1 server canary

L08i is the first HaloFPX milestone to demonstrate an actual target-native
persistent snapshot through the server boundary: miss, publication, process
restart, authenticated hit, and corruption-driven safe recomputation. It is a
Linux-only, four-gate laboratory canary and remains off in every normal build
and runtime.

The implementation parent is HaloFPX
`8e9d18743e8ea31593d2abcddf9304ad1d8ed219`, tree
`b39259cc7ddb04cd6357c01d2cc3d58306a3853e`. The locked ROCmFPX base remains
`61f2f2d7bc4955e9bca821095ef69125837133b5`, tree
`0a35143f33a7b99a81c824fa8ffd8f743f7ae0dd`.

The server edge is explicit rather than opportunistic. Publish returns one
64-hex selected-manifest handle. A later process must present that exact handle,
the exact checkpoint lineage, and the exact expected token sequence. The
adapter derives a fixed manifest path from the parsed digest, uses Linux
`openat2` containment, verifies the manifest signature and closed compatibility
domain, reconstructs the admitted object roster, and then delegates exact
anchor reconciliation and lookup to L08h-b. Decode validates all state before
the existing L07 live restore seam mutates llama state. Incomplete,
unauthenticated, incompatible, corrupt, or missing material is a miss. A failed
live restore clears the destination slot before the response returns.

On Strix Halo target nimo-1, a Release CPU canary using the 15M stories fixture
passed this exact process sequence:

1. an absent explicit handle missed;
2. cold deterministic continuation was recorded;
3. a separate process captured and published the prompt state;
4. after process stop, a new process restored an authenticated hit;
5. warm continuation exactly equaled the cold continuation;
6. one byte of `anchor.v1` was corrupted and synchronized;
7. another new process rejected the restore and recomputed the same continuation.

The final selected manifest was
`7158c3137b9d3f720dfb8fca974de600b4a93c176f116465d681c6897be0ce97`.
The gated server SHA-256 was
`e2b78a4644c588f2c7f2872fd647095b6805b30de7633bf69ad121a6f561dba2`;
the tiny model SHA-256 was
`66967fbece6dbe97886593fdbb73589584927e29119ec31f08090732d1861739`.
Raw logs, tuple, result, and synchronized object hashes remain under the
nimo-1 evidence root recorded in the receipt.

The gated focused/inherited chain passed 14/14 after its excluded executables
were explicitly built. The feature-off/L02/codec-isolation control passed 3/3.
One real contract defect was corrected: the codec's static-isolation check now
recognizes its single extra reference through the explicit full-v1 server gate,
while still rejecting an ungated server edge. The normal feature-off CLI does
not expose `full-v1-rw-canary`.

Testing was deliberately risk-proportionate. This milestone does not multiply
tenant, hostile-path, fault-boundary, disk-full, reserve, concurrency, restart,
distributed, or large-state permutations. Those matrices are deferred until a
corresponding feature or defect makes them material. The one controller-thread
server path, authenticated corruption-as-miss behavior, feature-off contract,
inherited state/codec/authority smoke set, and independent review are the L08i
promotion boundary.

Residual P3 hardening is explicit: add an adapter-local serialization guard
before widening beyond the server queue, improve startup root-layout diagnostics,
and securely wipe all transient encoded/read-only frame containers before any
large or non-disposable state admission. The focused canary now requires exact
`miss-not-found` and `miss-corrupt` statuses. Its disposable operator-key file
is removed after the run; only the hash is retained.

Review against the canonical Wiki found the canary aligned with exact-state
admission, private authenticated scope, target-owned bounded format, immutable
objects, explicit handles, synchronized publication, and corruption-as-miss.
It does not close production durability, retention/quota/reserve, observability,
distributed ownership, large-state soak, or final performance gates. The
known-good `minimax-m27-q6-server.service` remained active and enabled. No
reference clone, deployment, model, remote, WebUI, donor code, dependency, or
license boundary changed.
