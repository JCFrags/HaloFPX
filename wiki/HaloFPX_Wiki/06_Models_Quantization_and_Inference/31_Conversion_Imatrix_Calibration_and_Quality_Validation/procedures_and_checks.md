---
section_id: "31"
title: "Reproducible conversion and validation procedure"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["charlie12345/ROCmFPX", "ggml-org/llama.cpp"]
  software_versions: ["a5605a7", "788e07d"]
  hardware_revisions: []
related_sections: ["30", "76", "78"]
---

# Procedure

Prerequisites: pinned checkout, immutable checkpoint, dependency lock/environment, free space checked, non-root shell. Paths below are examples and create new artifacts.

## 1. Inventory and convert

```bash
git rev-parse HEAD
sha256sum /models/source/config.json /models/source/tokenizer* /models/source/*.safetensors > source.sha256
python3 convert_hf_to_gguf.py /models/source \
  --outfile /staging/model-BF16.gguf --outtype bf16
sha256sum /staging/model-BF16*.gguf > bf16.sha256
```

Capture full stdout/stderr and dependency versions. Audit metadata, tensor names/counts/shapes, special tokens, BOS/EOS, added tokens, chat template, RoPE/scaling, context, experts and MTP/recurrent metadata.

## 2. Build a resumable imatrix

```bash
./build/bin/llama-imatrix -m /staging/model-BF16.gguf \
  -f /calibration/mixture.txt -o /staging/imatrix.gguf \
  --output-frequency 10 --save-frequency 50 --parse-special -ngl 999
./build/bin/llama-imatrix --in-file /staging/imatrix.gguf --show-statistics
sha256sum /calibration/mixture.txt /staging/imatrix.gguf
```

If interrupted, use the last complete snapshot and `--chunk`/`--from-chunk`, or merge disjoint completed imatrices with repeated `--in-file`. Record exact chunk ranges to avoid silent duplication.

## 3. Quantize without overwriting

```bash
./build/bin/llama-quantize --dry-run --imatrix /staging/imatrix.gguf \
  /staging/model-BF16.gguf /staging/model-Q4_K_M.gguf Q4_K_M
./build/bin/llama-quantize --imatrix /staging/imatrix.gguf --keep-split \
  /staging/model-BF16.gguf /staging/model-Q4_K_M.gguf Q4_K_M
sha256sum /staging/model-Q4_K_M*.gguf > quant.sha256
```

For ROCmFPX use Section 30's wrapper at pinned commit. Do not add `--allow-requantize` unless the experiment explicitly studies requantization.

## 4. Validate

1. Tokenizer golden vectors: plain text, Unicode, code, whitespace, every special/tool token and chat template.
2. Source vs BF16 GGUF logits on fixed short sequences; investigate any tolerance failure.
3. BF16 vs each quant on the same PPL/KLD corpus; record raw per-token/per-chunk output.
4. Paired coding, tool-call/schema, instruction, long-context retrieval, reasoning, and model-family tasks.
5. CPU, HIP, Vulkan and both nodes with identical prompt/seed/options; capture fallbacks and memory.
6. State save/restore and corruption/mismatch rejection.

Promotion requires all hard gates and an approved tier. Failed artifacts remain immutable evidence or are clearly quarantined; never silently replace the current model.

