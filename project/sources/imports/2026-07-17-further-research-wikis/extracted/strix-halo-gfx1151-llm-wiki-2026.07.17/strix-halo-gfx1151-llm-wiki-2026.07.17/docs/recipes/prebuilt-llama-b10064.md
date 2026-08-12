# Recipe: verified llama.cpp b10064 ROCm 7.2 binary

**Classification:** official upstream binary; host compatibility remains external  
**Source:** [LLAMA-RELEASE-B10064](../sources.md#llama-release-b10064)

```bash
../../scripts/download-llama-b10064-rocm72.sh
```

Pinned asset:

```text
URL: https://github.com/ggml-org/llama.cpp/releases/download/b10064/llama-b10064-bin-ubuntu-rocm-7.2-x64.tar.gz
SHA256: 42a00452f42b04598d32db66c5249b3e8855cd99bf9448e22fd2a738aaa89c82
```

Verify dynamic dependencies and devices:

```bash
cd downloads/llama-b10064-rocm72
ldd ./llama-cli | sort
./llama-cli --version
./llama-cli --list-devices
```

Run only on a host with a qualified kernel, fixed firmware, and the intended ROCm 7.2 runtime line. For ROCm 7.14, use the source-build recipe.
