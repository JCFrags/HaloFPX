# L10d bounded authenticated exact-key catalog

Status: **implemented and target-qualified as a default-off laboratory
milestone; production persistence remains closed**

L10d makes more than one immutable exact prompt reusable without weakening the
L10c request boundary or the L09 generation-one authority. It adds no
deletion, eviction, replacement, prefix matching, shared reuse, distributed
restore, or production enablement.

## Authority and storage boundary

The Linux-only compile gate
`HALOFPX_CONTEXT_STORE_EXACT_KEY_CATALOG_CANARY` defaults to `OFF`, requires the
L10c gate, and alone admits runtime mode
`full-v1-exact-key-catalog-canary`. Feature-off builds expose neither the mode
nor the catalog implementation.

The configured capacity is fixed between two and eight positions. Every
position owns independent pre-provisioned data and anchor roots and composes
the unchanged `max_entries=1` generation-one authority. A position therefore
never replaces another prompt or advances a generation.

The catalog performs a bounded allow-list validation of the fixed configured
positions and their retained child-anchor directory identities, but selection
authority never comes from directory names or scan order. An authenticated
canonical reservation binds the private scope, exact-session lineage, closed
compatibility root, store and slot identity, inspected catalog and child-root identities,
producer, global plan, rank ownership and placement, topology epoch, and
predicted generation-one manifest. The final record repeats those facts and
the manifest actually returned by the child publisher. Both use a separately
derived catalog HMAC key. A filename, directory order, scan result, or client
field is never sufficient authority.

Publication holds a lifetime catalog writer lock and an adapter-local try-lock.
It publishes and synchronizes the reservation without replacement, invokes the
existing atomic generation-one publisher, then publishes and synchronizes the
final record without replacement. Only an authenticated coherent reservation
plus final record can select a child; the child must then pass its existing
anchor, manifest, compatibility, object, token, codec, topology, and live-state
checks.

Missing state is a cold miss. Malformed, unauthenticated, duplicated,
ambiguous, wrong-root, wrong-scope, incompatible, or corrupt catalog/child
state never becomes a hit. An interrupted reservation or pending record
occupies its fixed position and is retained for offline whole-root retirement.

The configured quota is conservatively partitioned after reserving the
fragment-rounded allocation upper bound for every catalog record, so capacity
does not multiply the global quota. Admission retains two record allocations
before reservation; a child on the shared catalog mount preserves the full
reserve plus the later final-record allocation, and final visibility rechecks
the full reserve plus that allocation. When no position is empty, an
unknown exact key returns `capacity-exhausted` during selection; the server
does not arm capture or publication, ordinary inference remains cold, and
existing entries remain readable.

## Focused Linux qualification

The nimo-2 Release CPU build used GCC 16.1.1 and the retained Stories 15M Q4_0
fixture. The integrated ON build and a separately configured L10d-OFF build
both completed. Eight focused ON tests passed: feature-off contract, private
scope, exact-session resolver and contract, exact-key runtime contract,
catalog behavior, independent exact-session golden, and inherited anchor-first
server selection. The L10d-OFF feature-surface contract also passed.

The catalog unit test proves two distinct entries publish and survive object
reconstruction, changed tokens and identities do not hit, wrong private scope
does not hit, final-record tampering produces a corrupt miss, child-object
corruption produces no state, catalog-root record transplantation, authenticated
pending duplicates, unknown files, and hard-linked records fail closed, and capacity refusal leaves the complete
filesystem tree unchanged while both prior entries still hit.

The process canary used disposable roots and five fresh server processes. Its
two distinct prompts each evaluated 11 tokens cold, then independently
evaluated one token after restart. Cold and restored continuations were exact
for each prompt. A third prompt at capacity evaluated 13 tokens cold and made
no data-root or anchor-root change. The two continuation SHA-256 values were
`d4befa4c08b0bdd9023bfa965064be3c3a8eda8804174fc74c286b2a66710860`
and `0125f512102ec7a97821aacb92fe3bec0e2fe2dd347cd6cb20f251f9238cb3b5`.

Raw evidence is retained on nimo-2 under the L10d evidence bundle named in the
qualification receipt. The known-good RPC worker remained active throughout;
the production coordinator, model, and deployment were not touched.

## Wiki review and boundary

ADR-0038 and the implementation agree with the canonical Wiki's requirements
to publish payloads before index visibility, authenticate and bound lengths,
isolate private principals, make corruption a miss, retain raw evidence, and
avoid trusting donor index formats. The milestone intentionally does not claim
the Wiki's still-open online retention/deletion, eviction, administrator,
distributed recovery, endurance, broad fault, or release-performance gates.

No donor implementation, GPL llama-ai code, CachyLLama transplant, new
dependency, WebUI, remote, reference clone, public artifact, model, or
production deployment changed.
