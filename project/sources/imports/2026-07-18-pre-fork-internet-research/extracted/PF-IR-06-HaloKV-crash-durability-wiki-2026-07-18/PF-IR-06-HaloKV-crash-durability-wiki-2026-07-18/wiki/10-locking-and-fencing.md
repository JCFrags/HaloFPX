# File locking and multi-process fencing

## Selected policy

One writer process per cache root. HaloKV and the RPC model-tensor cache use distinct roots and lock files. A writer opens the lock file once, acquires an exclusive nonblocking OFD lock or `flock`, and retains that open file description until orderly shutdown.

## Why OFD lock or flock

Both follow an open-file-description lifetime. Duplicated descriptors share the lock and the last close releases it. Linux OFD locks avoid the classic process-lock behavior where closing any descriptor for the same inode can drop all of the process's record locks.

## Limits

These locks are advisory. A noncooperating process can still modify files if permissions allow. The lock does not persist through process death, does not serialize hosts, does not prove that a previous writer's storage reached media, and does not establish a fencing token visible inside every object.

After acquiring a lock released by process death, the new writer must run recovery and revalidate the filesystem profile before writing. For any future multi-host mode, use an external lease/consensus service with monotonically increasing fencing tokens embedded in the format and checked by the storage service; ordinary local locks are insufficient.

## Permissions and containment

Use directory ownership/mode, a dedicated service account, mount namespace controls and optional LSM policy so only the cooperating writer can mutate the root. Readers should open immutable objects read-only. The lock file is never renamed or replaced as part of cache publication.
