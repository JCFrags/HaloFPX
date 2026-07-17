---
section_id: "29"
title: "Model admission procedures"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["charlie12345/ROCmFPX", "ggml-org/llama.cpp"]
  software_versions: ["a5605a7", "788e07d"]
  hardware_revisions: []
related_sections: ["31", "33", "76"]
---

# Admission procedure

Prerequisites: non-root shell, pinned source checkpoint, target binary commit, sufficient free disk. Commands are read-only except for explicitly named output files.

1. Record immutable source revision, license file, config, tokenizer files, chat template, shard list, byte counts, and SHA-256 hashes.
2. Confirm architecture in the exact converter and graph source; do not infer support from the enum alone.
3. Convert to BF16/F16 GGUF per Section 31, inspect with `llama-gguf`, and compare tensor count/names/shapes and tokenizer probes.
4. Run CPU, HIP, then Vulkan short deterministic prompts with logs captured. Record any CPU fallback operations.
5. Run state save/restore at multiple prefix lengths. Compare continuation tokens/logits under deterministic settings.
6. Run quality gates against the publisher/source framework and BF16 GGUF.
7. Repeat on both Strix Halo nodes; record firmware, ROCm/Mesa, backend, build flags, memory, context, slot count, and cache types.

Example inventory commands:

```bash
git rev-parse HEAD
sha256sum config.json tokenizer.json tokenizer_config.json model*.safetensors
python3 -m gguf.scripts.gguf_dump model-BF16.gguf --no-tensors
./build/bin/llama-cli -m model-BF16.gguf -ngl 0 -n 32 -s 1 -p 'Return only: OK'
./build/bin/llama-cli -m model-BF16.gguf -ngl 999 -n 32 -s 1 -p 'Return only: OK'
```

**[RECOMMENDATION]** Fail admission if source identity is mutable/unknown, the license is unresolved, conversion drops unexplained tensors, tokenizer probes differ, required ops fall back unexpectedly, state restore diverges, or Section 31 quality gates fail.
