# L78 terminal result

Status: **NOT PROMOTED**

L78 consumed its single authorized replacement transition at source HEAD
`2f5252a6b5a870ec0550f8c156410723e66bf571`. The evidence-directory schema
correction passed focused tests and independent review, and the exact primary
artifact, rebuilt binaries, capacity, and production authority passed a fresh
read-only preflight.

The transition then refused before worker, canary, model, or RPC execution:

`L57 binary provenance mismatch: nimo-1`

The exact source-proven boundary is `child_environment()`. It classifies only
`halofpx.l48.fixture-manifest.v1` as the L48-composed environment family.
Although the controller validates `halofpx.l77.primary-manifest.v1` as the same
composed contract, it did not export the L48 provenance/component/semantic/
composition/response environment for L77.

The child directly observed the correct current worker provenance:

`schema=halofpx.l57.binary-provenance.v1|source_root=71d72d8f93709b92bea43b8f985bacfabc6fb1e424110822689c1915afbb2e62|build_id=5401deeeac66260401794dc1eff05888a6fee0c2a58895c0029d48edc2a77242|binary=rpc-server`

Because `HALOFPX_PROVENANCE_SOURCE_ROOT` and `HALOFPX_PROVENANCE_BUILD_ID`
were absent, the child compared that value with its retained L55 fallback and
refused correctly. The missing server publication journal is secondary: no
handler attempt occurred.

No token, capture, restore, distributed-state correctness, cache correctness,
or performance conclusion is supported. No retry was made.

Recovery completed worker-first then coordinator. Exact production units,
commands, listeners, `NRestarts=0`, and HTTP 200 reconciled. All manifest
disposable paths and channel-key paths were independently verified absent.

Smallest future correction, only with new Lead authority: classify exactly the
validated L77 schema as part of the existing L48-composed environment family in
`child_environment()`, with exact-schema and near-match focused tests. No
protocol, grammar, cache, model, or production change is indicated.

