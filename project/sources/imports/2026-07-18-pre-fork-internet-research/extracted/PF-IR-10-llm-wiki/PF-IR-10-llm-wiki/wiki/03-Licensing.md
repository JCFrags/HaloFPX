# Licensing and redistribution

## Accepted classes

| Class | License | Evidence |
|---|---|---|
| Generated fixtures, recipes, comparators, wiki | CC0-1.0 | `evidence/licenses/CC0-1.0.txt` |
| Pinned llama.cpp source/license evidence | MIT | `evidence/licenses/llama.cpp-LICENSE.txt` |
| Pinned ROCmFPX source/license evidence | MIT | `evidence/licenses/ROCmFPX-LICENSE.txt`; third-party notice |
| Pinned CachyLLama source/license evidence | MIT | `evidence/licenses/CachyLLama-LICENSE.txt` |

ROCmFPX’s notice expressly separates source-code licensing from model-weight licensing. That separation is applied throughout this corpus. Selected upstream source-range captures are redistributed under the corresponding candidate repository’s captured MIT license; they are evidence only and were not compiled or executed.

## Excluded upstream binaries

| Asset | Disposition | Reason label |
|---|---|---|
| `ggml-org/vocabs` binary assets | excluded | `EXCLUDED-LICENSE-UNCLEAR` |
| `stories15M-q4_0.gguf` | excluded | `EXCLUDED-PROVENANCE-GAP` |
| Big-endian stories15M derivative | excluded | `EXCLUDED-PROVENANCE-GAP` |
| Publisher MTP/EAGLE/DFlash/draft weights | excluded | `EXCLUDED-LICENSE-UNCLEAR` |

A checksum, an MIT source project, or a model reference does not by itself establish the exact converted file’s redistribution chain. See `manifests/excluded-assets.json` and `evidence/raw/research/upstream-asset-audit.json`.

## Per-fixture evidence

Every fixture manifest row records `license`, `license_evidence`, file SHA-256, and a specific locator.
