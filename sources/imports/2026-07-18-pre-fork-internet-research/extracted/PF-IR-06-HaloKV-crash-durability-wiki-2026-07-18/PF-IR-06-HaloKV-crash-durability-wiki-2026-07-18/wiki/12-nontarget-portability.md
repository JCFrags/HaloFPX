# Explicit non-target portability notes

## Windows

Windows handle, cache-manager, volume and remote-file semantics require a separate design. `FlushFileBuffers`, write-through/unbuffered flags and `ReplaceFile` do not map mechanically to Linux `fdatasync` + rename + directory `fsync`. In particular, Windows replacement flags and directory-handle flushing behavior must be researched and tested independently before claiming parity.

## macOS

macOS documents a distinction between `fsync` and `F_FULLFSYNC`, with device/firmware caveats. Directory and APFS/HFS+ transaction behavior require a separate implementation contract and power-cut matrix.

## Network filesystems

NFS rename failure can be ambiguous after server crash/retry. Advisory locks can be translated, emulated or lost, and errors can surface through lease/recovery behavior. SMB/CIFS has server and protocol durability modes that differ from local Linux VFS assumptions. None is in the supported profile.

## Layered/pseudo filesystems

overlayfs, FUSE, virtiofs, 9p, container-managed mounts and cloud volume proxies can change synchronization and error behavior. A local-looking path is not enough. The profile probe rejects these until a specific contract and test record exists.
