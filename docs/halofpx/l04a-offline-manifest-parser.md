# L04a offline bounded manifest parser

Status: accepted implementation milestone. See the linked L04a review.

L04a implements a memory-only parser for exactly
`authenticated-manifest-v1` from the hash-locked L02 CDDL. It is a standalone
static target with format-local value types and a test source; it neither
includes nor links the provider seam, enters `server-context` or `llama-server`,
nor is called by production code.

## Result boundary

The sole success disposition is `structural_only`. It means the bytes match the
closed deterministic-CBOR envelope, bounds, and local cross-field rules. It is
explicitly untrusted and unadmitted. It cannot create a provider candidate,
restore state, or become a hit.

L04a does not verify the manifest HMAC, look up a key, recompute compatibility
or topology-component hashes, authorize scope, check a replay anchor, inspect
an object path, read/hash/decode a payload, check profile completeness, or admit
a profile/stream/codec. Those remain miss reasons until later gates.

## Parser contract

- Input is a caller-owned immutable memory span, at most 1 MiB. Null/empty and
  oversized input is rejected before cursor work.
- One non-recursive cursor pass accepts only definite deterministic CBOR with
  shortest unsigned/length encoding. Tags, floats, indefinite values, wrong
  simple values, duplicate/unlisted/missing/out-of-order integer keys, wrong
  fixed lengths, trailing bytes, and truncation are rejected.
- Exact map sizes are 2/4/15/16/6/3/13 for envelope, authentication input,
  body, compatibility, topology, rank, and object descriptor.
- Rank and descriptor arrays are each 1..128. The parser uses fixed arrays and
  performs no input-sized allocation.
- Registered IDs are 1..128 exact UTF-8 bytes with no NUL or normalization.
  Overlong forms, invalid continuations, surrogate code points, and values above
  U+10FFFF are rejected. Registry membership is not claimed at structural-only
  stage; later admission may narrow particular ID classes to ASCII registries.
- Rank ownership is encoded in ascending logical-rank order and must be the
  exact set `0..world_size-1`. Descriptor rank must exist; its ownership digest
  must match that rank. Descriptor compatibility root must match the body, and
  duplicate object IDs are rejected.
- Generation 1 requires a null predecessor; later generations require a
  predecessor digest. Major/minor are 1/0, HMAC algorithm is 1, required is
  true, frame length is positive, and durability is 0..2.
- Failure returns a bounded status and the cursor position where rejection was
  detected. This diagnostic is not promised to identify the first invalid byte.
  No resynchronization, logging, metric, quarantine, deletion, or other I/O
  occurs.

## Conservative ambiguity resolutions

- Descriptor array order is manifest-significant and preserved. Its semantic
  order remains profile-defined; the parser never sorts it.
- A null descriptor rank is structurally permitted for a future coordinator
  owner, but L04a does not interpret its ownership digest and cannot declare it
  semantically eligible.
- Logical position and output boundary are parsed as unsigned values without an
  invented relational rule; their units/order await an admitted profile.
- Key generation zero is structurally permitted by the L02 CDDL but cannot pass
  a later key-registry/admission decision unless explicitly registered.
- Because L02 admits no profile or codec, every structural success remains
  unsupported/unadmitted for provider purposes.

## Synthetic qualification

The target-owned test encoder produces no imported fixture or donor format. It
covers minimum and maximum rank/object manifests, generation encoding boundaries
23/24/255/256, coordinator/rank-local forms, every-byte truncation, appended
data, oversize input, indefinite/wrong top-level types, non-minimal integers,
  wrong versions/algorithm/durability/tag length, valid multibyte and invalid/
  overlong/surrogate UTF-8 IDs, predecessor contradictions,
rank count/order/duplicate/range failures, duplicate objects, false required
flags, wrong ownership/root, invalid/oversized IDs, input immutability, and
concurrent deterministic parsing.

The static contract rejects filesystem, runtime-state, provider/candidate,
inherited-cache, logging, and donor dependencies and scans all production
server/common sources for a parser call. HMAC/golden vectors, hash corruption,
unknown/revoked keys, compatibility rejection, sanitizer fuzzing, and payload
corruption are L04b work, not claims of this milestone.
