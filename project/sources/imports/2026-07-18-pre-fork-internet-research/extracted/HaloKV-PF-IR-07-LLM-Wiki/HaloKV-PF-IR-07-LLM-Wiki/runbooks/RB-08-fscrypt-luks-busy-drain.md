# RB-08 — Busy files, mappings, kernel keys, and process drain

## Goal

Bound continued access before declaring key revocation or shutdown complete.
This runbook does not claim universal memory wiping.

## Drain sequence

1. Stop new cache requests and background writes for affected scope.
2. Stop manifest publication and wait for in-flight immutable writes to commit or be discarded.
3. Revoke broker leases and prevent new key derivation/unwrap.
4. Enumerate/terminate worker processes, file descriptors, memory maps, child processes, current-working-directory/root references, and IPC consumers according to platform tooling.
5. Zeroize application buffers with reviewed primitives and terminate workers to release remaining address-space state.
6. If fscrypt is used, request key removal, inspect removal status, close remaining users, and retry. Record files/key status that remain busy.
7. Unmount the cache filesystem. Do not force a corrupting unmount merely to claim revocation.
8. Close the dm-crypt mapping. Treat deferred close as pending; verify final removal from device-mapper state.
9. Remove explicitly linked volume keys and related kernel-key references according to configuration.
10. Verify no expected mount/mapping/key reference remains. Record any uncertainty, swap/dump/cache exposure, and whether host restart/power-off is required.
11. Update incident/rotation/deletion evidence. Do not declare CE based solely on this drain.

## Common false completion signals

* fscrypt master-key removal request returned but open files retain per-file keys;
* LUKS keyslot was removed while mapping remains active;
* `cryptsetup close --deferred` was accepted but device still has users;
* service stopped but child/debugger/core dump/swap retains material;
* volume detached but a linked kernel volume key remains;
* host rebooted but hibernation, snapshots, crash dumps, or backups retain copies.

[CLAIM:PFIR07-C005][CLASS:SOURCE][STATUS:SUPPORTED][SRC:LINUX-FSCRYPT-7.2RC3 §Removing keys]

[CLAIM:PFIR07-C019][CLASS:SOURCE][STATUS:SUPPORTED][SRC:CRYPTSETUP-2.8.6 cryptsetup-close]

[CLAIM:PFIR07-C026][CLASS:SOURCE][STATUS:SUPPORTED][SRC:SYSTEMD-CRYPTTAB-261.1 link-volume-key option]
