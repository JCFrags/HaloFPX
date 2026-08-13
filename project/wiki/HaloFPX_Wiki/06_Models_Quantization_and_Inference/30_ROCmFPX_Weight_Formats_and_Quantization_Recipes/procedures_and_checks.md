---
section_id: "30"
title: "ROCmFPX quantization procedures"
status: "needs-machine-validation"
last_verified: "2026-08-12"
applies_to:
  repositories: ["charlie12345/ROCmFPX", "JCFrags/HaloFPX"]
  software_versions: ["a5605a7", "4a156395db62604cf37e27e6459e3ee0e3949c48", "6c88472bf5f567a1064f27f4d8a90fc8e2b47a02", "b77f2bce6e7875ab065e09894f45915585c9f156"]
  hardware_revisions: ["gfx1151 pending"]
related_sections: ["31", "37"]
---

# Safe qualification procedure

Prerequisites: pinned ROCmFPX checkout, BF16/F16/F32 GGUF, enough RAM/disk, non-root shell.

```bash
git rev-parse HEAD
scripts/check-rocmfpx-reference.sh
cmake --build build-strix-rocmfp4 --target llama-quantize test-backend-ops -j 8
build-strix-rocmfp4/bin/test-backend-ops test -o MUL_MAT,MUL_MAT_ID,GET_ROWS,CPY,SET_ROWS -b ROCm0
build-strix-rocmfp4/bin/test-backend-ops test -o MUL_MAT,MUL_MAT_ID,GET_ROWS,CPY,SET_ROWS -b Vulkan0
```

Q2 is a narrower lane. Build and run its frozen CPU reference, then restrict
CUDA/HIP static/runtime qualification to admitted operations. Test generic
same-type contiguous copy separately; do not request Q2 conversion or
noncontiguous `CPY`, `SET_ROWS`, Vulkan, common-CLI cache, or agent-wrapper
coverage:

```bash
scripts/check-rocmfp2-reference.sh
build-strix-rocmfp4/bin/llama-quantize \
  /models/model-BF16.gguf /models/model-Q2_0_ROCMFPX.gguf Q2_0_ROCMFPX
build-strix-rocmfp4/bin/test-backend-ops test \
  -o MUL_MAT,MUL_MAT_ID,GET_ROWS -b ROCm0
```

Dry-run size before writing an artifact:

```bash
DRY_RUN=1 FORMAT=rocmfp4 PROFILE=strix-lean \
  SRC=/models/model-BF16.gguf OUT=/models/model-ROCmFP4.gguf \
  scripts/quantize-rocmfpx-agent.sh
```

Production candidate conversion:

```bash
FORMAT=rocmfp4 PROFILE=strix-lean KEEP_SPLIT=1 \
  IMATRIX=/evidence/imatrix.gguf \
  SRC=/models/model-BF16.gguf OUT=/models/model-ROCmFP4-STRIX_LEAN.gguf \
  scripts/quantize-rocmfpx-agent.sh
sha256sum /models/model-ROCmFP4-STRIX_LEAN*.gguf
```

Record quantizer stdout, source/output hashes, byte sizes, actual BPW, tensor-type histogram, imatrix hash, build flags, and recipe ID. Run CPU plus HIP/Vulkan backend-op tests and Section 31 quality gates. Root is not required.

## Portable small-fixture control

For an off-target conversion/load control that does not require the primary
model, use the [Qwen3-0.6B portable fixture recipe](../../../../../docs/halofpx/fixtures/qwen3-0.6b-rocmfpx/README.md).
It downloads an immutable BF16 GGUF, requires the exact compatible artifact-
producer commit, emits pure Q3/Q6/Q8 ROCmFPX files through verified partials,
checks their hashes and tensor census, then smokes them with exact pinned b77
on CPU [S30-L02]. This lane is not a quality or performance gate and cannot
replace HIP/Vulkan or target-machine qualification.

**[RECOMMENDATION]** Reject any artifact with invalid scale bytes, unexpected tensor fallback/type, unexplained tensor omission, non-finite output, backend-op failure, or a quality regression beyond its approved tier.
