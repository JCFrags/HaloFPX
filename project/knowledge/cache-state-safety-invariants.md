# Cache and state safety invariants

These are normative candidate requirements distilled from the reviewed Wiki and fork plan. **[OPEN]** No HaloFPX persistent implementation or procedure is validated yet.

## Never conflate these objects

| Object kind | Meaning | Acceptance boundary |
|---|---|---|
| Model tensor cache | static/derived model tensors or remote transfer copies | exact tensor/model/runtime identity; corruption rebuilds or misses |
| Prefix inference state | engine state after an exact reusable prefix | complete execution fingerprint, exact input boundary, authorized sharing scope, suffix-only equivalence |
| Session checkpoint | all mutable continuation state at a visible boundary | complete required-stream manifest, session authorization, output acknowledgement, exact continuation contract |
| System prefix | administrator-approved rendered system/tool prefix | ordinary prefix checks plus immutable rendered-token boundary and sharing-policy digest |
| Runtime transient | RAM slot state and uncommitted scratch | never advertised as durable persistence |

**[VERIFIED]** The current deployed RPC tensor cache belongs only to the first class. A valid tensor file grants no prefix or continuation authority. [RPC cache audit](../sources/measurements/2026-07-17-strix-halo-live-inventory/rpc-cache-audit.md)

## Acceptance equation

**[RECOMMENDATION]** A hit exists only when the declared object kind is authorized, compatible, complete, structurally and cryptographically valid, committed at the required local/distributed boundary, and imported into an isolated destination without error. Any false or uncertain term yields a typed miss/recompute/reset; never partial acceptance.

## Non-negotiable invariants

1. **[RECOMMENDATION]** Object kind is explicit; readers never infer semantics from filenames or payload shape.
2. **[RECOMMENDATION]** Compatibility binds exact model/shard bytes, tokenizer/effective template, adapters, quantization and K/V representation, context/RoPE/window behavior, runtime/build/state-codec IDs, backend/device layout, feature modes, target/draft identity, rank/world size, partition plan, and security scope where these affect values, layout, or authorization.
3. **[RECOMMENDATION]** Prefix identity binds the exact token/input boundary and every multimodal, embedding, adapter, mask, position, and policy input that affects state.
4. **[RECOMMENDATION]** Each object kind has a versioned required-stream profile. Unknown, absent, corrupt, or mismatched required state is a miss.
5. **[VERIFIED]** Exact continuation can require more than model KV: hybrid attention/recurrent memory and donor target/draft/speculative state exist, while the pinned donor SSD record lacks an explicit sampler/grammar/RNG field. **[OPEN]** The complete required-state inventory and bit-exact stochastic restart contract are unresolved. [Wiki Section 61](../wiki/HaloFPX_Wiki/09_HaloKV_Persistent_Cache/61_Attention_KV_Recurrent_MTP_Speculative_Sampling_and_RNG_State/README.md)
6. **[RECOMMENDATION]** All mandatory components validate before any live context or GPU state is mutated. Import occurs into an isolated/transactional destination; failure leaves the live destination unchanged.
7. **[RECOMMENDATION]** Target, draft, speculative/MTP, recurrent, sampler/RNG/grammar, sequence topology, and output-boundary streams publish and restore as one logical transaction when required by the declared profile.
8. **[RECOMMENDATION]** A write uses a unique staging path, bounded length, digest verification, required flush, immutable/atomic publication, directory synchronization, and only then a reachable manifest/index update. A failed write cannot destroy an older committed generation.
9. **[RECOMMENDATION]** Persistent files are untrusted input: bounded parsing and allocation, duplicate-key rejection, fixed relative component names, safe-open rules, and rejection of symlinks, traversal, devices, truncation, oversize, or swapped components are mandatory.
10. **[RECOMMENDATION]** Authentication and opaque namespace derivation happen before existence lookup. Raw tenant IDs and prompt text do not become paths or routine metric labels; explicit scopes never fall back to anonymous or another scope.
11. **[RECOMMENDATION]** Quotas account separately for committed bytes, staging headroom, quarantine, indexes, and filesystem reserve. Eviction is reachability-aware and active-reader-safe.
12. **[RECOMMENDATION]** In distributed mode, rank-local valid objects are only prepared state. One authoritative generation commits the exact expected rank set and boundary; missing, corrupt, or uncertain rank state causes whole-restore miss or an explicitly validated complete fallback.
13. **[RECOMMENDATION]** Optional draft/speculative state may fall back to target-only only when the state contract proves reconstruction safe. Otherwise recompute.
14. **[RECOMMENDATION]** Older binaries never read a newer persistent root unless reader compatibility is proven. Format evolution is reader-before-writer, versioned, and never in-place.

## OPEN before persistent writes

- `OPEN-FMT-01`: canonical schema, parser bounds, compatibility/invalidation, migration.
- `OPEN-STATE-01`: required versus reconstructible state for every admitted model and mode.
- `OPEN-SCOPE-01`: trusted principal binding, anonymous policy, tenant/system/public sharing, key rotation.
- `OPEN-STORAGE-01`: nimo-1 capacity, reserve, quota, durability tier, disposable fault-test location.
- `OPEN-ACCEPT-01`: correctness and performance thresholds derived from matched baseline variance.

The detailed reconciliation and experiment backlog remains in the [HaloKV intake review](../reviews/intake/2026-07-17__halokv-cache__review__v01.md).
