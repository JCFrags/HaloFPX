# L13 primary-model worker-local state canary independent review

Date: 2026-07-21

Scope: read-only adversarial review of the L13 source, frozen configuration,
raw evidence, production recovery, cleanup, milestone narrative, and receipt.

Verdict: **ACCEPT THE NEGATIVE SAFETY-STOP OUTCOME — NO P1/P2 FINDING REMAINS**

## Findings and reconciliation

The initial draft had two P2 evidence-attribution errors:

1. It presented the post-fix retry-preflight binary and compatibility identity
   as though they described the first failed capture. The corrected milestone
   and receipt now distinguish attempt 1 (`52f7991`, coordinator binary
   `7b030595...`) from the unexecuted retry-preflight build (`bed36b7`,
   coordinator binary `3a64f1bc...`). The first build reached the long-prompt
   assertion. The second binary never launched because the production
   stop-order gate fired first.
2. It labeled 1,128 as the prompt-token count. The checkpoint authority records
   1,129 prompt tokens and a saved boundary of 1,128; both fields are now
   explicit.

The review also required the `bed36b7` batching correction to remain
unqualified. Its chunking change is source-plausible, but every retained
post-fix smoke attempt failed or exited without a successful result and the
primary retry never launched. A future authorized lane must first prove a
successful `count > n_batch` runtime case.

## Evidence verdict

The retained evidence reconciles the exact model size and SHA-256, request,
configuration, topology, capacity, protected paths, focused protocol probes,
and both source builds. The first capture log shows the assertion before any
state capture. Its worker journal has no CAPTURE, STAGE, READY, or APPLIED
operation, so no worker-local object was published.

The production evidence records the coordinator abort after the worker was
stopped out of order and its later clean start. At final review:

- nimo-2 worker was active as PID 1247685 on port 50052 with `NRestarts=0`;
- nimo-1 coordinator was active as PID 2068256 on port 8081, HTTP 200, with
  `NRestarts=0`;
- no listener remained on disposable ports 50176 or 50177;
- disposable processes, units, roots, keys, state roots, builds, and exact
  clones were absent; and
- the live coordinator command named the standard production UD-Q6 model, not
  the ROCmFPX canary artifact.

The evidence therefore supports a negative operational milestone only. It
does not support decoded/token equality, performance, zero-state-transfer,
restore, corruption, mismatch, or runtime-off claims. Primary-model promotion
is correctly denied.
