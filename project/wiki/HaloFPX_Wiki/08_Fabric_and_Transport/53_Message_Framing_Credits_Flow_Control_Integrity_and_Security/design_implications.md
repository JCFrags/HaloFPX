---
section_id: "53"
title: "HaloFPX version-1 framed protocol design"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: []
  software_versions: ["HaloFPX wire protocol v1 proposal"]
  hardware_revisions: ["Two-host Strix Halo USB4 cluster; exact revisions OPEN"]
related_sections: ["38", "48", "49", "50", "51", "52", "54", "55", "71", "75", "80"]
---

# HaloFPX version-1 framed protocol design

Everything in this page is a **[RECOMMENDATION]** unless explicitly labeled otherwise. It refines the accepted local probe contract; it is not implemented or verified.

## Version-1 wire contract

All integers are unsigned network byte order. The fixed header is exactly 64 bytes:

| Offset | Field | Bytes | Validation |
|---:|---|---:|---|
| 0 | magic | 4 | constant `HFPX`; mismatch is not scanned past on a live session |
| 4 | version | 2 | `1` after negotiation |
| 6 | header_bytes | 2 | exactly `64` for v1 |
| 8 | type | 2 | known or negotiated extension |
| 10 | flags | 2 | reserved bits zero |
| 12 | rail_id | 2 | negotiated rail and actual descriptor agree |
| 14 | channel_id | 2 | `0=CONTROL`, `1=BULK`; others negotiated |
| 16 | session_epoch | 8 | exact active global epoch |
| 24 | message_id | 8 | monotonic logical-message/control sequence within epoch; zero reserved for handshake |
| 32 | logical_offset | 8 | checked `offset + payload_bytes <= logical_total` |
| 40 | logical_total | 8 | nonzero for DATA; within negotiated message/receive budget |
| 48 | payload_bytes | 4 | within negotiated record cap before allocation |
| 52 | credit_bytes | 4 | positive delta only on CREDIT; otherwise zero |
| 56 | header_crc32c | 4 | computed with this field zero |
| 60 | payload_crc32c | 4 | plaintext payload; zero only where type requires no payload |

Flags: `FIRST=0x0001`, `LAST=0x0002`, `ACK_REQUIRED=0x0004`, `ENCRYPTED=0x0008`, `IGNORABLE_EXTENSION=0x0010`; all other v1 bits are zero. `FIRST/LAST` are consistency hints; exact offset coverage is authoritative.

Each ordered rail/direction also has an implicit 64-bit `record_seq`, initialized to zero only after handshake confirmation and incremented once for every emitted record. It is not transmitted in the fixed header. Both endpoints use it for AEAD nonce construction and transcript/counter diagnostics. Any parse/tag failure, gap, counter disagreement, or impending wrap terminates the epoch; sequence state is never guessed forward.

### Section 49 requirement-to-wire trace

The fixed record header is only one part of the logical message envelope. Connection-scoped values are authenticated in negotiation; record-routing values are in the 64-byte header; operation-scoped values are in a canonical upper-layer descriptor at the start of each logical DATA message.

| Section 49 requirement | v1 location and binding | Required observation / rejection |
|---|---|---|
| protocol major/minor | negotiated transcript plus fixed-header `version` | log selected range/version; reject unsupported or downgraded selection |
| peer ID | expected initiator/responder node UUIDs and rail/service map in transcript | log stable peer/rail binding; reject wrong UUID, role, or descriptor |
| session ID | fresh 256-bit `session_id` plus `session_epoch` in transcript; epoch repeated in every header | log non-secret session/epoch; reject cross-session or stale-epoch record |
| source/destination rank IDs | canonical upper-layer descriptor, validated against transcript-bound topology/rank map | log rank pair; reject an unowned or topology-inconsistent rank |
| lane | fixed-header `channel_id` plus negotiated channel registry | per-channel counters; reject unknown/unnegotiated channel |
| message type | fixed-header `type`; application operation kind in upper-layer descriptor | per-type counters; reject illegal type/state combination |
| message ID | fixed-header `message_id`, unique/monotonic under the declared type rules | log and duplicate-check within epoch |
| correlation/operation ID | canonical upper-layer descriptor | propagate to trace/ACK; reject malformed or reused conflicting identity |
| step/decode iteration | canonical upper-layer descriptor | log at apply/commit boundary; reject stale or rank-inconsistent step |
| payload length | `payload_bytes`, `logical_total`, negotiated caps | account wire/logical bytes; reject overflow, cap, and coverage violations |
| flags/checksum policy | fixed-header `flags`; negotiated security/integrity profile | log profile and flags; reject unknown flags or missing/invalid tag |
| chunk coordinates | `logical_offset`, `payload_bytes`, `logical_total`, `FIRST/LAST` hints | interval/duplicate metrics; reject overlap, gap-at-deadline, or conflicting total |

