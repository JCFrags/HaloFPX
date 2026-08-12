---
section_id: "30"
title: "ROCmFPX format facts and constraints"
status: "needs-machine-validation"
last_verified: "2026-08-12"
applies_to:
  repositories: ["charlie12345/ROCmFPX", "JCFrags/HaloFPX"]
  software_versions: ["a5605a7", "4a156395"]
  hardware_revisions: []
related_sections: ["31", "33", "37"]
---

# Exact tensor formats

All listed ROCmFP weight blocks cover 32 weights. BPW is structural block bytes times eight divided by 32; mixed presets have artifact-specific BPW.

| GGML/GGUF tensor type | Numeric ID | Payload and scale | Block bytes | BPW | Status in fork |
|---|---:|---|---:|---:|---|
| `Q4_0_ROCMFP4` | 100 | 16 packed E2M1-derived nibble bytes + two unsigned UE4M3 scales, one per 16 weights | 18 | 4.50 | promoted custom dual-scale layout |
| `Q4_0_ROCMFP4_FAST` | 101 | same nibble payload + one UE4M3 scale per 32 weights | 17 | 4.25 | speed layout, separate type |
| `Q6_0_ROCMFPX` | 102 | 24 packed 6-bit bytes + two UE4M3 scale bytes; signed range `[-32,31]`; Vulkan expands to 34 device bytes | 26 serialized | 6.50 | experimental family |
| `Q8_0_ROCMFPX` | 103 | 32 signed int8 values + one UE4M3 scale byte | 33 | 8.25 | experimental high-quality reference |
| `Q3_0_ROCMFPX` | 104 | 12 packed 3-bit bytes + two UE4M3 scale bytes; levels `0,+/-1,+/-2,+/-4` | 14 | 3.50 | experimental low-bit family |
| `TURBO3_0` | 105 | TurboQuant K/V type | fork documents 3.5 BPW | 3.5 | KV-only experiment, not weight preset |
| `TURBO4_0` | 106 | TurboQuant K/V type | fork documents 4.5 BPW | 4.5 | KV-only experiment, not weight preset |
| `Q2_0_ROCMFPX` | 107 | 8 packed 2-bit S40-codebook bytes (`-4,-1,+1,+4`) + two UE4M3 scale bytes | 10 | 2.50 | experimental; direct quantizer only; absent from common application cache CLI allowlist |

The first statement is the pinned-fork baseline and is preserved verbatim.
The 2026-08-12 statement that follows adds the Q2-specific exception that the
family-level wording did not express.

**[VERIFIED]** Scale validation rejects sign-bit scale bytes and `0x7f`; FP3/FP6 use reconstruction-MSE scale selection per 16-weight half-block, while FP8 uses one full-block scale [S30-02]. **[VERIFIED]** CPU reference routines and HIP/Vulkan code paths exist for the family, but repository claims of passing particular tests are not measurements from HaloFPX machines.

**[VERIFIED]** At HaloFPX `4a156395` on 2026-08-12, FP2/FP3/FP6 use
reconstruction-MSE scale selection per 16-weight half-block. Q2 has CPU plus
CUDA/HIP dequantization/`GET_ROWS`/MMVQ/MMQ wiring. Generic same-type contiguous
device copy remains available, but conversion/noncontiguous `CPY`, `SET_ROWS`,
and Vulkan are absent; Q3/Q4/Q4_FAST/Q6/Q8 have CPU/CUDA/HIP/Vulkan paths
[S30-L01]. Static wiring and repository test claims are not current target
performance results.

# Presets are mixtures

| Preset family | Resolved intent at pinned commit |
|---|---|
| `Q4_0_ROCMFP4` | dual-scale base with protected projections selected in `llama-quant.cpp` |
| `..._LEAN` | ROCmFP4 dense tensors; token embeddings protected with `Q5_K` |
| `..._COHERENT` | token embeddings protected with `Q6_K` |
| `..._FAST` | single-scale 4.25-BPW base |
| `..._FAST_COHERENT` | FAST transformer tensors plus `Q6_K` embeddings |
| `..._STRIX` | FAST bulk tensors, dual-scale attention K/V, `Q6_K` embeddings |
| `..._STRIX_LEAN` | STRIX attention protection; `Q5_K` embeddings/output |
| `Q3/Q6/Q8_0_ROCMFPX_AGENT` | architecture-sensitive attention/FFN boosts layered over base policy |
| `Q6_0_ROCMFPX_LEAN`, `..._AGENT_LEAN` | compact Q6 mixtures; exact tensor rules are source authority |

**[VERIFIED]** Ftype IDs are distinct from tensor-type IDs and run from 100 upward in the fork [S30-03]. Do not write an ftype value into GGUF tensor metadata or assume a preset's name equals every tensor's storage type.

# Quality constraints

- Requantizing already quantized weights can compound error; prefer BF16/F16/F32 sources.
- Protected tensor policies were tuned on particular models. **[INFERENCE]** They may not transfer across dense, MoE, MLA, recurrent, multimodal, or MTP topologies.
- Nominal block BPW excludes metadata and higher-precision tensors.
- Weight quantization does not reduce runtime KV state unless a cache type is separately selected.
- Q2 is absent from the common application cache-type CLI allowlist.
  Lower-level callers such as `llama-bench` can pass an arbitrary parsed GGML
  type, so this is not a universal runtime prohibition. Q3/Q4/Q4_FAST/Q6/Q8
  are parsed as common-CLI cache candidates, but Q3 K cache is promoted to Q6
  by the current safety policy. Turbo3/4 remain dedicated cache types, not
  model weights.

