# L74 server-recorder ownership and Stories15M result

Status: **NOT PROMOTED**

L74 replaced the server's misuse of the client preexecute grammar with an
explicit role-specific server grammar. The client grammar table remains
unchanged. Exact server productions contain only authenticated admission
accept/refuse, physical prepare, verified execute-intent consumption, backend
result, actual response publication, and close/abort facts. Every published
server record binds a common admission object, expected-admission digest,
execution sequence, split UID/ordinal/backend, graph digest, connection and
allocation epochs, and explicit verified/absent receipt state.

The initial source review found and blocked one provenance defect before
runtime: unauthenticated request bytes could populate the receipt field. The
corrected source records `ABSENT` with a zero receipt before verified intent,
and captures the authenticated request tag/digest only after full handler
validation, immediately before atomic consumption. Final focused source review
passed with no P1/P2. Required Linux targets, structural negatives, real
no-model success/refusal fixtures, publication, and feature-off inertness
passed.

Exactly one feature-on Stories15M request was consumed. It returned token
`29916` and output `x` (`78` hex), exactly matching the retained L68
feature-off control. Client terminal authority was accepted. Mutable
reconciliation/status was `1`, census `49`, receipt
`d141b57762bd09280046e42c35d53d87c65d2ddd15a1b992ee9a50de5184c209`.
Server prepare/execute and client graph receipt agree on sequence `1`, split
UID `27`, and graph digest
`03630ca6d2e58012dea774437b6733a373e3a18344ae074e1f80c24e68659f86`.

The terminal review nevertheless rejected promotion with one
evidence-completeness P2. The disposable controller did not harvest the exact
model attempt's immutable server terminal file before cleanup. Because the
response reaches the client before server terminal publication, client success
and absence of a refusal log do not prove durable accepted server
terminalization. No retry was attempted.

Exact build identity:

- source root:
  `c895f70de194be7e236b5f0bfb911be7a6b496e8a18c16fdc41b3e79845894ce`
- build ID:
  `30f9693e8634154f2a55ac8ba9ca93f18f3411fef2f4568733786d10f0cc5d72`
- worker SHA-256:
  `390ffd854606371adbaa008861c5b7cf6ebb9a357b306fd98f9680327fa93880`
- canary SHA-256:
  `f83dcfcf7b0d454588a01c839476abc9c3424b18471c5d88f7cee17ec73a4555`

Cleanup removed transient units, disposable paths, keys, and staged source
roots. Production preflight/final snapshots are byte-identical SHA-256
`511f05d2b638277e19ae7af44573eb9e244d829458f24cdefc46e3f88ffd6ded`,
with unchanged PIDs, HTTP `200`, and zero restarts.
