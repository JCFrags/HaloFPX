# Build and artifact validation

**Validation date:** 2026-07-18  
**Scope:** packaging/workspace checks only; **not** deployed-filesystem durability evidence.

## Environment

- Compiler: `cc (Debian 14.2.0-19) 14.2.0`
- Python: `Python 3.13.5`
- Host: `Linux a3d1b0e69f37 4.4.0 #1 SMP Sun Jan 10 15:06:54 PST 2016 x86_64 GNU/Linux`
- Test-directory mount: `/ none overlay rw`
- liburing development package: `not installed`

## Passed checks

- C probes compiled with `-std=c11 -O2 -Wall -Wextra -Werror`.
- Python scripts passed bytecode compilation; shell harnesses passed `bash -n`.
- Unprivileged error-path harness passed positive short writes, `EINTR`, zero-progress abort, immediate `ENOSPC`/`EIO`, file-sync failure, rename failure, and directory-sync failure/no-ack handling.
- Process-kill harness completed all five failpoints.
- Demonstration object validator accepted a complete object and rejected a truncated object (rejection rc `1`).
- Internal source/test ID cross-references and rendered local links are checked during packaging.

## Capability outcomes, not failures

- `O_TMPFILE` create/link probe returned rc `1` in the workspace. Output:

```
O_TMPFILE open: Is a directory (errno=21)
```

- Filesystem profile probe returned rc `78`; the workspace mount is a non-target layer and is expected to be rejected. Relevant output:

```
PF-IR-06 filesystem profile
timestamp_utc=2026-07-18T01:36:40Z
path=/tmp/pf-ir-06-finalqa.NXEbfW/fs/profile
kernel=Linux 4.4.0 #1 SMP Sun Jan 10 15:06:54 PST 2016 x86_64 GNU/Linux
uid_gid=0:0

[findmnt]
/ none overlay rw /   0:19   

[statfs]
type=overlayfs type_hex=794c7630 block_size=4096 blocks=2251799813685247 free_blocks=2251799813683673 avail_blocks=2251799813683673 files=0 free_files=0

[mountinfo matching mount id]
stat_mountpoint=/
{
   "filesystems": [
      {
         "target": "/",
         "source": "none",
         "fstype": "overlay",
         "options": "rw"
      }
   ]
}

[block topology]
source=none

[filesystem-specific]

[O_TMPFILE basic open probe]
supported_open=false errno=21 name=EISDIR detail=[Errno 21] Is a directory: '/tmp/pf-ir-06-finalqa.NXEbfW/fs/profile'

[denylist decision]
fstype_candidate=false reason=non-target-layer
```

## Deliberately not executed here

No privileged loopback ext4/XFS/Btrfs ENOSPC test, quota test, dm-error/dm-flakey test, io_uring T030–T036 harness, VM reset, real host power cut, or deployed block-stack flush/FUA test was run. Those remain release-gating evidence requirements. This validation page does not change `durability_claim: false` in `decision-record.json`.

## Harness excerpts

### Immediate error-path run

```
payload_len=16 fnv1a=bfad94ed2e171b5f
positive-short-write: PASS
payload_len=17 fnv1a=db27d026a0cb3d27
EINTR-retry: PASS
zero-write: PASS (rc=1)
pwrite-enospc: PASS (rc=1)
pwrite-eio: PASS (rc=1)
fdatasync-enospc: PASS (rc=1)
fdatasync-eio: PASS (rc=1)
renameat-EIO-old-generation-preserved: PASS
directory-fsync-EIO-no-ack-complete-visible-object: PASS
Error-path matrix completed. This injector is not a delayed-writeback, journal, block-layer, or power-cut test.
```

### Process-kill run

```
== after_create ==
PF_FAILPOINT=after_create: SIGKILL
/mnt/data/PF-IR-06-HaloKV-crash-durability-wiki-2026-07-18/scripts/fault-tests/run_kill_matrix.sh: line 12:  6494 Killed                  PF_FAILPOINT=$point "$BUILD/publish_probe" "$ROOT_DIR" "$name" "$payload"
exit=137
.pf-ir-06.tmp.6494 0 bytes
kill 60 bytes
== after_write ==
payload_len=29 fnv1a=12c4606fa46d3bba
PF_FAILPOINT=after_write: SIGKILL
/mnt/data/PF-IR-06-HaloKV-crash-durability-wiki-2026-07-18/scripts/fault-tests/run_kill_matrix.sh: line 12:  6498 Killed                  PF_FAILPOINT=$point "$BUILD/publish_probe" "$ROOT_DIR" "$name" "$payload"
exit=137
.pf-ir-06.tmp.6498 29 bytes
kill 80 bytes
== after_file_sync ==
payload_len=29 fnv1a=12c4606fa46d3bba
PF_FAILPOINT=after_file_sync: SIGKILL
/mnt/data/PF-IR-06-HaloKV-crash-durability-wiki-2026-07-18/scripts/fault-tests/run_kill_matrix.sh: line 12:  6502 Killed                  PF_FAILPOINT=$point "$BUILD/publish_probe" "$ROOT_DIR" "$name" "$payload"
exit=137
.pf-ir-06.tmp.6502 29 bytes
kill 100 bytes
== after_publish ==
payload_len=29 fnv1a=12c4606fa46d3bba
PF_FAILPOINT=after_publish: SIGKILL
/mnt/data/PF-IR-06-HaloKV-crash-durability-wiki-2026-07-18/scripts/fault-tests/run_kill_matrix.sh: line 12:  6506 Killed                  PF_FAILPOINT=$point "$BUILD/publish_probe" "$ROOT_DIR" "$name" "$payload"
exit=137
kill 140 bytes
obj-after_publish 29 bytes
== after_dir_sync ==
payload_len=29 fnv1a=12c4606fa46d3bba
PF_FAILPOINT=after_dir_sync: SIGKILL
/mnt/data/PF-IR-06-HaloKV-crash-durability-wiki-2026-07-18/scripts/fault-tests/run_kill_matrix.sh: line 12:  6510 Killed                  PF_FAILPOINT=$point "$BUILD/publish_probe" "$ROOT_DIR" "$name" "$payload"
exit=137
kill 180 bytes
obj-after_dir_sync 29 bytes
obj-after_publish 29 bytes
Process-kill matrix completed. This is not a power-cut durability test.
```
