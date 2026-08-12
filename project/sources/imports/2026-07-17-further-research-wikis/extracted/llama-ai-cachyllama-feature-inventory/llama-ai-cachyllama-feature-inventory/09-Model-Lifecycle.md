# Model lifecycle

The server router can discover, load, autoload, reload, proxy, and LRU-unload models. Parent scripts add download and heuristic profile selection. Router capabilities should be reconciled with ROCmFPX's newer base; GPL parent automation should be reimplemented or kept separate.

| Decision | Count |
|---|---:|
| RETAIN | 8 |
| REDESIGN | 2 |
| REJECT | 0 |

<a id="f-057"></a>
### F-057 — Router-managed child model processes

**Decision:** `RETAIN` · **Portability:** `Medium` · **Maturity:** `M2` · **Confidence:** `High`

**Observed behavior.** Runs each loaded model in a child llama-server process behind a router and proxies requests to the selected child.

**Implementation.** `tools/server/server-models.cpp`, `tools/server/server.cpp`

**Dependencies.** sheredom/subprocess; loopback HTTP; model presets

**License.** MIT

**ROCmFPX overlap.** ROCmFPX inherits modern llama.cpp router capabilities or can reconcile against them.

**Porting rationale.** Retain upstream/target router implementation rather than transplanting an older fork copy.

**Risks / caveats.** Router mode is explicitly experimental and unsuitable for untrusted direct exposure.

