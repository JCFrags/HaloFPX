# Methodology and evidence policy

**Access date:** 2026-07-18. **Priority:** P1.

The measured product, board, firmware, PCI, SSD, and graphics identities were used as search keys. Authority order was: OEM support and product pages; AMD Product Security; official AMD/ROCm documentation; Crucial/Micron support and product literature; LVFS/fwupd; linux-firmware; upstream Linux documentation and source.

Applicability was not inferred merely from similar names. A product row is marked exact only where the authority names `AMD Ryzen AI Max 300 Series`, `AMD Ryzen AI MAX`, `Strix Halo`, `StrixHaloPI-FP11`, or another exact measured identifier. Broader architecture and interface documents are marked conditional.

A release date from AMD to an OEM is not proof that NIMO integrated the PI into BIOS 3.05. A public repository filename or detached metadata signature is not proof that a device authenticates the payload or enforces anti-rollback. A generic kernel facility is not proof that target hardware exposes its counters.

## Capture model

Binary official artifacts available to this environment are preserved byte-for-byte. Official web pages are represented by normalized, access-dated text captures containing source URL, title, exact product/version rows, claim labels, and capture limits. Server-side raw HTML could not be exported from the browsing subsystem and is therefore not represented as a verbatim raw response. See `raw_official/NORMALIZED_CAPTURE_LIMITATION.md`.

## Confidence

- **High:** exact product/family named in an official table and the row is unambiguous.
- **Medium:** official family mapping or upstream source supports a conditional conclusion but local capability/version proof is absent.
- **Low:** identifier association is plausible but not authoritatively mapped, or only a public-index absence was observed.
