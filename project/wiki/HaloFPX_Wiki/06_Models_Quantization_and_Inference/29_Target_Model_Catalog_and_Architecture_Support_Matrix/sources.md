---
section_id: "29"
title: "Model catalog sources"
status: "verified"
last_verified: "2026-08-12"
applies_to:
  repositories: ["charlie12345/ROCmFPX", "ggml-org/llama.cpp", "JCFrags/HaloFPX"]
  software_versions: ["a5605a7", "788e07d", "b77f2bce"]
  hardware_revisions: []
related_sections: ["30", "31", "33"]
---

# Sources

| ID | Primary source and revision | Supports | Limitation |
|---|---|---|---|
| S29-01 | [llama.cpp architecture registry](https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/src/llama-arch.h), commit `788e07dc91d266ad3162a1ce9037665656269689`, accessed 2026-07-16 | declared architecture identifiers | enum is not end-to-end compatibility |
| S29-02 | [Qwen2.5-Coder-32B config](https://huggingface.co/Qwen/Qwen2.5-Coder-32B-Instruct/blob/381fc969f78efac66bc87ff7ddeadb7e73c218a7/config.json), revision `381fc969...`; [model card](https://huggingface.co/Qwen/Qwen2.5-Coder-32B-Instruct/tree/381fc969f78efac66bc87ff7ddeadb7e73c218a7), accessed 2026-07-16 | dimensions, context, license | no HaloFPX validation |
| S29-03 | [Qwen3-30B-A3B config](https://huggingface.co/Qwen/Qwen3-30B-A3B/blob/ad44e777bcd18fa416d9da3bd8f70d33ebb85d39/config.json), revision `ad44e777...`; [model card](https://huggingface.co/Qwen/Qwen3-30B-A3B/tree/ad44e777bcd18fa416d9da3bd8f70d33ebb85d39), accessed 2026-07-16 | MoE/GQA dimensions and publisher parameter label | active count is publisher label |
| S29-04 | [DeepSeek-V3 config](https://huggingface.co/deepseek-ai/DeepSeek-V3/blob/e815299b0bcbac849fa540c768ef21845365c9eb/config.json); [technical report](https://arxiv.org/abs/2412.19437), revision `e815299b...`, accessed 2026-07-16 | MLA, MoE, MTP, context and dimensions | license must be read from exact checkpoint distribution |
| S29-05 | [Mistral Small 3.1 config](https://huggingface.co/mistralai/Mistral-Small-3.1-24B-Instruct-2503/blob/68faf511d618ef198fef186659617cfd2eb8e33a/config.json), revision `68faf511...`, accessed 2026-07-16 | text/vision config and Apache-2.0 metadata | multimodal path separately validated |
| S29-06 | [Nemotron-3-Nano config](https://huggingface.co/nvidia/NVIDIA-Nemotron-3-Nano-30B-A3B-BF16/blob/cbd3fa9f933d55ef16a84236559f4ee2a0526848/config.json), revision `cbd3fa9f...`; [model tree/license](https://huggingface.co/nvidia/NVIDIA-Nemotron-3-Nano-30B-A3B-BF16/tree/cbd3fa9f933d55ef16a84236559f4ee2a0526848), accessed 2026-07-16 | hybrid pattern, SSM state, MoE, long context | non-Apache license; ordinary KV formula invalid |
| S29-07 | [Tracked Qwen3-0.6B fixture source/provenance record](../../../../../docs/halofpx/fixtures/qwen3-0.6b-rocmfpx/README.md), including the pinned card/config/license captures; [Qwen3-0.6B GGUF file](https://huggingface.co/unsloth/Qwen3-0.6B-GGUF/blob/28675487b4ea2d7766af79bf32527c73ec715cae/Qwen3-0.6B-BF16.gguf); and [local fixture evidence](../../../../../docs/halofpx/evidence/2026-08-12-qwen3-0.6b-rocmfpx-fixture/README.md), accessed 2026-08-12 PDT / 2026-08-13 UTC | immutable BF16 identity, config/license declaration, pure Q3/Q6/Q8 identities and off-target CPU smoke | distribution card omits the exact upstream base-checkpoint revision; no quality, target backend, or performance qualification |
