# Fault-test program

The canonical matrix is `../matrices/experiment-matrix.md`. The scripts directory supplies unprivileged probes and templates for privileged/VM tests.

## Test tiers

1. **Process fault tier:** short writes, injected errno, kill failpoints, collisions, lock loss and recovery. Fast CI on a local test filesystem.
2. **Loopback/dm tier:** ext4/XFS/Btrfs images; data and metadata ENOSPC; quota; dm-error/dm-flakey; mount-option profiles. Requires root in an isolated runner.
3. **VM power tier:** host kills or resets the guest at persisted failpoints; remount/replay; validates acknowledged generation set. The guest must not perform orderly shutdown.
4. **Deployed-stack tier:** actual filesystem, volume manager, encryption, RAID, hypervisor/controller/device and firmware profile. This is the release evidence.

## Required oracle

For every key, recovery may select only a file that passes format identity, exact declared length and hash. Before the final directory-sync/ack boundary, replacement recovery may expose either the old complete generation or the new complete generation; create-only recovery may expose the complete new object or no entry. After the certified directory-sync/ack boundary, the new complete generation must be present. A torn/mixed object is never permitted. An unacknowledged operation can be retried idempotently.

## Evidence record

Archive: test ID, binary/source hash, kernel, filesystem and feature flags, mount output, block topology, device/firmware, failpoint, injected fault, syscall/CQE trace, kernel log, recovered namespace, object hashes, and pass/fail reason. Hash the complete run directory.
