---
section_id: "31"
title: "Conversion and calibration facts"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["charlie12345/ROCmFPX", "ggml-org/llama.cpp"]
  software_versions: ["a5605a7", "788e07d"]
  hardware_revisions: []
related_sections: ["29", "30"]
---

# Facts and constraints

- **[VERIFIED]** `convert_hf_to_gguf.py` dispatches by registered architecture and supports local or remote model input; its support must be checked at the exact commit [S31-02].
- **[VERIFIED]** `llama-quantize` accepts `--imatrix`, per-tensor regex overrides, output/embedding type overrides, split preservation, and a dry-run; `--allow-requantize` is explicitly risky [S31-01].
- **[VERIFIED]** `llama-imatrix` requires a model and text file, writes GGUF by default, saves progress periodically, can make snapshots, merge prior files, parse special tokens, resume from a chunk, and report statistics [S31-03].
- **[VERIFIED]** Imatrix values are derived from squared activations. The tool warns that statistics over squared rather than raw activations can make some measures misleading.
- **[VERIFIED]** `output.weight` imatrix collection is off by default because upstream reports it is typically better not to use it there.
- **[VERIFIED]** The ROCmFPX wrapper passes an imatrix to FP3/4/6/8 recipes; fork reference tests claim weighted scale-search coverage, but HaloFPX has not reproduced that claim [S31-04].

# Provenance minimum

Record source repo/revision, license, every source shard hash/size, config/tokenizer/chat-template hashes, converter commit and Python dependency lock, conversion command/log, BF16 GGUF shard hashes, calibration corpus IDs/licenses/hashes, imatrix command/hash/statistics, quantizer commit/command, resolved tensor-type manifest, output hashes, and machine/backend environment.

# Disk and recovery

**[RECOMMENDATION]** Before conversion, reserve source + BF16 GGUF + all candidate outputs + 20% temporary margin. Do not rely on parameter arithmetic alone; use actual shard totals and quantizer dry-run. Write outputs to a new path, hash them, then atomically promote a manifest pointer. Never overwrite the only BF16 source.

On interruption, retain logs and partial files with a `.partial` marker. Resume imatrix from a checkpoint/snapshot; restart conversion/quantization unless the tool has an explicit verified resume contract.

