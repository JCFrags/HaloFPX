# Two-rank cache coordinator contract

This is the first bounded implementation slice for
[GitHub issue #26](https://github.com/JCFrags/HaloFPX/issues/26). It is an
isolated, test-only state machine for the fixed HaloFPX topology of coordinator
rank 0 and remote rank 1. It does not make the existing world-1 cache path
distributed and does not link into `server-context`, `llama-server`, llama,
ggml RPC, a command-line interface, or a filesystem provider.

## Exact boundary

The build gate `HALOFPX_CONTEXT_STORE_TWO_RANK_CONTRACT` defaults to `OFF`.
When selected, it creates only the `STATIC EXCLUDE_FROM_ALL` target
`halofpx-context-store-two-rank-contract` and its focused tests. Existing
world-1 admission remains `world_size=1, rank=0`; the new contract refuses any
roster other than ordered ranks `{0,1}`.

The checkpoint identity binds these exact values:

- model and shard digest;
- runtime/state ABI digest;
- K/V representation digest;
- partition-plan and topology digests;
- checkpoint generation and digest;
- prompt token count, exact boundary, and prefix digest; and
- ordered rank-ownership digests for ranks 0 and 1.

Every operation also carries a nonzero fresh attempt nonce. The adapter must
retain one noncopyable coordinator instance for its full process lifetime and
serialize calls to it. That instance refuses nonce reuse before invoking a
provider. Once all 64 ledger slots are consumed it refuses further attempts
rather than evicting replay evidence. The fake-provider seam is trusted to return already authenticated
provider results; this slice exact-matches all typed receipt fields but does
not itself define a receipt wire format, key lifecycle, clock, or durable
replay ledger.

## Capture publication gate

The coordinator calls rank 0 capture, validates its exact `DURABLE` receipt,
then does the same for rank 1. Only after both receipts match their provider
slot, checkpoint identity, attempt, ownership, object, byte count, and
component count does it build the ordered manifest and authorize one
publication callback.

Missing, corrupt, incompatible, timed-out, rejected, malformed, swapped, or
duplicate receipts stop before publication and best-effort abort every provider
already contacted in reverse order. Equal object content digests are not
duplicates: attempt, phase, logical rank, and receipt nonce define receipt
identity. Within one phase, the rank and receipt nonce are the duplicate key.
Publication returns one of `published`, `definitely_not_published`, or
`outcome_unknown`. A definite non-publication also aborts both captured objects;
an unknown outcome retains them because a manifest may now refer to them.
After an unknown publication result, the adapter must not retry, delete, or
republish that checkpoint generation until publisher reconciliation or an
idempotency proof resolves the authoritative outcome.

## Restore gate and failure boundary

Restore first validates the complete ordered manifest. It stages rank 0 and
rank 1 without live mutation and validates both exact nonce-bound `READY`
receipts before authorizing any commit. A pre-commit failure best-effort aborts
every provider already contacted and requires cold recomputation, but does not
claim that a live context needs recreation.

After both ranks are ready, the contract commits remote rank 1 and only then
applies local rank 0:

| Last exact result | Next result | Contract restore accepted | `recreation_required` |
|---|---|---:|---:|
| Before remote commit | Any refusal | No | No |
| Remote definitely not applied | Refusal | No | No |
| Remote outcome unknown | Stop | No | Yes |
| Remote applied | Local applied | Yes | No |
| Remote applied | Local failure or uncertain result | No | Yes |

An `APPLIED` disposition with a mismatched receipt is treated as possible live
mutation and requires recreation. Abort after commit is cleanup only; it is
not rollback and cannot restore a partially changed residency.
Whenever `recreation_required` is returned, the adapter must stop all restore
and inference use of the affected rank contexts until both live contexts have
been recreated. This test-only slice reports that obligation; a persistent
poison latch and recovery/reset authority remain part of the real adapter.

## Focused qualification

Configure and run only the isolated lane:

```bash
cmake -S . -B build/two-rank-cache \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_SHARED_LIBS=OFF \
  -DGGML_NATIVE=OFF \
  -DGGML_RPC=OFF \
  -DLLAMA_BUILD_TESTS=ON \
  -DLLAMA_BUILD_TOOLS=ON \
  -DLLAMA_BUILD_SERVER=ON \
  -DLLAMA_BUILD_WEBUI=OFF \
  -DHALOFPX_CONTEXT_STORE_TWO_RANK_CONTRACT=ON
cmake --build build/two-rank-cache \
  --target test-halofpx-context-store-two-rank-contract --parallel 2
ctest --test-dir build/two-rank-cache --output-on-failure \
  -R '^test-halofpx-context-store-two-rank-contract(-boundary)?$'
```

The behavior test covers ordered success traces, every negative provider
status, exact-field mutation, swapped and duplicate receipts, attempt replay,
world-1 refusal, manifest rejection, two-stage-before-commit ordering, remote
definite failure, remote uncertainty, and post-remote-commit local failure.
The source-boundary test guards that the option remains default-off, the target is
excluded from product link statements, and the existing world-1 profile and
codec checks remain present.

## What remains open

This slice does not create or restore a real cache object, define persistent
manifest bytes, authenticate a rank receipt, alter RPC wire commands, expose a
server mode, run inference, touch either Strix Halo machine, or establish a
cache hit or performance result. The next issue #26 slice must adapt
coordinator-local rank 0 state and the existing authenticated rank-1 RPC
capture/stage/commit/abort results behind a separate default-off gate. It must
preserve the three-way commit outcome: response loss or malformed authority
after dispatch is `outcome_unknown`, never “definitely not applied.”

Pinned `fewtarius/CachyLlama` commit
`6be745998f568e379ea197fcf827baec73ff9940` remains the saved-cache behavioral
reference. It supplies no two-rank manifest or coordination proof, and no donor
source was copied into this contract.
