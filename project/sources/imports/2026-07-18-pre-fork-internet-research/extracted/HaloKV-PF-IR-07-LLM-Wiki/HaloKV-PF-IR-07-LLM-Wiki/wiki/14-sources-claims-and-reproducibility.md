# Sources, claim labels, revisions, licenses, and reproducibility

## Evidence rules used

1. Prefer final standards and official kernel/platform/project documentation.
2. Record exact standard revision, release tag, commit/blob identifier where available, publication/release date, and research access date.
3. Preserve raw bytes where the toolchain permits; preserve an extracted text copy only as a convenience, not as a substitute for the PDF.
4. Hash every preserved source file in `manifests/SOURCE_SHA256SUMS`.
5. Separate direct source claims, supported inferences, local synthesis recommendations, residual risks, and rejected/unsupported claims.
6. Treat selected release maturity labels literally; do not upgrade “experimental” into a production guarantee.
7. Avoid compliance, certification, cryptographic-erasure, or secure-deletion conclusions without deployment evidence.

## Literal claim-label syntax

```text
[CLAIM:PFIR07-CNNN][CLASS:SOURCE|INFERENCE|SYNTHESIS|UNSUPPORTED]
[STATUS:SUPPORTED|RECOMMENDED|REQUIRED|CONDITIONAL|RESIDUAL-RISK|REJECTED]
[SRC:source-id/section or claim basis]
```

The complete ledger is available as [`claims.csv`](../matrices/claims.csv) and [`claims.json`](../matrices/claims.json). Labels remain literal text in the Markdown and are styled in HTML.

## Source catalog

