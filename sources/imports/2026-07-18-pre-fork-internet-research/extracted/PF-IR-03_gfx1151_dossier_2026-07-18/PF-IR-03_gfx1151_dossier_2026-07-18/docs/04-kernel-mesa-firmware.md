# Kernel, amdgpu/KFD, firmware, Mesa and RADV

## amdgpu/KFD requirements

For the official 7.14 Ryzen lane, use the inbox driver from a documented Ubuntu/kernel pair. Do not import the Instinct amdgpu package-version table into a Ryzen tuple.

[KNOWN_ISSUE] The gfx1151 context save/restore area was incorrectly sized in affected kernels and could hang ROCm. The Ubuntu SRU identifies two coupled fixes:

- bump the minimum VGPR size for gfx1151;
- export `cwsr_size` and `ctl_stack_size` to userspace.

The fixes originated in upstream 6.19-rc1 and were backported into OEM/HWE lines. The accepted Noble HWE record includes them in `linux-hwe-6.17 6.17.0-16.16~24.04.1`. This is evidence for the fixes, not a command to freeze that early proposed build forever; capture the exact final package used.

## Kernel evidence to retain

```text
uname -a
cat /proc/version_signature
apt-cache policy linux-image-$(uname -r)
dpkg-query -W 'linux-image*' 'linux-modules*' 'linux-firmware*'
modinfo amdgpu
zgrep 'CONFIG_DRM_AMDGPU\|CONFIG_HSA_AMD\|CONFIG_USB4' /proc/config.gz
journalctl -k -b
```

For source kernels, retain the signed tag/tarball, signer fingerprint, digest, `.config`, local patches, build toolchain and reproducible build log.

## Firmware

[PROVENANCE_GAP] The current Ryzen matrix does not pin a gfx1151-specific `linux-firmware` version or blob digest. A valid local tuple must record:

1. distribution package version, repository metadata and package SHA-256;
2. `WHENCE` and all license files from that exact package;
3. firmware filenames and versions requested by the running `amdgpu` driver;
4. SHA-256 for every used file under `/lib/firmware/amdgpu`;
5. relevant BIOS/UEFI and platform firmware versions where available.

## Mesa/RADV boundary

Mesa/RADV supplies Vulkan/OpenGL graphics, while ROCm compute uses HIP/HSA/KFD. Mesa can materially affect display, Vulkan interop and shared-buffer paths, but it is not the ROCm compatibility matrix's compute userspace baseline.

The local Mesa 26.1.4 comparison has an official source checksum:

```text
SHA256  072705caa9adf4740f1489194b13e278ad959166863b5271fe423a86353c9ab6
SHA512  39d15574e876005fd1a483d1cb3c801b08297ec73820a89cbf368d0f2a922cc770d0e623e99acc66b71791332cdadc6cc3095f059f713b09128921ff2d0197a8
```

That checksum identifies the upstream source tarball, not the installed distro package, AMD Mesa fork, build flags or runtime file set. Pin those separately.
