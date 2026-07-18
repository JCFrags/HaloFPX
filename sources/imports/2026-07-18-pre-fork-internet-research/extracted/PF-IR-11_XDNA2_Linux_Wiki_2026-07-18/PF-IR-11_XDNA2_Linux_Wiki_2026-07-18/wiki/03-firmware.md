# 3. Firmware boundary

## Upstream firmware namespace

[UPSTREAM] The pinned kernel requests firmware from `amdnpu/17f0_11/` and tries `npu_7.sbin` before `npu.sbin`.

[UPSTREAM] The pinned `linux-firmware` commit is `924d73c9a2501a256d18a26cbe640548c70b3a9a`. Its WHENCE entry records:

| Logical filename | Versioned payload |
|---|---|
| `amdnpu/17f0_11/npu.sbin` | `npu.sbin.1.0.0.166` |
| `amdnpu/17f0_11/npu_7.sbin` | `npu.sbin.1.1.2.65` |

Exact captured WHENCE excerpt: [`../sources/excerpts/vendor-docs/linux-firmware-WHENCE.amdxdna.txt`](../sources/excerpts/vendor-docs/linux-firmware-WHENCE.amdxdna.txt).

[UPSTREAM] WHENCE classifies the payload as redistributable and refers to `LICENSE.amdnpu`.

## Vendor legacy namespace

[VENDOR-ONLY] AMD's pinned legacy npu5 source is different:

- expected file: `amdnpu/17f0_11/npu.dev.sbin`;
- minimum firmware version: 6.12;
- VBNV: `NPU Strix Halo`.

Exact source: [`../sources/raw/amd-xdna-driver/legacy-npu5_regs.c`](../sources/raw/amd-xdna-driver/legacy-npu5_regs.c).

[DECISION] Do not mix the upstream `npu.sbin`/`npu_7.sbin` namespace with the legacy `npu.dev.sbin` path. They are separate compatibility paths.

## Version coupling

[VENDOR-ONLY] AMD's driver repository warns that stale or mismatched firmware commonly causes `ERT_CMD_STATE_ABORT`, mailbox timeout, or immediate post-load failure.

[VENDOR-ONLY] The plugin package installs a matched firmware copy under `/usr/lib/firmware/amdnpu`.

[INFERENCE] A firmware symlink resolving to a valid file is necessary but not sufficient. Its exact hash and provenance must match the selected driver/plugin release.

## Secure load and lifecycle

[UPSTREAM] The driver asks the PSP to securely load signed NPU firmware and starts the NPU microcontroller.

[UPSTREAM] The NPU is switched off during system suspend; resume starts hardware and reloads firmware before contexts are resumed.

[UNKNOWN] The implementation does not by itself prove transparent application recovery under all target suspend, firmware fault, mailbox timeout, or partial-hang scenarios.

## Bundle handling

[MISSING] Firmware binaries are intentionally not redistributed here.

This bundle preserves:

- exact kernel request paths;
- exact current `linux-firmware` commit;
- current versioned WHENCE names;
- the license reference;
- a read-only target-host hashing procedure.

## Target checks

The probe records:

1. symlink targets and filesystem metadata;
2. SHA-256 of compressed or uncompressed payloads;
3. decompressed SHA-256 for `.zst` files when `zstdcat` is available;
4. package ownership/inventory;
5. module firmware declarations;
6. firmware load messages.
