# ADR-0002: scope and authority

- Status: accepted for L02
- Date: 2026-07-17
- Resolves: `OPEN-SCOPE-01`

## Decision

The default persistent scope is private, authenticated, and per effective
principal. Authority comes only from the trusted server authentication and
policy layer; request-body identity is never authority. Authorization and
security-domain selection happen before lookup, enumeration, metrics, or any
existence-dependent response.

An opaque namespace identifier is derived as:

```text
namespace_id = HMAC-SHA-256(
  policy_key,
  "halofpx.namespace.v1\0" || DCBOR(namespace-preimage-v1))
```

The exact integer-key `namespace-preimage-v1` schema binds policy key ID,
authentication issuer, canonical opaque subject bytes, security domain, policy
version/epoch, private scope class, and compatibility root. All byte strings
are definite-length; registered IDs are ASCII without NUL. The subject comes
from the authentication layer, not request data. The output is exactly 64
lowercase hexadecimal characters. A missing/unknown key, unknown class,
noncanonical input, or canonicalizer collision/fault is a miss before
filesystem access and a security event.

The compatibility root binds exact model, adapters, template/system context,
and all other output-affecting fields in ADR-0001 and the closed schema. Raw
identity, prompt, or template text is never used as a path or exposed in cache
metadata. Filesystem roots are fixed by trusted configuration.

Anonymous persistent reads and writes are disabled. Cross-principal reuse,
content deduplication, implicit scope fallback, and shared caches are disabled.
A future operator-public scope must be separately rooted, explicitly enabled by
privileged policy, authenticated for publication, versioned, revocable, and
tested for disclosure; ordinary requests cannot publish into it.

Key rotation creates a new namespace generation. Old roots become read-disabled
unless an authenticated, offline, reviewed migration explicitly revalidates and
republishes them. There is no automatic directory scan or fallback to an older
key.

## Security result

Scope denial returns an opaque miss indistinguishable from absence and emits
only bounded, redacted operational evidence. Quotas and retention are enforced
per scope as well as globally before persistent writes can be enabled.
