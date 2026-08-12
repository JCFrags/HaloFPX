# Glossary

**gfx1151** — LLVM/ROCm target identifier used by Strix Halo’s RDNA 3.5 GPU.

**amdgpu** — Linux kernel DRM/compute driver for AMD GPUs.

**MES** — Micro Engine Scheduler firmware and queue-management path implicated in several historical hangs.

**CWSR** — Compute Wave Save/Restore; a legacy ROCm 7.1 issue used `amdgpu.cwsr_enable=0` as containment.

**ROCm** — AMD’s open GPU compute software platform.

**HIP** — ROCm’s C++ runtime and programming model; current llama.cpp requires HIP 6.1 or newer.

**ROCr** — ROCm runtime layer implementing HSA services.

**RADV** — Mesa’s open-source AMD Vulkan driver.

**AMDVLK** — AMD’s open-source Vulkan driver distinct from Mesa RADV.

**ICD** — Installable Client Driver manifest used by the Vulkan loader.

**VMM** — Virtual memory management API. Current upstream llama.cpp defaults `GGML_HIP_NO_VMM=ON`.

**TTM/GTT** — Linux GPU memory-management limits used to expose a controlled portion of unified system memory.

**rocWMMA** — ROCm wave matrix multiplication library; llama.cpp’s rocWMMA FlashAttention option is left OFF in the gfx1151 baseline.

**ROCmFPX** — Experimental community GGUF quantization and kernel family in the pinned fork.

**ThunderboltIP** — IP networking protocol over Thunderbolt/USB4, implemented by Linux `thunderbolt_net`.

**RXE** — Software RoCE implementation over a normal IP-capable network interface.

**DKMS** — Framework that rebuilds an out-of-tree kernel module for each installed kernel.
