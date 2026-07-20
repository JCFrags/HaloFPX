# ADR-0028 generation-one protected-session canary review

Date: 2026-07-20

Verdict: **ACCEPT FOR RESTRICTED IMPLEMENTATION.**

The initial draft required revision in two places. First, re-reading an anchor
after an ambiguous create proved visibility but not directory durability.
Second, the draft did not freeze exact key derivations, canonical anchor paths,
wire identity, full-envelope equality, and product linkage boundaries.

The accepted contract now permits recovered success only after exact
authenticated re-read, a fresh successful anchor-parent synchronization, and
post-sync re-open plus exact-envelope revalidation. Conclusive absence leaves
unreachable material; any other outcome quarantines the lineage and preserves
cold inference.

The revised contract fixes all KDF domains and inputs, exact lowercase
namespace/session path derivation under a pinned root descriptor, and the
canonical ADR-0008 envelope as the wire object. Admission, retry, and
reconciliation compare every canonical byte. The product must use a
target-native canary-owned encoder/provider cross-checked against the offline
codec and must not link excluded L05 codec, coordinator, simulator, bootstrap,
registry, or synthetic backend targets.

ADR-0028 is therefore coherent as a build-time and runtime default-off,
disposable, generation-one canary. It does not bypass the missing full-v1
manifest encoder or qualified CAS/attempt-fencing backend, and it does not
close ADR-0004, L08, production persistence, generation advancement, or
power-loss qualification.
