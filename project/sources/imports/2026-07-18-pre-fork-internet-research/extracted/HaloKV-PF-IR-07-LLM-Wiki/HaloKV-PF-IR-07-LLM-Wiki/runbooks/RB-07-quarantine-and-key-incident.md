# RB-07 — Quarantine and key/object incident

## Immediate actions

1. Stop serving and writes for the smallest safely identifiable affected scope.
2. Preserve caller-facing behavior as `MISS_RECOMPUTE`; do not expose failure class.
3. Disable affected unwrap/derive permissions and mark key/epoch compromised when indicated.
4. Isolate suspect ciphertext, manifests, allocator records, and lower-layer metadata in non-serving quarantine.
5. Record hashes, time, rank, scope, key IDs, detection reason, and collector; never copy plaintext keys into evidence.
6. Drain processes/files/mappings through RB-08 when key exposure is plausible.
7. Establish new epoch/allocator identity; invalidate old generation and recompute.

## Investigation questions

* Was the failure random corruption, deliberate substitution, wrong principal/key mapping, replay, format bug, or nonce reuse?
* Which ranks/tenants/projects/prefixes shared the key, identifier index, backup, or publisher?
* Did any detailed error, timing, or index behavior cross tenant boundaries?
* Which backups, snapshots, headers, escrow, exports, dumps, or peers contain affected data/key paths?
* Is the authoritative recomputation source trustworthy?
* Does the event invalidate a prior deletion/CE/sanitization statement?

## Recovery and closure

Release no quarantined object directly. Authenticate and re-encrypt into a new
context or recompute. Close only after key/policy revocation, manifest floors,
copy inventory, root cause, customer/publisher communications, and residual-risk
updates are complete.

[CLAIM:PFIR07-C058][CLASS:SYNTHESIS][STATUS:REQUIRED][SRC:C003,C016,C030,C034]

[CLAIM:PFIR07-C070][CLASS:SYNTHESIS][STATUS:REQUIRED][SRC:C031-C033,C040,C058-C065]
