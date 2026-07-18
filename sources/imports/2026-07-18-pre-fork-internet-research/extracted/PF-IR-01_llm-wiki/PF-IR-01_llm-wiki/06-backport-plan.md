# Required backport and policy plan

## P0 candidates

| Label | Priority | Candidate | Exact source reference | Rationale | Acceptance |
|---|---|---|---|---|---|
| [RECOMMENDATION] | P0 | Remove RPC from the standard release workflow | Downstream workflow change; reverse the attack-surface effect represented by upstream `1f67436c5ee6f4c99e71a8518bdfc214c27ce934` | The standard artifact currently includes an unauthenticated raw TCP service contrary to upstream untrusted-network guidance. | No `-DGGML_RPC=ON`; no `rpc-server`; no RPC backend registration/symbols in standard artifact. |
| [RECOMMENDATION] | P0 | Restore slots-off default | Upstream server security control `458367a90606448a9c0262b276947c9e536086e0` | Fork default `endpoint_slots=true` expands unauthenticated administrative/state surface when keys are empty. | Default false; explicit CLI opt-in; authenticated route test. |
| [RECOMMENDATION] | P0 | Fail non-loopback HTTP startup closed | Downstream policy patch | Loopback is a default, not an invariant; empty API-key set disables auth. | Startup refuses non-loopback without approved auth mode/explicit exception. |
| [RECOMMENDATION] | P0 | Preserve corrected RPC pointer handling on any lab-only branch | `ba38f3becce7d1283585c73d796eb47d72bbbd30` | Emergency fix alone breaks legitimate CPU-buffer graphs and is not the final semantics. | Source sentinel and local unit test pass. |
| [RECOMMENDATION] | P0 | Add source/build sentinels to CI | `tests/static_sentinels.py` in this pack | Prevents accidental reintroduction when merging large upstream/fork deltas. | All required sentinels pass; prohibited release flags fail the job. |
| [RECOMMENDATION] | P0 | Emit signed build provenance | Downstream release engineering | Source equivalence is not binary proof. | Artifact SHA-256, source SHA, submodule SHA, build flags, compiler, libraries, SBOM, and signature published together. |

## Source-fix disposition

| Label | Family | Candidate status | Backport decision |
|---|---|---|---|
| [MEASURED] | Current GGUF advisories | Material guards present | No source backport identified. |
| [MEASURED] | Current vocabulary advisories | Material guards present | No source backport identified. |
| [MEASURED] | 2024 RPC GET/SET/type advisories | Material guards present | No source backport; deny build/exposure. |
| [MEASURED] | 2026 GRAPH_COMPUTE advisory | Corrected semantics present | No source backport; official no-patched-version caveat remains. |
| [MEASURED] | Negative n_discard advisory | Clamp present | No source backport; launcher/config is not a substitute for API validation. |
| [MEASURED] | Captured cpp-httplib advisories | Candidate vendors 0.47.0 | No dependency bump required for captured advisories; preserve version/blob lock. |

## P1 hardening

| Label | Candidate | Reason |
|---|---|---|
| [RECOMMENDATION] | Put any networked HTTP deployment behind an authenticated, rate-limited reverse proxy or service mesh | The embedded server is not upstream-supported as an untrusted-network security boundary. |
| [RECOMMENDATION] | Set explicit request/header/body/time/concurrency limits | Version fixes remove known bugs but do not make resource-exhaustion risk disappear. |
| [RECOMMENDATION] | Run conversion and model inspection in a separate unprivileged worker | GGUF is a complex untrusted binary format and offline tools often allocate data. |
| [RECOMMENDATION] | Mount model inputs read-only and cache/state outputs on a dedicated owner-only filesystem | Reduces same-UID, symlink, and replacement risk. |
| [RECOMMENDATION] | Refresh official advisories and dependency versions at every release candidate | The present capture expires as upstream changes. |