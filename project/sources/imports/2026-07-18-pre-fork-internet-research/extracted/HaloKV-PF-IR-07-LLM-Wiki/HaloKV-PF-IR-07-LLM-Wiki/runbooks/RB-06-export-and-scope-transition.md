# RB-06 — Export and cryptographic scope transition

## Preconditions

* Source and destination authorities/scopes are explicit.
* Caller is authorized for source decrypt and destination write.
* Destination sharing class and publisher/tenant ownership are approved.
* Export codec and data-minimization policy are defined.

## Procedure

1. Load source policy/freshness state and authenticate source manifest/object.
2. Return miss/recompute for any invalid source cache entry; use authoritative source if export still required.
3. Decrypt only into bounded memory in the authorized export service.
4. Apply the approved semantic transformation and data-minimization rules.
5. Compute a destination-scope CID using destination `K_id` or public publication policy.
6. Allocate destination epoch/key/nonce and construct destination AAD.
7. AEAD-seal and publish a new destination manifest/generation.
8. Record source/destination authorities, principal, purpose, object counts, and policy hashes without logging plaintext/key material.
9. Zeroize temporary buffers to practical software limits and terminate isolated worker if required.

## Public/system promotion

Private content may enter a public/system prefix only through a distinct
publication authorization and review. Equality with other tenants' private CIDs
is not a publication signal. The publisher signs/authenticates an immutable
manifest and assumes rollback/revoke responsibility.

## Prohibited shortcuts

* Treating a copied ciphertext file as authorized destination content.
* Reusing source-private CID or source key at destination.
* Exporting after tag/AAD/freshness failure.
* Changing only path/metadata while retaining source ciphertext/AAD.

[CLAIM:PFIR07-C066][CLASS:SYNTHESIS][STATUS:REQUIRED][SRC:C030,C034,C052-C057]
