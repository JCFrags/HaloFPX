# L05n synthetic protected-anchor bootstrap create and reconciliation

Status: accepted as the final permanently synthetic bootstrap transition after
independent adversarial review.

L05n consumes an L05m proof before any validation, models authoritative anchor
inspection and atomic generation-one create-if-absent, and requires exact full-
envelope readback, synchronization, and a closed terminal confirmation. A
pre-existing exact anchor returns no proof. Lost or malformed create responses
retain the sole material proof inside the backend and permit exactly one
phase-attributed fence-and-observe reconciliation.

Independent review found and closed incorrect recovery attribution for a pre-
existing exact anchor, undefined repeated-reconciliation authority, loss of the
pending material proof after create-linearized nominal errors, incomplete close
validation on non-recovery terminals, missing phase ownership, insufficient
state-machine qualification, and incorrect OOM classification. The final
implementation binds a base-wrapper-owned phase into every reconciliation
commitment and confirmation, validates fence and close on every definite path,
keeps 512 create attempts without eviction, makes reconciliation one-shot, and
maps pre-backend history allocation failure to resource exhaustion after source
invalidation and before any callback.

The fresh Windows CPU/WebUI-off Release build passed 33/33 HaloFPX CTests and
seven focused inherited tests. Authority/state-machine, static isolation, and
independent golden checks then passed 600/600 separate processes. The golden
oracle performs 145 mutations over all eight applicable one-NUL commitment
domains. Exact hashes are retained in
`evidence/l05n-bootstrap-anchor-repeat-receipt.json`.

L05n is excluded from all product targets and has no filesystem, path, server,
provider, cache-admission, or node API. Its proof is permanently synthetic and
cannot enter a concrete backend. Concrete work must introduce new proof types
and qualify the protected registry, material writer, and protected anchor on a
disposable Linux target before persistent writes can be considered.
