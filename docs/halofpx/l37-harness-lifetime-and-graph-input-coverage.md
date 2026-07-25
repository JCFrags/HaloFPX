# L37 harness lifetime and graph-input coverage

Status: `[MEASURED] PASS` for the bounded no-production, no-primary-load
milestone. This result does not reinterpret L36, identify the primary-model
cause, promote cache behavior, or authorize a primary run.

## Proven lifetime correction

Source inspection found two result paths. The general restore path aliased
`run_ctx` to `disposable_ctx`, freed `disposable_ctx`, and then read
`llama_n_batch(run_ctx)`. The capture-only path also emitted an unauthenticated
result separately. Both paths now snapshot every result-authority field while
the context is live, HMAC the complete canonical record, write it to a private
bounded file, `fsync` it, emit and flush the same record, and only then free the
context exactly once.

The runner verifies the durable record independently and compares it field for
field with stdout. The live fixture authority was `n_batch=512` for both capture
and honest fresh-residency restore. Zero and the observed L36 garbage value
`3386108400` are rejected. L36's pre-free replay records remain directional,
non-promoted evidence only.

## Replay-to-logits coverage census

The default-off graph-input diagnostic walks the actual replay graph after input
binding and scheduler allocation. It hashes bounded admitted mutable graph
inputs and records type, byte size, dimensions, strides, and backend. Unknown,
unnamed, unbuffered, oversized, or backend-less mutable inputs refuse before
compute.

| Authority class | L37 disposition |
|---|---|
| token, position, output-row, K/V index, mask, rotation and attention-scale inputs | closed named allowlist with domain-separated content hashes when materialized in the active graph |
| mutually exclusive embedding input | classified as inactive only when the token-input SELECT path is active; it is not silently hashed as initialized content |
| graph topology and scheduler placement | ordered per-node op/type/backend/source-storage relation and view offset are hashed; node and cross-backend-edge counts are explicit |
| K/V cache geometry and attention views | retained from L35, including prepare/apply cells, heads, `n_kv`, K/V types, dimensions, strides, offsets and assigned backends |
| FA and output authority | FA selection, logits backend, output row/count/swaps and synchronized full-logits hash retained |
| RPC source storage | scheduler backend is preferred; immutable source tensors without scheduler ownership use their actual buffer-type authority |
| RPC wire tensor IDs and internal copy/split commands | not safely exposed by the current public graph/scheduler interface; not claimed covered |
| backend workspace contents | not read as a mutable graph input in the admitted fixture; no claim that opaque backend workspace is deterministic across the primary model |

The exact C++ admission/content-digest helper used by the collector has a
Release-built synthetic test that perturbs every admitted mutable input class
and proves its digest changes; it also proves unknown, null and empty inputs
refuse. Independent verifier sentinels prove the authenticated record changes.
The verifier rejects unknown names, malformed geometry, count/byte disagreement,
duplicate fields, tampering, and unrecognized inactive inputs.

## Combined disposable qualification

The accepted stories15M Q4_0 fixture
(`66967fbece6dbe97886593fdbb73589584927e29119ec31f08090732d1861739`,
19,077,344 bytes) ran once with the established 1,129-token prompt, 1,128-token
boundary, F16 K/V and flash attention off. It used two honest model residencies
and distinct worker epochs on isolated port 50237.

- capture and restore both reported live `n_batch=512`;
- both replayed token `29871` once at position 1127 to 1128;
- the authenticated active graph input was `inp_tokens`, 4 bytes, SHA-256
  `aef80afc5789c62ea8c8709bf840931f46205e707394419781b78bca15542a99`;
- both had 222 graph nodes and node-assignment SHA-256
  `fc4f4d3b9025bc7fe255dbda7ce484b2f4f3f07dd44816c983ae441d6c94e1cd`;
- synchronized logits were identical:
  `f6d0fa35238815d19f10cb97a0af1c75349080fa431064a25a4969f8d9b177b1`;
- both selected token `4245`, with identical token and decoded suffix hashes;
- worker capture/stage/apply/recapture aggregates matched across 1,156
  components and 5,197,824 bytes; legacy state-window GET/SET count was zero.

Attempts `a` and `b` preserved a stale capture-only result path that lacked the
new durable record. Attempt `c` qualified the corrected lifetime path with
diagnostics unintentionally off. Attempts `d` through `g` are preserved
fail-closed instrumentation bring-up records. Attempt `h` is the single complete
combined qualification result. None touched production or the primary artifact.

## Residual uncertainty and future discriminator

L37 does not prove ROCm/RPC numerical determinism or complete primary graph
coverage. A future primary discriminator, if separately authorized, must retain
the L37 input and node-assignment record and add source-owned RPC serialization
metadata for graph tensor identifiers, buffer group/offset/view/source
relationships and cross-backend copy/split commands. Any primary-only mutable
input absent from the closed allowlist must refuse before generation.

At closeout, the system-scoped production units remained continuously active:
nimo-2 worker PID 1535639 on port 50052 and nimo-1 coordinator PID 2356329 on
port 8081, HTTP 200, both `NRestarts=0`. All L37 units, port 50237 listeners,
keys, roots, builds and staging archives were removed.
