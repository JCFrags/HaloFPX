# Upstream Sync Checklist

- [ ] Fetched all remotes with `--no-tags` and locked SHAs.
- [ ] Created `sync/upstream/YYYYMMDD-<sha12>` from `origin/main`.
- [ ] Classified donor path deltas against upstream.
- [ ] Resolved conflicts through Conflict Map ownership.
- [ ] Asserted known upstream-owned Vulkan change remains present.
- [ ] Reviewed libllama ABI and state serialization changes.
- [ ] Ran feature-off canonical matrix.
- [ ] Ran ROCmFPX quantization, HIP/ROCm, Vulkan, MTP, and server cache gates.
- [ ] Rebased/range-diffed open lanes.
- [ ] Completed sync record and appended `AI_CHANGES.md`.
- [ ] Created/verified rollback tag before merge.
