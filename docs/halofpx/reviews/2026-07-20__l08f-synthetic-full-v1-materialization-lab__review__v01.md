# L08f synthetic full-v1 materialization lab independent review v01

Verdict: **ACCEPT** for the explicitly non-authoritative, disposable L08f
write-mechanics lab. No P1/P2 finding remains.

## Blocking findings corrected

The initial draft was not admissible as a writer. Review required:

- replacing per-instance serialization with a fixed root OFD lock;
- exact post-sync reread and stable-identity verification before rename;
- safe-open exact synchronization for all-equal existing files;
- exception handling for all allocations inside the `noexcept` operation;
- copying the selected digest from the validated candidate rather than rereading
  caller authority;
- explicit non-authoritative class and result semantics;
- an all-absent/all-exact namespace preflight; and
- whole-root discard after every failure from the first write attempt onward.

All were corrected and covered by the focused contract/lifecycle tests.

## Review conclusions

- Source material is authenticated and exact-matched before writer-lock or
  fixed-child access and before mutation I/O in the materialization attempt.
  It remains synthetic caller input and grants no positive product authority.
- OFD locking on a permanent exact private file fences cooperating writers
  across publisher instances and processes.
- Fixed `openat2` containment, attempt-scoped `O_EXCL` staging, exact reread,
  file synchronization, identity stability, and `RENAME_NOREPLACE` preserve the
  immutable namespace.
- All objects and their affected directories synchronize before manifest
  visibility. All-exact reuse synchronizes files and parent directories before
  returning its explicitly non-authoritative result.
- A partial/mixed root conflicts. Once writing starts, every non-success and
  exception requires whole-root discard, so no retryable-looking status escapes.
- The target remains Linux-only and `STATIC EXCLUDE_FROM_ALL`, with no server,
  anchor, generation, codec, restore, donor, install, or service edge.
- The implementation is target-native and adds no GPL llama-ai implementation,
  CachyLLama transplant, WebUI asset, dependency, remote, or provenance duty.

## P3 and deferred boundary

Preflight of a mixed root may synchronize exact existing files, so it promises
no byte or namespace mutation rather than literally no filesystem effect.
Publish-oriented internal filenames and target/status type names remain minor
clarity debt.

Persistent attempt/restart fencing, protected material authority, anchor and
generation advancement, product durability, retention, quotas, a server edge,
and live restore remain unimplemented and must not be inferred from L08f.
