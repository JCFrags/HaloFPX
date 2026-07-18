# L03a inert context-store provider seam

Status: accepted.

L03a adds a target-owned internal interface and disabled provider to
`tools/server`. It is compiled into `server-context` and independently unit
tested, but it is not instantiated or called by `llama-server`. There is no
flag, environment variable, route, request field, slot hook, state capture,
parser, matcher, codec, reader, writer, path, logging, metric, or I/O operation.

## Locked behavior

- The disabled provider reports no persistent read/write/enumeration,
  anonymous/shared scope, admitted state profile, or admitted codec capability.
- Every lookup returns `miss_disabled` with no candidate.
- Every publication attempt returns `disabled`.
- A hit result cannot contain a null candidate; invalid hit construction becomes
  `miss_incomplete`.
- Provider methods are const/noexcept and the null implementation is stateless;
  repeated concurrent calls have the same result and do not mutate inputs.
- Requests and generation pointers are borrowed only for a synchronous call and
  cannot be retained or mutated. Lookup results exclusively own immutable
  candidates; identity references last for their owning object's lifetime.
  Provider implementations must catch internal failures and return the typed
  storage miss/error rather than allowing an exception across `noexcept`.
- The interface accepts only fixed-size opaque identity digests and an epoch.
  It has no raw principal, prompt, token, JSON, filesystem path, or
  `llama_context` surface.

## Architecture boundary

The internal `server-context` library is the narrowest server-owned build
boundary and avoids changing public `llama.h`, core serializers, or common CLI
configuration. The future owner is one provider at `server_context_impl`, not a
provider per slot, but L03a deliberately adds no owner field or allocation.

The inherited `server_prompt_cache` remains unchanged. Its RAM behavior is part
of the feature-off control, and its optional `--cache-disk` state files are not
HaloFPX v1 format, scope, authentication, or durability evidence. It is not
wrapped, renamed, or claimed as a conforming adapter.

Current server API-key validation does not propagate an authenticated effective
principal/security domain into tasks. Therefore no context-store lookup can
enter a request, route, scheduler, slot, or restore path until the trusted-scope
lane supplies that authority. Request-body identity is never a substitute.

## Deferred L03 work

The accepted v03 plan grouped a baseline codec/current-cache adapter with the
provider seam. The user's later implementation order and accepted L02 contract
require interfaces first and codecs one at a time after exact continuation
evidence. L03a is the first bisectable piece only. It does not admit the
inherited prompt-cache format or any baseline codec. A future process-local
ephemeral adapter and baseline codec require separate contracts, tests, review,
and milestone commit.

## Tests

- `test-halofpx-context-store` exercises disabled capabilities, zero and hostile
  identities, lookup/publish behavior, hit-result invariants, input immutability,
  and concurrent stability with assertions active in Release.
- `test-halofpx-context-store-contract` rejects filesystem/state/donor/logging
  dependencies in the seam and scans all production `common` and `tools/server`
  C++ sources/headers to reject any runtime hookup.
- L01 feature-off, L02 contracts, focused inherited tests, full server build,
  and byte-identical baseline/current `llama-server --help` remain milestone
  controls.
