# L109 independent feasibility review

Verdict: **SEMANTIC BLOCKER — do not retain a scheduler/event-only prototype**

Reviewed base: `56a0526cb4e9fdb1a0d597ca391f47bbcedb9bb5`

## Findings

1. The MiniMax graph is strictly layer-dependent:
   `src/models/minimax-m2.cpp:309-483` completes attention, MoE, residual join,
   and `inpL = cur` before the next layer can begin.
2. Scheduler splits are contiguous backend runs and execute in a strict
   split-index loop (`ggml/src/ggml-backend.cpp:1875-2055,2242-2531`).
   Existing event waits/records govern copy reuse, not independent DAG
   frontiers.
3. RPC graph compute blocks on request/response, its synchronize operation is
   a no-op because no asynchronous work exists, and its backend interface has
   no event, async-copy, or graph-plan operations
   (`ggml/src/ggml-rpc/ggml-rpc.cpp:3767-3770,4009-4330,9356-9358`).
4. The experimental MiniMax shadow path at
   `src/models/minimax-m2.cpp:393-474` is the only source form with separate
   local and peer expert branches. Extending it across layers is graph/model
   ownership redesign and revisits the deferred expert-parallel lane.

## Risks

- **P1 correctness:** early next-layer execution reads unavailable or stale
  activations/routing.
- **P1 ownership/lifecycle:** host-threaded RPC would race tensor writes and
  bypass the current ordered authenticated execute/receipt lifecycle.
- **P2 evidence:** host enqueue, CPU thread, or socket-wait overlap is not
  device overlap; RPC exposes no device event handle with which to prove the
  required timeline.

The reviewer found no eligible prototype within L109. A valid continuation
would require a separately scoped multi-layer expert-parallel graph,
rank-partial protocol/ownership, true asynchronous RPC completion, and a
dependency-aware scheduler.