The upper-layer descriptor uses canonical fixed-order, length-delimited, network-order fields: descriptor version, operation kind, source rank, destination rank or group, correlation/operation ID, step/decode iteration, commit/idempotency class, and whole-message digest algorithm/value. It is counted inside `logical_total`, begins at offset zero, is covered by per-record authentication, and is bound to the final ACK/commit result. Implementations must not reconstruct these values solely from an unauthenticated socket address or process-local default. Descriptor encoding and golden vectors remain part of S53-EXP-001; field-value policies remain owned by the rank/operation sections.

## Message types and state legality

| Code | Type | Channel | Legal purpose |
|---:|---|---|---|
| `0x0001` | HELLO | control | version range, role/node, nonce, topology and limits |
| `0x0002` | CHALLENGE | control | responder nonce and selected capabilities |
| `0x0003` | CONFIRM | control | transcript authenticator; opens session after both confirm |
| `0x0004` | CREDIT | control | idempotent positive byte-credit delta |
| `0x0005` | ACK | control | whole-message commit or control acknowledgement |
| `0x0006` | NACK | control | typed rejection; diagnostic only, never implicit retry |
| `0x0007` | CANCEL | control | cooperative cancellation request for `(epoch,message_id)` |
| `0x0008` | CANCEL_ACK | control | `CANCELLED`, `TOO_LATE`, or `UNKNOWN` |
| `0x0009` | HEARTBEAT | control | liveness and last-processed IDs; no credit |
| `0x000a` | DRAIN | control | stop new DATA and reach a record/message boundary |
| `0x000b` | CLOSE | control | authenticated graceful epoch close |
| `0x000c` | RESET | control | fatal typed reason; peer must close epoch |
| `0x000d` | ERROR | control | bounded diagnostic, no secret or reflected bulk data |
| `0x0100` | DATA | bulk | one non-overlapping logical-message chunk |

Unknown types fail the epoch unless both peers negotiated an extension namespace and the record carries `IGNORABLE_EXTENSION`; even then its declared bytes consume credit and are integrity-checked before discard.

## Compatibility negotiation

HELLO/CHALLENGE use a canonical fixed-order length-delimited field encoding, not native structs. The transcript binds: domain separator; initiator/responder roles and stable node IDs; expected peer UUIDs; complete rail/service and rank-ownership map; fresh 256-bit `session_id`; proposed epoch; 256-bit random nonces; supported version interval; security profiles/algorithms; record/message/in-flight limits; channels; credit mode; timeout ranges; feature bits; and implementation build ID.

Select the highest mutually supported version and an explicit intersection. Required unknown feature bits, a downgrade from local minimum policy, duplicate fields, non-canonical encoding, inconsistent repeated values, or excessive negotiation bytes fail closed. No credits or DATA precede mutual CONFIRM; no zero-RTT resumption exists in v1.

## Security profiles

| Profile | Protection | Allowed scope |
|---|---|---|
| `AUTH_INTEGRITY` | HMAC-SHA-256 transcript/CONFIRM; HMAC-SHA-256 tag on every post-handshake control and DATA record; CRC32C retained only for accidental-corruption diagnosis | Direct administratively controlled link only after Section 71 explicitly accepts omitted confidentiality; peer identity, integrity, freshness, and replay protection remain mandatory |
| `AEAD_CHACHA20_POLY1305` | Same handshake; HKDF-SHA-256 derives independent direction/rail control and bulk keys/IVs; every post-handshake record uses ChaCha20-Poly1305 with the 64-byte header as associated data and a 16-byte trailer tag | Required when policy demands confidentiality or untrusted transit |

