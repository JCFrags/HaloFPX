---
section_id: "30"
title: "ROCmFPX format sources"
status: "verified"
last_verified: "2026-08-12"
applies_to:
  repositories: ["charlie12345/ROCmFPX", "JCFrags/HaloFPX"]
  software_versions: ["a5605a7", "4a156395db62604cf37e27e6459e3ee0e3949c48", "6c88472bf5f567a1064f27f4d8a90fc8e2b47a02", "b77f2bce6e7875ab065e09894f45915585c9f156"]
  hardware_revisions: []
related_sections: ["31", "33"]
---

# Sources

| ID | Primary source | Claims supported | Limitations |
|---|---|---|---|
| S30-L01 | HaloFPX source at `4a156395db62604cf37e27e6459e3ee0e3949c48`: `ggml/rocmfpx`, CPU/CUDA/HIP/Vulkan backends, quantizer, and common application cache parsing | current layouts, code ranges, Q2 operator omissions, common-CLI cache eligibility, Q6 Vulkan expansion | static source audit only; no current model/backend performance qualification |
| S30-L02 | [Tracked Qwen3-0.6B fixture source/provenance record](../../../../../docs/halofpx/fixtures/qwen3-0.6b-rocmfpx/README.md) and [conversion/load evidence](../../../../../docs/halofpx/evidence/2026-08-12-qwen3-0.6b-rocmfpx-fixture/README.md), exact producer `6c88472bf5f567a1064f27f4d8a90fc8e2b47a02`, exact smoke consumer `b77f2bce6e7875ab065e09894f45915585c9f156`, recorded 2026-08-12 PDT | pure Q3/Q6/Q8 artifact identities, census, producer compatibility boundary, Q3 repeat identity recorded after validation, and bounded off-target CPU load/generate smoke | Q6/Q8 were not repeated; no imatrix or quality result; no HIP, Vulkan, Strix Halo, distributed, cache, or performance qualification |
| S30-01 | [ggml.h custom type registry](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/include/ggml.h), commit `a5605a7`, accessed 2026-07-16 | exact numeric tensor/ftype IDs | fork-only IDs may collide after rebase |
| S30-02 | [ROCmFPX layouts](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/rocmfpx/README.md), [header](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/rocmfpx/rocmfpx.h), accessed 2026-07-16 | blocks, scales, BPW, code ranges and claimed ops | test claims are repository-scoped, not locally reproduced |
| S30-03 | [ROCmFP4 header](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/rocmfp4/rocmfp4.h), [format README](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/rocmfp4/README.md), accessed 2026-07-16 | dual/FAST block layouts and preset intent | included measurements are another environment |
| S30-04 | [quant policy implementation](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/src/llama-quant.cpp), accessed 2026-07-16 | authoritative per-category routing | policies evolve with code |
| S30-05 | [agent quant wrapper](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/scripts/quantize-rocmfpx-agent.sh), accessed 2026-07-16 | CLI mapping for formats/profiles/imatrix | wrapper omits FP2 and does not prove quality |
| S30-06 | [released recipe catalog](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/docs/recipes/README.md), accessed 2026-07-16 | architecture-qualified recipe contract | published artifacts are not automatically HaloFPX-approved |
