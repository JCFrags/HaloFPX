# G0A candidate source-lock receipt

Date: 2026-07-17

Reason: preserve a reproducible, offline-capable evidence package for the accepted L00A local source/provenance-freeze lane.

| Action | Source | Destination | Status | Notes |
|---|---|---|---|---|
| Inventory ROCmFPX | read-only reference clone | repository-records/charlie12345__rocmfpx--record | verified | Candidate and research-control objects preserved |
| Inventory llama-ai | read-only reference clone | repository-records/fewtarius__llama-ai--record | verified | CachyLLama gitlink recorded; not initialized or executed |
| Inventory CachyLLama | read-only reference clone | repository-records/fewtarius__cachyllama--record | verified | Donor head and upstream comparison parent preserved |
| Inventory llama.cpp | read-only reference clone | repository-records/ggml-org__llama.cpp--record | verified | Captured head and both comparison controls preserved |
| Create offline bundles | all refs locally present | bundles/ | verified | Four bundle verify checks passed |
| Record relevant deltas | five exact commit pairs | patch-ids/ | verified | Aggregate and non-merge series patch IDs retained |
| Hash package | all package files except checksum file | SHA256SUMS.txt | verified | SHA-256; paths are package-relative |

## Bundle receipt

| Repository | Bytes | SHA-256 |
|---|---:|---|
| charlie12345/ROCmFPX | 36,143,527 | bcbe6cf910f4dd183d8ad96ea0d936ac85bd636a1cfe570179599d3fc5e307fa |
| fewtarius/llama-ai | 1,221,275 | 65915e35a7e628ecdea103d119efcf6f716eb3e9fbbdcaf2a3268812abd72dd4 |
| fewtarius/CachyLLama | 377,303,761 | fc9b1e067de688527580f9f5a2b297b277e076e131d37544dbd7e29456975d80 |
| ggml-org/llama.cpp | 430,919,351 | 66f5e48ec9b175c191d6a57eeef4d0da98e4fdb6ae86edc1a9d1bab40082623a |

## Verification

- Source checked: four repositories, all non-shallow; git fsck --full --strict exited successfully.
- Destination checked: package is inside sources/repositories/source-locks/2026-07-17-pre-fork.
- Count/hash checked: four bundles verified; every package artifact is covered by SHA256SUMS.txt except that checksum file itself.
- Clone preservation checked: before/after status and refs match for all four repositories.
- Related docs updated: sources/repositories/README.md routes to this package.
- Network and target systems: untouched.

OPEN-PIN-01 remains open. The package nominates 61f2f2d7bc4955e9bca821095ef69125837133b5 for later qualification but does not select it.
