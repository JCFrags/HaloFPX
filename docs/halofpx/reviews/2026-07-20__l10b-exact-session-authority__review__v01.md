# L10b exact-session authority independent review

Verdict: **ACCEPT after correcting two inaccurate qualification claims.** The
resolver is suitable for the narrow product-excluded L10b authority seam. This
verdict does not authorize runtime lookup, publication, production persistence,
prefix reuse, shared scope, or a multi-entry cache.

## Scope and authorities reviewed

The review covered the uncommitted resolver implementation and header, focused
C++ and Python-golden tests, CMake graph contract, qualification receipt, and
nimo-2 evidence. It checked them against ADR-0001, ADR-0002, ADR-0037, the L02
contracts, and canonical Wiki Sections 57 and 60.

## Findings

1. **The encoding is deterministic and collision-unambiguous for the closed
   schema.** The HMAC preimage is the NUL-terminated target-owned domain followed
   by a canonical 13-pair integer-keyed map. Each admitted token is validated as
   a nonnegative `int32_t` and encoded as exactly four unsigned big-endian bytes
   inside one definite-length byte string. The bounded token count makes the
   four-byte size calculation safe. Map ordering, definite lengths, explicit
   token schema version, and fixed-width tokens prevent sequence-boundary and
   decimal-concatenation ambiguity. The independent Python implementation
   reproduces the C++ golden
   `90240152fe0449ba92a1746dcdf804d7cca55f0034d7a96d846cc59d87f1a25c`.
2. **Authority and output-affecting bindings are complete for this seam.** The
   resolver binds the already authenticated opaque private namespace, closed
   compatibility root, exact tokens, logical and output boundaries, the sole
   admitted greedy-memoryless profile, global plan, rank ownership and rank
   placement digests, topology epoch, world size, and rank. Model, tokenizer,
   template/system context, adapters, runtime/backend/quantization, and the
   remaining admitted output-affecting configuration stay transitively bound by
   the closed compatibility root; L10b does not weaken that authority. Raw
   principal bytes enter only the separately reviewed scope resolver in the
   test and are never an L10b input, output, path, log, or retained value.
3. **Validation fails closed.** Null, wrong-sized, or all-zero keys; zero scope
   or compatibility roots; absent, empty, oversized, or negative-token input;
   inconsistent/zero boundaries; unset or unknown profiles; absent topology
   digests; and invalid epoch/world-size/rank tuples all reject before HMAC
   output. The result is zero-initialized, every rejection retains the all-zero
   identifier, and the HMAC implementation wipes pad, intermediate digest, and
   SHA contexts. No partially derived identifier is returned.
4. **Default-off product isolation is explicit and tested.** The library is
   `STATIC EXCLUDE_FROM_ALL`; production server, argument, and context sources
   contain no resolver hook; and the contract rejects any production CMake
   linkage. It adds no option, filesystem access, lookup, write, response
   change, or persistent root. The normal feature-off server contains no L10b
   domain marker.
5. **Provenance remains target-native.** The diff contains no donor
   implementation, GPL llama-ai code, CachyLLama transplant, new dependency,
   WebUI, remote, or reference-clone modification.

## Corrections made during review

The milestone prose said logical and output boundaries had to be equal and that
only world-size one/rank zero was accepted. The implementation and tests instead
correctly admit a nonzero output boundary at or before the token-count logical
boundary and bind any bounded world size with `rank < world_size`. Those are
consistent with ADR-0001's distinct output-commit boundary and ADR-0037's
topology/rank binding. The prose and receipt were corrected; product code and
the risk-proportionate test set did not need changes.

## Evidence checked

On nimo-2, `/var/tmp/halofpx-l10b-exact-session-evidence-20260720-v2.tar.zst`
has SHA-256
`d0f0117f9bafa8fb54e49335b2b52995b0aae203de3065c6c4ce4bf4b9df4574`.
Its focused CTest log records 3/3 passing tests, and the retained feature-off
exclusion record confirms the domain marker is absent. The receipt's source and
test-binary hashes match the qualified v2 evidence inventory. Empty inherited
authentication/scope logs mean those binaries exited successfully without
stdout; they are supporting inherited smoke, not the primary L10b proof.

## Remaining gates

L10b does not transport the identifier through a normal completion, perform
pre-prompt anchor-first restore, capture at the decoded prompt boundary, or run
the process-level miss/write/restart/hit canary. Those remain separate
compile-and-runtime opt-in work under ADR-0037. No blocker was found to the next
bounded integration milestone.
