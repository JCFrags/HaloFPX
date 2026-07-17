# Release and Rollback Checklist

## Before release

- [ ] All acceptance gates A–I pass.
- [ ] Previous binary/image retained and smoke-tested.
- [ ] Runtime kill switches tested.
- [ ] Persistent root version isolated from ephemeral root.
- [ ] Canary metrics/alerts active.
- [ ] Store quota includes staging and quarantine headroom.
- [ ] Rollback branch/tag signed and immutable.
- [ ] Rollback drill completed without deleting persistent state.

## During rollback

- [ ] Stop admission and capture metrics/build/store IDs.
- [ ] Disable writes or persistent provider.
- [ ] Snapshot/quarantine store; do not delete.
- [ ] Start ephemeral/off mode and verify cold correctness.
- [ ] Deploy previous binary if needed.
- [ ] Confirm previous binary is not pointed at newer store.
- [ ] Open incident and preserve forensic evidence.

## Before re-enable

- [ ] Regression test reproduces and passes the fix.
- [ ] Offline scan validates or quarantines affected entries.
- [ ] Format compatibility re-reviewed.
- [ ] Canary restarts from read-only stage.
