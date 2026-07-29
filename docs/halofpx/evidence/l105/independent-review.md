# Independent adversarial review

Verdict: **CONFIRMED SEMANTIC BLOCKER**

Severity if bypassed: **P1 correctness/state integrity**

The reviewer independently confirmed:

- `process_ubatch()` commits `llama_memory_context_i::apply()` before graph
  parameters, graph construction, allocation, census, split binding, or
  preflight.
- KV `apply()` changes cells/heads and only then derives the `n_kv` consumed by
  attention graph dimensions and views.
- Recurrent and hybrid implementations have their own mutating `apply()`
  behavior, so a KV-only shortcut is not a valid general contract.
- decode's `seq_rm()` failure cleanup is not an exact rollback of overwritten
  cells, heads, stream mappings, recurrent state, shifts/copies, or scheduler
  allocation.
- server lookup currently precedes slot launch, while the actual decode batch
  is assembled later from compatible live slots; a hand-built per-task plan is
  not execution-identical under continuous batching.

The reviewer found no safe implementation using only the existing memory APIs.
Calling `apply()` during planning mutates before cache authentication; skipping
it produces an undefined or incorrect graph memory geometry; independently
rebuilding violates plan/execution identity.

Recommended prerequisite: a typed transactional memory planning API whose
non-mutating preview owns exact ubatches, slot/stream placement, graph-facing
memory metadata, and a source-memory generation; whose atomic single-use commit
validates that generation and applies precisely that placement; and whose abort
is non-mutating. The server lookup must occur after the real continuous batch is
frozen, or cache eligibility must safely constrain batching, and decode must
consume the same preview handle.

Review performed against source HEAD
`23f088deea65833a714271d7033d9c1c5f46c733`; no source edits were made.
