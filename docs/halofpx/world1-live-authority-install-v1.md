# World-1 live-authority install boundary v1

Status: implemented as a default-off fail-closed install boundary. No positive
production source exists.

Audit baseline: exact commit
`9bfccf25d43af0c446df591035e9cdac0b74d6c0`, 2026-08-12. The table below is a
source audit, not a compatibility or performance result.

## Exact fact-gap table

“Partial” means some live data exists but not a canonical immutable preimage
with complete custody. Partial data is refused; it is not promoted to
authority.

| Required fact | Exact current source | Retained at the server install boundary | Gap before positive authority |
|---|---|---:|---|
| Exact ordered model/shard bytes | `llama_model_loader::files` inside stack-local `llama_model_load()` in `src/llama.cpp` | No | Freeze receipts from the exact opened handles while the loader owns them; reopening a path later is not the same custody. |
| Typed GGUF metadata and tensor file/order/offset records | `llama_model_loader::metadata`, `contexts`, and `weights_map` | No | Retain canonical typed scalars/arrays and tensor records before loader destruction. `llama_model::gguf_kv` skips arrays and stores display strings. |
| Exact tokenizer facts | Loader GGUF plus `llama_vocab` internal token/merge/policy state | Partial | Freeze a registered canonical tokenizer snapshot, including exact bytes, score bits, merge order, special roles, and policy. Public token accessors are not that snapshot. |
| Effective chat template and renderer | `common_chat_templates_init()` and `server_chat_params` in `server_context_impl::init()` | No at store initialization | Move authority/store composition after final template precedence and renderer selection, and bind exact renderer identity. Current store init runs earlier in `load_model()`. |
| System/tool context | Request parsing and rendered canonical tokens | No global immutable preimage | Define request-scoped closed component `[4]`; do not substitute a server default or inferred text boundary. |
| Adapters/projectors | Model LoRA set, multimodal context, control-vector and request flags | Partial | Canonicalize the live set, including an explicit registered “none” profile for the currently eligible path. |
| Immutable runtime build | `common/build-info.cpp.in` exposes build number, commit, compiler, and target strings | No | Retain exact source-tree and dirty-tree bytes, executable and loaded-library bytes, toolchain, and typed build options. `unknown` or a commit string alone is insufficient. |
| State ABI | State implementation and build source | No registered live receipt | Define and freeze the exact state serializer/restore ABI and all feature-affecting build identities. |
| Backend/device ABI | Live backends/devices in model/context internals | Partial | Canonicalize backend library/device identities and resolved capabilities from live objects, not requested device names. |
| Quantization and K/V semantics | Loaded tensors, `llama_context_params`, memory construction | Partial | Freeze exact tensor types/layouts and actual K/V types after validation and fallback. |
| Context, RoPE, and window semantics | Internal `llama_context::cparams`; selected public size queries | Partial | Export one canonical resolved snapshot after auto RoPE, attention, batching, and window decisions. Public size queries are incomplete. |
| Sampler/logits policy | Per-request sampler construction | No reusable closed preimage | Bind the exact admitted greedy-memoryless profile per request; never infer it from a mode name. |
| Grammar/parser/tool policy | Per-request parsing and tool/grammar objects | No reusable closed preimage | Bind a registered explicit “none” profile for the eligible path and reject any non-none state. |
| RNG semantics | Per-request sampling state | No reusable closed preimage | Bind a registered memoryless/no-RNG profile; other RNG state remains excluded. |
| Target/draft/MTP/speculative semantics | Target and optional draft contexts in `load_model()` | Partial | Bind a registered target-only profile and reject draft, MTP, speculative, and multimodal composition before authority use. |
| Global allocation plan | `llama_model_get_allocation_plan()` returns a diagnostic live plan | Partial | Define stable canonical plan bytes and verify completeness after allocation/fallback. Diagnostic structs are not yet a registered component preimage. |
| Rank ownership | World-one target state is implicit in the current product profile | No distinct canonical digest source | Emit distinct ownership facts from the live plan, even for rank 0; do not reuse the global-plan bytes or an operator assertion. |
| Rank placement | Live tensor buffers/devices and allocation plan | Partial | Canonicalize actual tensor-to-buffer/device placement after automatic fitting, with stable device identity. |
| Stable topology and epoch | World-one/rank-zero product admission | Partial | Own a nonzero stable configuration epoch and registered topology preimage. World/rank constants alone are insufficient. |
| Security and scope | Existing scope resolver and request authority | Partial | Compose the exact closed security/scope preimage under trusted principal custody; never derive it from a path or model name. |
| Producer identity | Existing store configuration expects one nonzero producer digest | No live derivation source | Derive a registered producer identity from the exact implementation/codec contract, not an operator digest. |
| Model/context generation | ADR-0054 capability field and task carrier | No independent lifecycle owner | Add a nonzero monotonic generation owned across load/destroy/sleep/resume/recreation and compare it before install, lookup, capture, and publication. |

## Implemented boundary

`HALOFPX_CONTEXT_STORE_WORLD1_LIVE_AUTHORITY_INSTALL` defaults to `OFF` and
requires the ADR-0054 product gate. When explicitly enabled, the server calls
the typed installer with no source and generation zero. The result owns no
authority, so the existing product remains cold and opens no store.

The installer has no model path, filesystem, environment, CLI component, or
standalone world-two builder input. A separately linked trusted source must
declare custody of every table row, present a structurally valid full
world-one capability, and match an independent lifecycle generation. Any
missing fact or uncertainty returns no authority.

The C++ source interface is itself a trust boundary: the source-kind tag and
fact mask do not authenticate an implementation or replace preimage custody.
A future non-null server source therefore requires separate review of its
loader/context/lifecycle ownership and wiring. At this commit the sole server
call passes a null source and generation zero, so no implementation can be
selected through a hidden operator or world-two route.

Likewise, the request generation field currently enforces only nonzero exact
equality with the captured capability. It is not an independent lifecycle
lease and does not close a reload race. A positive source must add monotonic
generation ownership plus pre/post-capture lease validation before this
boundary may be treated as live authority.

The complete-source success in the unit test is a deterministic fake used to
verify ownership and validation. It is not a server source or positive cache
qualification.

## Reproduction

The install-boundary-on host lane configures all ADR-0054 prerequisites plus:

```text
-DHALOFPX_CONTEXT_STORE_WORLD1_LIVE_AUTHORITY_INSTALL=ON
```

It builds `llama-server` and
`test-halofpx-context-store-world1-live-authority-install-v1`, then runs the
behavior, boundary, product, selector, exact-session, state-transformer, and
feature contract tests. A separate install-boundary-off build checks the generated
graph and server contract. Exact commands and results belong in the PR
qualification receipt after the final committed tree is tested.

No target commands are part of this slice.
