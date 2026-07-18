# Deployed filesystem profile — unresolved first-class input

<span class="badge u">UNKNOWN-DEPLOYMENT</span> The production filesystem, mount options and block topology were not provided. This is the first release gate, not a detail to infer from a typical Linux installation.

Run:

```sh
./scripts/inspect_fs_profile.sh /absolute/path/to/halokv-cache
./scripts/inspect_fs_profile.sh /absolute/path/to/tensor-cache
```

Archive the output with the test run. The two roots can differ and therefore require separate certification.

## Profile record

Record at minimum:

- kernel release and architecture;
- mount ID, filesystem type, source device, superblock and mount options;
- namespace layers: bind mounts, overlay, container volume, FUSE or network client;
- block stack: partition, LVM/dm-crypt/dm-cache, md/RAID, multipath, virtual disk, hypervisor, controller and device;
- volatile write-cache and power-loss-protection claims;
- filesystem feature flags and mkfs geometry;
- `O_TMPFILE` create/link support;
- `renameat2` flag support;
- directory `fsync` result;
- `STATX_DIOALIGN` values if DIO is considered;
- free data blocks, metadata/inodes and quota policy.

## Immediate stop conditions

Persistent mode should refuse: unknown fstype; `overlay`, `fuse.*`, `nfs*`, `cifs`, `smb3`, `virtiofs`, `9p`; read-only or error-remounted root; denied mount options; inability to synchronize the parent directory; untested block remapping; or a profile drift from the certified record.

`findmnt` output is evidence of the current namespace, not proof of physical media behavior. `T040` remains mandatory.
