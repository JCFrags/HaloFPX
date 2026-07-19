# L05v predecessor authentication and launcher-pin matching

L05v is the second implementation slice under M63-01b. It authenticates the
exact sealed fd4 predecessor under the locked 32-byte secret supplied in fd3
and requires every launcher-pinned compatibility field to match. It does not
establish credential origin or freshness, open a registry root, acquire
`writer.lock`, mutate storage, or initialize anything.

## Admitted behavior

The L05u transport contract remains unchanged: the process masks blockable
signals, unshares `CLONE_FILES`, requires one task, admits exact distinct
sealed memfds 3 and 4, reads them at offset zero into locked fixed storage,
revalidates their identities and aliases, and wipes all sensitive storage
before unlock and unmap.

The initializer constructs a single active key record that borrows the locked
credential secret. A target-owned facts-only verifier authenticates the exact
fd4 protected-registry envelope and exposes only the public authenticated body,
key tuple, and key-continuity commitment. It does not return an envelope copy,
protected-carrier digest, private authority binding, secret, credential,
descriptor, or reusable witness.

Success requires exact agreement with the launcher receipt for the
registry-lab predecessor digest, registry ID, nonzero epoch, nonzero
authority-base scope commitment, nonzero policy commitment, bounded high
water, key ID, nonzero key generation, and nonzero key-continuity commitment.
Only the aggregate result
`predecessor_authenticated_pins_matched_no_root_access` is published.

## Failure, replay, and isolation

Malformed, truncated, appended, corrupted, incompatible, unauthenticated, or
pin-mismatched inputs become a miss-equivalent rejection with no root access or
mutation. Authentication and receipt-match audit facts are cleared on every
final failure, so no per-field partial-result oracle is exposed.

Authentication proves integrity under the caller-supplied credential and
consistency with the caller-supplied receipt. It does not prove who issued the
credential, protected origin, principal identity, current epoch, latestness,
revocation status beyond the supplied key record, monotonic high water,
rollback resistance, or production key custody. A fully matched old snapshot
can succeed again in a fresh process; the same-process one-shot flag is not a
freshness mechanism.

The initializer archive remains Linux-only, default-off, `STATIC
EXCLUDE_FROM_ALL`, uninstalled, unexported, and isolated from all product,
provider, cache, inference, HIP, Vulkan, RPC, and WebUI targets. Its two-object
closure admits exactly the two-stage registry-lab digest lineage and the
facts-only protected-registry verifier. Root/fixture names, `writer.lock`,
public credential parsing, carrier-returning verification, mutation syscalls,
and persistent writes remain forbidden.

## Promotion boundary

This milestone authorizes design and implementation of the separately gated
root/fixture admission and discard-only `writer.lock` anchor. It does not
authorize persistence, publication, cache hits, restore, inference
integration, or L14Q quantized-KV work.

Qualification and exact hashes are pinned in
`evidence/l05v-predecessor-authentication-receipt.json`; the independent review
is `reviews/2026-07-19__l05v-predecessor-authentication__review__v01.md`.
