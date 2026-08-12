---
section_id: "53"
title: "Framed transport source register"
status: "draft"
last_verified: "2026-07-17"
applies_to:
  repositories: ["torvalds/linux"]
  software_versions: ["Linux v7.2-rc2", "RFC 9293", "RFC 9000", "RFC 8446", "RFC 2104", "RFC 5869", "RFC 8439", "RFC 7143"]
  hardware_revisions: ["Scoped two-host Strix Halo USB4 cluster"]
related_sections: ["02", "20", "23", "50", "52", "54", "55", "71"]
---

# Sources

Access date for every source: 2026-07-16.

## S53-SRC-001

- **Title/origin:** `D-2026-07-12-035: Keep USB4NET default and bound USB4STREAM to a standalone probe` — local project decision ledger
- **Path:** `C:/Users/britt/AgentRoot/10_projects/11_active/11.02_strix-halo-usb4-cluster/DECISIONS.md`
- **Revision/date:** accepted 2026-07-12; decision ID `D-2026-07-12-035`
- **Supports:** default/fallback carrier, standalone scope, non-zero-copy status, one-million-message and performance gates, cleanup/reversibility requirements.
- **Limitations:** local accepted project decision, not external technical proof or a measurement; ledger is mutable, so cite the stable decision ID.

## S53-SRC-002

- **Title/origin:** M2 USB4STREAM Feasibility and Standalone Transport Plan — local project artifact
- **Path:** `C:/Users/britt/AgentRoot/10_projects/11_active/11.02_strix-halo-usb4-cluster/01_discovery/output/2026-07-12__m2-usb4stream-transport__feasibility-plan__v01.md`
- **Revision/date:** v01, 2026-07-12; SHA-256 `34f99324f27ed91064441536661081ad2afcf33a779b815faf0936e7c0a22ef5` recorded by S53-SRC-001
- **Supports:** scoped machine state, approved probe architecture, 64-byte header, delta credits, HMAC handshake, short-I/O/fault rules, security/permission boundary, test and promotion gates.
- **Limitations:** planning artifact; it explicitly reports no USB4STREAM run or benchmark and cannot establish current machine state or universal design correctness.

## S53-SRC-003

- **Title/repository:** Linux USB4STREAM driver `drivers/thunderbolt/stream.c` — Linux kernel
- **URL:** https://github.com/torvalds/linux/blob/8cdeaa50eae8dad34885515f62559ee83e7e8dda/drivers/thunderbolt/stream.c
- **Revision/date:** Linux v7.2-rc2 commit `8cdeaa50eae8dad34885515f62559ee83e7e8dda`
- **Supports:** page-backed rings, CRC/overrun signaling, read/write iterator and poll behavior, write fragmentation and short completion, no application protocol/security.
- **Limitations:** release-candidate source reference, not proof of the installed kernel or future stable ABI; kernel-layer semantics only.

## S53-SRC-004

- **Title/publisher:** RFC 9293, Transmission Control Protocol — IETF/RFC Editor
- **URL:** https://www.rfc-editor.org/rfc/rfc9293.html
- **Revision/date:** Internet Standard, August 2022
- **Supports:** reliable ordered byte-stream semantics and TCP connection behavior.
- **Limitations:** TCP does not define HaloFPX records, application credits, authentication or distributed commit semantics.

## S53-SRC-005

- **Title/publisher:** RFC 9000, QUIC: A UDP-Based Multiplexed and Secure Transport — IETF/RFC Editor
- **URL:** https://www.rfc-editor.org/rfc/rfc9000.html
- **Revision/date:** Proposed Standard, May 2021
- **Supports:** receiver-advertised connection/stream byte limits, monotonic limit increases, flow-control violations and blocked signaling.
- **Limitations:** design precedent only; HaloFPX is not QUIC-compatible and the accepted local v1 uses idempotent delta credits rather than QUIC absolute offsets.

## S53-SRC-006

