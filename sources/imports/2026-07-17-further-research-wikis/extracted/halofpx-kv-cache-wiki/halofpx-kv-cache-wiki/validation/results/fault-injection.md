# Fault-injection results

**All expectations passed:** `True`

| Case | Expected | Actual | Import candidate | Hit eligible | Pass | Note |
|---|---|---|---:|---:|---:|---|
| `legacy_bad_magic` | MISS_RECOMPUTE | `MISS_RECOMPUTE / BAD_MAGIC` | False | False | True |  |
| `legacy_bad_version` | MISS_RECOMPUTE | `MISS_RECOMPUTE / UNSUPPORTED_VERSION` | False | False | True |  |
| `legacy_truncated_payload` | MISS_RECOMPUTE | `MISS_RECOMPUTE / LENGTH_MISMATCH` | False | False | True |  |
| `legacy_token_count_overflow` | MISS_RECOMPUTE | `MISS_RECOMPUTE / TOKEN_COUNT_OVERFLOW` | False | False | True |  |
| `legacy_length_inflated` | MISS_RECOMPUTE | `MISS_RECOMPUTE / OBJECT_TOO_LARGE` | False | False | True |  |
| `legacy_compatibility_mismatch` | MISS_RECOMPUTE | `MISS_RECOMPUTE / FINGERPRINT_MISMATCH` | False | False | True |  |
| `legacy_same_length_payload_bitflip` | LEGACY_STRUCTURALLY_VALID_UNAUTHENTICATED (not hit eligible) | `LEGACY_STRUCTURALLY_VALID_UNAUTHENTICATED / NO_PAYLOAD_INTEGRITY` | False | False | True | Expected demonstration: the observed outer format has no payload digest. |
| `halo_bad_magic` | MISS_RECOMPUTE | `MISS_RECOMPUTE / BAD_MAGIC` | False | False | True |  |
| `halo_bad_version` | MISS_RECOMPUTE | `MISS_RECOMPUTE / UNSUPPORTED_VERSION` | False | False | True |  |
| `halo_truncated_payload` | MISS_RECOMPUTE | `MISS_RECOMPUTE / LENGTH_MISMATCH` | False | False | True |  |
| `halo_metadata_bitflip` | MISS_RECOMPUTE | `MISS_RECOMPUTE / METADATA_DIGEST_MISMATCH` | False | False | True |  |
| `halo_same_length_payload_bitflip` | MISS_RECOMPUTE | `MISS_RECOMPUTE / PAYLOAD_DIGEST_MISMATCH` | False | False | True |  |
| `halo_length_inflated` | MISS_RECOMPUTE | `MISS_RECOMPUTE / LENGTH_MISMATCH` | False | False | True |  |
| `manifest_auth_key_unavailable` | MISS_RECOMPUTE | `MISS_RECOMPUTE / MANIFEST_AUTH_KEY_UNAVAILABLE` | False | False | True |  |
| `manifest_hmac_failure` | MISS_RECOMPUTE | `MISS_RECOMPUTE / MANIFEST_AUTH_FAILURE` | False | False | True |  |
| `manifest_object_alias` | MISS_RECOMPUTE | `MISS_RECOMPUTE / REFERENCED_OBJECT_INVALID` | False | False | True |  |
| `manifest_missing_required_segment` | MISS_RECOMPUTE | `MISS_RECOMPUTE / MISSING_REQUIRED_SEGMENT` | False | False | True |  |
| `manifest_cache_key_alias` | MISS_RECOMPUTE | `MISS_RECOMPUTE / REFERENCED_OBJECT_INVALID` | False | False | True |  |
