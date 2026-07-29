# L109 scheduler-wide rank-overlap screen

Status: **NOT PROMOTED — source-proven semantic blocker**

Base: `56a0526cb4e9fdb1a0d597ca391f47bbcedb9bb5`

Predeclared synthetic screen threshold: at least 10% matched end-to-end
improvement, with exact output and device-event proof of concurrent RPC0 and
ROCm0 execution across multiple layers.

No candidate source is retained. No build, host, accelerator, model,
production, or runtime action occurred.

## Profiling reconciliation

P08 measured approximately alternating 30 ms remote and 30 ms local phases,
roughly half GPU duty on both ranks, and low aggregate link utilization. It
identified serialized rank work as a high-leverage target but did not prove
that the existing graph contains independent rank work.

Current source closes that distinction:

- `src/models/minimax-m2.cpp:309-483` constructs a strict layer chain. Layer
  `N+1` consumes `inpL`, which is assigned only after attention, the complete
  MoE result, and the residual join for layer `N`.
- `ggml/src/ggml-backend.cpp:2242-2531` executes backend splits in order.
  Its events protect copy-buffer reuse; they are not a DAG-frontier scheduler.
- `ggml/src/ggml-rpc/ggml-rpc.cpp:4009-4330` implements RPC graph execution as
  a blocking request/response operation and exposes neither backend events nor
  asynchronous copy/plan primitives.

Consequently, there is no next-layer ROCm0 work that is device-ready while the
preceding RPC0 layer work is outstanding. A host thread could overlap enqueue
or socket wait, but it could not prove concurrent device execution and would
not satisfy the L109 gate.

## Prior evidence

P11 and P13 identify the only source-backed route to independent work: split
each layer's selected experts into local and remote ownership branches and join
the rank partials before the next layer. P09 and P10 already found
multi-percent exact-model regressions in related graph-level/fused
expert-partial attempts. P13 retained only a low-leverage private direct HIP
canary and explicitly closed RPC/model integration absent a design affecting
multiple layers or scheduler-wide serialization. P14 found no useful row-split
signal.

Applying that model across layers would require graph/model ownership redesign,
an asynchronous RPC completion authority, and scheduler DAG-frontier support.
That is materially the deferred expert-parallel architecture lane, not the
small default-off scheduler/event prototype authorized for L109.

## Decision

The independent review verdict is **FAIL-to-prototype / semantic blocker**.
Launching layer `N+1` before the complete layer-`N` join would race unavailable
activations and routing, violate tensor lifetime/ownership, and break exact
output. Concurrent RPC commands would additionally require lifecycle and
receipt ordering that the current authenticated RPC path does not provide.

L109 therefore stops before implementation. The 10% performance screen cannot
be run honestly under the authorized boundary, and no synthetic speed claim is
made.

The smallest technically viable future effort is a separately decomposed
multi-layer expert-parallel architecture: one routing result per layer,
concurrent rank-owned partials, exact fixed-order join, true asynchronous RPC
completion, and dependency-aware scheduling. It must not be presented as a
scheduler-only patch.
