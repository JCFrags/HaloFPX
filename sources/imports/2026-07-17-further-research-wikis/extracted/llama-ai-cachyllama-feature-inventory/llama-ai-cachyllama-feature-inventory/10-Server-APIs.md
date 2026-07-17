# Server APIs

The pinned server exposes OpenAI, Responses, Anthropic, native completion, embeddings, reranking, transcription, slot, stream, model, LoRA, expert, UI, structured-output, and other compatibility surfaces. Experimental host tools and MCP proxy are included because they materially affect the trust boundary.

| Decision | Count |
|---|---:|
| RETAIN | 18 |
| REDESIGN | 2 |
| REJECT | 2 |

<a id="f-066"></a>
### F-066 — OpenAI chat and completions compatibility

**Decision:** `RETAIN` · **Portability:** `High` · **Maturity:** `M3` · **Confidence:** `High`

**Observed behavior.** Serves OpenAI-compatible chat-completions and completions endpoints, including streaming.

**Implementation.** `tools/server/server.cpp`, `tools/server/server-context.cpp`, `tools/server/server-task.cpp`

**Dependencies.** HTTP server; JSON; chat templates

**License.** MIT

**ROCmFPX overlap.** ROCmFPX is built on a newer llama.cpp server.

**Porting rationale.** Keep target-native API behavior and add cache/user extensions through namespaced fields or headers.

**Risks / caveats.** Compatibility fields evolve; avoid fork-only semantics without versioning.

