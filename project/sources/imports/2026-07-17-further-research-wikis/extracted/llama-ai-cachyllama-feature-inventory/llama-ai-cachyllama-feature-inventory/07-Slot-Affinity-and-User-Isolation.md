# Slot affinity and user isolation

The pinned component introduces user-aware admission, slot affinity, and cache directories. These are useful scheduling hints, but not an authorization system. Several implementation gaps require redesign before multi-tenant use.

| Decision | Count |
|---|---:|
| RETAIN | 2 |
| REDESIGN | 6 |
| REJECT | 4 |

<a id="f-036"></a>
### F-036 — OpenAI request user hint

**Decision:** `REDESIGN` · **Portability:** `High` · **Maturity:** `M2` · **Confidence:** `High`

**Observed behavior.** Accepts llama_user_id in OpenAI-style request bodies and propagates it to task scheduling.

**Implementation.** `tools/server/server-task.cpp`, `tools/server/server-context.cpp`

**Dependencies.** JSON request parser; task metadata

**License.** MIT

**ROCmFPX overlap.** ROCmFPX has OpenAI-compatible endpoints but no verified authenticated tenant cache scope.

**Porting rationale.** Accept a user hint only behind a trusted gateway; bind an internal tenant principal from API-key/JWT context.

**Risks / caveats.** Client-supplied identity is spoofable.