**Evidence.** [E-094](20-Evidence-Index.md#e-094), [E-105](20-Evidence-Index.md#e-105)

<a id="f-058"></a>
### F-058 — Model load and unload API

**Decision:** `RETAIN` · **Portability:** `High` · **Maturity:** `M2` · **Confidence:** `High`

**Observed behavior.** Exposes explicit model load and unload operations through JSON endpoints.

**Implementation.** `tools/server/server-models.cpp`, `tools/server/server.cpp`

**Dependencies.** router mode; child process manager

**License.** MIT

**ROCmFPX overlap.** ROCmFPX's current base may already contain equivalent model routes.

**Porting rationale.** Use target-native routes and apply authentication, authorization, resource quotas, and audit logging.

**Risks / caveats.** Loading arbitrary presets can execute resource-heavy or unsafe configurations.

**Evidence.** [E-095](20-Evidence-Index.md#e-095), [E-096](20-Evidence-Index.md#e-096)

<a id="f-059"></a>
### F-059 — Model autoload on first routed request

**Decision:** `RETAIN` · **Portability:** `Medium` · **Maturity:** `M2` · **Confidence:** `High`

**Observed behavior.** Optionally starts an unloaded configured model when a request names it.

**Implementation.** `tools/server/server-models.cpp`, `common/common.h`, `common/arg.cpp`

**Dependencies.** model catalog; child process manager; HTTP proxy

**License.** MIT

**ROCmFPX overlap.** ROCmFPX documents model router controls in its inherited server.

**Porting rationale.** Retain behind an explicit policy and admission controller.

**Risks / caveats.** Cold-start latency and denial-of-service by model thrashing.

**Evidence.** [E-094](20-Evidence-Index.md#e-094), [E-103](20-Evidence-Index.md#e-103)

<a id="f-060"></a>
### F-060 — Maximum loaded-model count with LRU unload

**Decision:** `RETAIN` · **Portability:** `High` · **Maturity:** `M2` · **Confidence:** `High`

**Observed behavior.** Caps active child models and unloads the least recently used model before loading another.

**Implementation.** `tools/server/server-models.cpp`

**Dependencies.** last-used timestamps; child process lifecycle

**License.** MIT

**ROCmFPX overlap.** Applicable to ROCmFPX where large quantized models compete for UMA/VRAM.

**Porting rationale.** Retain target-native LRU but make memory pressure and pinned models first-class inputs.

**Risks / caveats.** Request-level recency alone may evict an expensive or pinned model.

**Evidence.** [E-094](20-Evidence-Index.md#e-094)

<a id="f-061"></a>
### F-061 — Model preset reload

**Decision:** `RETAIN` · **Portability:** `Medium` · **Maturity:** `M2` · **Confidence:** `High`

**Observed behavior.** Reloads model definitions, updates metadata, autoloads newly configured models, and broadcasts a lifecycle event.

**Implementation.** `tools/server/server-models.cpp`

**Dependencies.** preset loader; model catalog; SSE

**License.** MIT

**ROCmFPX overlap.** Useful for ROCmFPX model presets and quant variants.

**Porting rationale.** Retain with schema validation and atomic catalog swap.

**Risks / caveats.** Partial reloads and stale child parameters.

**Evidence.** [E-106](20-Evidence-Index.md#e-106)

<a id="f-062"></a>
### F-062 — Local and cached model discovery

**Decision:** `RETAIN` · **Portability:** `Medium` · **Maturity:** `M2` · **Confidence:** `High`

**Observed behavior.** Loads model presets from cache, a configured models directory, and preset sources.

**Implementation.** `tools/server/server-models.cpp`

**Dependencies.** filesystem; preset parser; model metadata

**License.** MIT

**ROCmFPX overlap.** ROCmFPX users manage many quantized variants.

**Porting rationale.** Keep target-native discovery with explicit trust boundaries and allowlisted roots.

**Risks / caveats.** Untrusted presets and path traversal.

**Evidence.** [E-094](20-Evidence-Index.md#e-094), [E-105](20-Evidence-Index.md#e-105)

<a id="f-063"></a>
### F-063 — Hugging Face GGUF discovery and split download

**Decision:** `REDESIGN` · **Portability:** `High` · **Maturity:** `M2` · **Confidence:** `High`

**Observed behavior.** The parent launcher resolves repository files and downloads complete or split GGUF artifacts using available Hugging Face tools.

**Implementation.** `llama-run.sh`

**Dependencies.** hf CLI or huggingface_hub; network; disk space

**License.** GPL-3.0

**ROCmFPX overlap.** ROCmFPX has its own distribution/build workflows.

**Porting rationale.** Implement or document an MIT-compatible downloader outside the inference core; verify checksums and free space.

**Risks / caveats.** Directly copying parent shell code would import GPL obligations; remote artifacts are untrusted input.

**Evidence.** [E-008](20-Evidence-Index.md#e-008)

<a id="f-064"></a>
### F-064 — Model-class profile selection

**Decision:** `REDESIGN` · **Portability:** `High` · **Maturity:** `M2` · **Confidence:** `High`

**Observed behavior.** The parent inspects GGUF metadata to choose dense, MoE, recurrent/SSM, and hardware-specific launch profiles.

**Implementation.** `llama-run.sh`, `scripts/detect-gpu.sh`

**Dependencies.** GGUF metadata; shell parsing; hardware profile

**License.** GPL-3.0

**ROCmFPX overlap.** ROCmFPX has multiple quant families, backends, MTP paths, and Strix profiles.

**Porting rationale.** Reimplement as a declarative, tested preset resolver in the target or an external deployment package.

**Risks / caveats.** Heuristic metadata scanning from the first file bytes is brittle.

**Evidence.** [E-007](20-Evidence-Index.md#e-007), [E-011](20-Evidence-Index.md#e-011), [E-118](20-Evidence-Index.md#e-118)

<a id="f-065"></a>
### F-065 — Cache scope tied to model compatibility

**Decision:** `RETAIN` · **Portability:** `High` · **Maturity:** `M2` · **Confidence:** `High`

**Observed behavior.** Persistent states are accepted only for a compatible model/cache fingerprint, naturally invalidating reuse after incompatible model changes.

**Implementation.** `common/kv-ssd-cache.cpp`, `tools/server/server-context-page-manager.cpp`

**Dependencies.** model fingerprint; cache namespace

**License.** MIT

**ROCmFPX overlap.** ROCmFPX frequently changes quant type and MTP layout.

**Porting rationale.** Namespace cache roots by exact model fingerprint and runtime state ABI, not just human model alias.

**Risks / caveats.** Aliases can point to different files over time.

**Evidence.** [E-038](20-Evidence-Index.md#e-038), [E-060](20-Evidence-Index.md#e-060)

<a id="f-118"></a>
### F-118 — Automatic device-memory fit

**Decision:** `RETAIN` · **Portability:** `Medium` · **Maturity:** `M2` · **Confidence:** `High`

**Observed behavior.** Can estimate device requirements and adjust unset launch parameters, including context and offload, toward a target memory margin.

**Implementation.** `common/arg.cpp`

**Dependencies.** backend memory estimator; device enumeration

**License.** MIT

**ROCmFPX overlap.** Highly relevant to Strix Halo UMA and multiple ROCmFPX quant formats.

**Porting rationale.** Retain target-native estimator and teach it persistent cache/RAM reservation costs.

**Risks / caveats.** Estimator errors can overcommit unified memory.

**Evidence.** [E-129](20-Evidence-Index.md#e-129)


