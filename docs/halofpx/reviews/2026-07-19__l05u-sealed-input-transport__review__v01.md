# L05u sealed initializer-input transport review v01

- Date: 2026-07-19
- Parent commit: `e918d1f8e254d9c17f043f0c896bda80894fdc2f`
- Scope: M63-01b sealed descriptor transport and locked input storage; no
  predecessor authentication, root access, or mutation
- Verdict: **ACCEPT**

## Outcome

The reviewed slice safely narrows inherited descriptors 3 and 4 into bounded,
non-authoritative transport facts. It preserves the Linux-only, default-off,
excluded archive boundary and adds no product, install, export, provider,
server, HIP, Vulkan, RPC, or WebUI edge.

Independent review initially rejected the implementation because descriptor
cleanup could affect a shared file table and sensitive state was locked on a
stack page. The implementation now blocks signals, calls
`unshare(CLONE_FILES)` before descriptor inspection, requires exactly one
`/proc/self/task` entry, and closes only the caller's private descriptor-table
copies. Sensitive bytes now live in a page-rounded anonymous mapping with
placement construction, full-mapping wipe, `munlock`, and `munmap`.

## Review dimensions

- Correctness: exact memfd names, type, link count, size, seals, close-on-exec,
  distinct identities, alias absence, offset-zero reads, EOF, key tuple, and
  predecessor digest are checked and then revalidated before cleanup.
- Security: transport facts grant no reusable authority. Missing, altered,
  stale, aliased, unlocked, oversized, truncated, non-tmpfs, multithreaded, or
  lock-exhausted inputs fail without root access or mutation.
- Isolation: archive and contract audits prove two objects, two callable
  definitions, only the two-stage HaloFPX digest lineage, a bounded POSIX/libc
  import surface, no root or mutation tokens, and no product/install/export
  edge.
- Testing: each qualified invocation ran 25 named child variants, all 1,248
  single-bit predecessor mutations, all 352 single-bit structural/key-tuple
  credential mutations, and a shared-thread rejection. Three Linux
  invocations therefore covered 4,875 fresh child cases plus three
  shared-thread rejection cases.
- Provenance: target-native implementation; no donor or GPL code, no P3
  admission, and no direct cherry-pick.
- Rollback and performance: defaults and inference graphs remain unchanged.
  Removal is source-only; CPU contract timings are not an inference-performance
  claim.

The corrected source passed Windows feature-off 40/40, nimo-1 Release 4/4,
nimo-2 Release 4/4, and nimo-2 ASan/UBSan 4/4. The final reviewer returned
**ACCEPT** with no remaining actionable correctness, security, provenance,
isolation, or overclaim finding.

This verdict does not admit predecessor authentication, credential authority,
root access, `writer.lock`, persistent writes, initialization, repair,
publication, cache hits, restore, inference, or L14Q work.
