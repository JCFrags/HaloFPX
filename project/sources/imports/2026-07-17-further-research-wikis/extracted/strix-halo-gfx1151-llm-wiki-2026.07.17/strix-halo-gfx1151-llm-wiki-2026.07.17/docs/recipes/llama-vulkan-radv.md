# Recipe: pinned llama.cpp with Mesa RADV

**Classification:** official upstream components plus community gfx1151 validation  
**Preferred driver:** RADV  
**llama.cpp pin:** b10064 / 86d86ed4396b4130922f7b9af26e3d9fc11a591b  
**Sources:** [MESA-2615](../sources.md#mesa-2615), [LLAMA-ISSUE-24438](../sources.md#llama-issue-24438), [KYUZ0-TOOLBOX-A7C71E9](../sources.md#kyuz0-toolbox-a7c71e9)

## Distro Mesa lane

Install the distribution’s Vulkan loader, tools, and RADV driver:

```bash
# Debian/Ubuntu
sudo apt-get install -y build-essential cmake git glslc libvulkan-dev \
  mesa-vulkan-drivers ninja-build spirv-tools vulkan-tools

# Fedora
sudo dnf install -y cmake gcc-c++ git glslc mesa-vulkan-drivers \
  ninja-build vulkan-loader-devel vulkan-tools
```

Check the selected driver:

```bash
vulkaninfo --summary
find /usr/share/vulkan/icd.d /etc/vulkan/icd.d -maxdepth 1 -name '*.json' -print 2>/dev/null
```

When multiple AMD ICDs exist:

```bash
export VK_DRIVER_FILES=/usr/share/vulkan/icd.d/radeon_icd.x86_64.json
VK_LOADER_DEBUG=all vulkaninfo --summary 2>vulkan-loader.log
```

Verify the path before export. `VK_ICD_FILENAMES` is the older deprecated control.

## Build

```bash
LLAMA_COMMIT=86d86ed4396b4130922f7b9af26e3d9fc11a591b \
  ../../scripts/build-llama-vulkan.sh
```

Expanded flags:

```bash
cmake -S llama.cpp -B llama.cpp/build-vulkan -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DGGML_VULKAN=ON \
  -DGGML_HIP=OFF \
  -DGGML_CUDA=OFF \
  -DLLAMA_BUILD_TESTS=OFF \
  -DGGML_BUILD_TESTS=OFF
```

## Load and benchmark

```bash
./llama.cpp/build-vulkan/bin/llama-cli --list-devices
./llama.cpp/build-vulkan/bin/llama-cli \
  -m MODEL.gguf -dev Vulkan0 -ngl 999 -fa 1 -p test -n 64

./llama.cpp/build-vulkan/bin/llama-bench \
  -m MODEL.gguf -dev Vulkan0 -p 512 -n 128 -ngl 999 -fa 1
```

If model load fails:

```bash
./llama.cpp/build-vulkan/bin/llama-cli \
  -m MODEL.gguf -dev Vulkan0 -ngl 999 -fa 1 \
  --no-direct-io --no-mmap -p test -n 16
```

## Upgrade gate

A reported Mesa 26.0.2 setup experienced a substantial prompt-processing regression between llama.cpp commits while generation stayed stable. Pin both Mesa and llama.cpp and retain pp512 plus tg128 baselines. Do not accept an upgrade based only on token-generation speed.

Avoid AMDVLK for large-model default use until the exact model passes a single-allocation test; an approximately 2 GiB allocation limitation has been reported.
