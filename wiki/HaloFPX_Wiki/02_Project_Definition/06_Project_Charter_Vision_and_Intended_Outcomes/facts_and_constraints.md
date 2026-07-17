---
section_id: "06"
title: "Charter Facts and Constraints"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["charlie12345/ROCmFPX", "fewtarius/CachyLLama", "fewtarius/llama-ai", "ggml-org/llama.cpp"]
  software_versions: ["pinned heads listed in sources.md"]
  hardware_revisions: ["two matched AMD Strix Halo systems; exact BOM open"]
related_sections: ["08", "09", "17", "18", "38", "49"]
---

# Facts and constraints

## Verified source facts

- **[VERIFIED]** ROCmFPX is an experimental llama.cpp-derived family with AMD-focused GGUF weight formats, CPU reference paths, and accelerated HIP/ROCm and Vulkan paths. Its own README warns that APIs, tuning, and performance can change [S06-01].
- **[VERIFIED]** The pinned ROCmFPX README reports local `gfx1151` results, not universal performance. This is evidence that the target is plausible, not a HaloFPX baseline [S06-01].
- **[VERIFIED]** Upstream `llama-server` exposes OpenAI-compatible routes, parallel decoding, continuous batching, speculative decoding, metrics, health, and slot-cache operations [S06-02].
- **[VERIFIED]** CachyLLama declares SSD-backed KV checkpoints, per-user cache namespacing/concurrency, slot affinity, and MoE expert telemetry. Its published measurements are from specified machines and configurations only [S06-03].
- **[VERIFIED]** `llama-ai` is explicitly aimed at offline AMD APU agent workloads and supplies scripts, detection, and benchmark artifacts around its CachyLLama submodule [S06-04].
- **[VERIFIED]** The Agent Harness authority requires evidence to flow through sources to wiki and forbids treating memory or repository claims as global truth [S06-05].

## Project premises not yet proven

- **[ASSUMPTION]** The two systems are sufficiently matched for symmetric execution modes.
- **[ASSUMPTION]** The two USB4 paths can operate independently and provide stable, useful aggregate transport behavior.
- **[ASSUMPTION]** The target models and quantizations meet an acceptable task-quality floor.
- **[ASSUMPTION]** Persistent cache gains outweigh restore, integrity, privacy, and endurance costs for target workloads.
- **[OPEN]** Exact ownership, budget, schedule, deployment audience, and risk tolerance have not been recorded.

## Hard constraints

1. Measurements must retain model hash, code commit, backend, prompt shape, context, concurrency, power/thermal state, and raw output.
2. A corrupted or incompatible cache entry must miss or be recomputed; it must never be accepted as valid state.
3. Distributed documentation must identify rank ownership, failure behavior, and single-node fallback.
4. No source benchmark establishes the dual-node product’s performance.

