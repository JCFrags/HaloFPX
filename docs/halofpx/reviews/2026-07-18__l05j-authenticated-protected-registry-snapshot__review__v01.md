# L05j authenticated protected-registry snapshot review v01

- Date: 2026-07-18
- Scope: registry wire and authentication, authority base/full/private scope,
  key separation, memory ownership, malformed input, evidence, rollback claims,
  and offline isolation
- Final verdict: **ACCEPT**

## Independent adversarial review

The first review returned REVISE because public base/full-scope helpers copied a
caller-controlled registered-ID length before checking its 129-byte backing
array. The shared bounded ID encoder now rejects zero, oversized, NUL, and non-
ASCII IDs before copying, and both public helpers reject oversized IDs for all
four key purposes.

The next review found a Windows-specific signed-`char` defect in the protected-
registry parser. Bytes `0x80..0xff` could compare as negative and pass a direct
`> 0x7f` check. Validation now casts each byte to `uint8_t`. Tests mutate a
canonical snapshot, recompute its exact HMAC independently through the public
primitive, and prove authenticated `0x80` and `0xff` IDs still reject as
structurally invalid.

Review also required the omitted anchor/admin tuple-collision case, forged-
status nonexposure, carrier ownership after source/key mutation, and proof that
a changed registry secret changes the private binding and authority snapshot.
All six tuple pairs and HMAC-effective-secret equivalences are now covered.
Final independent re-review returned ACCEPT.

## Verification

| Gate | Result |
|---|---|
| Clean Windows CPU/WebUI-off Release build including `llama-server` | Pass |
| Full HaloFPX-labeled CTests | Pass, 24/24 |
| Focused inherited CTests | Pass, 7/7 |
| Registry process repetitions | Pass, 200/200 |
| Authority process repetitions | Pass, 200/200 |
| Independent golden-vector repetitions | Pass, 200/200 |
| Independent adversarial review | ACCEPT after ID-safety and coverage corrections |

The independent vector fixes a 156-byte envelope, tag
`8f563d0a255e6b09a44b87be0867e84b4b8368d043e5f885fa86348662c509d4`,
envelope digest
`a7b731bccfdea83a4595d5257ffa34ef9248bb61499b40a37874895cff6bc1ec`,
and private binding
`f88b0080d50222b31e66879ecd4c14789279b9d15b786f6b22f32240d2ea5f7a`.
Exact hashes are retained in the repeat receipt.

One clean-build invocation was accidentally resumed concurrently after its
shell yielded, and the second runner observed a transient object-file
permission denial. The original process was allowed to finish; the build was
then serialized and completed successfully. This was orchestration noise, not
an attributed source failure.

## Promotion boundary

L05j authenticates an exact registry declaration and removes raw dynamic
registry scalars from authority input. It does not prove latestness, protected
origin, rollback resistance, durable sequence advancement, consumption,
operator identity, policy semantics, anchor absence, execution, filesystem
durability, cache admission, server integration, or node behavior.
