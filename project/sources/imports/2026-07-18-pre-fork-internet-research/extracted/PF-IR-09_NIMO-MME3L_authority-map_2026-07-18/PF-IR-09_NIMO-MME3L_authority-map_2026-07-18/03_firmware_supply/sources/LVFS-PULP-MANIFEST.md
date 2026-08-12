# LVFS public PULP_MANIFEST and literal target search

- **Source ID:** `LVFS-PULP-MANIFEST`
- **Authority:** Linux Vendor Firmware Service
- **Canonical URL:** https://cdn.fwupd.org/downloads/PULP_MANIFEST
- **Access date:** 2026-07-18
- **Source type:** Official public repository manifest; raw artifact preserved
- **Applicability label:** [OFFICIAL] [OPEN]
- **Confidence:** High for literal filename search; Low for device applicability

## Decision-relevant official claims

- [OFFICIAL] Raw public manifest is preserved at `raw_official/lvfs/PULP_MANIFEST`.
- [OFFICIAL] The public PULP manifest can be used to mirror public LVFS content; private and embargo firmware are not included.
- [OPEN] No filename entry contains `NIMO`, `MME3L`, `P310`, `CT1000P310SSD2`, `VACR001`, or the exact PCI string `1002:1586`. The substring `1586` occurs 19 times only inside unrelated hashes, filenames, numeric identifiers, or size fields and does not establish target coverage.

## Explicit gaps and limits

- Manifest filenames are not the full LVFS GUID/instance-ID applicability catalog.
- Absence does not prove no LVFS match; local `fwupdmgr get-devices --json` GUIDs and current metadata are required.
- Private/embargo firmware is intentionally outside the public manifest.

## License / reuse

LVFS public firmware is intended for redistribution/mirroring under vendor metadata and firmware-specific licenses. Individual payload licenses must be checked; this raw manifest is retained with source attribution.

## Capture note

This is an access-dated normalized factual capture unless a raw artifact path is explicitly identified. It is not represented as a byte-for-byte server response.
