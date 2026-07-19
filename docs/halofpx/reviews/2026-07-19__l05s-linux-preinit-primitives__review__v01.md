# L05s Linux pre-initialization primitives review v01

- Date: 2026-07-19
- Decision authority: ADR-0025 and canonical Wiki Section 63
- Verdict: **ACCEPT** for the default-off, excluded, nonmutating Linux primitive seam

## Outcome

L05s supplies the smallest target-native Linux substrate needed to qualify a
future registry initializer without opening initialization or persistence. It
accepts only an exact sealed inherited credential, anchored and revalidated
Btrfs identities, the exact empty-root/single-lock layout, sufficient reserve,
and an OFD whole-file writer lock. Every accepted result is explicitly
`ok_non_authoritative`; it cannot create registry authority or enter a product
path.

The seam is Linux-only, `STATIC EXCLUDE_FROM_ALL`, and disabled by default.
Linux mutation remains a fatal configuration error. No server, fake engine,
cache hit, restore, publication, HIP, Vulkan, RPC, or distributed edge reaches
the target.

## Adversarial review

The independent review found five material issues during implementation: final
identity/layout revalidation was incomplete; the root-keyed same-process guard
was not exercised in-process; credential alias-scan errors could fail open; a
malformed overlapping call could close the active fd 3; and the first live
guard probe used an unreliable EBADF inference after descriptor reuse. Each was
repaired and retested. The final review returned **ACCEPT** with no remaining
actionable finding.

The resulting live probe proves an active admitted session, a same-root `busy`
return with zero credential/filesystem facts, and a different-root
`invalid_request` with the same zero-syscall boundary. Full final revalidation
covers the parent, root, fixture, lock, layout, mount, reserve, read-only state,
and lock ownership before success.

## Qualification

Nimo-1 passed a clean Linux Release all-target build and all 42 HaloFPX tests.
It then passed 100 fresh-process admissions, 100 contention pairs, eight pinned
identity mismatches, six hostile layouts, a read-only remount/recovery, reserve
exhaustion/recovery, and controlled holder death with immediate OFD-lock
reacquisition. Initial and final fixture facts matched.

Nimo-2 independently passed 100 Release and 100 ASan/UBSan fresh-process runs,
100 Release and 100 sanitizer contention pairs, live guard probes, and Release
and sanitizer process-death canaries. Its final sanitizer all-target build and
all 42 HaloFPX tests passed with no sanitizer finding. The two nodes produced
different binaries from independent builds, as expected, while exercising the
same source contract and deterministic result matrices.

The promotion gap found by final review was then closed on the final exact
binaries. Each node passed 25 traversal attacks, 25 pin-then-overlay attacks
using a second real Btrfs mount, and 25 actual lock-inode replacements: 150/150
final cases with two stable-tree equalities per node. Release probes on both
nodes and ASan/UBSan probes on nimo-2 also proved the full inherited-alias busy
deadline with no stale-break, release only after alias death, and absence of the
pinned lock descriptor after exec while the exec sentinel remained alive.

Windows Release retained feature-off compatibility: the Linux target was
absent and all 40 HaloFPX tests passed. A pre-existing static audit initially
recognized only Visual Studio's `*.obj` archive member and rejected Ninja/MSVC's
exact `*.cpp.obj` spelling. The audit now admits only those two generator
spellings before applying the unchanged fail-closed COFF section and reviewed
machine-code comparator body check. A subsequent test launch without the MSVC
developer environment failed only because a compile-negative probe could not
find `<array>`; the correctly provisioned rerun passed 40/40. Neither failed
controller/environment attempt supports promotion.

The retained raw evidence contains 501 files across both nodes, including full
build and CTest logs, source and executable hashes, 300 accepted fresh-process
runs, 300 accepted contention pairs, strace evidence, corruption/identity and
layout matrices, 150 final-binary attack cases, lifetime probes with exact PIDs,
kernel/toolchain manifests, before/after service probes, fault canaries, fixture
facts, and cleanup receipts. Superseded controller attempts remain labeled and
excluded instead of being deleted.

## Provenance, rollback, and performance

The implementation is target-native. No GPL llama-ai code, CachyLlama code,
donor format, or documentation entered the MIT engine, so no P3 admission record
is required. All four immutable reference clones remain clean at their locked
commits and trees. HaloFPX still has no remote.

The original two 1 GiB loopback Btrfs fixtures and the two main/two secondary
attack images were unmounted, detached from their exact loop devices, and
deleted only after identity verification; qualification sources, builds, and
evidence remain. The known-good nimo-1 inference server and nimo-2 RPC daemon
retained their original PIDs and passed post-test probes.

Rollback is removal of the two default-off CMake controls, the excluded Linux
target and its tests, and the narrow Ninja/MSVC audit spelling allowance. Since
the feature is absent from feature-off product graphs, this milestone makes no
inference-performance claim and cannot affect the selected ROCmFPX control.

The next initializer milestone must remain default-off and may not publish or
persist anything until its separately reviewed contract explicitly admits each
filesystem mutation and crash boundary.
