# Upstream Repository Inventory

> **Wiki status:** Proposed · **Evidence state:** D0 — design only · **Last reviewed:** 2026-07-17  
> **Machine-validation status:** Not run. Missing measurements are `INSUFFICIENT_EVIDENCE`, never an implicit pass.


| Surface | Canonical source | High-risk paths/terms |
|---|---|---|
| llama.cpp | `ggml-org/llama.cpp` | `tools/rpc`, `ggml-rpc`, `tools/server`, KV/cache, HIP/ROCm, Vulkan, tokenizer/model loaders, benchmark |
| ROCm meta/releases | `ROCm/ROCm` | releases, compatibility, `gfx1151`, Ryzen, HIP, LLVM, inference |
| ROCm build/nightly | `ROCm/TheRock` | `SUPPORTED_GPUS`, `gfx1151`, package/build regressions, PyTorch/llama.cpp artifacts |
| Telemetry/profilers | `ROCm/rocm-systems` | AMD SMI, systems profiler, compute profiler, rocprofiler SDK |
| Linux | `torvalds/linux`, `gregkh/linux` | `drivers/gpu/drm/amd`, `amdkfd`, `drivers/thunderbolt`, `thunderbolt-net`, USB4 ABI |
| Mesa | `mesa/mesa` on freedesktop GitLab | `src/amd`, RADV, `gfx1151`, release notes |
| LLVM | `llvm/llvm-project` and ROCm component pins | AMDGPU target, `gfx1151`, codegen, compiler regressions |
| vLLM/PyTorch | selected adapter repos | ROCm scheduler, metrics, attention/MoE, APU/gfx1151 support |
| Security | AMD Product Security, GitHub advisories, distro notices | APU/firmware/GPU/kernel/runtime vulnerabilities |
