# ADR-0027: default-off direct-session persistent canary

Status: accepted only for the restricted L08a disposable laboratory canary.
L08 promotion, production persistence, automatic discovery, shared reuse, and
distributed restore remain closed.

## Context

The accepted v03 plan requires a trusted private scope, an explicitly admitted
state codec, and a target-owned transactional format before a server can
produce a persistent hit. The inherited `--cache-disk` path is per-process and
ephemeral, while the L05 publication laboratories deliberately do not expose a
server/provider edge. Reusing either as an implicit cross-restart hit would
bypass the accepted state, scope, and storage contracts.

The project owner authorized the three previously open L02 decisions and asked
for the smallest usable persistent canary before broadening the test matrix.

## Decision

HaloFPX admits one explicit Linux-only direct-session canary with two gates:

1. `HALOFPX_CONTEXT_STORE_CANARY=ON` must be selected at build time. It is
   `OFF` by default, and an off build contains no canary options or product
   linkage.
2. The operator must explicitly select
   `--halofpx-context-store-mode direct-rw` and provide an already-created
   owner-only root, an exact 32-byte owner-only key file, a nonzero quota, a
   reserve, an entry limit, API keys, and a lowercase 256-bit compatibility
   root.

The only API operations are explicit authenticated slot actions:

- `halofpx-publish` publishes the current idle slot under an opaque 256-bit
  session ID;
- `halofpx-restore` restores that exact ID into an empty idle slot.

There is no enumeration, fuzzy/prefix lookup, automatic lookup, shared scope,
anonymous access, legacy-format trust, or fallback to a different object.

## Trusted scope

The authentication-layer API key is revalidated at the route and immediately
mapped through the exact ADR-0002 domain-separated HMAC into a private
namespace. Its canonical DCBOR preimage binds policy-key ID, authentication
issuer, principal bytes, registered security-domain ID, policy epoch, private
scope class, and the compatibility root. The raw
principal is not placed in a task, path, response, manifest, or log and the
handler copy is wiped after derivation. A second admitted API principal resolves
to a different namespace and therefore receives a miss for the same session
ID. Cross-principal reuse is unavailable.

The operator key derives independent scope and store subkeys. The raw key file
must be a regular file owned by the effective UID, mode `0600`, link count one,
exactly 32 bytes, and opened with `O_NOFOLLOW`. Scope and store keys are wiped
at their lifetime boundaries.

## Admitted state profile

Codec `transformer-sequence-v1` admits only:

- target sequence state;
- transformer architecture;
- world size one, rank zero;
- exact compatibility identity and exact token sequence;
- greedy memoryless sampling; and
- one bounded target context.

It rejects recurrent or hybrid architectures, draft/speculative/MTP state,
multimodal state, adapters, grammar, tools, sampler state, incomplete identity,
empty tokens, excess tokens/state, or a mismatched profile. Publication also
requires the preceding slot task to satisfy the admitted greedy profile.

A failed lookup or prevalidation performs no llama-state restore. If the
underlying llama restore reports failure after it may have begun mutation, the
destination slot is cleared and cold recomputation remains authoritative.

## Compatibility authority

The canary treats the operator-supplied compatibility root as an authenticated
configuration authority; it does not claim that an arbitrary operator digest
is complete. Qualification computes that root from the exact model bytes,
server and dynamically loaded runtime-library hashes, backend, KV types,
context, topology, platform, API/template/tool/grammar mode, adapters, and
sampling profile.

Production or automatic enablement remains blocked until the engine owns a
canonical compatibility builder for every admitted backend and output-affecting
state component. A missing, stale, ambiguous, or incomplete authority root is
an operational configuration error, never permission to widen matching.

## Storage and publication

The target-owned `HFPXLD01` direct-store laboratory format uses immutable scope/session
directories containing exactly `manifest`, `tokens`, and `state`. The fixed
manifest is versioned, exact-sized, canonically big-endian, HMAC-SHA256
authenticated, and binds exact scope, session, compatibility, lengths, token
count, and SHA-256 payload digests.

The provider:

- pins root owner, device, mount ID, mode, and directory descriptors;
- rejects symlinks, extra layout entries, wrong types/modes/links, truncation,
  corruption, wrong compatibility, and malformed lengths;
- holds one nonblocking writer lock for the provider lifetime;
- enforces startup accounting, quota, reserve, and entry bounds;
- creates and synchronizes immutable staging files and the staging directory;
- publishes with Linux `renameat2(RENAME_NOREPLACE)`; and
- synchronizes the destination scope directory before reporting `published`.

An equal retry after the successful no-replace rename reports
`already-exists`; an unequal retry reports `conflict` after authenticating and
comparing the existing object. The object is never overwritten.

This format deliberately does **not** claim the trusted v1 publication contract
from ADR-0003/0004. It has no separately protected anchor, generation, replay
high-water mark, or L05 reconciliation carrier. A power loss after rename but
before destination-directory synchronization can leave filesystem-dependent
visibility, and a previously copied authenticated object can be replayed at the
same scope/session path. Consequently, this canary is allowed only in a private,
owner-controlled, disposable qualification root with opaque single-use session
IDs. Its observed restart result is a laboratory hit, not a production-trusted
hit and not evidence that L08 is complete.

## Consequences and deferred work

The first canary demonstrates useful state restoration across a process restart
but remains manual,
private, bounded, and default-off. It does not change the deployed services,
the inherited prompt cache, ROCmFPX/TurboQuant/MTP/RPC/HIP/Vulkan routing, or
the feature-off performance path.

Required before L08 promotion: integrate the protected anchor/generation and
replay/failure reconciliation authority already frozen in ADR-0004 through
ADR-0026, then qualify its crash boundaries.

Deferred until a later risk-backed milestone: automatic prompt matching,
retention/eviction administration, abandoned-staging cleanup, production key
custody, canonical in-engine compatibility construction, broader codecs,
samplers, grammar/tools, adapters, multimodal/recurrent state, two-node rank
ownership, exhaustive crash points, and production enablement.

No donor implementation was used. The implementation is target-native MIT
engine code and introduces no GPL llama-ai code, CachyLLama transplant,
dependency, remote, WebUI, or release surface.
