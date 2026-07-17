---
section_id: "32"
title: "Lifecycle tracing and validation"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["ggml-org/llama.cpp", "charlie12345/ROCmFPX"]
  software_versions: ["788e07d", "a5605a7"]
  hardware_revisions: []
related_sections: ["33", "61", "68", "76"]
---

# Trace procedure

Prerequisites: exact binary/model hashes, debug logging build if needed, non-root shell.

1. Capture build commit/options, loaded backend registry, model shard hashes and GGUF metadata.
2. Start one slot at small context with deterministic sampling; save complete startup log.
3. Record each tensor's selected buffer/backend and size; summarize CPU fallbacks.
4. Trace prompt and single-token decode separately: batch/ubatch, graph nodes, scheduler splits/copies, allocation/reserve, compute/sync and output copies.
5. Repeat identical prompt to distinguish server prompt-cache reuse from graph reuse.
6. Save state after a prefix; continue N tokens; restore into a clean compatible slot and require the same continuation.
7. Mutate one fingerprint field and one payload byte; both must reject or recompute, never accept corrupt state.
8. Repeat on HIP/Vulkan and both nodes, then with the intended two-rank mode. Verify cancellation and remote-rank loss clean all slot/rank resources.

Representative commands (actual debug flags may differ by build):

```bash
git rev-parse HEAD
sha256sum build/bin/llama-server model*.gguf
./build/bin/llama-server -m model.gguf -c 4096 -np 1 -ngl 999 \
  --metrics --host 127.0.0.1 --port 8080
curl -sS http://127.0.0.1:8080/metrics
```

**[RECOMMENDATION]** An integration gate passes only if no unexplained fallback/copy occurs, graph/state fingerprints are complete, restore is deterministic, corruption is rejected, cancellation frees rank resources, and single-node fallback produces a valid response or explicit pre-execution error.

