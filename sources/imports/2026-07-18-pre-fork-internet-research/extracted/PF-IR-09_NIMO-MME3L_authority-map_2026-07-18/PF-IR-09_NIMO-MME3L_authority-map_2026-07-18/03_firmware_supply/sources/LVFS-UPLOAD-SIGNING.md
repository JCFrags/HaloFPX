# LVFS upload, detached signing, and remote visibility model

- **Source ID:** `LVFS-UPLOAD-SIGNING`
- **Authority:** LVFS documentation
- **Canonical URL:** https://lvfs.readthedocs.io/en/latest/upload.html
- **Access date:** 2026-07-18
- **Source type:** Official project documentation
- **Applicability label:** [OFFICIAL]
- **Confidence:** High

## Decision-relevant official claims

- [OFFICIAL] LVFS repacks an uploaded cabinet and applies a detached GPG or PKCS#7 signature so clients can verify LVFS origin.
- [OFFICIAL] Private and embargo remotes are not public; testing and stable remotes are public.
- [OFFICIAL] Existing Windows Update signatures are copied, while the LVFS detached signature is the Linux distribution trust layer.

## Explicit gaps and limits

- LVFS repository signing is not proof of vendor payload signing or device-side authentication.
- It does not prove hardware anti-rollback.

## License / reuse

LVFS documentation © Richard Hughes / project contributors; factual extract with attribution. Check site/source license for broader reuse.

## Capture note

This is an access-dated normalized factual capture unless a raw artifact path is explicitly identified. It is not represented as a byte-for-byte server response.
