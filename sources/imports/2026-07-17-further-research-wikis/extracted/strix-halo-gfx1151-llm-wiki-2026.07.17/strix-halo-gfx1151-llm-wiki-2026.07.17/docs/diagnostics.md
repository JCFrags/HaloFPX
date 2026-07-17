# Diagnostics and acceptance commands

Run the bundled collector first:

```bash
./scripts/collect-diagnostics.sh
```

It creates a timestamped directory and tarball without modifying the host. Root-only commands are attempted through noninteractive `sudo` and skipped when unavailable.

## Host identity

```bash
uname -a
cat /etc/os-release
cat /proc/cmdline
lscpu
lspci -nnk | grep -A4 -Ei 'VGA|Display|USB4|Thunderbolt|Network'
```

## Kernel, amdgpu, and firmware

```bash
modinfo amdgpu | sed -n '1,120p'
lsmod | grep -E 'amdgpu|thunderbolt|thunderbolt_net'
cat /sys/module/ttm/parameters/pages_limit 2>/dev/null
cat /sys/module/amdgpu/parameters/gttsize 2>/dev/null
find /lib/firmware/amdgpu -maxdepth 1 -type f -printf '%f %s bytes
' | sort
find /lib/firmware/amdgpu -maxdepth 1 -type f -print0 | sort -z | xargs -0 sha256sum
sudo dmesg -T | grep -Eai 'amdgpu|MES|gfxhub|page fault|GPU reset|ring timeout|firmware'
```

Record the **distribution package version** and **file hashes**. A date-like firmware package version is not portable across distributions.

## ROCm/HIP/compiler

```bash
command -v hipconfig rocminfo rocm-smi amd-smi
hipconfig --full
rocminfo
/opt/rocm/llvm/bin/clang --version 2>/dev/null || true
cmake --version
ldd --version | head -n1
ldconfig -p | grep -E 'libamdhip64|libhsa-runtime64|librocblas' | sort
```

Minimal compile/run:

```bash
./scripts/smoke-hip.sh
```

Debug an asynchronous failure:

```bash
HIP_LAUNCH_BLOCKING=1 ./scripts/smoke-hip.sh
```

Do not compare performance with `HIP_LAUNCH_BLOCKING=1`.

## Vulkan/Mesa

```bash
find /usr/share/vulkan/icd.d /etc/vulkan/icd.d -maxdepth 1 -type f -name '*.json' -print 2>/dev/null
vulkaninfo --summary
VK_LOADER_DEBUG=all vulkaninfo --summary 2>vulkan-loader.log
```

When both RADV and AMDVLK are installed, force one manifest at a time with `VK_DRIVER_FILES`, after verifying the path.

## llama.cpp

```bash
./build-hip/bin/llama-cli --version
./build-hip/bin/llama-cli --list-devices
./build-vulkan/bin/llama-cli --list-devices

./build-hip/bin/llama-bench -m MODEL.gguf -p 512 -n 128 -ngl 999 -fa 1
./build-vulkan/bin/llama-bench -m MODEL.gguf -p 512 -n 128 -ngl 999 -fa 1
```

For model-load isolation:

```bash
./build-vulkan/bin/llama-cli -m MODEL.gguf -ngl 999 -fa 1 --no-direct-io --no-mmap -p test -n 16
```

For ROCmFPX, select the backend explicitly:

```bash
./build-strix-rocmfp4/bin/llama-cli -m MODEL.gguf -dev ROCm0 -ngl 999 -fa on -p test -n 16
./build-strix-rocmfp4/bin/llama-cli -m MODEL.gguf -dev Vulkan0 -ngl 999 -fa on -p test -n 16
```

## USB4 / Thunderbolt networking

```bash
boltctl list 2>/dev/null || true
ls -l /sys/bus/thunderbolt/devices
ip -details link show type thunderbolt 2>/dev/null || ip link show
ethtool -i thunderbolt0 2>/dev/null || true
ping -c 10 192.168.44.2
iperf3 -c 192.168.44.2 -P 4 -t 30
iperf3 -c 192.168.44.2 -P 4 -t 30 -R
```

Experimental verbs path:

```bash
ibv_devices
rdma link
ibv_devinfo -d usb4_rdma0
sudo cat /sys/kernel/debug/thunderbolt_ibverbs/summary 2>/dev/null
```

## Acceptance thresholds

A practical release gate should require:

- zero new `amdgpu`, `MES`, `gfxhub`, page-fault, or ring-timeout messages;
- stable device enumeration across five process starts;
- HIP smoke and Vulkan summary success;
- model load and 30-minute generation loop;
- median pp/tg performance within 5% of the pinned local baseline;
- no D-state workload or management processes after exit;
- USB4 packet loss of zero and throughput within the locally documented cable/topology range.
