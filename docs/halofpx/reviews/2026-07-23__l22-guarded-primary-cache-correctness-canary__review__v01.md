# Independent review: L22 guarded primary cache correctness canary

Date: 2026-07-23

Verdict: **NOT PROMOTED**

## Findings

1. The retained invocation used the exact pinned artifact and request tuple:
   `RPC0,ROCm0`, layer split `1,1`, context 4096, batch/ubatch 512, Q8_0
   K/V, flash attention on, 1,129 prompt tokens, boundary 1,128, 128
   generated tokens, seed 1234, and temperature zero.
2. CAPS admitted exact rank 1/world 2, state protocol 1.0, and RPC 4.0.1.
   Placement admitted ordered RPC0 then ROCm0, with 32 RPC and 30 ROCm
   repeating layers and output on ROCm0. The worker journal records an actual
   80,950,550,528-byte RPC material allocation. No equally direct local ROCm
   material byte total was retained; this is a residual evidence gap.
3. Capture stored 2,454,528 rank-local bytes across 64 components.
   Coordinator-local and control bytes were 2,301,688 and 15,048. Capture and
   restore state windows contain zero legacy `GET_TENSOR` or `SET_TENSOR`
   operations and include stored, ready, and apply evidence.
4. Capture, clean cold, missing-object fallback, plan-mismatch fallback, and
   mode-off cold continuations are exact. The true worker-restart restore is
   not: token SHA-256 changed from `d8c882...b6d9` to `d32aca...1470`, and
   decoded SHA-256 changed from `3c9bcc...026f` to `b51fb7...edf1e`.
   This is a correctness failure, not a performance result.
5. The controller recovered the production worker before the coordinator.
   Exact commands, models, listeners, HTTP 200, and `NRestarts=0` reconcile.
   A live closeout check found the worker PID 1396163 on 50052 and coordinator
   PID 2213675 on 8081. All disposable units, paths, key files, and port 50180
   were absent.

The focused suite passed 62/62 and `git diff --check` passed. The reviewer
requires a terminal L22 NOT PROMOTED closeout with immutable evidence retained,
no retry, and no L23.
