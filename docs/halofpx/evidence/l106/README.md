# L106 KV-only request-plan transaction

Status: **BLOCKED before candidate retention**

Base and inspected HEAD: `a35816e52f4bb2510936fa1a29e623c3b9249521`

L106 deliberately narrowed the request-plan transaction to ordinary transformer
KV memory and required unsupported memory implementations to refuse. Source
work confirmed that a KV placement preview itself can be derived from the
existing temporary `llama_kv_cache::prepare()` simulation. Integration with a
fresh-residency cache hit, however, exposes a separate transaction boundary
that the authorized placement contract does not cover.

## Source-proven blocker

The existing coordinator state “prepare” API defers tensor-byte application but
mutates live KV metadata:

1. `llama_context::state_seq_prepare_data()` constructs a
   `llama_io_read_device` with `prepare_only=true`, then calls
   `state_seq_read_data()` (`src/llama-context.cpp:4259-4276`).
2. `state_seq_read_data()` calls the live memory implementation's
   `state_read()` (`src/llama-context.cpp:4464-4469`).
3. KV `state_read()` invokes `state_read_meta()` before staging tensor data
   (`src/llama-kv-cache.cpp:2139-2174`).
4. Sequence metadata restore removes the live destination sequence, finds live
   slots, and calls `apply_ubatch()` (`src/llama-kv-cache.cpp:2309-2363`).
   This changes cells, positions, sequence ownership, and heads.
5. `prepare_only` affects only tensor-buffer construction in
   `llama_io_read_device::finalize()` (`src/llama-context.cpp:3996-4053`).
   It is not propagated to KV metadata handling.
6. On preparation failure, the API clears staged storage but does not restore
   live metadata (`src/llama-context.cpp:4277-4281`). KV restore failure likewise
   clears/removes state rather than rolling back the exact prior metadata.

For a genuinely fresh-residency hit, the captured candidate contains prompt KV
cells/head state that differs from the current empty residency. A placement
preview frozen before lookup is therefore invalidated when the candidate is
prepared. Committing that old preview can select wrong/occupied slots or use the
wrong graph-facing `n_kv`. Rebuilding after preparation violates L106's
single-handle lookup/execution identity. If remote stage or commit later fails,
the coordinator can already contain partially restored metadata.

The existing two-host canary avoids this by creating a fully disposable context,
preparing/restoring into that context, and using it only on complete success
(`tests/test-halofpx-distributed-state-canary.cpp:1080-1125`). llama-server has
no atomic live-context KV metadata/data swap that can combine this with the
already-frozen request plan.

## Minimum safe prerequisite

A candidate-state shadow transaction is required in addition to placement
preview:

- authenticate and decode candidate metadata plus local tensors into shadow KV
  state without touching the live context;
- construct the exact ubatch placement preview against that shadow state;
- bind candidate/component/topology identity and the original live generation;
- stage/authenticate the remote object;
- atomically validate and install shadow metadata/local data together with the
  exact previewed ubatch placement;
- define coordinated remote-commit recovery and expose no partial live mutation;
- execute with that same consumed handle.

This is substantively different from the authorized KV placement preview. No
current `llama_memory_i` or KV API supplies shadow clone/swap/atomic install.
Bypassing it is a P1 correctness and state-integrity risk.

## Actions and safety

- An exploratory KV preview change was removed in full after the boundary was
  proven. No candidate source remains.
- A focused Windows compilation reached final link; the only link error was the
  previously documented feature-on Windows unresolved
  `ggml_backend_rpc_halofpx_mutable_negotiate_preflight` symbol. This build is
  not qualification evidence.
- No host, model, production, protocol, cache publication, or runtime action
  occurred.
- Feature-off, world1, recurrent, hybrid, ISWA, and ordinary decode source remain
  byte/behavior unchanged.
- Local ignored `build-l106` and `build-l106-win` scratch directories may remain;
  recursive removal was refused by the execution policy. They are not evidence
  or tracked source.

