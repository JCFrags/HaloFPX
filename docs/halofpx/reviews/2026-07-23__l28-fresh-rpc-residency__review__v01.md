# L28 fresh RPC residency adversarial review

Date: 2026-07-23

Verdict: **PASS after correction**

The review independently inspected the wired pre-staging guard, A-to-B
ordering, manifest-to-Popen binding, two-residency lifecycle, focused tests,
raw evidence, production snapshots, and cleanup.

The initial review rejected the unauthenticated capture-epoch audit because it
did not prove that epoch A belonged to the exact persisted worker object. The
correction adds a hash-pinned helper which creates and fsyncs an
HMAC-authenticated sidecar before worker A stops. Its payload binds the captured
object digest, worker-A PID and InvocationID, and coordinator-A PID. After model
B readiness and current worker-B revalidation, the runner verifies the exact
sidecar immediately before the only `restore-authorized` handoff.

The reviewer independently confirmed missing, tampered, object-mismatch, and
epoch-mismatch refusal coverage and the 78/78 focused test pass. Final r4
evidence supports distinct A/B worker and coordinator identities, two model
residencies, exact token/suffix and state-digest agreement, zero legacy state
page transfer, unchanged HTTP-200 production snapshots, and complete cleanup.
No material finding remains.

This review does not authorize a primary run, production mutation, cache
promotion, transparent RPC recovery, or L29.

