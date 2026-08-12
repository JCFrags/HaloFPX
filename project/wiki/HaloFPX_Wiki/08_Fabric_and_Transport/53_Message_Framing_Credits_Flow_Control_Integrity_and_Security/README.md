---
section_id: "53"
title: "Message Framing, Credits, Flow Control, Integrity, and Security"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: []
  software_versions: ["HaloFPX wire protocol v1 proposal", "Linux v7.2-rc2 source reference 8cdeaa50eae8dad34885515f62559ee83e7e8dda"]
  hardware_revisions: ["Two-host Strix Halo USB4 cluster; exact revisions OPEN"]
related_sections: ["02", "18", "20", "23", "38", "48", "49", "50", "51", "52", "54", "55", "71", "75", "80"]
---

# Message framing, credits, flow control, integrity, and security

## Protocol position

- **[VERIFIED]** Project decision `D-2026-07-12-035` keeps USB4NET/TCP/MPTCP as the default, comparator, fallback, control, and recovery path. USB4STREAM is restricted to a standalone reversible probe and is not a prerequisite for distributed execution ([S53-SRC-001](sources.md#s53-src-001)).
- **[VERIFIED]** The approved feasibility plan requires an identical framed/credit protocol over TCP and USB4STREAM, bounded queues, short-I/O handling, checksums, sequence tracking, reconnect tests, and one million error-free messages before an RPC-plugin proposal ([S53-SRC-002](sources.md#s53-src-002)). This is scoped project evidence, not a universal transport result.
- **[VERIFIED]** Linux USB4STREAM exposes file-like `read_iter`, `write_iter`, and `poll`; large writes are split into page-sized-or-smaller driver frames and can complete partially. It does not supply application message boundaries, authentication, replay protection, reassembly, or application credits ([S53-SRC-003](sources.md#s53-src-003)).
- **[RECOMMENDATION]** Use the [v1 wire contract](design_implications.md#version-1-wire-contract) above both carriers. A carrier failure ends the global session epoch; no partial record or message silently moves to another rail.
- **[RECOMMENDATION]** The non-encrypting `AUTH_INTEGRITY` profile authenticates every post-handshake control and DATA record. CRC32C remains an accidental-corruption diagnostic and is never the security boundary. Omitting confidentiality remains a Section 71 threat-acceptance decision, not an implicit property of a trusted cable.
- **[OPEN]** No implementation or machine result currently proves parser safety, liveness, credit correctness, cryptographic interoperability, USB4STREAM availability, or performance. Resolve [S53-EXP-001 through S53-EXP-006](procedures_and_checks.md#experiment-register).

## Page map

- [Facts and constraints](facts_and_constraints.md)
- [Design implications and v1 wire contract](design_implications.md)
- [Procedures and checks](procedures_and_checks.md)
- [Open questions](open_questions.md)
- [Source register](sources.md)

## Research split

1. **Internet/source-code research complete now:** byte-stream/character-device boundaries, credit patterns, CRC separation, HMAC/HKDF/AEAD primitives, TLS replay lessons, and pinned Linux behavior.
2. **Off-node and machine work required:** codec vectors, fuzzing, credit/liveness model tests, identical-codec TCP comparison, short-I/O/fault injection, cryptographic interop, dual-rail reconnect, and the required correctness soak.
3. **Contingent decisions:** USB4STREAM promotion, credit window, record/message caps, batching, timeout values, encryption default, priority scheduler, retry policy, and any RPC integration remain gated on those results.

## Authority boundary

**[RECOMMENDATION]** This section is the authoritative proposed application protocol. Section 50 owns USB4STREAM provisioning and device semantics; 52 owns multipath policy; 54 owns buffer visibility/copy behavior; 55/75 own transport measurements; 71 owns deployment trust/secrets; and 80 owns destructive fault-injection authorization.
