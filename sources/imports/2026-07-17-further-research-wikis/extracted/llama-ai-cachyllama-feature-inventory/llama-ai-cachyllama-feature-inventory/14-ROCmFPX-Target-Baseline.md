# ROCmFPX target baseline

This page records the target capabilities that control the disposition of CachyLlama features. It is pinned to `a5605a72768c6562241b248e268e33dc92787394` and is not a claim about later ROCmFPX revisions.

| Decision | Count |
|---|---:|
| RETAIN | 6 |
| REDESIGN | 0 |
| REJECT | 0 |

<a id="f-105"></a>
### F-105 — Run-scoped disk prompt cache

**Decision:** `RETAIN` · **Portability:** `High` · **Maturity:** `M3` · **Confidence:** `High`

**Observed behavior.** ROCmFPX already spills prompt state to a private, byte-bounded disk namespace for the current server run.

**Implementation.** `tools/server/server-task.cpp`, `common/common.h`, `tools/server/README.md`

**Dependencies.** llama state file API; filesystem; prompt cache

**License.** MIT

**ROCmFPX overlap.** This is the target implementation.

**Porting rationale.** Keep as the default non-persistent mode and extend rather than replace it.

**Risks / caveats.** Not reusable after clean restart by design.

**Evidence.** [E-203](20-Evidence-Index.md#e-203), [E-204](20-Evidence-Index.md#e-204), [E-205](20-Evidence-Index.md#e-205)

<a id="f-106"></a>
### F-106 — Atomic target/draft disk commit

**Decision:** `RETAIN` · **Portability:** `High` · **Maturity:** `M3` · **Confidence:** `High`

**Observed behavior.** Writes target and optional draft state through temporary files, validates size, flushes, renames atomically, and syncs the directory.

**Implementation.** `tools/server/server-task.cpp`

**Dependencies.** filesystem durability; llama state files

**License.** MIT

**ROCmFPX overlap.** This is the strongest persistence primitive in the target.

**Porting rationale.** Use it as the foundation for persistent CachyLlama-style checkpoints.

**Risks / caveats.** Manifest publication must be made equally atomic.

**Evidence.** [E-206](20-Evidence-Index.md#e-206)

<a id="f-107"></a>
### F-107 — Disk-cache failure circuit breaker

**Decision:** `RETAIN` · **Portability:** `High` · **Maturity:** `M3` · **Confidence:** `High`

**Observed behavior.** Disables further disk saves after an I/O failure while preserving safe fallback behavior and accounting.

**Implementation.** `tools/server/server-task.cpp`

**Dependencies.** error injection; cache fallback

**License.** MIT

**ROCmFPX overlap.** This is the target implementation.

**Porting rationale.** Preserve for persistent mode and expose breaker state in metrics/health.

**Risks / caveats.** A permanent breaker may require an administrative reset or retry policy.

**Evidence.** [E-208](20-Evidence-Index.md#e-208), [E-210](20-Evidence-Index.md#e-210), [E-211](20-Evidence-Index.md#e-211)

<a id="f-108"></a>
### F-108 — Focused disk-cache test suite

**Decision:** `RETAIN` · **Portability:** `High` · **Maturity:** `M3` · **Confidence:** `High`

**Observed behavior.** Tests hits, LRU, target/draft pairs, corruption, failure handling, cleanup, and accounting.

**Implementation.** `tools/server/tests/unit/test_prompt_cache_disk.py`, `tools/server/tests/utils.py`

**Dependencies.** pytest; server test harness

**License.** MIT

**ROCmFPX overlap.** This is target validation infrastructure.

**Porting rationale.** Extend it with restart persistence, manifest corruption, tenant isolation, concurrent instances, and upgrade tests.

**Risks / caveats.** Hardware-independent tests do not replace live ROCm/Vulkan validation.

**Evidence.** [E-210](20-Evidence-Index.md#e-210), [E-211](20-Evidence-Index.md#e-211)

<a id="f-109"></a>
### F-109 — Stateful MTP exact-boundary cache semantics

**Decision:** `RETAIN` · **Portability:** `High` · **Maturity:** `M3` · **Confidence:** `High`

**Observed behavior.** Treats stateful MTP cache records as exact-token-boundary entries while allowing ordinary stateless prefixes to subsume shorter entries.

**Implementation.** `tools/server/server-task.cpp`

**Dependencies.** state_spec; MTP context; LCP matcher

**License.** MIT

**ROCmFPX overlap.** This is target behavior.

**Porting rationale.** Preserve when adding persistence and system-prefix caching.

**Risks / caveats.** Relaxing exact-boundary rules can restore invalid speculative state.

**Evidence.** [E-207](20-Evidence-Index.md#e-207)

<a id="f-110"></a>
### F-110 — Strix Halo ROCmFPX and MTP baseline

**Decision:** `RETAIN` · **Portability:** `High` · **Maturity:** `M3` · **Confidence:** `High`

**Observed behavior.** Already supplies Strix Halo build scripts, HIP/Vulkan paths, ROCmFPX quants, MTP execution, and published local validation.

**Implementation.** `README.md`, `scripts/build-strix-rocmfp4-mtp.sh`

**Dependencies.** ROCm/HIP; Vulkan; gfx1151; ROCmFPX kernels

**License.** MIT

**ROCmFPX overlap.** This is the target baseline.

**Porting rationale.** Port server/cache capabilities without overwriting newer quant/backend/MTP work.

**Risks / caveats.** The target is explicitly experimental and performance is workload-specific.

**Evidence.** [E-202](20-Evidence-Index.md#e-202), [E-212](20-Evidence-Index.md#e-212)


