---
section_id: "31"
title: "Conversion and validation sources"
status: "verified"
last_verified: "2026-07-16"
applies_to:
  repositories: ["charlie12345/ROCmFPX", "ggml-org/llama.cpp"]
  software_versions: ["a5605a7", "788e07d"]
  hardware_revisions: []
related_sections: ["30"]
---

# Sources

| ID | Primary source | Supports | Limitation |
|---|---|---|---|
| S31-01 | [llama.cpp quantize documentation](https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/tools/quantize/README.md), commit `788e07d`, accessed 2026-07-16 | two-stage flow, options, requant warning, disk/RAM guidance | example benchmark environment is not HaloFPX |
| S31-02 | [HF-to-GGUF converter](https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/convert_hf_to_gguf.py), [conversion registry](https://github.com/ggml-org/llama.cpp/tree/788e07dc91d266ad3162a1ce9037665656269689/conversion), accessed 2026-07-16 | current architecture dispatch and CLI | support is architecture/revision-specific |
| S31-03 | [imatrix documentation](https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/tools/imatrix/README.md), accessed 2026-07-16 | format, resume/merge, statistics and caveats | no universal calibration recipe or threshold |
| S31-04 | [ROCmFPX quant wrapper](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/scripts/quantize-rocmfpx-agent.sh), [reference test](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/scripts/check-rocmfpx-reference.sh), accessed 2026-07-16 | fork imatrix pass-through and local test entrypoint | fork claims not reproduced here |
| S31-05 | [importance matrix PR 4861](https://github.com/ggml-org/llama.cpp/pull/4861), merged 2024-01, accessed 2026-07-16 | method rationale and calibration sensitivity | historical discussion; current code is authoritative |

