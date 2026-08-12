# Recipe: Mesa 26.1.5 RADV source candidate

**Classification:** current official upstream source; gfx1151 acceptance required  
**Source:** [MESA-2615](../sources.md#mesa-2615)

## Pin and verify

```bash
MESA_VERSION=26.1.5
MESA_SHA256=79e421c7ce18cd9e790b8375920325779f10798630bf30e0b22f1a21c8617122
curl -fL "https://archive.mesa3d.org/mesa-${MESA_VERSION}.tar.xz" \
  -o "mesa-${MESA_VERSION}.tar.xz"
printf '%s  %s\n' "$MESA_SHA256" "mesa-${MESA_VERSION}.tar.xz" | sha256sum -c -
tar -xf "mesa-${MESA_VERSION}.tar.xz"
```

## Debian/Ubuntu build prerequisites

```bash
sudo apt-get update
sudo apt-get install -y \
  bison build-essential flex glslang-tools libdrm-dev libelf-dev \
  libexpat1-dev libllvm-dev libudev-dev libvulkan-dev libwayland-dev \
  libx11-xcb-dev libxcb-dri3-dev libxcb-present-dev libxcb-randr0-dev \
  libxcb-shm0-dev libxcb-xfixes0-dev libxshmfence-dev libxxf86vm-dev \
  meson ninja-build pkg-config python3-mako python3-ply python3-yaml \
  wayland-protocols zlib1g-dev
```

Dependency names vary by distribution. Meson will report missing development packages.

## Build isolated RADV/Radeonsi

```bash
cd mesa-26.1.5
meson setup build \
  --prefix=/opt/mesa-26.1.5 \
  --libdir=lib \
  --buildtype=release \
  -Dvulkan-drivers=amd \
  -Dgallium-drivers=radeonsi \
  -Dllvm=enabled \
  -Dshared-llvm=enabled \
  -Dplatforms=x11,wayland
ninja -C build
sudo ninja -C build install
```

## Select without replacing distro Mesa

```bash
export LD_LIBRARY_PATH=/opt/mesa-26.1.5/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}
export VK_DRIVER_FILES=/opt/mesa-26.1.5/share/vulkan/icd.d/radeon_icd.x86_64.json
VK_LOADER_DEBUG=all vulkaninfo --summary 2>mesa-26.1.5-loader.log
```

If the installed manifest name differs, use the actual file from the prefix. Do not copy files into `/usr` for the first qualification.

## Acceptance

Run the same b10064 llama.cpp binary against distro RADV and Mesa 26.1.5, with one shell environment at a time. Compare:

```text
vulkaninfo driverName/driverInfo
pp512 and pp2048
TG128
model-load time
new kernel warnings
```

Promote Mesa 26.1.5 from “upstream current candidate” to “locally validated” only after this A/B.
