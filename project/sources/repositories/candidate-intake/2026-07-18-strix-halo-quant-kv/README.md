# Strix Halo quantized-KV FlashAttention candidate intake

Status: preserved candidate evidence; not approved implementation.

## Source identities

| Item | Exact identity | Local preservation |
|---|---|---|
| Nathanw1014/llama.cpp combined branch | `strix-halo-fa-fixes@a18067a85e986f7798f43d98345ed5b86b55cf88`, tree `130e9cac828f8d8ef877d87ea9c192e24b07c9af` | clean reference clone and complete verified Git bundle |
| Vulkan patch range | `635cdd5fcc5b..4355d03e8608` | `patches/vulkan-coopmat1-dequant-transpose-and-fallback.patch` |
| HIP patch range | `4355d03e8608..2a24abc63` | `patches/hip-tile-quant-kv-dequant-on-load-and-tests.patch` |
| upstream Vulkan proposal | `ggml-org/llama.cpp#25494` | GitHub metadata and patch snapshot |
| Reddit report and MiniMax reproduction | [r/StrixHalo post](https://www.reddit.com/r/StrixHalo/comments/1uzqg5m/i_made_quantized_kv_cache_workable_on_strix_halo/) | URL and access record only; Reddit rejected the attempted local snapshot |

The candidate repository carries the llama.cpp MIT license. This record does not
approve a code import: file/commit-level P3 provenance, attribution, dependency
closure, and review remain required.

## Integrity manifest

| File | Bytes | SHA-256 |
|---|---:|---|
| `bundles/Nathanw1014__llama.cpp--strix-halo-fa-fixes.bundle` | 379353651 | `79C61718BD60ECCE3E5EA3919FB18D8C709B3D26032C5FE9D4F24055ADE3BC3F` |
| `patches/hip-tile-quant-kv-dequant-on-load-and-tests.patch` | 33907 | `66DDB0E33301AB7231C736BE318483E90CFBA813701FBEB5269F547A65C4F42D` |
| `patches/vulkan-coopmat1-dequant-transpose-and-fallback.patch` | 12882 | `AF351194081882C510D38BF86AEB4194BE03A86D389DA756CF2FB73DA1259FDA` |
| `source-snapshots/ggml-org-llama.cpp-pr-25494.patch` | 15600 | `FBF97DBED16F1F2F8E974335D1C87FE4D83C9217F7298C230C90A700969449D0` |
| `source-snapshots/github-pr-25494-metadata.json` | 19017 | `C2BBDD6D5BCE0EC6BAE65FB93E47FA7DE6525A3BEC5888F7ADFAE948092E32C9` |
| `source-snapshots/github-repository-metadata.json` | 15643 | `D9E2AF1D5F6C33B1CB50B8B3F2951F17D9AD27344B8EF7616366B00F2E91C635` |

`git bundle verify` reports a complete history containing
`refs/heads/strix-halo-fa-fixes` at the exact commit above. Imported scripts
were not executed.

## Compatibility observation

Read-only `git apply --check` against the canonical ROCmFPX reference at
`61f2f2d7bc4955e9bca821095ef69125837133b5` failed for both patch snapshots.
The Vulkan conflict is in `ggml-vulkan.cpp`; the HIP conflicts include
`fattn-tile.cuh` and `tests/test-backend-ops.cpp`. Therefore the candidate must
be manually ported in narrow backend-specific lanes while preserving ROCmFPX's
newer TurboQuant/FA routing. No patch was applied to ROCmFPX or HaloFPX.
