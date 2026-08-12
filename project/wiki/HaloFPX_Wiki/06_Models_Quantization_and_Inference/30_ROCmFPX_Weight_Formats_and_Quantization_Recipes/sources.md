---
section_id: "30"
title: "ROCmFPX format sources"
status: "verified"
last_verified: "2026-07-16"
applies_to:
  repositories: ["charlie12345/ROCmFPX"]
  software_versions: ["a5605a7"]
  hardware_revisions: []
related_sections: ["31", "33"]
---

# Sources

| ID | Primary source | Claims supported | Limitations |
|---|---|---|---|
| S30-01 | [ggml.h custom type registry](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/include/ggml.h), commit `a5605a7`, accessed 2026-07-16 | exact numeric tensor/ftype IDs | fork-only IDs may collide after rebase |
| S30-02 | [ROCmFPX layouts](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/rocmfpx/README.md), [header](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/rocmfpx/rocmfpx.h), accessed 2026-07-16 | blocks, scales, BPW, code ranges and claimed ops | test claims are repository-scoped, not locally reproduced |
| S30-03 | [ROCmFP4 header](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/rocmfp4/rocmfp4.h), [format README](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/rocmfp4/README.md), accessed 2026-07-16 | dual/FAST block layouts and preset intent | included measurements are another environment |
| S30-04 | [quant policy implementation](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/src/llama-quant.cpp), accessed 2026-07-16 | authoritative per-category routing | policies evolve with code |
| S30-05 | [agent quant wrapper](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/scripts/quantize-rocmfpx-agent.sh), accessed 2026-07-16 | CLI mapping for formats/profiles/imatrix | wrapper omits FP2 and does not prove quality |
| S30-06 | [released recipe catalog](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/docs/recipes/README.md), accessed 2026-07-16 | architecture-qualified recipe contract | published artifacts are not automatically HaloFPX-approved |