- **Title/publisher:** RFC 8446, The Transport Layer Security Protocol Version 1.3 — IETF/RFC Editor
- **URL:** https://www.rfc-editor.org/rfc/rfc8446.html
- **Revision/date:** Proposed Standard, August 2018
- **Supports:** AEAD record protection, sequence-derived nonces, wrap handling, key updates and early-data replay risks.
- **Limitations:** HaloFPX's proposed record layer is not TLS and must not claim TLS security or interoperability.

## S53-SRC-007

- **Title/publisher:** RFC 2104, HMAC: Keyed-Hashing for Message Authentication — IETF/RFC Editor
- **URL:** https://www.rfc-editor.org/rfc/rfc2104.html
- **Revision/date:** Informational, February 1997
- **Supports:** HMAC construction, purpose and key requirements.
- **Limitations:** generic construction; SHA-256 selection, transcript design, comparison API and key lifecycle are HaloFPX choices requiring review.

## S53-SRC-008

- **Title/publisher:** RFC 5869, HMAC-based Extract-and-Expand Key Derivation Function — IETF/RFC Editor
- **URL:** https://www.rfc-editor.org/rfc/rfc5869.html
- **Revision/date:** Informational, May 2010
- **Supports:** HKDF extract/expand construction and context separation basis.
- **Limitations:** does not define HaloFPX labels, salt/info transcript or PSK provisioning.

## S53-SRC-009

- **Title/publisher:** RFC 8439, ChaCha20 and Poly1305 for IETF Protocols — IETF/RFC Editor
- **URL:** https://www.rfc-editor.org/rfc/rfc8439.html
- **Revision/date:** Informational, June 2018
- **Supports:** ChaCha20-Poly1305 key/nonce/tag sizes, associated data and nonce-uniqueness requirement.
- **Limitations:** primitive specification, not a complete session protocol; usage limits, key derivation and implementation selection remain open.

## S53-SRC-010

- **Title/publisher:** RFC 7143, Internet Small Computer System Interface (iSCSI) Protocol (Consolidated) — IETF/RFC Editor
- **URL:** https://www.rfc-editor.org/rfc/rfc7143.html
- **Revision/date:** Proposed Standard, April 2014
- **Supports:** explicit PDU framing and separate negotiated header/data CRC32C digests as a mature protocol precedent.
- **Limitations:** HaloFPX is not iSCSI; its opcodes, recovery, SCSI ordering and digest negotiation are not adopted.

## S53-SRC-011

- **Title/origin:** Distributed protocol model-checking strategy — local research follow-up
- **Path:** `reviews/follow-ups/2026-07-16__distributed-protocol-model-checking__research__v01.md`
- **Revision/date:** proposed follow-up, researched 2026-07-17
- **Supports:** bounded model scope, credit-loss conservation rule, required safety/liveness configurations, evidence retention and abstraction limits.
- **Limitations:** research proposal only; no HaloFPX model or checker run exists.

## S53-SRC-012

- **Title/origin:** [TLA+ Tools stable release v1.7.4](https://github.com/tlaplus/tlaplus/releases/tag/v1.7.4) — tlaplus/tlaplus
- **Revision/date:** tag commit `5a47802b5c391f59ecdd44117981f4ff8c0656ba`, released 2024-08-05; accessed 2026-07-17
- **Supports:** pinned TLC tool release and the release note for its multi-worker liveness-checking fix.
- **Limitations:** general tool provenance; not a HaloFPX correctness result. The project must record the downloaded JAR SHA-256.

## Evidence precedence and conflicts

- **[VERIFIED]** S53-SRC-001 makes S53-SRC-002 the scoped future-probe contract. It does not supersede primary standards or Linux source for universal technical claims.
- **[RECOMMENDATION]** If a stable USB4STREAM source differs from S53-SRC-003, record the diff and revise section 50/53 before machine use. If security review changes the wire contract, allocate a new version; do not silently reinterpret v1 fields.
