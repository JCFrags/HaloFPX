---
section_id: "08"
title: "Scope Facts and Boundary Matrix"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["charlie12345/ROCmFPX", "fewtarius/CachyLLama", "fewtarius/llama-ai", "ggml-org/llama.cpp"]
  software_versions: ["commits in sources.md"]
  hardware_revisions: ["two-node Strix Halo target"]
related_sections: ["11", "13", "14", "15", "16", "23", "38", "49", "60"]
---

# Facts, constraints, and boundaries

## Verified dependency facts

- **[VERIFIED]** ROCmFPX is a llama.cpp-derived experimental codebase with CPU, Vulkan, and HIP/ROCm paths; its README recommends exact testing because behavior varies by hardware, driver, model, prompt, and recipe [S08-01].
- **[VERIFIED]** Upstream llama.cpp owns GGUF parsing, model graph/backend abstractions, tokenization, sampling, and the baseline server surface used as the compatibility reference [S08-02].
- **[VERIFIED]** CachyLLama adds runtime-configured persistent cache, user-isolation, and expert-telemetry behavior to its llama.cpp fork [S08-03].
- **[VERIFIED]** llama-ai provides end-to-end AMD APU scripts and carries CachyLLama as a submodule, rather than defining a neutral protocol standard [S08-04].
- **[VERIFIED]** No reviewed source establishes the proposed dual-USB4, two-rank HaloFPX runtime as an existing feature.

## Ownership boundary matrix

| Component | Owns | HaloFPX responsibility | Must not assume |
|---|---|---|---|
| Client application | Request construction, presentation, tool execution | Document supported API subset; validate/cancel requests | Every OpenAI client behaves identically |
| `llama-server`/llama.cpp | Baseline APIs, model/runtime abstractions | Minimize divergence; add distributed coordination deliberately | “Compatible” means complete API parity |
| ROCmFPX | AMD-oriented weight formats/kernels and reference paths | Pin, test, and carry reviewed integration patches | Fork benchmarks generalize to both nodes |
| CachyLLama | Candidate cache/isolation/telemetry implementation | Audit format, integrity, lifecycle, and import selected changes | SSD state is safe across arbitrary versions |
| llama-ai | Candidate scripts and workload evidence | Reuse only traceable, applicable operations | Its detected hardware/config is the HaloFPX BOM |
| Coordinator | Session admission, mode plan, client response, health synthesis | Define protocol, fencing, cancellation, fallback | A surviving rank can continue every mode |
| Rank worker | Rank-local model shard, compute, KV/recurrent state, cache | Enforce ownership and epoch/session identity | Rank state is transparently interchangeable |
| Transport | Framing, flow control, integrity, link health | Abstract dual-link policy and expose failures | Two physical ports are independent or additive |
| Linux/kernel/drivers | USB4/network/block/GPU primitives | Pin supported matrix and validate topology | A source build can compensate for unsupported firmware |
| Model tooling/files | Conversion, quantization, metadata, hashes/licenses | Admit only validated artifacts and templates | Model name implies exact compatible content |
| NVMe/storage | Durable bytes and filesystem behavior | Atomic publish, quota, integrity, eviction, backup | Persistence implies correctness or confidentiality |

## Initial scope choices

- **[RECOMMENDATION]** Linux-only v1; exact distribution, kernel, firmware, ROCm, and Mesa matrix remains Section 23 work.
- **[RECOMMENDATION]** Supported shapes: one node, matched two-node pair, trusted authenticated LAN clients. More than two ranks is out of scope.
- **[RECOMMENDATION]** HIP/ROCm and Vulkan are candidates; the per-model backend must be measured rather than fixed globally.
- **[OPEN]** Exact model architectures, context limits, formats, and client endpoints remain compatibility-matrix decisions.

