# Executable validation procedures

## Purpose and trust boundary

The included tools turn the integrity rules into repeatable checks. They validate the **outer persisted object** before any engine API would be called; they do not deserialize opaque llama state or authenticate a production principal.

The reference programs are offline/quiescent-file validators, not a drop-in hostile multi-process reader. They use ordinary path-based opens for portability. A production reader must hold one safely resolved descriptor, compare `fstat` identity/size, reject symlinks and mount/path escapes, and perform all parsing and hashing through that descriptor so an attacker or concurrent writer cannot substitute bytes between checks.

The suite demonstrates three separate facts:

1. structural corruption such as truncation, wrong magic, wrong version and inconsistent lengths is rejected;
2. a same-length bit flip in a CachyLLama payload remains structurally valid because the observed format has no body digest; and
3. the HaloFPX reference object rejects the same mutation by digest, while the keyed manifest HMAC rejects unauthorized pointer/key/metadata changes.

An internally authenticated but request-unbound entry is `CATALOG_ENTRY_VALID`. A fully caller-bound offline success is `IMPORT_CANDIDATE_VALID`. Neither is a public cache hit. The integrated cache may return `HIT_VERIFIED` only after namespace authorization and complete isolated engine import. Any failed or unavailable gate is `MISS_RECOMPUTE`.

## Requirements

- Python 3.10 or newer;
- POSIX shell for `run_all.sh`;
- no third-party Python packages.

## Run everything

```bash
cd validation
./run_all.sh
```

Expected output files:

```text
validation/results/unit-tests.txt
validation/results/halofpx-catalog-entry.json
validation/results/halofpx-manifest.json
validation/results/fault-injection.json
validation/results/fault-injection.md
validation/results/endurance-report.md
validation/results/validation-summary.json
```

The script exits nonzero on a violated expectation.

## Validator interface

```bash
python3 validate_cache.py --help
```

The global `--pretty` option appears before the subcommand.

### CachyLLama checkpoint

```bash
python3 validate_cache.py --pretty cachyllama-checkpoint \
  fixtures/cachyllama/valid/ckpt-1.bin \
  --expected-compat 0x1122334455667788
```

Checks:

- LP64/little-endian header availability;
- magic and v3 record version;
- filename ID agreement;
- token count at most 4,096 and no greater than total tokens;
- persisted segment sizes within configured caps;
- exact total file size;
- expected compatibility value.

The upstream implementation does not enforce all of these checks. The reference parser therefore shows a minimum bounded legacy parser, but a structurally valid legacy file returns `LEGACY_STRUCTURALLY_VALID_UNAUTHENTICATED` and remains ineligible for engine import because its payload has no checksum or authentication tag.

### CachyLLama index and system file

```bash
python3 validate_cache.py --pretty cachyllama-index \
  fixtures/cachyllama/valid/index.bin \
  --expected-compat 0x1122334455667788

SYSTEM="$(python3 -c 'import json; print(json.load(open("fixtures/fixture-values.json"))["cachyllama"]["system"])')"
python3 validate_cache.py --pretty cachyllama-system \
  "fixtures/$SYSTEM" \
  --expected-compat 0x1122334455667788
```

### HaloFPX object

```bash
OBJECT="$(python3 -c 'import json; print(json.load(open("fixtures/fixture-values.json"))["halofpx"]["object"])')"
COMPAT="$(python3 -c 'import json; print(json.load(open("fixtures/fixture-values.json"))["halofpx"]["compatibility_fingerprint_sha256"])')"

python3 validate_cache.py --pretty halofpx-object \
  "fixtures/$OBJECT" \
  --expected-compat "$COMPAT"
```

Checks:

- fixed header and supported version;
- exact object size and bounded arithmetic;
- canonical metadata encoding and digest;
- payload digest;
- segment table count, coverage and per-segment digest;
- object filename digest;
- compatibility, cache-key and prompt-root bindings when supplied;
- required target segment;
- declared codec/encryption support.

A valid standalone object returns `OBJECT_VALID`. This proves accidental-corruption integrity, not tenant authorization or keyed authenticity. The content digest becomes keyed-authenticated transitively only when a valid HMAC-protected manifest binds that digest.

### Keyed manifest and object binding