| Source ID | Title | Revision | Published/released | Accessed | Archive status | License/notice |
| --- | --- | --- | --- | --- | --- | --- |
| LINUX-FSCRYPT-7.2RC3 | Filesystem-level encryption (fscrypt) | Linux documentation snapshot 7.2.0-rc3; source tag v7.2-rc3 | 2026-07 (release-candidate snapshot) | 2026-07-18 | byte-preserved local text snapshot | Linux source-tree license notices; see copied COPYING and license-rules |
| LINUX-DMCRYPT-7.2RC3 | dm-crypt target documentation | Linux documentation snapshot 7.2.0-rc3; source tag v7.2-rc3 | 2026-07 (release-candidate snapshot) | 2026-07-18 | byte-preserved local text snapshot | Linux source-tree license notices |
| LINUX-DMINTEGRITY-7.2RC3 | dm-integrity target documentation | Linux documentation snapshot 7.2.0-rc3; source tag v7.2-rc3 | 2026-07 (release-candidate snapshot) | 2026-07-18 | byte-preserved local text snapshot | Linux source-tree license notices |
| LINUX-KEYS-7.2RC3 | Kernel key retention service — core documentation | Linux documentation snapshot 7.2.0-rc3; source tag v7.2-rc3 | 2026-07 (release-candidate snapshot) | 2026-07-18 | byte-preserved local text snapshot | Linux source-tree license notices |
| LUKS2-SPEC-1.1.4 | LUKS2 On-Disk Format Specification | version 1.1.4; repository commit a81aa2fef8d176debdbec297c1e0b74bb50cfee8 | 2025-06-16 | 2026-07-18 | byte-preserved PDF plus text extract | CC BY-SA 4.0 (stated in document and copied repository license) |
| CRYPTSETUP-2.8.6 | cryptsetup 2.8.6 selected release notes and manual sources | stable release 2.8.6; tag v2.8.6 | 2026-04-02 | 2026-07-18 | selected byte-preserved manual sources; release tarball not embedded | cryptsetup COPYING retained in sources/licenses |
| SYSTEMD-CRYPTTAB-261.1 | systemd crypttab(5) source | v261.1; tag commit eff9446d505d62c075bed37d606860b38cfe51fb; man/crypttab.xml blob SHA-1 38ba4ceafbe80aead3025bc0866a3573a0a36ad5 | 2026-06-26 | 2026-07-18 | reference plus derivative extract; raw XML not byte-preserved | LGPL-2.1-or-later SPDX identifier in source; LGPL text retained |
| NIST-SP800-38D | NIST SP 800-38D — GCM and GMAC | NIST SP 800-38D, final | 2007-11 | 2026-07-18 | byte-preserved final publication PDF plus text extract | Document-embedded NIST notices; no blanket relicensing claim by this bundle |
| NIST-SP800-38E | NIST SP 800-38E — XTS-AES for confidentiality on storage devices | NIST SP 800-38E, final | 2010-01 | 2026-07-18 | byte-preserved final publication PDF plus text extract | Document-embedded NIST notices; no blanket relicensing claim by this bundle |
| NIST-SP800-57P1R5 | NIST SP 800-57 Part 1 Revision 5 — Key Management | NIST SP 800-57 Part 1 Rev. 5, final | 2020-05 | 2026-07-18 | byte-preserved final publication PDF plus text extract | Document-embedded NIST notices; no blanket relicensing claim by this bundle |
| NIST-SP800-88R2 | NIST SP 800-88 Revision 2 — Guidelines for Media Sanitization | NIST SP 800-88 Rev. 2, final | 2025-09 | 2026-07-18 | byte-preserved final publication PDF plus text extract | Document-embedded NIST notices; no blanket relicensing claim by this bundle |
| NIST-SP800-108R1U1 | NIST SP 800-108 Revision 1 Update 1 — KDFs using PRFs | NIST SP 800-108 Rev. 1 Update 1; updates through 2024-02-02 | 2022-08 / updated 2024-02-02 | 2026-07-18 | byte-preserved final publication PDF plus text extract | Document-embedded NIST notices; no blanket relicensing claim by this bundle |
| NIST-SP800-90AR1 | NIST SP 800-90A Revision 1 — Deterministic Random Bit Generators | NIST SP 800-90A Rev. 1, final | 2015-06 | 2026-07-18 | byte-preserved final publication PDF plus text extract | Document-embedded NIST notices; no blanket relicensing claim by this bundle |
| NIST-SP800-111 | NIST SP 800-111 — Guide to Storage Encryption Technologies for End User Devices | NIST SP 800-111, final | 2007-11 | 2026-07-18 | byte-preserved final publication PDF plus text extract | Document-embedded NIST notices; no blanket relicensing claim by this bundle |
| NIST-SP800-133R2 | NIST SP 800-133 Revision 2 — Cryptographic Key Generation | NIST SP 800-133 Rev. 2, final | 2020-06 | 2026-07-18 | byte-preserved final publication PDF plus text extract | Document-embedded NIST notices; no blanket relicensing claim by this bundle |
| NIST-FIPS180-4 | FIPS 180-4 — Secure Hash Standard | FIPS 180-4 | 2015-08 | 2026-07-18 | byte-preserved final publication PDF plus text extract | Document-embedded NIST notices; no blanket relicensing claim by this bundle |
| NIST-FIPS197-U1 | FIPS 197 Update 1 — Advanced Encryption Standard | FIPS 197; updated 2023-05-09 | 2001-11-26 / updated 2023-05-09 | 2026-07-18 | byte-preserved final publication PDF plus text extract | Document-embedded NIST notices; no blanket relicensing claim by this bundle |
| NIST-FIPS198-1 | FIPS 198-1 — HMAC | FIPS 198-1 | 2008-07 | 2026-07-18 | byte-preserved final publication PDF plus text extract | Document-embedded NIST notices; no blanket relicensing claim by this bundle |
| RFC5116 | RFC5116 — An Interface and Algorithms for Authenticated Encryption | RFC 5116 | 2008-01 | 2026-07-18 | byte-preserved RFC text | Document-embedded IETF Trust copyright and legal-provisions notice |
| RFC8439 | RFC8439 — ChaCha20 and Poly1305 for IETF Protocols | RFC 8439 | 2018-06 | 2026-07-18 | byte-preserved RFC text | Document-embedded IETF Trust copyright and legal-provisions notice |
| RFC8452 | RFC8452 — AES-GCM-SIV: Nonce Misuse-Resistant Authenticated Encryption | RFC 8452 | 2019-04 | 2026-07-18 | byte-preserved RFC text | Document-embedded IETF Trust copyright and legal-provisions notice |
| RFC5869 | RFC5869 — HMAC-based Extract-and-Expand Key Derivation Function (HKDF) | RFC 5869 | 2010-05 | 2026-07-18 | byte-preserved RFC text | Document-embedded IETF Trust copyright and legal-provisions notice |
| RFC2104 | RFC2104 — HMAC: Keyed-Hashing for Message Authentication | RFC 2104 | 1997-02 | 2026-07-18 | byte-preserved RFC text | Document-embedded IETF Trust copyright and legal-provisions notice |
| RFC9106 | RFC9106 — Argon2 Memory-Hard Function for Password Hashing and Proof-of-Work | RFC 9106 | 2021-09 | 2026-07-18 | byte-preserved RFC text | Document-embedded IETF Trust copyright and legal-provisions notice |
| RFC8018 | RFC8018 — PKCS #5: Password-Based Cryptography Specification Version 2.1 | RFC 8018 | 2017-01 | 2026-07-18 | byte-preserved RFC text | Document-embedded IETF Trust copyright and legal-provisions notice |
| RFC8949 | RFC8949 — Concise Binary Object Representation (CBOR) | RFC 8949 | 2020-12 | 2026-07-18 | byte-preserved RFC text | Document-embedded IETF Trust copyright and legal-provisions notice |

