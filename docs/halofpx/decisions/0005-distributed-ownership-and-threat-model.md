# ADR-0005: distributed ownership and threat model

- Status: accepted for L02
- Date: 2026-07-17

## Distributed decision

Rank-local objects bind logical rank, exact ownership digest and ranges, global
plan digest, world size, placement/partition, generation, and epoch. Physical
host/device identity is included only when it changes state layout or backend
ABI; logical rank and plan identity are always required.

Each rank independently validates and stages its complete local component set.
`READY` carries only protocol version, cluster/policy epoch, checkpoint and
generation, logical rank/world size, manifest/plan/ownership digests, verified
count/status, coordinator attempt ID, and a fresh 256-bit commit nonce; it never
carries state payload. `READY`, `COMMIT`, and `ABORT` are accepted only over an
admitted mutually authenticated confidential/integrity-protected channel bound
to authorized coordinator and rank identities. Every message also has a
canonical full-message authenticator or equivalent transcript-bound signature
covering all fields plus channel/session binding. Missing authentication,
unknown/revoked identity or key, downgrade, duplicate rank/nonce, replay, or
authorization failure is a whole-generation miss and security event. The
current unproven RPC path remains lab-only/default-off and cannot authorize
publication.
The closed CDDL defines the authentication input/envelope and separate
`READY`, `COMMIT`, and `ABORT` bodies. The tag is
`HMAC-SHA-256(K_control, "halofpx.control-auth.v1\0" ||
DCBOR(control-auth-input-v1))`; its input includes key ID/algorithm/generation,
cluster and policy/topology epoch, attempt ID, channel/transcript binding,
message type, and body. Only the tag is excluded. `K_control` is derived from a
protected cluster key as `HMAC-SHA-256(K_cluster_master,
"halofpx.control-key.v1\0" || key_id_len:u16be || key_id ||
key_generation:u64be || cluster_id_len:u16be || cluster_id)`. Message type `0`
requires exactly `ready-body-v1`, `1` requires `commit-body-v1`, and `2`
requires `abort-body-v1`; any other pairing is malformed before authorization.
The coordinator can publish only the exact complete rank set. A missing,
corrupt, timed-out, wrong-rank, wrong-topology, wrong-generation, or stale-nonce
participant makes the whole restore a miss. Generations are never mixed.

Single-node fallback uses a separately compatible single-node checkpoint or
cold recomputation. Two rank blobs are never concatenated or relabeled. Normal
restore never transfers cache state across USB4; repair or migration is an
offline authenticated administrative workflow.

## Threat model

Persistent cache bytes, indexes, manifests, paths derived from stored data, and
legacy input are untrusted. The trusted computing base is the authenticated
server/policy layer, configured roots and keys, admitted parsers/codecs, runtime
binary and libraries, and protected replay anchor. An attacker may truncate,
flip, replay, duplicate, reorder, replace, link, or oversize cache material and
may attempt cross-principal discovery. Simultaneous compromise of the trusted
OS/service account and protected keys is outside this cache-integrity claim.

Validation is bounded and complete before live-context mutation. Authentication
precedes enumeration. Diagnostics, timing classes, and metrics must not expose
principal existence, tokens, templates, paths, or full identities. Quarantine
is non-authoritative and never becomes a fallback reader. Security ambiguity is
a cold miss; explicit administrative shutdown is the only cache-related fatal
mode.

## Required tests before enablement

Wrong rank/world/plan/epoch/nonce/identity/authenticator/channel binding,
missing peer, mixed generation, control-plane
payload overflow, single-node fallback, two-node failure/recovery, tenant
isolation, replay, and redacted-observability tests join the single-node format,
corruption, crash, upgrade, rollback, and old-binary coexistence matrix.