```bash
MANIFEST="$(python3 -c 'import json; print(json.load(open("fixtures/fixture-values.json"))["halofpx"]["manifest"])')"
OBJECT_ROOT="$(python3 -c 'import json; print(json.load(open("fixtures/fixture-values.json"))["halofpx"]["object_root"])')"
HMAC_KEY="$(python3 -c 'import json; print(json.load(open("fixtures/fixture-values.json"))["halofpx"]["manifest_hmac_key"])')"
NAMESPACE="$(python3 -c 'import json; print(json.load(open("fixtures/fixture-values.json"))["halofpx"]["namespace_id"])')"
COMPAT="$(python3 -c 'import json; print(json.load(open("fixtures/fixture-values.json"))["halofpx"]["compatibility_fingerprint_sha256"])')"
CACHE_KEY="$(python3 -c 'import json; print(json.load(open("fixtures/fixture-values.json"))["halofpx"]["cache_key_sha256"])')"
PROMPT_ROOT="$(python3 -c 'import json; print(json.load(open("fixtures/fixture-values.json"))["halofpx"]["prompt_root_sha256"])')"
ENGINE="$(python3 -c 'import json; print(json.load(open("fixtures/fixture-values.json"))["halofpx"]["engine_family"])')"

python3 validate_cache.py --pretty halofpx-manifest \
  "fixtures/$MANIFEST" \
  --object-root "fixtures/$OBJECT_ROOT" \
  --manifest-hmac-key-file "fixtures/$HMAC_KEY" \
  --expected-namespace "$NAMESPACE" \
  --expected-compat "$COMPAT" \
  --expected-cache-key "$CACHE_KEY" \
  --expected-prompt-root "$PROMPT_ROOT" \
  --expected-engine-family "$ENGINE"
```

The manifest validator checks canonical schema, fixed-length fields, generation, committed state, HMAC-SHA-256 catalog authentication, object path/digest/size, duplicated key/fingerprint/prompt binding, encryption policy and required segments. With all five current-request bindings supplied and matched, it returns `IMPORT_CANDIDATE_VALID`, with `eligible_for_engine_import=true` and `eligible_for_hit=false`. Omitting any of those bindings returns `CATALOG_ENTRY_VALID` with both eligibility flags false; that status proves catalog consistency only.

The generated HMAC key is public deterministic **test material**. A production implementation should obtain its catalog-authentication key through a KMS, protected descriptor or process secret; it should not place production keys under the cache root.

## Status and exit codes

| Code | Meaning |
|---:|---|
| 0 | The requested offline validation completed without a structural/integrity miss. Inspect `status`; this can be legacy structural validity, object validity, authenticated catalog-entry validity, request-bound import-candidate validity or scan completion. It is not proof of a public cache hit. |
| 10 | `MISS_RECOMPUTE` due to invalid, incompatible, unsupported, unauthenticated or unavailable state. |
| 2 | Command-line/usage error. |
| 1 | Unexpected validator failure/bug. |

Every JSON result includes `status`, `reason`, `path`, `eligible_for_engine_import`, `eligible_for_hit`, and bounded diagnostic metadata. Only an integrated engine adapter may set hit eligibility after a complete transactional import.

## Fault injection

```bash
python3 fault_inject.py --fixtures fixtures --results results
```

Cases include:

- truncation at header and payload boundaries;
- bad magic and unsupported version;
- length inflation and token-count overflow;
- metadata and payload mutation;
- missing catalog-authentication key and HMAC failure;
- manifest/object digest, compatibility and semantic-key mismatch;
- missing required segment.

The expected matrix is:

| Case | Legacy CachyLLama structural parser | HaloFPX validator |
|---|---|---|
| Truncate | Miss | Miss |
| Wrong magic/version | Miss | Miss |
| Corrupt length/token count | Miss | Miss |
| Same-length payload bit flip | **Structurally valid / untrusted** | **Miss: digest failure** |
| Manifest mutation without valid HMAC | N/A | **Miss: authentication failure** |
| Compatibility mismatch | Miss | Miss |
| Manifest/object alias | N/A | Miss |

The legacy payload blind spot is an expected negative result and is recorded explicitly; the suite never labels the corrupted checkpoint safe.

## Power-cut procedure for an implementation

The repository includes logical fault injection, not physical power-cut testing. A production HaloFPX implementation should additionally:

1. instrument every commit boundary from temporary creation through manifest directory sync;
2. run the writer in a VM or dedicated test machine with a kill/power-fault harness at each boundary;
3. remount/restart after each fault;
4. run a strict catalog/object scan;
5. assert the only public outcomes are old valid hit, new valid hit, or miss;
6. assert no partial state is installed and no sequential-ID collision occurs;
7. repeat under ENOSPC, injected EIO, concurrent writers/readers and eviction.

## Fuzzing targets

Recommended fuzz targets:

- fixed envelope and manifest parser with arbitrary bytes;
- length arithmetic near 0, configured maxima, `2^32`, `2^63`, and host `SIZE_MAX`;
- segment overlap, gaps, duplicate names and missing required roles;
- deeply nested or oversized JSON/CBOR;
- codec ratio/output limits;
- AEAD nonce/tag/key-ID and catalog-HMAC handling;
- compatibility canonicalization across languages;
- engine import adapter using a fresh isolated context.

A parser exception is a miss. A process crash, unbounded allocation, destination mutation, accepted corrupt payload, or accepted unauthenticated manifest is a test failure.

## Reproducing native layouts

The generated fixtures use the explicit struct formats documented in [Storage schemas](storage-schemas.md). To compare with an upstream build, compile a small C++ program at the pinned commit that prints `sizeof` and `offsetof` for every native struct. If any offset differs, use a platform-specific legacy parser and treat cross-platform files as incompatible.
