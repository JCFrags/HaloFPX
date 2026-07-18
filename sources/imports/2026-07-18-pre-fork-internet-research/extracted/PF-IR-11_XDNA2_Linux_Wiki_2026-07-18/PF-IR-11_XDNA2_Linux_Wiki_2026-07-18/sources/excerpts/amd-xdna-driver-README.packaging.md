### Default driver in the plugin package

This repository contains **two** independent driver source trees for the
AMD XDNA NPU:

* `drivers/accel/amdxdna/` — the upstream (staging) driver, mirrored
  from the same path in the Linux kernel tree and being upstreamed.
* `src/driver/amdxdna/` — the out-of-tree (OOT, also referred to as
  "legacy") driver. Maintained here for compatibility and bring-up.

`./build.sh -release` always builds **both** trees and packages the
resulting kernel modules into the plugin DEB:

* `amdxdna.ko` — built from the upstream (staging) tree. This is the
  primary driver: DKMS compiles it on install and `modprobe` loads it
  automatically.
* `amdxdna_legacy.ko` — built from the out-of-tree (OOT) tree, installed
  alongside `amdxdna.ko` for compatibility and bring-up.

See `./build.sh -h` for the flags that swap which module ships as the
primary `amdxdna.ko`.

You will find `xrt_plugin.<version>_<distro-version>-<arch>-amdxdna.deb` (Ubuntu/Debian) or `xrt_plugin.<version>_-<arch>-amdxdna.tar.gz` (Arch Linux) in the `Release/` folder. This package includes:
* The `.so` library files, which will be installed into `/opt/xilinx/xrt/lib` folder
* The XDNA driver source and DKMS script that build, install, and load the
  primary `amdxdna.ko` on the target machine. A second module,
  `amdxdna_legacy.ko`, is also shipped for compatibility (see
  [Default driver in the plugin package](#default-driver-in-the-plugin-package)
  for how to swap them).
* The firmware binary files, which will be installed to `/usr/lib/firmware/amdnpu` folder
