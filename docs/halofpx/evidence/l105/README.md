# L105 request-plan ownership seam

Status: **BLOCKED before candidate source**

Base and inspected HEAD: `23f088deea65833a714271d7033d9c1c5f46c733`

L105 was authorized to create a single-use, non-executing plan handle from the
exact request ubatch, run the execution-identical graph build/allocation/census/
split/preflight path, and use the same owned plan for cache hit reconciliation or
miss execution without rebuilding.

## Source-proven boundary

The current memory/graph interface cannot produce that exact plan without first
mutating live model memory:

1. `llama_context::decode()` performs pending memory updates and creates a
   batch memory context before entering the ubatch loop
   (`src/llama-context.cpp:2768`, `:2773`).
2. `process_ubatch()` calls `mctx->apply()` before computing graph parameters or
   building the graph (`src/llama-context.cpp:1571-1583`).
3. The memory interface explicitly defines `apply()` as its mutation operation
   (`src/llama-memory.h:49-59`).
4. For the transformer KV implementation, `apply()` commits the ubatch into the
   KV cells/heads and only then derives `n_kv`
   (`src/llama-kv-cache.cpp:2602-2628`). The graph parameters consume this
   applied memory context, so an execution-identical graph cannot be built from
   the current interface before the mutation.
5. `llama_kv_cache::prepare()` does have an internal temporary mutation and
   restoration stack for slot search (`src/llama-kv-cache.cpp:835-913`), but it
   returns only a context whose public operation is the committing `apply()`.
   It exposes neither a non-mutating preview value for graph construction nor a
   later atomic commit operation.
6. Decode failure cleanup removes positions at and after the failed ubatch
   (`src/llama-context.cpp:2848-2867`). It is not a general rollback contract
   for overwritten cells, prior heads, recurrent/hybrid memory, pending shifts,
   copies, or other state changed by `apply()`.
7. This is not confined to the ordinary KV implementation. Recurrent `apply()`
   calls the mutating slot selection and resets rollback indices while graph
   accessors read the resulting head/state (`src/llama-memory-recurrent.cpp:
   1176-1239`); hybrid `apply()` commits both attention and recurrent contexts
   (`src/llama-memory-hybrid.cpp:245-251`).

The server ordering reinforces, but does not cause, this boundary. Exact-key
restore currently occurs per task before slot launch
(`tools/server/server-context.cpp:1406`, `:2784`), while the actual decode batch
is assembled later from live slots and passed to `llama_decode*`
(`tools/server/server-context.cpp:3918`, `:4086-4087`). Moving lookup to the
later scheduler point is possible, but it does not make graph planning
non-mutating.

## Required semantic decision

The smallest safe prerequisite is a typed transactional memory-plan contract:

- non-mutating preview of the exact ubatch slot/head/cell and graph-facing
  memory values (including `n_kv`);
- immutable ownership of that preview by one request-plan handle;
- atomic single-use commit immediately before execution;
- exact stale/drift/conflict validation against live memory;
- complete rollback/drop semantics across KV, recurrent, hybrid, pending
  shift/copy, and multi-ubatch behavior.

Emulating this with `apply()` followed by `seq_rm()` would violate L105's
non-mutating requirement and could lose or corrupt pre-existing memory. Holding
candidate-derived graph authority without the exact applied memory state would
instead violate plan-vs-execution identity. No candidate source was created.

## Scope and safety

- No build, host, model, production, protocol, cache publication, or runtime
  action occurred.
- Feature-off and world1 behavior are unchanged because source is unchanged.
- Pre-existing untracked evidence and archives were not modified.
