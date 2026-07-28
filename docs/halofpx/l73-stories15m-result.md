# L73 Stories15M feature-on replacement

Status: **NOT PROMOTED**

L73 corrected the canonical census projection so disposition is the primary
sort key within each RPC backend: one contiguous REGISTER block precedes one
contiguous EXCLUDE block, followed by the existing pointer-independent
provenance, stable identity, copy, and role keys. Sealing and runtime iteration
continue to consume the same immutable list. Grammar v1, counts, APIs, and
duplicate/conflict semantics were not changed.

Focused tests produced self-test mask `0xfffff`: intentionally interleaved
candidates export REGISTER then EXCLUDE, exact duplicates still collapse,
conflicts still refuse, sealed root/counts match the exported list, runtime
iteration consumes it exactly, and feature-off remains inert. Required Linux
feature-on targets built. The focused pre-runtime independent review passed
with no P1/P2.

Exactly one authorized feature-on Stories15M replacement then completed:

- token `29916`, output `x` (`78` hex), exactly matching the retained L68
  feature-off control;
- authenticated scheduler prepared/final status `1`;
- mutable reconcile/status `1`, census `49`, authenticated receipt;
- server prepare and execute for sequence `1`, UID `27`, digest
  `03630ca6d2e58012dea774437b6733a373e3a18344ae074e1f80c24e68659f86`;
- authenticated graph status `2`, matching digest and receipt;
- successful canary exit and bounded cleanup.

Promotion is nevertheless rejected. After authenticated execution and client
success, the separate server preexecute recorder terminally refused during
teardown because it had only L44 begin plus expected census and had not
recorded/imported the other full-grammar lifecycle facts. Its terminal record
was `begin=1 l42=0 l44=1 register=0/13 exclude=0/36 prepare=0 commit=0
decision=0 transport=0 abort=1 end=0`. The terminal reviewer classified this
contradictory server-local authority as one correctness/evidence P2; there was
no P1. No retry or semantic expansion was attempted.

Build identity:

- source root:
  `d1c890121f8535f9b6192ac077d253991c9fd30bee474cd9f2d7fa9da3162e47`
- build ID:
  `c558440c4c3af96e81c208824f8af78914fb857f04042fb94dc2081a5eddd611`
- worker SHA-256:
  `d8b04149cb9e4d3080eadbd514e26262cf72548fb416be17278ea2d0bc8715dd`
- canary SHA-256:
  `8efba96d4f9edf1184b9016e281b14d765dcd61e6098c3742822c57286ccb5c2`

Cleanup removed disposable units, keys, paths, and staged source roots.
Production preflight and final snapshots are byte-identical SHA-256
`511f05d2b638277e19ae7af44573eb9e244d829458f24cdefc46e3f88ffd6ded`,
with unchanged PIDs, HTTP `200`, and zero restarts.
