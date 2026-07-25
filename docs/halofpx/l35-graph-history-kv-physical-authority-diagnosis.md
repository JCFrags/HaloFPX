# L35 graph-history and KV physical-authority diagnosis

Status: `[MEASURED] PASS` for the bounded no-primary diagnostic contract; it
does not identify the primary-model root cause.

Scope is deliberately no-production and no-primary-load. L35 does not authorize a
primary discriminator, cache promotion, performance work, or a follow-on lane.

## Replay mutable-state coverage census

The exact replay is one token decoded immediately after the 1,128-token sequence
boundary. The table enumerates mutable state read from `llama_context::decode`,
`process_ubatch`, `llama_kv_cache::{prepare,apply_ubatch,get_k,get_v}`, graph input
binding, graph execution, logits extraction, and greedy sampling.

| Mutable authority | Source owner / read path | Existing authenticated authority | L35 disposition |
|---|---|---|---|
| replay token, count, batch position, sequence id | canary `suffix`; batch allocator; graph inputs | L34 semantic provenance | covered |
| causal-attention and flash-attention selection | `llama_cparams`; graph parameters | frozen request only | authenticated L35 metadata |
| graph topology parameters and reusable graph result | `graph_params`; `gf_res_prev::can_reuse` | uncovered | graph reuse/reset decision and node count |
| scheduler allocations and reset/rebuild history | `ggml_backend_sched_*`; `process_ubatch` | uncovered | scheduler reset decision; canonical reset control |
| token input tensor | graph-result `set_inputs`; token embedding lookup | replay token authenticated by L34 | value covered; graph tensor/buffer binding uncovered |
| absolute position input | `llm_graph_input_pos::set_input` | L34 before/after positions | value covered; graph tensor/buffer binding uncovered |
| sequence-id membership input | memory graph inputs | L32 sequence authority | authenticated L35 `kv_sequence_ids`; tensor binding uncovered |
| KQ mask / causal mask values | attention graph input setters | causal mode frozen; values not serialized | uncovered direct tensor content/binding |
| K-shift / rotation inputs | KV update graph inputs | no shift in the one-token replay | not active in observed fixture; primary activation remains to be measured |
| KV write indices | KV `set_input_k_idxs` / `set_input_v_idxs` | uncovered before L35 | exact prepare/apply slots and current-residency indices |
| attention KV gather/view inputs | `get_k/get_v`, KV graph inputs | uncovered before L35 | exact layer/kind/view offset/dims/strides and `n_kv`; any separately materialized index tensor content remains uncovered |
| cache-length scalar | `llama_kv_cache_context::get_n_kv` | uncovered before L35 | authenticated exact `kv_n` |
| backend/device order and logits backend | context `backend_ptrs`; scheduler tensor backend | frozen placement | authenticated ordered backend list and logits backend |
| output count, batch-token-to-row mapping, lazy swaps | `n_outputs`, `output_ids`, `output_swaps` | L34 only proves final logits hash | authenticated output mapping metadata |
| KV prepare slot selection | `llama_kv_cache::prepare/find_slot` | uncovered | exact stream/cell indices |
| KV actual apply slot selection | `llama_kv_cache_context::apply` | uncovered | exact stream/cell indices; verifier refuses prepare/apply inequality |
| KV allocator search cursors (`v_heads`) | `find_slot`, `apply_ubatch` | not serialized by design | authenticated before/after cursors |
| KV cell sequence ownership and positions | `apply_ubatch`; `llama_kv_cells` | L32 sequence/cell semantic receipt | covered semantically; physical indices added by L35 |
| attention cache length (`n_kv`) | `get_n_kv`; memory context | uncovered | authenticated exact value |
| K/V storage type, geometry, strides, buffer-relative range | KV layers and graph views | L30 component identity/content and logical range | authenticated base tensor tuples; no cross-process pointer equality |
| attention K/V view offsets and strides | `get_k/get_v` | uncovered before L35 | directly emitted authenticated layer/kind/view-offset/dimension/stride tuples |
| asynchronous KV writes and logits copies | backend scheduler; async tensor copy | L30 synchronization correction and L34 synchronized logits | covered by required synchronize before hashing |
| synchronized full replay logits and selected token | host logits buffer; greedy sampler | L34 authenticated SHA-256/argmax/token | retained unchanged |
| sampler history | greedy sampler freshly constructed in each suffix | specialist audit; no mutable history | not applicable |
| recurrent/non-KV memory | model architecture selection | specialist audit: MiniMax M2 uses standard KV | not applicable to the target hypothesis |

The census does **not** claim that different physical KV cells across fresh
residencies are defective. Physical indices are allocator-local. The relevant
invariant is that prepare and actual apply in one replay agree and that the graph
uses the resulting current-residency authority.

## Frozen discriminator

One combined stories15M two-residency fixture will execute ordinary capture and
restore history, then the same bounded lifecycle with a synchronized canonical
graph/scheduler/output reset immediately before replay in both paths. Each replay
emits an HMAC-authenticated metadata-only record plus the existing synchronized
full-logits hash. The verifier rejects malformed, missing, duplicate, tampered, or
prepare/apply-inconsistent authority.

Binding a fresh restore to captured physical cell/head indices is not source-safe:
those indices belong to the capture residency's allocator and can collide with or
misrepresent the fresh target residency. L35 therefore observes and validates
current-residency selection but does not force cross-residency pointer or cell
identity.

## Residual uncertainty

Direct token/position/mask graph-tensor buffer bindings, any separately
materialized attention-index tensor content, and per-node scheduler assignment
remain uncovered. Attention K/V view tuples themselves are emitted directly.
These gaps are recorded rather than converted into speculative authority. No
primary conclusion can be drawn under L35.

## Qualification result

The combined stories15M discriminator used the accepted 1,129-token boundary and
two fresh distributed model residencies per subcase:

- Ordinary history: capture and restore both rebuilt the replay graph
  (`graph_reused=0`, `scheduler_reset=1`, 222 nodes), prepare and actual apply
  both selected stream 0/cell 1128, `n_kv=1280`, and all 12 K/V attention views
  and 12 backing tensors were authenticated.
- Canonical reset: the same exact authority and result were observed after the
  explicit synchronized graph/scheduler/output reset in both paths.
- Both subcases produced full-logits SHA-256
  `f6d0fa35238815d19f10cb97a0af1c75349080fa431064a25a4969f8d9b177b1`
  and greedy token `4245` in capture and restore. Worker state was 1,156
  components / 5,197,824 bytes. State-window legacy GET/SET count was zero.
- A first harness attempt is retained immutably. The model restore itself
  completed correctly, but the controller rejected its honest `n_batch=0`
  restore report using a capture-only `n_batch=512` assertion. The narrow
  correction admits zero only for semantic-diagnostic restore; capture remains
  pinned to 512.
- The final source compiled both the canary and RPC worker with the local-state
  feature enabled, while all replay instrumentation remains runtime-default-off.

This result does not support graph-history or KV prepare/apply divergence on the
accepted fixture. The smallest future primary discriminator is a single fresh-
residency restore that compares the authenticated L35 replay records and logits
already specified here. L35 does not authorize it.