The machine-readable catalog is [`sources/catalog.csv`](../sources/catalog.csv) and [`sources/catalog.json`](../sources/catalog.json).

## Material archival limitation

The systemd `man/crypttab.xml` source was inspected at `v261.1`, tag commit `eff9446d505d62c075bed37d606860b38cfe51fb`, with Git blob SHA-1 `38ba4ceafbe80aead3025bc0866a3573a0a36ad5`. The connected file-download transport rejected the XML media type, so this bundle contains a clearly marked derivative extract and exact source reference—not a byte-identical raw XML copy. Re-fetch and verify the tagged blob before using it as raw evidentiary source.

The cryptsetup 2.8.6 official release-archive SHA-256 is recorded, but the tarball and detached signature were not embedded or independently verified in this run. The selected tagged manual sources and release notes are locally hashed.

## Revision cautions

* Linux documentation is pinned to `v7.2-rc3`, matching the current documentation snapshot accessed on 2026-07-18; it is a release-candidate revision.
* cryptsetup is pinned to the then-current stable `2.8.6`, not the `2.8.7-rc2` pre-release visible in the official archive.
* systemd is pinned to `v261.1` and its exact tag commit.
* NIST PDFs are final publications as named; draft replacements are not treated as final requirements.
* RFC text is pinned by RFC number; errata and later updates may affect implementation interpretation and should be reviewed at implementation time.
* FIPS 198-1 is retained as a historical HMAC source; the catalog and implementation decision must use the current NIST status at deployment time rather than infer validation from the copied PDF.

## Hashes and manifests

* `manifests/SOURCE_SHA256SUMS`: all files in `sources/`.
* `manifests/SHA256SUMS`: all generated and copied files outside `manifests/`.
* `manifests/files.json`: path, size, and SHA-256 inventory.
* `manifests/manifest-root.json`: hashes of the manifests and bundle metadata.
* `validation/verify_bundle.py`: offline verifier.

A SHA-256 match proves byte consistency with this bundle's manifest. It does not prove publisher authenticity unless the upstream signature or immutable source identity was independently verified and recorded.

## License handling

`LICENSE-SYNTHESIS.txt` applies only to original analysis and static-wiki code. Third-party source material retains its own terms. See `sources/LICENSE-METADATA.md`, `THIRD_PARTY_NOTICES.md`, copied license files, and document-embedded notices.
