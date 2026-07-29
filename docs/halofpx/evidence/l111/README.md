# L111 atomic rank-partition gate

Status: **PASS — RETAINED AT LOADER-FOUNDATION REVIEW BOUNDARY**

Base: `6c88472bf5f567a1064f27f4d8a90fc8e2b47a02`

Branch: `codex/integration-base-61f2f2d`

L111 retains one bounded, loader-internal transaction for an exact two-rank
axis-2 partition. It does not wire a graph, run a model, add asynchronous RPC,
change scheduling, touch production, or make a performance claim.

## Retained behavior

- The admitted rank set is exactly `{0,1}` on two distinct non-host devices
  using each device's exact default buffer type and an existing loader context.
- The two ordered ranges must provide positive, gap-free, non-overlapping full
  coverage of a packed rank-3 GGUF tensor.
- GGML creates both physical tensors under one private transaction record.
  The record binds the exact contexts and tensors, the loader owner, the live
  `partition_generation` authority, and the before/after context tails.
- Failure after either creation or any later precommit registry/accounting
  mutation restores both contexts and all loader-owned state.
- Commit counts one logical source tensor while retaining two exact physical
  allocations and advancing one loader generation.
- `done_getting_tensors`, `init_mappings`, and `load_all_data` seal further
  partition creation.
- Every concrete tensor has explicit source-offset authority. Unknown authority
  returns `std::nullopt` or a typed exception and never aliases byte zero.
- The secondary physical partition and full implementation-only duplicates are
  hidden from public name lookup without a public raw exclusion API.
- The obsolete source-slice API is removed. Requesting the old MiniMax
  peer-half mode returns `std::invalid_argument`; absent configuration retains
  the full-duplicate path.

## Focused qualification

The focused test creates a tiny GGUF and two genuinely distinct mock backend
devices. Release and Debug each pass the single L111 test. The test covers
exact bytes, shapes, offsets, buffer sizes, logical and physical accounting,
progress, mmap ranges, lookup, RAII/free counts, full-duplicate parity,
configuration refusal, every specified negative request, and all six
precommit failure boundaries.

A separate Release static compile with `GGML_RPC=OFF` and
`GGML_RPC_HALOFPX_LOCAL_STATE=OFF` passes the `llama` target.

See `build-receipt.txt` and `source-receipt.txt` for exact hashes and options.

## Independent review

Fresh exact-diff review returned **PASS / RETAIN** with no P0 or P1 findings.
The reviewer accepted the header-absent transaction symbols as implementation
plumbing rather than a raw checkpoint API because no checkpoint or handle is
exposed and rollback requires the exact tensor pair plus live generation
authority.

Low-severity limitations are recorded in `independent-review.md`. They do not
authorize model, graph, RPC, runtime, or production work.

## Preservation and disclosure

All pre-existing L83, L85, L97, and L98 evidence and archives remain
untouched. The disposable `nimo-1` source/build root was removed after receipts
were captured. No model artifacts, accelerator runtime, listener, service, or
production state were accessed.

Implementation and documentation were AI-assisted under the human-authored
L111 specification and repository policy.