**Evidence.** [E-081](20-Evidence-Index.md#e-081), [E-082](20-Evidence-Index.md#e-082), [E-087](20-Evidence-Index.md#e-087)

<a id="f-037"></a>
### F-037 — Anthropic metadata user hint

**Decision:** `RETAIN` · **Portability:** `Medium` · **Maturity:** `M2` · **Confidence:** `High`

**Observed behavior.** Maps metadata.user_id from Anthropic-style requests into the same scheduling identity field.

**Implementation.** `tools/server/server-task.cpp`, `tools/server/server-context.cpp`

**Dependencies.** Anthropic request converter; task metadata

**License.** MIT

**ROCmFPX overlap.** ROCmFPX may retain Anthropic compatibility from its llama.cpp base.

**Porting rationale.** Keep protocol parsing as a hint, then map through the same authenticated principal layer as OpenAI requests.

**Risks / caveats.** Protocol parity must not create different authorization rules.

**Evidence.** [E-081](20-Evidence-Index.md#e-081), [E-087](20-Evidence-Index.md#e-087)

<a id="f-038"></a>
### F-038 — User-ID validation

**Decision:** `RETAIN` · **Portability:** `High` · **Maturity:** `M2` · **Confidence:** `High`

**Observed behavior.** Validates length and allowed characters before using an identifier in scheduling or filesystem namespace selection.

**Implementation.** `tools/server/server-task.cpp`, `docs/development/user-isolation-design.md`

**Dependencies.** validation function; request parser

**License.** MIT

**ROCmFPX overlap.** ROCmFPX should validate any external label before logging or lookup.

**Porting rationale.** Retain strict syntax validation but store a keyed hash/opaque internal ID instead of the raw value.

**Risks / caveats.** Validation prevents traversal but not identity spoofing.

**Evidence.** [E-080](20-Evidence-Index.md#e-080), [E-081](20-Evidence-Index.md#e-081)

<a id="f-039"></a>
### F-039 — Per-user concurrent request cap

**Decision:** `REDESIGN` · **Portability:** `High` · **Maturity:** `M2` · **Confidence:** `High`

**Observed behavior.** Rejects or defers requests when a non-empty user identity reaches the configured in-flight limit.

**Implementation.** `common/common.h`, `common/arg.cpp`, `tools/server/server-context.cpp`

**Dependencies.** active-user counter; server slots; HTTP admission

**License.** MIT

**ROCmFPX overlap.** ROCmFPX has global slot limits but no assessed tenant quota.

**Porting rationale.** Implement one authoritative scheduler quota keyed by authenticated tenant, with atomic admission and release.

**Risks / caveats.** Pinned code duplicates checks at HTTP and allocator layers; races and accounting drift are possible.

**Evidence.** [E-022](20-Evidence-Index.md#e-022), [E-023](20-Evidence-Index.md#e-023), [E-082](20-Evidence-Index.md#e-082), [E-083](20-Evidence-Index.md#e-083), [E-085](20-Evidence-Index.md#e-085)

<a id="f-040"></a>
### F-040 — Anonymous-bucket concurrency policy

**Decision:** `REJECT` · **Portability:** `High` · **Maturity:** `M1` · **Confidence:** `High`

**Observed behavior.** Documentation describes a shared anonymous bucket, but the pinned code explicitly exempts empty user IDs from per-user accounting.

**Implementation.** `tools/server/server-context.cpp`, `tools/server/README.md`, `docs/development/user-isolation-design.md`

**Dependencies.** active-user counter; documentation

**License.** MIT

**ROCmFPX overlap.** ROCmFPX should not inherit the contradiction.

**Porting rationale.** Define an explicit unauthenticated/global policy; production multi-tenant deployments should require authentication.

**Risks / caveats.** Anonymous traffic can consume all global slots.

**Evidence.** [E-080](20-Evidence-Index.md#e-080), [E-084](20-Evidence-Index.md#e-084), [E-085](20-Evidence-Index.md#e-085)

<a id="f-041"></a>
### F-041 — Same-user slot affinity

**Decision:** `REDESIGN` · **Portability:** `High` · **Maturity:** `M2` · **Confidence:** `High`

**Observed behavior.** Prefers a least-recently-used slot previously associated with the same user to improve reuse.

**Implementation.** `tools/server/server-context.cpp`

**Dependencies.** slot user_id field; LRU timestamps

**License.** MIT

**ROCmFPX overlap.** ROCmFPX already chooses slots using prompt similarity and availability.

**Porting rationale.** Combine authorization, user affinity, and prefix score in a single eligibility/ranking pass; never select cross-tenant state.

**Risks / caveats.** Separate selection passes can accidentally prioritize prefix reuse before isolation.

**Evidence.** [E-082](20-Evidence-Index.md#e-082), [E-085](20-Evidence-Index.md#e-085)

<a id="f-042"></a>
### F-042 — User-scoped persistent cache directories

**Decision:** `REDESIGN` · **Portability:** `High` · **Maturity:** `M1` · **Confidence:** `High`

**Observed behavior.** Creates persistent cache wrappers and paths under a user namespace.

**Implementation.** `tools/server/server-context-page-manager.cpp`, `common/kv-ssd-cache.h`

**Dependencies.** validated identity; filesystem

**License.** MIT

**ROCmFPX overlap.** ROCmFPX has private process directories, not persistent tenant namespaces.

**Porting rationale.** Namespace by authenticated tenant hash and model fingerprint, with per-tenant owner/key policy.

**Risks / caveats.** Raw IDs in paths leak identity and allow spoofed cache access.

**Evidence.** [E-061](20-Evidence-Index.md#e-061), [E-080](20-Evidence-Index.md#e-080)

<a id="f-043"></a>
### F-043 — Per-user cache statistics

**Decision:** `REJECT` · **Portability:** `High` · **Maturity:** `M1` · **Confidence:** `High`

**Observed behavior.** The architecture has user cache maps, but pinned get_stats aggregation omits them.

**Implementation.** `tools/server/server-context-page-manager.cpp`

**Dependencies.** cache maps; statistics aggregation

**License.** MIT

**ROCmFPX overlap.** ROCmFPX has cache counters but no assessed per-tenant view.

**Porting rationale.** Do not port the omission; define aggregate and tenant-scoped metrics with cardinality controls.

**Risks / caveats.** Operational dashboards would underreport persistent usage.

**Evidence.** [E-060](20-Evidence-Index.md#e-060), [E-064](20-Evidence-Index.md#e-064)

<a id="f-044"></a>
### F-044 — Per-user tier aging on turn completion

**Decision:** `REJECT` · **Portability:** `High` · **Maturity:** `M1` · **Confidence:** `High`

**Observed behavior.** The pinned turn-completion path advances anonymous caches but omits user cache wrappers.

**Implementation.** `tools/server/server-context-page-manager.cpp`

**Dependencies.** turn counter; cache maps

**License.** MIT

**ROCmFPX overlap.** No direct target equivalent.

**Porting rationale.** Centralize all cache instances in one registry and iterate one lifecycle interface.

**Risks / caveats.** User entries can remain hot indefinitely or avoid intended eviction.

**Evidence.** [E-064](20-Evidence-Index.md#e-064)

<a id="f-045"></a>
### F-045 — Raw user identifiers in logs

**Decision:** `REJECT` · **Portability:** `High` · **Maturity:** `M1` · **Confidence:** `High`

**Observed behavior.** Logs scheduling/rate-limit decisions with the unredacted user identifier.

**Implementation.** `tools/server/server-context.cpp`

**Dependencies.** server logger

**License.** MIT

**ROCmFPX overlap.** ROCmFPX logs are shared operational output.

**Porting rationale.** Log a stable keyed pseudonym, not raw identity; keep raw values out of metrics labels.

**Risks / caveats.** PII/secrets leakage and high-cardinality observability.

**Evidence.** [E-086](20-Evidence-Index.md#e-086)

<a id="f-046"></a>
### F-046 — Identity as routing metadata, not authentication

**Decision:** `REDESIGN` · **Portability:** `High` · **Maturity:** `M1` · **Confidence:** `High`

**Observed behavior.** The user identifier is accepted from the request body and is not itself a credential or verified principal.

**Implementation.** `tools/server/server-task.cpp`, `tools/server/README.md`, `common/arg.cpp`

**Dependencies.** API key/TLS optionally elsewhere; request metadata

**License.** MIT

**ROCmFPX overlap.** ROCmFPX supports API keys/TLS but needs explicit identity binding.

**Porting rationale.** Resolve tenant identity in authenticated middleware and ignore/validate client hints according to gateway policy.

**Risks / caveats.** Using body metadata as a security boundary enables cache theft.

**Evidence.** [E-081](20-Evidence-Index.md#e-081), [E-087](20-Evidence-Index.md#e-087), [E-103](20-Evidence-Index.md#e-103)

<a id="f-047"></a>
### F-047 — Unified cache and slot ownership lifecycle

**Decision:** `REDESIGN` · **Portability:** `Medium` · **Maturity:** `M1` · **Confidence:** `High`

**Observed behavior.** Ownership state is currently distributed across task parsing, slot counters, slot user fields, and separate page-manager maps.

**Implementation.** `tools/server/server-task.cpp`, `tools/server/server-context.cpp`, `tools/server/server-context-page-manager.cpp`

**Dependencies.** scheduler; cache registry; request identity

**License.** MIT

**ROCmFPX overlap.** ROCmFPX has its own scheduler and cache engine.

**Porting rationale.** Introduce one TenantContext/CacheScope object passed through admission, selection, restore, save, metrics, and release.

**Risks / caveats.** Distributed accounting is difficult to audit and test.

**Evidence.** [E-060](20-Evidence-Index.md#e-060), [E-081](20-Evidence-Index.md#e-081), [E-082](20-Evidence-Index.md#e-082), [E-085](20-Evidence-Index.md#e-085)


