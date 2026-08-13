# Qwen3-0.6B ROCmFPX modification and provenance notice

This notice accompanies three modified model artifacts derived from the pinned
`unsloth/Qwen3-0.6B-GGUF` distribution. The distribution model card declares
Apache-2.0 and names `Qwen/Qwen3-0.6B` as the base model. The exact upstream
base-checkpoint revision used by the publisher is not encoded in that card.

Source authority:

- repository: `unsloth/Qwen3-0.6B-GGUF`;
- revision: `28675487b4ea2d7766af79bf32527c73ec715cae`;
- file: `Qwen3-0.6B-BF16.gguf`;
- size: `1198182848` bytes;
- SHA-256: `f9c9f1d3c1e21755b82d4e165f88dbbbd4355646d632fb5d6cef7c66ed4ee04e`.

Modification performed on 2026-08-12 (America/Los_Angeles): the source weights
were converted with `llama-quantize --pure` from HaloFPX commit
`6c88472bf5f567a1064f27f4d8a90fc8e2b47a02` into these custom ROCmFPX GGUF
formats:

- `Qwen3-0.6B-Q3_0_ROCMFPX-pure.gguf`, `266957248` bytes, SHA-256
  `d1404c1afc61ffe49357c14c6d3dbfb252a72e87744fb7e491e7a2e205321fff`;
- `Qwen3-0.6B-Q6_0_ROCMFPX-pure.gguf`, `490451392` bytes, SHA-256
  `8d5c0eb545651c7518508632d9c00138cb64c22902eb83f5b8d7d52cf5fae8cc`;
- `Qwen3-0.6B-Q8_0_ROCMFPX-pure.gguf`, `620822976` bytes, SHA-256
  `ec152fed6e498cad29e75c32e11c8d520fed34bea26c5ad5bfef8a4e210a4bd7`.

The files are changed quantized derivatives, not publisher-original weights.
The exact license text, pinned distribution card and config, registry, and
byte/hash manifest must remain beside any redistributed copy. HaloFPX makes no
model-quality, inference-performance, HIP, Vulkan, single-node Strix Halo, or
dual-node compatibility claim for these off-target fixture artifacts.
