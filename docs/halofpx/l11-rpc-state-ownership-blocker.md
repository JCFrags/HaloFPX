# L11 RPC state ownership and distributed restore blocker

Date: 2026-07-21

Base: `6862ffb99a8056552f62827658f3ffdcc79b9af4`

Outcome: **BLOCKED BY CURRENT RPC OWNERSHIP — no canary implemented**

## Authoritative answer to Wiki O58-02/O58-03

For the measured current ROCmFPX RPC layer-split plan, the coordinator owns the
only `llama_context`, token sequence, server slot, sampling chain, request
boundary, and sequence-state file. The worker owns process-lifetime remote ggml
buffers for offloaded weights, activations, and its placed KV tensors, plus
graph execution. It does not own a rank-local sequence object or checkpoint
writer.

`llama_state_seq_*` serializes the coordinator context's memory module. KV and
recurrent implementations describe tensor ranges, then the selected backend
performs the byte movement. Local tensors are read locally; RPC tensors use
GET/SET tensor commands. The resulting file is therefore a monolithic,
coordinator-owned global blob assembled from local and remote tensor pages.
Sampler state is not part of that sequence blob. Draft/speculative state would
belong to separate coordinator contexts/adapters and was not present in the
measured fixture.

This answers O58-03 for the exact current layer-split RPC tuple: serialization
can be behaviorally complete, but it is neither rank-local nor control-only.
It partially answers O58-02 for attention KV and sampler ownership; recurrent,
draft, MTP, MoE, and true tensor-parallel plans remain open and may not be
extrapolated from this fixture.

## Source trace

| Stage | Current call path | Authority consequence |
|---|---|---|
| sequence capture | `state_seq_write_data -> memory->state_write` | one coordinator context selects all memory state |
| KV capture | `llama_kv_cache::state_write_data -> io.write_tensor` | K/V tensor ranges follow their backend placement |
| recurrent capture | `llama_memory_recurrent::state_write -> io.write_tensor` | same placement-dependent byte path; not exercised by the attention-only fixture |
| host/file capture | `ggml_backend_tensor_get` | remote buffers must return their pages |
| RPC capture | `RPC_CMD_GET_TENSOR` response | state payload crosses worker-to-coordinator |
| host/file restore | `ggml_backend_tensor_set` | blob pages are copied into current buffers |
| RPC restore | `RPC_CMD_SET_TENSOR` request | state payload crosses coordinator-to-worker |
| sampler | coordinator `server_slot::smpl` / `common_sampler` | not serialized by `llama_state_seq_*`; no worker sampler |

The four traced source blobs at current HEAD are:

- `src/llama-context.cpp`: `01fd4c697a14d899b355bd38de94e5b63bd0fc92`
- `src/llama-kv-cache.cpp`: `7afd9e5c86f69817e242e7a1463e4ba52e7b5af7`
- `src/llama-memory-recurrent.cpp`: `addc427b620de54a8e8deaf17542c3d7b1be943f`
- `ggml/src/ggml-rpc/ggml-rpc.cpp`: `2b03f5208874d3f62406009981a90bde81a5ce4d`

## Disposable two-host evidence

| Item | Result |
|---|---:|
| model SHA-256 | `66967fbece6dbe97886593fdbb73589584927e29119ec31f08090732d1861739` |
| evidence plan fingerprint | `121cf2ee200f3a2397d306a62a7f8219537d6df17fbf3bc8c3b53974896f084f` |
| evidence topology fingerprint | `9baf437f2f3ca50fc727d57be522fa86d5ce61754f93bd005da2c49d9746e4e2` |
| coordinator binary SHA-256 | `0a05e2f56869d9b45b43cb17b6c5fd280604ee523f5dbc3afd6e4c15352d61f4` |
| worker binary SHA-256 | `8fcc2d44e0fd0b42fa2dc6da5f9a8479045cc76048ca320b6e21659c4c0ded08` |
| serialized blob | 124,876 bytes |
| capture remote KV transfer | 4 x 10,368 = 41,472 response bytes |
| restore remote KV transfer | 4 x 10,368 = 41,472 request bytes |
| saved/restored token count | 18 / 18 |
| uninterrupted first suffix | token 10534, ` waiting` |
| restored first suffix | token 10534, ` waiting` |

API success and output equivalence were treated only as semantic confirmation.
The RPC command names, direction, sizes, source call path, plan tuple, binaries,
model, and first post-restore token establish the ownership conclusion.

The plan fingerprint binds the measured endpoint, device list, layer split,
proportion, offload count, context, parallelism, logical rank labels, binaries,
and model. A separate topology fingerprint binds both node names, logical rank
labels, `10.44.0.2 -> 10.44.0.1:50062`, `thunderbolt0`, TCP, and world size two.
The exact canonical strings are retained beside the raw logs. Current llama
sequence bytes do not carry that plan/world/rank
authority themselves; compatibility wrappers may reject mismatches, but cannot
convert the coordinator blob into independent rank-local objects.

Raw evidence is retained on nimo-2 in the bundle named by the receipt. Both
disposable user services were stopped afterward. The known-good coordinator
and worker services remained active with `NRestarts=0`.

## Boundary and next gate

ADR-0039 closes this milestone as a blocker. The smallest future gate is not a
wrapper around the current blob: it is a separately approved worker-local
serialization and persistence protocol with exact component ownership,
topology/plan fingerprints, attempt nonce, authenticated readiness, validation
before mutation, all-rank commit-live, bounded control messages, and cold
fallback. Only after a small two-rank model proves that protocol may a plan for
the 160 GB deployment be promoted.

No primary-model run, production mutation, cache corruption, topology fault,
eviction, shared/prefix reuse, or broad fault matrix occurred.
