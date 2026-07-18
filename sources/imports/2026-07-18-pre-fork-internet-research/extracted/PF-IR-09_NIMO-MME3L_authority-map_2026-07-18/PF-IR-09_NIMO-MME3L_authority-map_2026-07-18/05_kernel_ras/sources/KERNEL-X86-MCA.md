# x86-64 machine-check interface

- **Source ID:** `KERNEL-X86-MCA`
- **Authority:** Linux kernel documentation
- **Canonical URL:** https://docs.kernel.org/arch/x86/x86_64/machinecheck.html
- **Access date:** 2026-07-18
- **Source type:** Official upstream documentation
- **Applicability label:** [SUPPORTED_IF_PRESENT]
- **Confidence:** High for generic x86 behavior; Low for exact bank decoding

## Decision-relevant official claims

- [OFFICIAL] Machine checks report CPU-detected internal hardware error conditions; corrected errors typically create log entries and uncorrected errors often cause a machine check and may panic.
- [OFFICIAL] Machine-check banks and subevents are CPU-specific.

## Explicit gaps and limits

- [UNKNOWN] Exact Strix Halo MCA banks, firmware-first/GHES routing, threshold interrupts, and decoded telemetry.
- Local rasdaemon/journal/trace and CPUID evidence required.

## License / reuse

Linux kernel documentation under kernel documentation licensing terms; preserve attribution.

## Capture note

This is an access-dated normalized factual capture unless a raw artifact path is explicitly identified. It is not represented as a byte-for-byte server response.
