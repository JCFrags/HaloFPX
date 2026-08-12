---
section_id: "53"
title: "Framing and flow-control facts and constraints"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: ["torvalds/linux"]
  software_versions: ["Linux v7.2-rc2 source reference 8cdeaa50eae8dad34885515f62559ee83e7e8dda", "TCP RFC 9293", "QUIC RFC 9000", "TLS 1.3 RFC 8446"]
  hardware_revisions: ["Two-host Strix Halo USB4 cluster; exact revisions OPEN"]
related_sections: ["20", "23", "48", "50", "52", "54", "55", "71", "80"]
---

# Facts and constraints

## Carrier boundaries

| ID | Fact | Constraint |
|---|---|---|
| S53-F01 | **[VERIFIED]** TCP provides a reliable ordered byte stream; application sends can be segmented and received differently from write boundaries ([S53-SRC-004](sources.md#s53-src-004)). | The application must parse length-delimited records and retain partial-read/write offsets. |
| S53-F02 | **[VERIFIED]** Pinned USB4STREAM source exposes read/write iterator and poll operations, uses page-backed rings, splits writes at `TB_MAX_FRAME_SIZE`, and can return after accepting only part of a write ([S53-SRC-003](sources.md#s53-src-003)). | The same incremental parser and short-I/O state machine must run over both backends. Do not equate kernel frames with application records. |
| S53-F03 | **[VERIFIED]** Pinned USB4STREAM reports kernel ring CRC errors and buffer overruns, but the application receives no end-to-end semantic integrity or message identity from that mechanism ([S53-SRC-003](sources.md#s53-src-003)). | Application header/payload integrity and whole-message verification remain mandatory. |
| S53-F04 | **[VERIFIED]** The local plan states current project nodes lacked the interface and treats USB4STREAM as non-zero-copy, optional, and standalone ([S53-SRC-001](sources.md#s53-src-001), [S53-SRC-002](sources.md#s53-src-002)). | No production or performance claim follows from the proposed protocol. |

## Flow-control and framing evidence

- **[VERIFIED]** QUIC uses receiver-advertised connection and per-stream byte limits; a sender must not exceed them, and smaller subsequent advertisements do not reduce an already granted limit ([S53-SRC-005](sources.md#s53-src-005)).
- **[INFERENCE]** HaloFPX needs both a session-wide memory budget and per-channel/per-rail budgets so one bulk stream cannot consume all receive storage. QUIC's absolute-offset scheme is precedent, not HaloFPX wire compatibility.
- **[VERIFIED]** iSCSI uses explicit PDU lengths and separately negotiated header/data CRC32C digests ([S53-SRC-010](sources.md#s53-src-010)).
- **[INFERENCE]** Separate header and payload CRC32C lets HaloFPX reject impossible lengths before allocation and identify corruption location. CRC32C is an accidental-corruption check, not authentication.
- **[VERIFIED]** The local accepted plan chose a fixed 64-byte network-order header, delta byte credits with idempotent credit IDs, a global epoch, and exact-coverage reassembly ([S53-SRC-002](sources.md#s53-src-002)).

## Authentication, encryption, and replay

- **[VERIFIED]** HMAC is a secret-key message-authentication construction; key selection and secrecy are essential ([S53-SRC-007](sources.md#s53-src-007)). HKDF is an extract-and-expand KDF suitable for deriving separated keys ([S53-SRC-008](sources.md#s53-src-008)).
- **[VERIFIED]** ChaCha20-Poly1305 is an AEAD construction with a 256-bit key, 96-bit nonce, associated data, and 128-bit tag; nonce uniqueness under a key is required ([S53-SRC-009](sources.md#s53-src-009)).
- **[VERIFIED]** TLS 1.3 derives per-record nonces from a secret IV and sequence number, requires rekey/termination before sequence wrap, and warns that early data is replayable ([S53-SRC-006](sources.md#s53-src-006)).
- **[INFERENCE]** HaloFPX should use fresh nonces, a transcript-bound authenticated handshake, independent directional/rail keys, implicit per-record counters, no zero-RTT, and a new key schedule on every epoch. This is a design application of the cited primitives, not TLS compatibility.

## Threat model

- **[ASSUMPTION]** Initial deployment is two directly cabled, administratively controlled hosts in a trusted lab; management access and peer provisioning are outside the data rails.
- **[VERIFIED]** The local plan records IOMMU disabled on both audited hosts and explicitly says Thunderbolt authorization is not application-peer authentication ([S53-SRC-002](sources.md#s53-src-002)). That is scoped historical evidence and must be refreshed before execution.
- **[RECOMMENDATION]** Defend against corruption, stale/replayed records, wrong peer/topology, process bugs, malformed input, resource exhaustion, rail failure, and accidental cross-session mixing even in the non-encrypting integrity profile. Every bulk DATA record still requires cryptographic authentication.
- **[RECOMMENDATION]** Treat endpoint compromise, PSK theft, malicious privileged kernel/firmware, traffic analysis, and denial by physical disconnect as outside what the record layer alone can prevent. Optional AEAD protects payload confidentiality/integrity in transit, not compromised endpoints.
