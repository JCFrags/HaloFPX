# Integrity invariants and miss contract

<span class="badge recommended">HALOFPX RECOMMENDATION</span>

These invariants are normative. A conforming implementation must make them testable at the public cache API.

## Hit predicate

```text
verified_hit :=
    namespace_authorized
 && manifest_schema_supported
 && object_schema_supported
 && all_integer_arithmetic_bounded
 && exact_file_lengths
 && catalog_authentication_verifies
 && manifest_key_matches_request
 && compatibility_fingerprint_matches
 && prompt_root_and_boundary_match
 && object_digest_matches_name_and_manifest
 && metadata_digest_matches
 && required_segment_set_complete
 && every_required_segment_integrity_check_passes
 && decompression_is_bounded_and_successful
 && engine_import_is_complete
 && generation_is_not_rolled_back
```

If any term is false, missing, unknown, timed out, or cannot be evaluated, return `MISS_RECOMPUTE`.

## Normative invariants

| ID | Invariant | Enforcement point |
|---|---|---|
| INV-01 | Authorization precedes cache existence lookup | Namespace resolver |
| INV-02 | Persisted lengths never drive allocation before checked addition, upper bounds and exact file-size comparison | Envelope parser |
| INV-03 | Format magic and major version are exact; unknown versions are misses | Manifest/object parser |
| INV-04 | Compatibility is a cryptographic digest of a canonical complete execution manifest | Key builder and reader |
| INV-05 | Prompt identity uses exact token/media/adapter inputs and parent chaining | Prefix key builder |
| INV-06 | Only complete engine-defined reuse units are addressable | Writer and match logic |
| INV-07 | An object becomes reachable only after bytes, digests and durability checks succeed | Commit protocol |
| INV-08 | Published objects are immutable | Object store |
| INV-09 | Every required segment has a verified digest; encrypted segments additionally pass AEAD | Integrity validator |
| INV-10 | The manifest/object pointer is keyed-authenticated by HMAC/signature or an equivalent authenticated catalog; unkeyed digests alone are never treated as writer authenticity | Manifest catalog |
| INV-11 | A reader validates through an open descriptor, not repeated path lookups | Reader |
| INV-12 | Engine import is isolated/transactional; the live destination remains unchanged on failure | Engine adapter |
| INV-13 | The first invalid/missing chunk terminates reusable prefix; no suffix reuse | Prefix matcher |
| INV-14 | Recurrent/global state is all-or-nothing unless composability is formally declared | Schema and adapter |
| INV-15 | Catalog generations are monotonic or rollback-authenticated | Manifest catalog |
| INV-16 | Multi-writer publication uses single-flight plus CAS/no-replace | Coordinator/catalog |
| INV-17 | Eviction cannot delete bytes still being read; logical reachability changes first | Evictor/GC |
| INV-18 | Unsupported migration never silently coerces bytes | Migrator |
| INV-19 | Encryption-key absence or AEAD failure is a miss, not plaintext fallback | Key manager/reader |
| INV-20 | Logs and paths do not expose raw principal IDs or prompt material | All components |
| INV-21 | Every invalid-state reason maps to a stable reason code and `MISS_RECOMPUTE` | Public API |
| INV-22 | Recompute uses trusted current inputs and cannot consume partially decoded cache data | Scheduler/engine adapter |

## Invalid-state decision table

| Validation result | Public result | Destination context | Object handling |
|---|---|---|---|
| Entry absent | `MISS_RECOMPUTE(NO_ENTRY)` | Unchanged | None |
| Namespace unauthorized | `MISS_RECOMPUTE(UNAUTHORIZED_NAMESPACE)` or policy-denied response | Unchanged | Do not reveal existence |
| Unsupported schema/version | `MISS_RECOMPUTE(UNSUPPORTED_VERSION)` | Unchanged | Retain for migrator or quarantine |
| Header/length bound failure | `MISS_RECOMPUTE(HEADER_BOUNDS/LENGTH_MISMATCH)` | Unchanged | Quarantine/delete after policy |
| Compatibility mismatch | `MISS_RECOMPUTE(FINGERPRINT_MISMATCH)` | Unchanged | Usually retain for other compatible process |
| Prompt/key mismatch | `MISS_RECOMPUTE(PROMPT_MISMATCH)` | Unchanged | Alert if manifest alias suspected |
| Digest/checksum mismatch | `MISS_RECOMPUTE(INTEGRITY_FAILURE)` | Unchanged | Quarantine and alert |
| Catalog key missing / manifest HMAC or signature fails | `MISS_RECOMPUTE(KEY_UNAVAILABLE/CATALOG_AUTH_FAILURE)` | Unchanged | Do not follow object pointer; alert on tag failure |
| Missing key / AEAD failure | `MISS_RECOMPUTE(KEY_UNAVAILABLE/AEAD_FAILURE)` | Unchanged | Retain or quarantine; never plaintext fallback |
| Decompression violation | `MISS_RECOMPUTE(DECOMPRESSION_FAILURE)` | Unchanged | Quarantine |
| Required segment missing | `MISS_RECOMPUTE(MISSING_SEGMENT)` | Unchanged | Quarantine manifest/object |
| Engine import rejects | `MISS_RECOMPUTE(ENGINE_IMPORT_REJECTED)` | Unchanged | Quarantine or mark incompatible |
| Stale/rolled-back generation | `MISS_RECOMPUTE(ROLLBACK_DETECTED)` | Unchanged | Alert; require catalog repair |
| Optional draft segment invalid, target valid, policy allows catch-up | `HIT_VERIFIED(target)` plus `OPTIONAL_SEGMENT_MISS` | Target committed only after full target import | Quarantine optional segment/object generation or regenerate draft |

## Prohibited behaviors

A conforming implementation must not:

- treat a file's existence as a hit;
- use a non-cryptographic hash as payload integrity;
- treat unkeyed SHA-256 fields as proof that an authorized writer selected an object;
- install a verified prefix and then continue with unverified suffix bytes;
- catch a parser error and return a shorter state as success;
- fall back to a different tenant namespace;
- allocate gigabytes from an unchecked persisted integer;
- decrypt without authenticating metadata and ciphertext;
- accept an old state merely because tensor sizes happen to match;
- mutate a live context while validation is still in progress;
- silently reinterpret unknown versions;
- overwrite the last good manifest before the replacement is durable;
- log token prefixes to diagnose corruption.

## API sketch

```text
LookupResult lookup(CacheRequest req) {
  try {
    ns = authorize_and_resolve_namespace(req.principal, req.sharing_policy)
    key = derive_key(req.trusted_inputs)
    catalog_key = resolve_catalog_auth_key(ns)
    manifest = read_and_authenticate_manifest(ns, key, catalog_key)
    object = open_and_validate_object(manifest)
    staged = engine.import_isolated(object.verified_segments)
    engine.commit(staged)
    return HIT_VERIFIED(object.verified_prefix_tokens)
  } catch (CacheValidationError e) {
    record_reason(e.code)
    quarantine_if_required(e)
    return MISS_RECOMPUTE(e.code)
  }
}
```

Operational exceptions such as I/O timeout and ENOSPC are validation failures from the cache's perspective. They may affect availability metrics, but they never justify unsafe reuse.
