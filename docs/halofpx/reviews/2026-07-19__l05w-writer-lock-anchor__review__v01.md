# L05w writer-lock anchor independent review

**Result: ACCEPT.**

The review covered correctness, freshness, clarity, provenance, security,
rollback, performance isolation, and reusable improvements for the L05w
discard-only anchor.

The final source keeps authenticated fd3/fd4 state file-private and locked,
closes both descriptors before root access, preserves the L05v compatibility
path, admits only the exact `writer.lock` create/chmod/file-sync/OFD-lock
prefix, revalidates held and reopened identities, and keeps every post-latch
ordinary outcome discard-required. Cleanup ordering, explicit OFD unlocks,
root-guard release, signal restoration, pre-latch positive-fact scrubbing,
default-off build gates, archive closure, and product/install/export isolation
are consistent with ADR-0026.

The test-only live launcher now pins paths and sealed inputs in a controller,
closes its copies, and releases an exec'd clean child. The child validates and
closes a sealed public request handoff, arms CLOEXEC on fd3/fd4, and then calls
the initializer without controller filesystem preflight. Its wait, kill/reap,
and SIGPIPE paths are bounded and controlled. The external ptrace controller
was separately reviewed after it proved exact child identity, syscall
entry/exit stops, SIGKILL termination, nonzero launcher outcome, lock release,
bounded cleanup, and synchronized receipts.

No GPL llama-ai implementation, CachyLlama unit, donor cherry-pick, new remote,
WebUI surface, persistent server write, completed initialization, or durability
claim entered the milestone. The two initial pre-split live canaries and the
nimo-1 ptrace smoke are retained as excluded intake; promoted totals use only
the final clean-child runs and final reviewed ptrace matrices.

Rollback is source-only: remove or revert the anchor include/API/tests,
test-only ptrace controller, and CMake routing. Because every target remains
default-off and `EXCLUDE_FROM_ALL` and no product, service, deployment, or
persistent server state changed, no runtime rollback is required. A
qualification root that crossed the latch remains discard-only and may never
be adopted or repaired; its detached loopback image may be retained solely as
evidence.

No actionable P0, P1, or P2 finding remains.
