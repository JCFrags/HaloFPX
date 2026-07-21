# ADR-0039: current RPC tensor-split distributed restore is not admissible

Status: accepted blocker for the current ROCmFPX RPC tensor-split path. No
two-rank persistent-restore canary is admitted.

Date: 2026-07-21

## Question

Can the current HaloFPX/ROCmFPX RPC tensor-split runtime produce complete,
rank-bound persistent state while each rank reads and writes only its local
storage and restore coordination carries identifiers rather than state pages?

This decision specializes Wiki Section 58, especially O58-02 and O58-03, for
the exact layer-split RPC tuple measured here. It does not claim to resolve
ownership for every future tensor-parallel, recurrent, draft, MTP, or MoE plan.

## Source authority

The traced files are unchanged between L10c and this milestone and therefore
describe commit `6862ffb99a8056552f62827658f3ffdcc79b9af4`:

- `llama_context::state_seq_write_data()` and `state_seq_read_data()` delegate
  only to the context memory module.
- Attention KV serialization calls `io.write_tensor()` for each selected K/V
  range; restore calls `io.read_tensor()`.
- File and ordinary host serializers implement those calls with
  `ggml_backend_tensor_get()` and `ggml_backend_tensor_set()`.
- An RPC-backed tensor maps those operations to `RPC_CMD_GET_TENSOR` and
  `RPC_CMD_SET_TENSOR`. The GET response and SET request contain the requested
  tensor bytes.
- Recurrent-memory serialization uses the same `io.write_tensor()` and
  `io.read_tensor()` mechanism. Placement therefore controls whether its pages
  cross RPC; the current fixture did not contain recurrent state.
- Server sampling is owned by the coordinator's `server_slot` and
  `common_sampler`; sequence-state serialization contains the memory module,
  not that sampler object. The worker has no server slot or sampler.

The RPC worker is a remote ggml buffer/graph executor. It owns the resident
bytes of remotely allocated tensors during the process lifetime, but it owns
no `llama_context`, sequence serializer, rank-local checkpoint manifest, local
persistent writer, attempt nonce, or ready/commit-live protocol.

## Measured fixture

A disposable two-host CPU fixture used nimo-2 as coordinator and nimo-1 as RPC
worker over `10.44.0.1:50062`. The 19 MB Stories 15M Q4_0 fixture was pinned by
SHA-256. The plan was `RPC0`, layer split, tensor proportion `1`, three
offloaded layers, world size two in the experimental ownership model, one
server slot, 256-token context, and no WebUI.

The evidence-only canonical plan string hashes to
`121cf2ee200f3a2397d306a62a7f8219537d6df17fbf3bc8c3b53974896f084f`.
The separate canonical two-host topology string hashes to
`9baf437f2f3ca50fc727d57be522fa86d5ce61754f93bd005da2c49d9746e4e2`.
Both exact canonical input strings are retained in the evidence bundle.
Logical ranks zero and one are experiment labels; current RPC exposes devices,
not an authenticated rank/world/plan identity. The raw sequence file checks
model architecture and memory structure but carries no explicit RPC endpoint,
device order, split ratio, logical rank, world size, placement, plan digest, or
topology epoch. Those facts can be bound by HaloFPX compatibility authority,
but binding does not make the bytes rank-local.

After an 11-token prompt and eight generated tokens:

- `llama_state_seq_save_file()` wrote one 124,876-byte coordinator-side blob;
- capture issued four remote GET operations of 10,368 bytes, transferring
  41,472 KV bytes from worker to coordinator;
- restart-restore issued four remote SET operations of 10,368 bytes,
  transferring the same 41,472 KV bytes from coordinator to worker;
- the uninterrupted and restored first suffix token both equalled token
  `10534`, text ` waiting`; and
- the RPC worker produced no independent local state object.

The exact output equivalence proves that the monolithic coordinator blob can
be globally complete for this exact plan. It does not make that blob
rank-local, topology-stable, or admissible under the distributed contract.

## Decision

Stop before implementation. The current API cannot provide complete
rank-bound persistent state without sending state pages over the RPC control
connection. This violates the locked L02 rule
`state_payload_on_restore_control_plane: false` and Wiki Section 58's
rank-local-object and identifier-only readiness requirements.

Binding the existing monolithic blob to world size, plan, rank placement, and
topology would reject mismatches but would not repair ownership. Adding a
nonce or all-ready wrapper around coordinator-side GET/SET traffic would still
move pages and would falsely label the RPC worker as an independently restoring
rank.

An admissible design requires a new, explicitly reviewed protocol in which the
worker can serialize its own owned state into local immutable objects, validate
them before live mutation, report only bounded authenticated readiness, and
participate in all-rank commit-live. That is a broad protocol and is outside
this milestone's authority.

## Consequences

- The L10d single-process exact-key path remains unchanged and default-off.
- No distributed restore mode, compile gate, runtime option, wire message,
  persistent write, or production behavior is added.
- Missing/corrupt/topology negative canary tests are not meaningful until
  rank-local serialization authority exists; ordinary distributed operation
  must recompute cold.
- The 160 GB primary model is not tested. Its exact next qualification plan is
  blocked on a separately approved worker-local state API and coordination ADR.
- Production services, reference clones, models, and network configuration
  remain unchanged.

Rollback is trivial: this milestone contains documentation and evidence only.
