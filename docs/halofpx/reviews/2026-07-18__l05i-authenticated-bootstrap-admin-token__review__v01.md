# L05i authenticated bootstrap-admin token review v01

- Date: 2026-07-18
- Scope: canonical token wire, complete authentication input, public scope and
  registry snapshot binding, exact next-sequence comparison, authority
  admission, evidence, and offline isolation
- Final verdict: **ACCEPT**

## Independent adversarial review

The first local review found a body-schema mismatch: registered IDs were byte
strings and the predecessor was represented by a boolean plus zero digest.
Those were corrected to deterministic-CBOR text strings and one CBOR `null`.

Independent review then returned REVISE because the draft outer envelope still
placed the admin key tuple and algorithm outside the authenticated bytes. The
codec and independent encoder were changed to the exact CDDL: a two-field
envelope whose key zero is the nested four-field authentication-input map and
whose key one is the tag. HMAC now covers that complete exact map, and the KDF
also uses a text-string admin ID.

The re-review found one test-integrity defect: the noncanonical-map fixture
encoded the old field count, so rejection did not isolate noncanonical length
encoding. It now uses `b8 02` followed by the otherwise unchanged canonical
two-field contents. A static contract also forbids the former five-field
encoder. Final independent re-review returned ACCEPT.

## Verification

| Gate | Result |
|---|---|
| Clean Windows CPU/WebUI-off Release build including `llama-server` | Pass |
| Full HaloFPX-labeled CTests | Pass, 21/21 |
| Focused inherited CTests | Pass, 7/7 |
| Token process repetitions | Pass, 200/200 |
| Authority process repetitions | Pass, 200/200 |
| Independent golden-vector repetitions | Pass, 200/200 |
| Independent adversarial review | ACCEPT after two wire corrections and one fixture correction |

The independent vector fixes a 399-byte envelope, tag
`e979b8ae4864f29e3a962a883cd66c5f8e06f533d45808661eaef76e2c37963c`,
and envelope digest
`3e2b07d2321dafe84510c5b2062425c4154ceb8e0cf77a2f81dc74b393a5e320`.
Exact executable, source, build, checker, and vector hashes are retained in the
repeat receipt.

## Promotion boundary

L05i authenticates the external administrative token and compares its exact
sequence to an immutable protected-registry snapshot. It does not consume the
token or advance a durable high-water value. Credential custody, principal
authentication, issuance/revocation, protected registry/replay storage,
rollback protection, anchor-absence proof, create-if-absent execution,
filesystem durability, cache admission, server integration, and nodes remain
closed.
