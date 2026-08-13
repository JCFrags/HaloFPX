# Live-derived cache authority v1

This is the first bounded implementation slice for
[GitHub issue #33](https://github.com/JCFrags/HaloFPX/issues/33) and one
prerequisite for the CachyLlama-reference milestone in
[issue #39](https://github.com/JCFrags/HaloFPX/issues/39).

## Exact implemented boundary

`HALOFPX_CONTEXT_STORE_LIVE_AUTHORITY` defaults to `OFF`. When explicitly
selected, it builds only the `STATIC EXCLUDE_FROM_ALL` library
`halofpx-context-store-live-authority-v1` and its two focused tests. It is not
linked by `server-context`, `llama-server`, llama, ggml, or RPC.

The synchronous pure builder accepts borrowed immutable views and returns an
owned compatibility expectation. It hashes exact model and runtime artifact
bytes internally, deterministically orders map-like inputs, preserves
semantic arrays, and emits the closed component digests and root. Mutation or
destruction of the caller's input after return cannot change that result.

The six strongly typed families are:

| Component | Concrete authority |
|---:|---|
| 0 | exact ordered model/shard bytes, roles, ordinals, and lengths |
| 1 | typed metadata including arrays and ordered tensor inventory |
| 2 | tokenizer artifacts, tokens, merges, specials, and typed policy |
| 3 | one effective template plus renderer ID and renderer binary bytes |
| 6 | exact source-tree manifest bytes, executable/libraries, toolchain/options, and state ABI |
| 14 | fixed ranks `{0,1}`, stable endpoints/devices, distinct plan/ownership/placement, and epoch |

All six are encoded as their closed
[`context-store-v1.cddl`](contracts/context-store-v1.cddl) input and hashed as:

```text
SHA-256("halofpx.compat-component.v1\0" ||
        label_len:u16be || label || DCBOR(component_input))
```

The source, global plan, rank ownership, and rank placement use separate framed
sub-identities before entering their CDDL digest fields. Rank placement binds
logical rank, stable endpoint, stable device, and exact placement bytes.

## Fail-closed behavior

The builder refuses and returns an all-zero expectation for:

- a missing shard, byte view, declared shard, typed metadata snapshot,
  tokenizer policy, effective template, executable, state ABI, or rank;
- duplicate shard role/order, metadata key, tensor identity/location, token
  ID, special role, parameter key, component preimage, endpoint, or device;
- a recognized placeholder or mutable source/build identity, including
  `unknown`, `latest`, branch-tip names, or a dirty tree without exact canonical
  patch and untracked-content bytes;
- any topology other than ordered world size two with ranks `{0,1}` and a
  nonzero stable configuration epoch; or
- equal raw global-plan, ownership, or placement facts and equal supplemental
  preimages.

The remaining ten closed components must be supplied as exact nonempty
deterministic-CBOR preimages and accepted by a separately trusted validator
capability for that component index. The builder then domain-hashes the exact
preimage itself. Implementing the product validator and the closed
registered-ID registry for the six typed families is an explicit adapter gap,
not a product trust claim.

## Focused qualification

The in-memory behavior test freezes a golden root, builds from independently
allocated and differently discovered inputs, mutates each strongly typed
family one at a time, and exercises the missing/ambiguous refusal matrix. The
boundary test proves the gate is default-off, the library remains absent from
product targets, the operator-digest path remains canary-only, and the existing
world-one route remains present.

```bash
cmake -S . -B build/cache-live-authority \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_SHARED_LIBS=OFF \
  -DGGML_NATIVE=OFF \
  -DGGML_HIP=OFF \
  -DGGML_RPC=OFF \
  -DLLAMA_BUILD_TESTS=ON \
  -DLLAMA_BUILD_TOOLS=ON \
  -DLLAMA_BUILD_SERVER=ON \
  -DLLAMA_BUILD_WEBUI=OFF \
  -DHALOFPX_CONTEXT_STORE_LIVE_AUTHORITY=ON
cmake --build build/cache-live-authority \
  --target test-halofpx-context-store-live-authority-v1 --parallel 2
ctest --test-dir build/cache-live-authority --output-on-failure \
  -R '^test-halofpx-context-store-live-authority-v1(-boundary)?$'
```

`[MEASURED]` On 2026-08-12, the isolated Linux/CPU build completed and both
focused tests passed in the local WSL environment. This establishes only pure
authority behavior and product isolation. It is not a Strix Halo cache hit,
restart, correctness, or speed measurement.

## Next adapter work

Capture typed facts inside `llama_model_loader` while the opened files and GGUF
contexts still exist; move authority/store construction after actual template
and context fallback resolution; retain the real two-rank placement plan; add
semantic encoders for the remaining ten components; compose request-specific
authority; and clear/rebuild authority on every reload. Only then may a
separate default-off product adapter compare the live-derived root against the
operator canary fixture and proceed to restart qualification.