The PSK is root-owned `0600`, never logged, and has at least 256 random bits. HKDF salt and info include both nonces, epoch, roles, topology hash, version and algorithm. AEAD nonce follows the TLS pattern: a per-key 96-bit IV XORed with the zero-padded implicit 64-bit `record_seq`. Separate keys make `(key,nonce)` unique across direction/rail/channel. Rekey by new epoch before wrap or implementation-specific usage limit. Use audited crypto-library APIs and constant-time tag comparison; do not implement primitives locally.

Security trailers are outside `payload_bytes`, so the serialized record length is `64 + payload_bytes + trailer_bytes`. HELLO and CHALLENGE have no trailer. CONFIRM carries a 32-byte HMAC-SHA-256 transcript trailer. After confirmation, every `AUTH_INTEGRITY` control and DATA record carries a 32-byte HMAC over a domain separator, implicit `record_seq`, the transmitted header, and payload. Every `AEAD_CHACHA20_POLY1305` record carries a 16-byte tag, keeps the same payload length after encryption, and authenticates the transmitted header as associated data. CRC32C is checked only as a diagnostic and never substitutes for the HMAC or AEAD tag.

CRC is retained in AEAD mode only as an early corruption/diagnostic field and is itself authenticated as header AAD; AEAD tag success is authoritative. Authentication failure sends no detailed oracle, drops all outstanding work, zeroizes epoch keys, and closes all rails.

## Credits, batching, and priority

- Maintain checked 64-bit available-byte credit per `(epoch,direction,bulk channel,rail)` plus a session-wide receive-budget ceiling. DATA reserves full payload bytes before enqueue. Local descriptor slots are a separate bound.
- A dropped DATA record retains its reserved credit. Same-identity retransmission reuses that reservation and must not silently refund or reserve a second unit. Capacity returns only after receiver delivery and storage release produces an authenticated credit, or an explicit global-epoch reset abandons and reinitializes all old-channel credit state.
- CREDIT carries an unsigned delta and a monotonic control `message_id`. Apply exactly once; identical duplicate is re-ACKed, while ID gaps, overflow, zero delta, or a total above the negotiated window fail the epoch.
- Return credits only when receiver-owned storage is released to its reusable pool, not on kernel arrival. Negotiated `max_logical_message <= reserved receive budget` prevents impossible reassembly commitments.
- Control has reserved descriptors/bytes and bounded priority. Process at most a negotiated control burst before servicing ready bulk, except RESET/CLOSE/CREDIT needed for liveness. Bulk can never consume control reserve.
- Batching may coalesce complete serialized records into one write call. It never changes record identity, credit accounting, deadlines, or integrity scope; short writes resume at the retained byte offset.

## Fragmentation and reassembly

One logical message may have many DATA records and span both rails. All share `(epoch,message_id,logical_total)`. Accept a chunk only if checked arithmetic places `[offset,offset+length)` inside `[0,total)`. Track bounded interval coverage. Identical duplicate bytes are counted and ignored; overlapping non-identical bytes, conflicting totals, excessive fragments, gaps at deadline, or bytes from another epoch fail that message and normally the epoch.

Completion requires exact non-overlapping coverage of `[0,total)`, payload CRC plus HMAC/AEAD success for every record, and the mandatory final whole-message digest in the upper-layer descriptor. No rail ordering is assumed. Retry occurs only for an uncommitted whole operation in a new epoch after an explicit upper-layer decision.

## Cancellation, timeout, and failure

- CANCEL is cooperative. Receiver stops uncommitted work, discards later chunks while still accounting/releasing their storage, and replies `CANCELLED`. If committed/visible, reply `TOO_LATE`; cancellation never rolls back rank state implicitly.
- Every operation has an absolute monotonic deadline propagated from the caller. Handshake, record-progress, reassembly, heartbeat, drain, and close limits are negotiated/capped and logged. Timer expiry completes affected work explicitly and closes the epoch when state may differ.
- Reconnect on either rail barriers all rails, fails/drains outstanding work, negotiates a fresh global epoch and keys, resets credits/counters, and admits no old-epoch bytes. There is no mid-record backend switch or management-LAN bulk fallback.

## Malformed-message policy

Parse into bounded integers before allocation. Reject bad magic/version/header length, CRC/tag, flags, rail/channel, epoch, state/type, nonzero reserved fields, length arithmetic, caps, credit rules, transcript, or reassembly invariants. Rate-limit fixed-size error telemetry; never echo attacker payload or secrets. A connection-level parser/authentication violation closes the epoch. Fuzzing and sanitizer gates precede privileged machine use.
