# L04c offline immutable object-frame verifier

Status: accepted offline implementation milestone after independent adversarial
review.

L04c verifies one caller-owned immutable object frame against a minimal object
reference retained from a fully authenticated L04b manifest result. The verifier
is a separate excluded static target with no filesystem, codec, provider,
candidate, logging, or runtime-server path.

## Terminal boundary

The sole positive disposition is `object_verified_unadmitted`. It means the
frame is structurally bounded and its exact length, stream type, and whole-frame
SHA-256 agree with one authenticated manifest descriptor. It remains a terminal
miss: no payload decode, state profile, codec, candidate, restore, or hit exists.

L04b copies minimal `{object_id, stream_type, frame_bytes}` references into its
result only at the final `authenticated_unadmitted` return and sets a private
carrier marker that ordinary aggregate construction cannot forge. All failure
results retain zero references and an unset marker. L04c accepts that result plus
an index; it never accepts a free-standing caller descriptor.

## Exact frame and checks

```text
magic[8] = 48 41 4c 4f 4f 42 4a 01
domain_len:u16be = 17
domain = "halofpx.object.v1"
type_len:u16be = 1..128
stream_type[type_len] = ASCII bytes 01..7f
payload_len:u64be
payload[payload_len]
EOF
```

The verifier uses subtraction-based cursor checks and never adds an untrusted
length. It rejects a payload length that does not fit `size_t`, exceeds the
trusted caller's positive payload cap, exceeds remaining bytes, or leaves a
trailing byte. Frame and payload caps are caller-supplied because L02 authorizes
no global numeric object maximum; the manifest's 1 MiB bound is not reused.
The frame cap must also be at most `UINT64_MAX / 8` because the inherited SHA
implementation converts its byte count to bits.

After structural checks, the verifier requires byte-exact descriptor stream
type and exact frame length, then computes SHA-256 over the complete original
frame from magic through the last payload byte. The descriptor object ID is not
a payload-only hash.

Success reports only a payload offset and length after the digest matches. The
caller must keep the complete backing bytes immutable during verification; the
verifier retains or mutates nothing.

## Conservative resolutions

- Zero payload is structurally valid. ADR-0003 requires the trusted configured
  payload cap to be positive; it does not require every payload to be nonempty.
  Empty bytes remain unregistered and unadmitted until a codec defines meaning.
- Frame stream types are ASCII, even though the generic manifest
  `registered-id` grammar permits UTF-8. NUL and bytes `80..ff`, including valid
  multibyte UTF-8, reject. L04c does not invent a printable regex or registry;
  ASCII control bytes `01..1f` are structurally representable but unregistered
  and unadmitted.
- No fixed object-size policy is invented. A later filesystem reader must derive
  an effective cap from trusted configuration, quota, and reserve before payload
  I/O or allocation.

## Crypto/build separation and provenance

The exact hash-pinned selected-base public-domain SHA-256 source now builds once
as `halofpx-context-store-sha256`, an excluded internal target shared by the
offline authentication and object targets. The target-owned bounded hash wrapper
keeps manifest/HMAC spans under their original cap while permitting an explicit
object-frame limit no greater than the inherited implementation's safe bit-count
range.

No CachyLLama or GPL llama-ai implementation or documentation entered L04c.
The direct-cherry-pick roster remains empty. Static contracts retain the exact
three selected-base SHA source hashes and reject any production CMake link or
runtime source call.

## Synthetic qualification

The independent empty-payload vector is type `x`, 38 total bytes, and
whole-frame SHA-256:

```text
b6915e1a81c18913dbd854bc548fbfd984b487dc2b6610dc5322033e4a464a11
```

The one-zero-byte payload vector is 39 total bytes and hashes to:

```text
d3ace5f0a24e7078cfb0a11987d4d0b31567317dd52125734b0500f8b5fb3f45
```

Tests cover maximum 128-byte type, payload at the caller cap, opaque high-bit
payloads, structural ASCII controls, unauthenticated/empty/index-invalid
manifests, invalid limits, null/empty/oversized input, every magic byte, domain
length/byte mutations, zero/oversized/non-ASCII types, UTF-8 rejection, valid
empty payload, short, long, huge, over-cap, truncated, and trailing payloads, every-byte frame
truncation, descriptor length/type/hash mismatches, stale hashes after payload
mutation, a rehashed changed payload that remains unadmitted, concurrent
determinism, and input immutability. Every failure exposes zero payload range.

## Gates still closed

- No trusted root, safe file open, symlink/reparse/device/path defense, streaming
  hash, partial I/O, object-set completeness, or payload codec exists.
- No frame bytes are allocated, copied, decoded, quarantined, or logged.
- No live manifest key registry, profile, state stream, provider hook, persistent
  read, or writer is enabled.
- A future streaming reader must independently qualify short reads, late I/O,
  file identity changes, storage reserve, and cancellation before runtime use.