**Evidence.** [E-090](20-Evidence-Index.md#e-090), [E-102](20-Evidence-Index.md#e-102), [E-093](20-Evidence-Index.md#e-093)

<a id="f-067"></a>
### F-067 — OpenAI Responses compatibility

**Decision:** `RETAIN` · **Portability:** `High` · **Maturity:** `M2` · **Confidence:** `High`

**Observed behavior.** Converts Responses API requests to chat-completion internals and emits Responses-style events.

**Implementation.** `tools/server/server.cpp`, `tools/server/server-context.cpp`, `tools/server/server-task.cpp`

**Dependencies.** request conversion; SSE event formatter

**License.** MIT

**ROCmFPX overlap.** ROCmFPX's target base already includes modern response formatting.

**Porting rationale.** Reconcile with target rather than cherry-picking fork code.

**Risks / caveats.** Event compatibility is sensitive to upstream changes.

**Evidence.** [E-090](20-Evidence-Index.md#e-090), [E-102](20-Evidence-Index.md#e-102), [E-093](20-Evidence-Index.md#e-093)

<a id="f-068"></a>
### F-068 — Anthropic Messages compatibility

**Decision:** `RETAIN` · **Portability:** `Medium` · **Maturity:** `M2` · **Confidence:** `High`

**Observed behavior.** Converts Anthropic Messages requests to internal chat completions and returns Anthropic-formatted streaming/non-streaming responses.

**Implementation.** `tools/server/server.cpp`, `tools/server/server-context.cpp`, `tools/server/server-task.cpp`

**Dependencies.** protocol converter; SSE

**License.** MIT

**ROCmFPX overlap.** ROCmFPX may already carry this upstream surface.

**Porting rationale.** Retain target-native implementation; map metadata.user_id only through trusted identity policy.

**Risks / caveats.** Protocol conversion can lose unsupported semantics.

**Evidence.** [E-090](20-Evidence-Index.md#e-090), [E-102](20-Evidence-Index.md#e-102), [E-081](20-Evidence-Index.md#e-081)

<a id="f-069"></a>
### F-069 — Legacy completion and infill endpoints

**Decision:** `RETAIN` · **Portability:** `High` · **Maturity:** `M3` · **Confidence:** `High`

**Observed behavior.** Exposes native completion and fill-in-the-middle operations in addition to OpenAI-compatible endpoints.

**Implementation.** `tools/server/server.cpp`, `tools/server/server-context.cpp`

**Dependencies.** model FIM tokens; prompt formatter

**License.** MIT

**ROCmFPX overlap.** Useful for coding models quantized by ROCmFPX.

**Porting rationale.** Keep target-native support and capability checks.

**Risks / caveats.** FIM token assumptions are model-specific.

**Evidence.** [E-090](20-Evidence-Index.md#e-090), [E-102](20-Evidence-Index.md#e-102)

<a id="f-070"></a>
### F-070 — Embeddings and reranking endpoints

**Decision:** `RETAIN` · **Portability:** `High` · **Maturity:** `M3` · **Confidence:** `High`

**Observed behavior.** Supports embedding generation and rank-model reranking when the server is started in the corresponding mode.

**Implementation.** `tools/server/server.cpp`, `tools/server/server-context.cpp`

**Dependencies.** embedding/rank model; pooling configuration

**License.** MIT

**ROCmFPX overlap.** Independent of cache port and likely already present in target.

**Porting rationale.** Retain from target base.

**Risks / caveats.** Persistent generation-state caching does not apply unchanged to embedding/rank workloads.

**Evidence.** [E-090](20-Evidence-Index.md#e-090), [E-102](20-Evidence-Index.md#e-102)

<a id="f-071"></a>
### F-071 — Audio transcription compatibility

**Decision:** `RETAIN` · **Portability:** `Medium` · **Maturity:** `M2` · **Confidence:** `High`

**Observed behavior.** Maps supported audio transcription requests through multimodal chat processing.

**Implementation.** `tools/server/server.cpp`, `tools/server/server-context.cpp`

**Dependencies.** mtmd; audio-capable model; multipart HTTP

**License.** MIT

**ROCmFPX overlap.** ROCmFPX model quantization can apply to multimodal-capable models if kernels/types support them.

**Porting rationale.** Keep target-native endpoint; exclude multimodal prompt states from persistent cache until deterministic state identity is defined.

**Risks / caveats.** Target disk cache explicitly skips multimodal token states.

**Evidence.** [E-090](20-Evidence-Index.md#e-090), [E-102](20-Evidence-Index.md#e-102)

<a id="f-072"></a>
### F-072 — Slot state administration API

**Decision:** `RETAIN` · **Portability:** `High` · **Maturity:** `M3` · **Confidence:** `High`

**Observed behavior.** Provides GET slot status and POST save/restore/erase operations for a selected slot.

**Implementation.** `tools/server/server.cpp`, `tools/server/server-context.cpp`

**Dependencies.** slot endpoint enablement; slot-save path

**License.** MIT

**ROCmFPX overlap.** Likely inherited by ROCmFPX.

**Porting rationale.** Keep with authentication and distinguish manual slot files from automatic persistent cache entries.

**Risks / caveats.** Manual restore can overwrite active state.

**Evidence.** [E-090](20-Evidence-Index.md#e-090), [E-091](20-Evidence-Index.md#e-091)

<a id="f-073"></a>
### F-073 — Resumable stream sessions

**Decision:** `RETAIN` · **Portability:** `Medium` · **Maturity:** `M2` · **Confidence:** `High`

**Observed behavior.** Associates SSE output with a conversation ID and exposes lookup, retrieval, and deletion for reconnecting clients.

**Implementation.** `tools/server/server.cpp`, `tools/server/server-stream.h`, `tools/server/server-stream.cpp`, `tools/server/server-models.cpp`

**Dependencies.** bounded ring buffer; conversation ID; router mapping

**License.** MIT

**ROCmFPX overlap.** ROCmFPX can benefit for long agent outputs independently of KV persistence.

**Porting rationale.** Port only if absent from the target base; bind stream lookup/deletion to authenticated ownership.

**Risks / caveats.** Conversation IDs can become bearer secrets.

**Evidence.** [E-097](20-Evidence-Index.md#e-097), [E-098](20-Evidence-Index.md#e-098), [E-099](20-Evidence-Index.md#e-099)

<a id="f-074"></a>
### F-074 — Model router lifecycle API

**Decision:** `RETAIN` · **Portability:** `Medium` · **Maturity:** `M2` · **Confidence:** `High`

**Observed behavior.** Lists configured models, reports status/presets, loads/unloads models, and streams lifecycle events.

**Implementation.** `tools/server/server-models.cpp`, `tools/server/server.cpp`

**Dependencies.** router mode; SSE; child process manager

**License.** MIT

**ROCmFPX overlap.** Useful for ROCmFPX quant variants.

**Porting rationale.** Use target-native router API with authorization and resource controls.

**Risks / caveats.** Experimental surface and sensitive preset exposure.

**Evidence.** [E-095](20-Evidence-Index.md#e-095), [E-106](20-Evidence-Index.md#e-106)

<a id="f-075"></a>
### F-075 — LoRA hot-swap API

**Decision:** `RETAIN` · **Portability:** `High` · **Maturity:** `M3` · **Confidence:** `High`

**Observed behavior.** Lists and changes active LoRA adapters without a process restart.

**Implementation.** `tools/server/server.cpp`, `tools/server/server-context.cpp`

**Dependencies.** LoRA loader; context adapter API

**License.** MIT

**ROCmFPX overlap.** Useful to ROCmFPX models and independent of cache implementation.

**Porting rationale.** Keep target-native implementation; include adapter set in cache compatibility fingerprint.

**Risks / caveats.** Restoring state produced under a different adapter set is invalid.

**Evidence.** [E-100](20-Evidence-Index.md#e-100), [E-101](20-Evidence-Index.md#e-101)

<a id="f-076"></a>
### F-076 — MoE expert observation/control API

**Decision:** `REDESIGN` · **Portability:** `Medium` · **Maturity:** `M1` · **Confidence:** `Medium`

**Observed behavior.** Registers expert tracking/statistics and control routes for MoE diagnostics and tuning.

**Implementation.** `tools/server/server.cpp`, `tools/server/server-context.cpp`

**Dependencies.** MoE model instrumentation; JSON/HTTP

**License.** MIT

**ROCmFPX overlap.** ROCmFPX has strong MoE/Strix interest but target instrumentation may differ.

**Porting rationale.** Treat as an optional diagnostic extension; reconcile against target graph/kernel architecture and protect control endpoints.

**Risks / caveats.** Limited test evidence and potential performance overhead.

**Evidence.** [E-100](20-Evidence-Index.md#e-100)

<a id="f-077"></a>
### F-077 — Tokenize, detokenize, and apply-template APIs

**Decision:** `RETAIN` · **Portability:** `High` · **Maturity:** `M3` · **Confidence:** `High`

**Observed behavior.** Exposes tokenizer conversion and chat-template rendering without running inference.

**Implementation.** `tools/server/server.cpp`, `tools/server/server-context.cpp`

**Dependencies.** model vocabulary; chat template engine

**License.** MIT

**ROCmFPX overlap.** Useful for exact persistent-cache key construction and clients.

**Porting rationale.** Keep target-native endpoints and include template identity in persistent cache keys.

**Risks / caveats.** Template changes alter token sequences and invalidate state.

**Evidence.** [E-090](20-Evidence-Index.md#e-090), [E-104](20-Evidence-Index.md#e-104)

<a id="f-078"></a>
### F-078 — Health and properties APIs

**Decision:** `RETAIN` · **Portability:** `High` · **Maturity:** `M3` · **Confidence:** `High`

**Observed behavior.** Reports health during load/sleep and exposes model/server capability metadata.

**Implementation.** `tools/server/server.cpp`, `tools/server/server-context.cpp`

**Dependencies.** HTTP server; model metadata

**License.** MIT

**ROCmFPX overlap.** ROCmFPX services need health and capability probes.

**Porting rationale.** Keep target-native behavior and add a cache readiness/degraded status field.

**Risks / caveats.** Health should distinguish serving readiness from process liveness.

**Evidence.** [E-090](20-Evidence-Index.md#e-090), [E-104](20-Evidence-Index.md#e-104)

<a id="f-111"></a>
### F-111 — Experimental built-in agent tools

**Decision:** `REJECT` · **Portability:** `High` · **Maturity:** `M1` · **Confidence:** `High`

**Observed behavior.** Can expose read, search, write, edit, diff, time, and shell-execution tools through /tools.

**Implementation.** `common/arg.cpp`, `tools/server/server.cpp`

**Dependencies.** server tool dispatcher; filesystem/shell access; HTTP

**License.** MIT

**ROCmFPX overlap.** ROCmFPX is an inference/quant target and should not couple privileged host tools to its public model server.

**Porting rationale.** Keep disabled; place tools in a separately sandboxed, authenticated agent runtime if needed.

**Risks / caveats.** Remote code execution and filesystem disclosure.

**Evidence.** [E-120](20-Evidence-Index.md#e-120), [E-121](20-Evidence-Index.md#e-121)

<a id="f-112"></a>
### F-112 — Experimental MCP CORS proxy

**Decision:** `REJECT` · **Portability:** `High` · **Maturity:** `M1` · **Confidence:** `High`

**Observed behavior.** Can expose a Web UI CORS proxy for MCP integrations.

**Implementation.** `common/arg.cpp`, `tools/server/server.cpp`

**Dependencies.** HTTP proxy; Web UI

**License.** MIT

**ROCmFPX overlap.** No cache-port dependency and high exposure risk.

**Porting rationale.** Do not enable in ROCmFPX's inference server; use a dedicated, allowlisted reverse proxy.

**Risks / caveats.** SSRF, credential forwarding, and trust-boundary collapse.

**Evidence.** [E-122](20-Evidence-Index.md#e-122), [E-123](20-Evidence-Index.md#e-123)

<a id="f-113"></a>
### F-113 — Google Cloud / Vertex AI compatibility

**Decision:** `RETAIN` · **Portability:** `Medium` · **Maturity:** `M2` · **Confidence:** `High`

**Observed behavior.** Registers compatibility handling for Google Cloud Platform serving conventions.

**Implementation.** `tools/server/server.cpp`

**Dependencies.** HTTP compatibility adapter

**License.** MIT

**ROCmFPX overlap.** May already exist in the target's newer llama.cpp server.

**Porting rationale.** Retain target-native implementation if required by deployments.

**Risks / caveats.** Compatibility surface requires upstream conformance tests.

**Evidence.** [E-124](20-Evidence-Index.md#e-124)

<a id="f-114"></a>
### F-114 — Idle sleep mode

**Decision:** `RETAIN` · **Portability:** `Medium` · **Maturity:** `M2` · **Confidence:** `High`

**Observed behavior.** Can put the server into a sleep state after a configured idle interval while preserving health access.

**Implementation.** `common/arg.cpp`, `tools/server/server-context.cpp`

**Dependencies.** idle timer; model/context lifecycle

**License.** MIT

**ROCmFPX overlap.** Useful for large UMA models sharing a workstation.

**Porting rationale.** Keep target-native behavior; define how persistent caches and router children behave across sleep/wake.

**Risks / caveats.** Wake latency and stale device state.

**Evidence.** [E-104](20-Evidence-Index.md#e-104), [E-125](20-Evidence-Index.md#e-125)

<a id="f-115"></a>
### F-115 — Reasoning parsing, budget, and history controls

**Decision:** `REDESIGN` · **Portability:** `Medium` · **Maturity:** `M2` · **Confidence:** `High`

**Observed behavior.** Supports reasoning mode selection, token budgets, and optional preservation of reasoning in prior assistant messages.

**Implementation.** `common/arg.cpp`, `llama-run.sh`

**Dependencies.** chat template capabilities; request formatter

**License.** Mixed MIT/GPL profile

**ROCmFPX overlap.** ROCmFPX serves agent/reasoning models but target templates may be newer.

**Porting rationale.** Use target-native reasoning controls and keep cache keys sensitive to template kwargs.

**Risks / caveats.** Changing preservation or template behavior changes tokens and invalidates cached state.

**Evidence.** [E-126](20-Evidence-Index.md#e-126), [E-006](20-Evidence-Index.md#e-006)

<a id="f-116"></a>
### F-116 — Grammar and JSON-Schema constrained generation

**Decision:** `RETAIN` · **Portability:** `High` · **Maturity:** `M3` · **Confidence:** `High`

**Observed behavior.** Constrains output with a grammar or generated grammar derived from JSON Schema.

**Implementation.** `common/arg.cpp`

**Dependencies.** grammar engine; JSON Schema converter

**License.** MIT

**ROCmFPX overlap.** Useful for structured agent outputs independent of cache port.

**Porting rationale.** Keep target-native implementation.

**Risks / caveats.** Schema feature coverage and grammar conversion limits.

**Evidence.** [E-127](20-Evidence-Index.md#e-127)

<a id="f-117"></a>
### F-117 — Embedded Web UI

**Decision:** `RETAIN` · **Portability:** `Medium` · **Maturity:** `M2` · **Confidence:** `High`

**Observed behavior.** Can host a configurable Web UI alongside the API.

**Implementation.** `common/arg.cpp`, `tools/server/server.cpp`

**Dependencies.** embedded assets; HTTP static serving

**License.** MIT

**ROCmFPX overlap.** ROCmFPX can use the inherited UI for local evaluation.

**Porting rationale.** Keep target-native UI; do not expose administrative/cache controls without authorization.

**Risks / caveats.** UI, API, proxy, and tool features share one trust boundary.

**Evidence.** [E-128](20-Evidence-Index.md#e-128), [E-090](20-Evidence-Index.md#e-090)

<a id="f-119"></a>
### F-119 — Multimodal capability gating

**Decision:** `RETAIN` · **Portability:** `High` · **Maturity:** `M2` · **Confidence:** `High`

**Observed behavior.** Routes multimodal and audio requests only when the loaded model and projector advertise the required modalities.

**Implementation.** `tools/server/server.cpp`, `tools/server/server-context.cpp`, `tools/server/server-models.cpp`

**Dependencies.** mtmd; model/projector metadata

**License.** MIT

**ROCmFPX overlap.** ROCmFPX may quantize multimodal-capable models.

**Porting rationale.** Keep target-native capability detection; exclude multimodal state from persistence until identity/serialization is proven.

**Risks / caveats.** Media content and projector versions affect cache compatibility.

**Evidence.** [E-102](20-Evidence-Index.md#e-102), [E-130](20-Evidence-Index.md#e-130)

<a id="f-120"></a>
### F-120 — Speculative decoding configuration

**Decision:** `RETAIN` · **Portability:** `High` · **Maturity:** `M3` · **Confidence:** `High`

**Observed behavior.** Supports draft-model and n-gram speculative decoding modes and reports acceptance statistics.

**Implementation.** `common/arg.cpp`, `tools/server/server-context.cpp`, `tools/server/server-task.cpp`

**Dependencies.** draft model or n-gram predictor; speculative state

**License.** MIT

**ROCmFPX overlap.** ROCmFPX has stateful MTP and speculative execution.

**Porting rationale.** Retain target-native MTP/speculation and preserve exact-boundary cache rules.

**Risks / caveats.** Draft/MTP state ABI changes must invalidate persistent checkpoints.

**Evidence.** [E-131](20-Evidence-Index.md#e-131), [E-063](20-Evidence-Index.md#e-063), [E-093](20-Evidence-Index.md#e-093)


