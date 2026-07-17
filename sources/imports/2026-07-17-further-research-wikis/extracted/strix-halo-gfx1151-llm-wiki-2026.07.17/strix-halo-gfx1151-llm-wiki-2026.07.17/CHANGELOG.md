# Changelog

## 2026.07.17

Initial versioned research snapshot.

- Added 21 compatibility profiles with separate official, upstream, community, experimental, historical, unsupported, and candidate classifications.
- Reconciled ROCm 7.14 Core SDK support with the narrower ROCm 7.2 Ryzen and RDNA 3.5 application/prebuilt scopes.
- Added kernel and firmware gates, including the 20251125/MES 0x83 regression.
- Pinned llama.cpp b10064 / 86d86ed and its verified ROCm 7.2 binary checksum.
- Added exact HIP, Vulkan, dual-backend, and ROCmFPX build flags.
- Added Mesa 26.1.5 source pin and SHA-256 while retaining 26.0.2/26.0.3 as the captured community validation set.
- Added standard USB4 IP and research-only USB4 verbs profiles.
- Added offline site, search, machine-readable matrices, diagnostics, containers, link checking, and release manifest.
