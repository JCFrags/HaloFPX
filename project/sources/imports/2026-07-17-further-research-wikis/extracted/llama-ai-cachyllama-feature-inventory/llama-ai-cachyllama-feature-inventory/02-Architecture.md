# Architecture

## Repository layering

```text
fewtarius/llama-ai  (GPL-3.0 orchestration)
├── llama-run.sh
│   ├── model/Hugging Face resolution
│   ├── GGUF metadata profile selection
│   ├── backend + cache flags
│   └── llama-server process launch
├── scripts/rebuild.sh
├── scripts/detect-gpu.sh
├── scripts/apply-ttm-kernel-params.sh
└── CachyLlama/  -> exact gitlink 6be745998f568e379ea197fcf827baec73ff9940

fewtarius/CachyLlama  (MIT component)
└── llama-server
    ├── HTTP routes / model router / stream sessions
    ├── task parsing and user metadata
    ├── slot scheduler and prompt cache
    ├── server_context_page_manager
    │   ├── anonymous conversation cache wrappers
    │   └── user cache wrappers under u/
    ├── server_context_ssd_cache
    ├── kv_ssd_cache
    ├── kv_ssd_system_cache
    └── extended llama sequence-state APIs
```

## Request-to-checkpoint flow

```text
HTTP request
  │
  ├─ protocol conversion (OpenAI / Responses / Anthropic / native)
  ├─ user hint extraction and validation
  ▼
server_task queue
  ▼
slot allocator
  ├─ active-user cap for non-empty IDs
  ├─ prompt-prefix similarity
  └─ same-user slot preference
  ▼
server slot
  ├─ live RAM prompt cache lookup
  ├─ system-prefix cache lookup
  ├─ per-conversation / per-user SSD checkpoint lookup
  ├─ target + draft + spec state restore
  └─ normal prefill fallback
  ▼
generation
  ├─ response timings and cache_n
  ├─ periodic/final checkpoint save
  └─ resumable SSE session buffer
```

## Cache data model

CachyLlama's primary persistent record combines:

- checkpoint ID, source slot, positions, token count, turn and access metadata;
- content/compatibility hash and format version;
- a bounded token prefix for matching;
- target state, optional draft state, and speculative state;
- hot/warm/cold classification.

A separate system cache keys reusable system-prefix state by prompt hash and model compatibility. The server page manager creates cache instances per conversation or user namespace and selects continuation candidates. Evidence: [E-030](20-Evidence-Index.md#e-030), [E-050](20-Evidence-Index.md#e-050), [E-060](20-Evidence-Index.md#e-060).

## ROCmFPX target contrast

```text
ROCmFPX server_prompt_cache at a5605a72768c6562241b248e268e33dc92787394
├── RAM byte budget
├── private owner-only run directory
├── target + optional draft pair
├── stateful MTP exact-boundary semantics
├── temp files -> flush -> atomic rename -> directory sync
├── byte-bounded LRU
├── save-failure circuit breaker
├── stale owned-run cleanup
└── clean-shutdown removal  <-- prevents restart persistence
```

The port should add a persistent manifest/namespace to this target architecture. Replacing it with CachyLlama's storage engine would lose tested crash/failure properties.

## Trust boundaries

| Boundary | Pinned behavior | Required target behavior |
|---|---|---|
| Client to HTTP | API key/TLS optional; body user field accepted | authenticated principal derived by middleware |
| Tenant to cache | raw validated user ID influences namespace | opaque tenant key; authorization before lookup/restore |
| Process to disk | component files are not owner-only | preserve target 0700/0600 and optional encryption |
| Router to child | loopback proxy and rendered presets | validate presets; withhold secrets; resource quotas |
| Admin to tools/proxy | experimental tools and CORS proxy can be enabled | separate sandbox/proxy; disabled in inference service |
