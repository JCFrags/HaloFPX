# Security considerations

## Data classification

KV and recurrent-state tensors are derived from prompts, system instructions, retrieved documents, media embeddings and model internals. Even when direct text recovery is imperfect, persisted state should be classified at least as sensitive as the source request. Cache metadata also reveals prompt length, model use, timing, tenant activity and reuse relationships.

## Threat model

Consider:

- accidental truncation, bit rot and filesystem faults;
- a malicious or compromised tenant attempting cache probing or poisoning;
- another local service account able to create, replace or read files in a shared path;
- a stale backup or snapshot causing rollback;
- operator mistakes mixing model versions, adapters or rank layouts;
- a process crash during any write/eviction/migration step;
- theft of an SSD or filesystem snapshot;
- denial of service through huge persisted lengths, decompression bombs, namespace flooding or eviction contention.

A fully privileged attacker controlling the inference process can generally access plaintext state and keys; application storage controls do not solve that endpoint compromise.

## Tenant isolation

Observed unkeyed FNV namespaces are not sufficient security boundaries. HaloFPX should:

- derive namespace IDs with HMAC under a service secret;
- bind manifest access to an authenticated principal/sharing policy;
- perform authorization before existence lookup;
- use separate quotas and preferably separate data-encryption keys per tenant;
- disallow automatic cross-tenant continuation/fuzzy matching;
- make public system-prefix sharing an administrator-approved policy with immutable content and exact model/template fingerprints;
- normalize or otherwise mitigate timing differences for unauthorized/negative probes when threat analysis requires it.

## Cache probing and timing leakage

An attacker can infer that a victim used a prefix if matching requests become faster. vLLM's cache salt is a useful key-domain isolation technique, but a salt supplied by an untrusted client is not authorization. HaloFPX should derive or authorize the sharing salt server-side and include it in both namespace and prompt-root domains.

Metrics and error responses should not distinguish "exists but unauthorized" from "absent" to an unauthorized caller.

## Poisoning and alias attacks

Attack paths include:

- replacing a file under a predictable sequential ID;
- creating a same-key object with incompatible model bytes;
- exploiting a non-cryptographic hash collision;
- changing metadata while retaining payload length;
- rollback to an older but valid object;
- symlink/hardlink substitution;
- racing an evictor or writer.

Mitigations:

- SHA-256 content addressing plus full semantic key/fingerprint binding;
- immutable no-replace objects;
- a mandatory keyed-authenticated catalog pointer (the reference profile uses HMAC-SHA-256 over the canonical manifest);
- AEAD for encrypted segments, with metadata bound as AAD;
- generationed manifests with CAS and key rotation or signatures where offline verification is required;
- directory-FD, no-follow path resolution;
- process-shared leases and bounded single-flight;
- strict parser caps and canonical encodings;
- rollback counter in a transactional catalog or signed checkpoint stream where rollback matters.


## Integrity versus authenticity

SHA-256 object, metadata and segment digests detect accidental truncation and corruption. They do **not** stop an attacker who can rewrite both bytes and their digests. HaloFPX therefore separates two controls:

1. the content-addressed object proves internal consistency; and
2. a manifest HMAC/signature or authenticated transactional catalog proves that an authorized writer selected that exact object for the semantic key and generation.

The reference schema uses HMAC-SHA-256 and returns a miss when the key is unavailable or the tag fails. A service may substitute an authenticated database or digital signature only if it preserves the same canonical binding and fail-closed behavior. Namespace authorization still occurs before key selection and lookup.

## Encryption possibilities

### Filesystem/block encryption

LUKS/dm-crypt, encrypted volumes or filesystem-native encryption can protect a powered-off/stolen device with minimal application changes. They do not provide per-object/catalog authenticity, tenant-specific cryptographic deletion, or protection from another process with mounted-volume access. They should be considered a baseline, not a replacement for cache-object authentication.

### Application-level AEAD

Recommended modes:

- **AES-256-GCM** where a vetted library and hardware acceleration are available;
- **XChaCha20-Poly1305** where a vetted library supports its larger nonce and software performance is preferred.

Requirements:

- unique nonce per encryption key; never derive a GCM nonce solely from a semantic cache key unless uniqueness is formally guaranteed;
- envelope encryption: random per-object or per-segment DEK, wrapped by a tenant KEK/KMS key;
- key ID and algorithm in authenticated metadata;
- catalog HMAC/signature key separation from data-encryption keys;
- AAD commits to compatibility, prompt root, segment role, lengths, codec and namespace policy;
- decrypt only after ciphertext length/digest bounds; decompress only after AEAD success;
- key absence or tag failure returns a miss;
- key rotation through DEK rewrap where possible, or lazy copy-on-read re-encryption to a new immutable object;
- no home-grown cryptography.

### Convergent encryption warning

Encrypting identical plaintext to identical ciphertext enables global deduplication but leaks equality across tenants and is vulnerable to confirmation attacks on guessable content. Do not use convergent encryption for private prompts without a deliberate, documented threat-model exception.

## Secure deletion and SSD behavior

Overwriting or unlinking a cache file does not reliably erase its NAND pages because SSD firmware performs wear leveling, garbage collection, remapping and overprovisioning. Recommended deletion hierarchy:

1. expire/remove the manifest so data is no longer reachable;
2. unlink the object and synchronize directory metadata;
3. for strong tenant erasure, destroy/rotate the tenant or object encryption key (crypto erase);
4. use device sanitize/secure erase only through an approved decommissioning procedure.

Do not claim byte-level secure deletion from repeated file overwrites.

## Filesystem and service hardening

- dedicated service account and cache mount;
- directories `0700`, files `0600`, restrictive umask;
- no execution from cache mount; consider `nodev,nosuid,noexec`;
- fixed root supplied by configuration, not request;
- reject symlinks, devices, FIFOs and unexpected link counts;
- impose object/metadata/segment/token/decompression caps;
- reserve free space and fail cache writes before inference availability is threatened;
- isolate quarantine and temp areas;
- scrub environment/config logs for KMS tokens and tenant identifiers;
- rate-limit fills and failed integrity probes.

## Logging and privacy

Allowed log fields:

- truncated object digest;
- opaque namespace handle or bucketed tenant metric;
- reason code;
- byte counts;
- schema/version;
- timing and operation ID.

Prohibited by default:

- raw user ID;
- prompt text or token prefix;
- system prompt hash if it can be dictionary attacked without a secret domain;
- media URL/content identifiers;
- encryption key, nonce reuse context or unredacted KMS response;
- full filesystem path containing tenant data.

## Availability and denial of service

Integrity failure should not crash the server. Parser and I/O work must be bounded:

- validate sizes against `stat` and configured maxima before allocation;
- stream digests in fixed chunks;
- cap manifest and metadata count/depth/string lengths;
- cap concurrent validation and fills;
- time out slow media with a miss;
- do not repeatedly retry the same quarantined digest;
- maintain free-space watermark and separate cache I/O from model-critical storage where possible.
