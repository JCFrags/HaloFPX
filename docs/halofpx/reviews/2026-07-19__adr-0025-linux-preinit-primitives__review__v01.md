# ADR-0025 Linux pre-initialization primitives review v01

- Date: 2026-07-19
- Final verdict: **ACCEPT**
- Reviewed ADR SHA-256: `d6da0f0bb046d9b3f638a76849e8bef556ddc90f141e55a577b64a9e627e6152`

Two independent read-only reviews evaluated ADR-0025 against ADR-0018,
ADR-0019, canonical Wiki Section 63, the exact L05r implementation boundary,
and the selected-base build graph. The initial draft was rejected before code
opened. Review identified and the final contract closed:

- enforceable credential-before-root ordering through one private one-shot
  orchestration gate;
- exact memfd identity, seal, alias, `mlock`, complete-scratch wiping, fd
  closure, and closed error mapping;
- independently pinned parent/root/fixture/key identities rather than circular
  discovery;
- anchored `openat2` traversal with exact flags and no weak fallback;
- protected-path, backing-device, root/fixture, and evidence disjointness;
- exact Btrfs filesystem/subvolume, device, mount, ownership, mode, reserve,
  empty-layout, and read-only-flag checks without claiming writability;
- pinned fixture inode identity and hostile replacement rejection;
- exact OFD whole-file lock, local guard, 10 ms/5 s monotonic retry, no
  fork/dup/callback, death/alias semantics, and cleanup ordering; and
- retained repeat, raw syscall, before/after nonmutation, service-health,
  cleanup, reference, sanitizer, feature-off, and link-isolation evidence.

The accepted scope is pre-initialization only. It does not initialize or
authenticate a registry, call fake operations through syscalls, issue an event
ID, write or synchronize a root, publish quarantine, grant a concrete
observation, or satisfy M63-01. Mutation remains compile-time unavailable.

No architectural or implementation blocker remains for the narrow L05s lane.
The implementation and exact node evidence still require independent final
review before promotion.
