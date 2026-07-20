# L05z semantic-authority correction independent review

**Result: ACCEPT_QUALIFIED_SEMANTIC_CONTRACT_NOT_L05Z_PROMOTION.**

The prior 8,612 returned-fault roster and 8,613 response-extended roster remain
frozen compatibility/execution authorities. Their IDs, hashes, shards, replay
selectors, and immutable receipts are not rewritten. The correction is
terminological and semantic: neither roster is a unique-behavior count.

The audit found exactly 17 retryable roles for which `PRE-EINTR` and
`EINTR-ONCE` encode the same controller behavior. In both profiles the first
syscall is suppressed at entry, the tracee observes `EINTR`, exactly one
same-role retry occurs, and that retry completes successfully. Keeping
`PRE-EINTR` as the representative and treating `EINTR-ONCE` as its alias yields
8,595 unique returned-fault cases. Adding `L05Z-RSP-LOSS-FULL-001`, which has no
alias in the returned-fault roster, yields 8,596 unique semantic cases.

The receipt records every alias pair and pins three base commitments: unique ID
set `a1ecb0f8...`, unique manifest `ed46674b...`, and alias manifest
`9a25b012...`. It also pins the response-extended unique ID set
`3988a34d...` and manifest `832f94b...`. The frozen compatibility/execution
hashes remain recorded separately so a consumer cannot silently exchange one
authority for the other.

Independent adversarial review required the implementation to freeze and check
the complete compatibility and unique-semantic 17-shard arrays and their exact
sums, exercise the actual controller-path helper predicates rather than only a
parallel manifest oracle, and preserve the previously accepted legacy stdout.
All three findings were fixed before final qualification. The legacy role
self-test remains byte-identical at SHA-256 `56be54b4...` and 844 bytes; the
legacy response self-test remains byte-identical at `171d1be9...` and 576
bytes. The new semantic self-test is `15892cc9...` and 910 bytes.

Both nimo-1 and nimo-2 independently compiled and linked the final sources with
`-Wall -Wextra -Wpedantic -Werror` on the identical Linux 7.1.3-1-cachyos
x86-64, GCC 16.1.1, CMake 4.3.4, and Ninja 1.13.2 tuple. The retained primary
build used GCC 16.1.1's project-default GNU++17 mode, not an explicit `-std`
option. Its 591,928-byte binary is identical on both nodes at SHA-256
`c6483b95...`; its 629,920-byte object is identical at `c60c9739...`.

An additional strict compile and link with explicit `-std=c++17` passed on both
nodes. That lane produced identical 596,856-byte binaries at SHA-256
`941b5f0d...` and identical 635,800-byte objects at `d94bfe27...`. Its role,
response, semantic, and `HRAW-B0000` executable checks all returned zero with
stdout byte-identical to the primary lane. The static seam contract is the same
source-level CMake command and does not depend on the controller binary's
language mode; it returned zero on both nodes with log `7b4f124f...` and 87
bytes. Exact final source hashes, full output hashes, `controller.verify17*`,
and the verify17 logs are retained at each node's
`/var/tmp/halofpx-l05z-semantic-correction-eba2e36-20260719` evidence root.

Focused inherited controls also returned zero on both nodes with identical
logs: feature-off contract `9ee971ef...` (39 bytes) and L02 contracts
`db24640b...` (65 bytes). Because the semantic evidence snapshot intentionally
copies only tests, these checks reused accepted prior full-source and
`llama-server` controls unchanged by this test-only patch. Nimo-1 used
`/var/tmp/halofpx-qualification/l05z-checkpoint-regression-20260719-nimo1/source`
with `build-release-final/llama-server`; nimo-2 used
`/var/tmp/halofpx-qualification/l05z-role-authority-canary-20260719-nimo2/source`
with `build-normal/llama-server`. This validates the unchanged product and
contract surface, not a server rebuilt from the semantic evidence snapshot.
No product source changed. The logs are retained as
`build/verify-feature-off.log` and `build/verify-l02-contracts.log` in both
semantic evidence roots.

This authority-only qualification created no media. Both nodes ended with zero
qualification-root mounts, loop devices, or controller processes. Nimo-1's
llama server remained PID 971 with HTTP 200, and nimo-2's RPC server remained
PID 3562775.

The relevant canonical Wiki sections 63 and 80 were reviewed. They require
exact fault boundaries, fail-closed outcomes, disposable targets, retained raw
evidence, and no inference from process liveness alone. They contain no L05z
semantic-count assertion, so this implementation-local correction does not
require a Wiki edit and does not promote an unqualified count into the Wiki.

The change remains Linux-only test authority around excluded targets. It is
default-off, has no product linkage, introduces no runtime or persistence edge,
and enables no cache hit, restore, provider, server write, or inference
behavior. It is target-native and uses no donor implementation or donor
documentation, no GPL llama-ai code, no new dependency, and no direct cherry
pick; a P3 admission record is therefore not required.

This review does not admit any of the 247 pending physical role selectors,
qualify a returned-fault cell, freeze physical execution cardinality, close
full fault-scale or sanitizer coverage, promote L05z, enable persistence, or
make a durability, inference-performance, or zero-regression claim.
