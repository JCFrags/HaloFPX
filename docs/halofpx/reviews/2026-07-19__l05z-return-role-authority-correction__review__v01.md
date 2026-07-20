# L05z returned-fault role-authority correction review

**Result: ACCEPT_FOR_AUTHORITY_CONTINUATION_NOT_L05Z_PROMOTION.**

The historical `8,468` checkpoint remains valid as a record of what was known
and accepted at that checkpoint, but it is superseded for current returned-fault
authority. The corrected manifest distinguishes `8,706` legacy structural rows
from `8,612` canonical semantic cases. Neither number is a physical execution
claim: `/proc/self/mountinfo` positive-fragment cardinality varies by live mount
namespace, so physical execution remains explicitly unfrozen.

The new test-only authority manifest contains 369 stable roles and expands them,
together with the unchanged 1,899 hostile IDs, into 8,612 canonical case IDs.
It pins full semantic metadata, role and case ID sets, dedup keys, and exact
17-shard distributions. The prior hostile ID-set hash `1028ac1b...` remains an
exact compatibility assertion. Of the 369 roles, 122 have source-exact windows
and 247 remain visibly unadmitted pending a trace role map; no aggregate-only
role is silently promoted.

The correction also raises the source-exact generic close ceiling from 48 to 52
and corrects `close-staging` to three occurrences: global close 35, 44, and 47.
It records the 37 successful-path opens, 52 closes, three returned-status secure
cleanup calls, and six mountinfo semantic roles. The raw syscall-signature retry
tripwire is accepted only as a partial guard and is not represented as complete
same-role retry proof for the 247 pending roles.

Strict C++17 compilation with `-Wall -Wextra -Wpedantic -Werror`, link, authority
self-test, hostile representative, and parser-boundary checks passed on both
nimo-1 and nimo-2. Both nodes produced binary SHA-256 `6a2519ec...`.
On nimo-2, a fresh focused CTest pass also completed 4/4: feature-off,
initializer anchor, initializer seam contract, and Linux build gates. Its log
SHA-256 is `28f234a2...`.

A fresh fully allocated 1 GiB nodiscard Btrfs canary on nimo-2 passed at
`/var/tmp/halofpx-qualification/l05z-role-authority-canary-20260719-nimo2`.
The newly admitted outer cleanup boundary `close-step4/pre/EIO/49` produced the
required child/launcher failures, sticky whole-root discard, exact retained
tree, exact audit and authority checks, both OFD locks released, and zero final
qualification residue. The RPC service remained continuously available as PID
3562775 with zero reported restarts. nimo-1 retained its PID 971 inference
service and returned HTTP 200.
The equivalent nimo-1 fresh-media canary was not started because `/var/tmp`
free space (43,394,568,192 bytes) was below the fixed 64 GiB host reserve; this
is an explicit nonpromotion limit, not evidence of two-node runtime closure.

Separately, three fresh response observations per node admitted one later
response-loss semantic case: the exact live child emitted one complete
`SYS_write` and no `SYS_writev` in every run. The retained nimo-1 and nimo-2
evidence-manifest hashes are `bccee878...` and `c5f2d11c...`. This review admits
only the observation; response suppression/fake-success implementation remains
deferred.

All four immutable reference clones remain clean at their locked commit and
tree. The change is target-owned test infrastructure: no donor or GPL code,
new dependency, product link, install/export edge, production fault hook, or
persistent server write was introduced.

This review does not admit the 247 pending roles, freeze physical execution
cardinality, implement response loss or storage-window faults, qualify full
returned-fault scale, promote L05z, enable persistence, or make a performance
claim. Those remain later gated work.
