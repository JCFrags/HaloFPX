# L08h-b generation-one protected full-v1 authority independent review v01

Verdict: **ACCEPT** for the Linux-only, excluded, single-controller L08h-b
boundary. No P1/P2 blocker remains.

Review initially found that the factory did not prove one coherent authority
domain, secret vectors could be released without wiping if later construction
threw, root identity was checked only at construction, and source commitment
could traverse or copy oversized caller buffers before all configured caps.
The accepted implementation now cross-checks anchor/replay/admission identity,
moves secrets only after all earlier throwing construction behind a wiping
guard, revalidates both roots at operation and reconciliation boundaries, and
preflights manifest, per-frame, and aggregate sizes before provider creation or
commitment computation.

A closeout repair also moved every allocating terminal-record operation inside
the `noexcept` catch boundary. Independent review confirmed the repair
preserves ACCEPT; local and nimo-1 authority source SHA-256 matched
`1d02f7243f9a963dcd7b7875637fdb6192273ce0ced9df744df379f9892f2dc2`, and the
focused lifecycle passed again.

Restart recovery requires authenticated terminal-success evidence for an exact
anchor with no pending record. A pending record plus exact anchor must verify
material before terminal success and pending removal. Corrupt, malformed,
missing, incompatible, or uncertain state quarantines or misses; it is never
accepted by shape or prefix alone.

The focused target-node test covers normal miss/publish/hit, restart after the
anchor boundary, corrupt-anchor quarantine, and incoherent-domain rejection.
The L08d-L08h chain and inherited feature-off/L02/context-store controls passed.
That is proportionate for an excluded, single-controller seam. Same-instance
concurrency, broader failpoint and hostile-filesystem matrices, generation
advancement, retention, distributed recovery, and soak remain explicit gates
before their corresponding product boundaries.

The target remains excluded from all normal builds and has no server, live
restore, product-registry, donor, dependency, WebUI, or remote edge. A server
canary must introduce serialization and receive a separate review.

Residual P3 work is explicit: replace or wipe the full-frame aggregate-source
commitment duplicate before broader or large-state use; improve live
anchor-corruption miss classification and observability; and add internal
serialization before any server edge. None is admissible as an implicit server
contract.
