# Local fault-test harness notes

## Build unprivileged probes

```sh
cc -O2 -Wall -Wextra -Werror ../publish_probe.c -o publish_probe
cc -O2 -Wall -Wextra -Werror ../o_tmpfile_probe.c -o o_tmpfile_probe
cc -shared -fPIC -O2 -Wall -Wextra ../inject_short_write.c -ldl -pthread -o inject_short_write.so
```

Examples:

```sh
PF_WRITE_CAP=7 LD_PRELOAD=$PWD/inject_short_write.so ./publish_probe /cache object payload
PF_INJECT_AT=1 PF_INJECT_ERRNO=ENOSPC LD_PRELOAD=$PWD/inject_short_write.so ./publish_probe /cache object payload
./run_kill_matrix.sh /cache/pf-ir-06-test
```

## Loopback profiles

Create separate small images for ext4, XFS and Btrfs. Never run destructive tests on a production volume. Record mkfs output, features and exact mount options. Use data fillers and inode-heavy directory creation to distinguish data-space from metadata/inode exhaustion.

## EIO

Use an isolated VM with `dm-error` or `dm-flakey` beneath the filesystem. Switch the mapping after buffered write but before file sync to exercise delayed error reporting. Capture dmesg, mount state and every syscall/CQE.

## Power cut

The host must terminate/reset the guest without guest shutdown. Use a durable failpoint marker written to a separate control channel so the host knows which point the guest reached. After reboot/remount, hash and validate all objects before resuming the next iteration.

## io_uring

Build the project-specific harness against the pinned/selected liburing. Cover T030–T036 under ASan/UBSan where possible. Resource memory must remain allocated until terminal operation CQEs and any release-tag CQE have been consumed.

## Unprivileged error-path matrix

```sh
./run_error_path_matrix.sh /local/test/directory
```

This compiles the exact-write publisher and two `LD_PRELOAD` injectors. It covers positive short writes, `EINTR`, zero-progress writes, immediate `ENOSPC`/`EIO`, file-sync failure, namespace failure, and directory-sync failure/no-ack handling. It intentionally does **not** emulate delayed writeback errors, filesystem shutdown, journal replay, block flush/FUA behavior, or power loss; use the loopback/dm and VM tiers for those guarantees.
