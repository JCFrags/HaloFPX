# Recipe: official ROCm 7.14 Core SDK host

**Classification:** official Core SDK host plus application build candidate  
**Target:** gfx1151  
**Supported host lanes captured:** Ubuntu 24.04.4 HWE kernel 6.17; Ubuntu 26.04 GA kernel 7.0  
**Sources:** [AMD-CORE-714](../sources.md#amd-core-714), [AMD-INSTALL-714](../sources.md#amd-install-714)

## 1. Establish the supported host

```bash
. /etc/os-release
printf 'OS=%s %s\n' "$ID" "$VERSION_ID"
uname -r
cat /proc/cmdline
```

Do not proceed on a generic Ubuntu 24.04 point release without confirming that the machine is actually on the captured 24.04.4/HWE 6.17 lane. The ROCm 7.14 matrix uses the inbox kernel driver; avoid mixing arbitrary amdgpu DKMS and firmware packages into this profile.

## 2. Reject known-bad firmware

```bash
dpkg-query -W -f='${Package} ${Version}\n' 'linux-firmware*' 2>/dev/null || true
sudo dmesg -T | grep -Eai 'amdgpu|MES|firmware|gfxhub|page fault' | tail -n 200
```

Any package or local pin corresponding to the late-2025 `20251125` / MES 0x83 regression must be removed before compute validation. Run [`collect-diagnostics.sh`](../../scripts/collect-diagnostics.sh) to record firmware files and hashes.

## 3. Install build prerequisites

```bash
sudo apt-get update
sudo apt-get install -y \
  build-essential ca-certificates cmake curl git ninja-build pkg-config \
  libnuma-dev libssl-dev libstdc++-dev pciutils python3 python3-pip pipx \
  libvulkan-dev vulkan-tools
```

Ubuntu 24.04’s CMake 3.28.x is the conservative baseline. CMake 4.3.4 is current upstream but should be a separate qualification change.

## 4. Install the exact gfx1151 SDK tarball

```bash
sudo install -d -m 0755 /opt/rocm-7.14.0
sudo chown "$USER":"$USER" /opt/rocm-7.14.0

# Supply an independently obtained checksum when AMD publishes or your
# organization records one. ALLOW_UNVERIFIED=1 is intentionally explicit.
ROCM_PREFIX=/opt/rocm-7.14.0 \
ROCM_TARBALL_SHA256='YOUR_RECORDED_SHA256' \
  ../../scripts/install-rocm-714-tarball.sh
```

The official URL pinned by the script is:

```text
https://repo.amd.com/rocm/tarball-multi-arch/therock-dist-linux-gfx1151-7.14.0.tar.gz
```

## 5. Activate one SDK root

```bash
cat > "$HOME/.config/strix-rocm-7.14.env" <<'EOF'
export ROCM_PATH=/opt/rocm-7.14.0
export PATH="$ROCM_PATH/bin:$PATH"
export LD_LIBRARY_PATH="$ROCM_PATH/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export HIPCXX="$ROCM_PATH/llvm/bin/clang"
export HIP_PATH="$ROCM_PATH"
unset HSA_OVERRIDE_GFX_VERSION
EOF

. "$HOME/.config/strix-rocm-7.14.env"
```

Check that every command resolves under one root:

```bash
command -v hipconfig rocminfo clang
hipconfig --full
"$ROCM_PATH/llvm/bin/clang" --version
ldd "$ROCM_PATH/bin/rocminfo" | grep -E 'rocm|hsa|hip' || true
```

## 6. Configure unified memory using AMD’s tool

Keep BIOS dedicated VRAM small unless a particular firmware requires otherwise. Install and inspect the AMD helper:

```bash
pipx install amd-debug-tools
amd-ttm
cat /sys/module/ttm/parameters/pages_limit
```

Choose a limit that leaves adequate RAM for the OS, page cache, model conversion, and networking. Apply the value through the tool’s documented interface:

```bash
sudo amd-ttm --set <NUM_GIB>
```

Do not copy a 124 GiB community profile onto systems with different RAM or security requirements.

## 7. Build pinned llama.cpp

```bash
LLAMA_COMMIT=86d86ed4396b4130922f7b9af26e3d9fc11a591b \
ROCM_PATH=/opt/rocm-7.14.0 \
  ../../scripts/build-llama-hip.sh
```

The recipe explicitly targets `gfx1151`, disables rocWMMA FlashAttention, keeps HIP VMM disabled, and keeps HIP graphs enabled.

## 8. Acceptance gate

```bash
ROCM_PATH=/opt/rocm-7.14.0 ../../scripts/smoke-hip.sh
./llama.cpp/build-hip/bin/llama-cli --list-devices
./llama.cpp/build-hip/bin/llama-bench -m MODEL.gguf -p 512 -n 128 -ngl 999 -fa 1
sudo dmesg -T | grep -Eai 'amdgpu|MES|gfxhub|page fault|ring timeout' | tail -n 200
```

Classify the resulting llama.cpp profile as **locally accepted** only after the smoke and workload gates pass. AMD’s Core SDK support is not an application certification.
