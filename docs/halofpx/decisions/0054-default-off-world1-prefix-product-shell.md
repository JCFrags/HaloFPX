# ADR-0054: default-off world-1 authenticated-prefix product shell

Status: accepted for the fail-closed world-size-1 server shell and hosted CPU
contract only. A positive product hit remains blocked on trusted live-loader
authority and model-backed qualification.

Date: 2026-08-12

## Context

ADR-0051 admits exact longest-prefix selection as a standalone unit seam but
forbids a product link. Issue #32 needs the next reversible step: route an
eligible ordinary-transformer request through authenticated checkpoint
discovery, exact prefix selection, live state installation, and normal suffix
prefill without weakening the existing exact full-request path. CachyLlama
commit `6be745998f568e379ea197fcf827baec73ff9940` remains a behavior-only
reference. Its fuzzy lookup, FNV identity, directory scan, and heuristic system
boundary are not authority.

The server does not yet have a trusted world-size-1 loader adapter that derives
the complete cache authority from the loaded model and inference plan. The
separate two-rank authority/coordinator work is not a substitute: it does not
provide this path's distinct world-1 global-plan, ownership, and placement
digests. Treating operator CLI components as live authority would make a cache
hit claim that the process cannot prove.

## Decision

Add a Linux-only `HALOFPX_CONTEXT_STORE_WORLD1_PREFIX_PRODUCT` build option
whose literal default is `OFF`. It requires the standalone selector and exposes
one explicit runtime mode, `full-v1-world1-prefix-product`. The existing exact
runtime modes and feature-off behavior remain separate.

The runtime mode owns an immutable typed authority capability containing the
canonical 16-component compatibility expectation, producer identity, distinct
global-plan/rank-ownership/rank-placement digests, topology epoch, model
generation, world size `1`, and rank `0`. Authority validation recomputes the
compatibility root from all components. There is no CLI or operator-component
fallback. Until a trusted live-loader adapter installs a complete capability,
server startup succeeds, storage is not opened, every request computes cold,
and native response telemetry reports `live-authority-unavailable`.
The standalone world-size-2 live-authority builder in ADR-0052 is intentionally
not linked into this world-size-1 product and does not install this capability.

Catalog discovery must match the capability's producer identity against the
trusted catalog configuration. A successful lookup retains the complete
capability, and live installation requires exact equality of every retained
compatibility component/root, producer, plan, ownership, placement, topology,
generation, world, and rank field. Installation also requires a nonnegative
single sequence ID inside the trusted context sequence limit; the whole-cache
`-1` state API sentinel and an out-of-range sequence are rejected before any
live-state call. The request carrier binds that same complete authority, and
capture/publication recheck exact equality before and after capture so an
authority replacement cannot cross lineages.

For a valid capability, catalog discovery derives candidate token counts only
from authenticated generation-one child manifests in the request's private
scope, compatibility partition, policy epoch, and admitted profile. Catalog-v1
records and their 12 canonical reserved bytes remain unchanged. Discovery
validates the complete fixed world-1 transformer manifest roster and configured
manifest/frame/state/token limits without loading state objects. Any relevant
corrupt, incomplete, ambiguous, pending, missing, incompatible, or uncertain
record clears the candidate set and forces a cold result.

The product coordinator obtains nonblocking catalog-mutation custody before
authenticated boundary discovery and retains it across the complete ADR-0051
longest-to-shortest selector call. Every catalog publication obtains the same
process-local mutation exclusion; a competing lookup or publication returns a
typed busy result and computes cold. Read-only catalog operations retain their
independent bounded operation locks. Thus no publication can interleave
between selector probes and create a mixed catalog view.

Once an authenticated record for a candidate identity is selected, any child
failure is terminal and cannot fall through to a shorter prefix. A hit is
re-bound to the exact selected-boundary identity, installed through the
existing transformer state API, consumed once, and explicitly wiped. A failed
or stale installation clears the destination state and computes cold. Old live
slot checkpoint blobs are discarded around external restart-state
installation. If request validation or sampler construction subsequently
rejects launch, the installed KV state, prefix tokens, and checkpoint metadata
are rolled back before the slot returns idle.

The server inserts only the authenticated prefix tokens into an empty slot.
The ordinary prompt path then derives `n_past` from those tokens and evaluates
the untouched residual suffix. A full-request exact hit retains the server's
mandatory one-token logits replay. The selector's internal logical residual is
`0`; response telemetry preserves that logical value and the full restored
state-token count. It separately reports actual prompt tokens from final
timings and avoided prompt tokens as the bounded request-minus-actual
difference. After a clean empty miss, or after extending a shorter hit,
publication captures the full request boundary only.
This slice does not automatically create a system/chat-role checkpoint and
does not infer semantic boundaries from text.

Eligibility is limited to a native nonstreaming single completion with
`cache_prompt=true`, one empty destination slot, target-only world-1/rank-0
ordinary transformer memory, and greedy memoryless sampling. Recurrent,
hybrid, encoder or encoder-decoder, draft, speculative, MTP, multimodal,
adapter, grammar, tool, sampler, control-vector, tensor-override, and
distributed state are excluded. A request routed to a nonempty live slot skips
persistent lookup and reports `live-slot-state-present`; it does not imply a
catalog miss.

Native product responses may include fixed non-sensitive `halofpx_cache`
telemetry: match kind (`cold`, `exact`, or `prefix`, not a storage tier),
selected-prefix, restored-state, logical-residual, actual, and avoided prompt
token counts plus a validity bit, candidates examined, total lookup time,
state-install-and-cleanup time, and fallback reason. Reuse tier is `none` or
`persistent-filesystem` for an empty-slot product lookup, or
`live-slot-memory` only when final server accounting confirms that in-process
slot state avoided prompt work; the core does not claim that configured roots
are SSD. A selected exact/prefix candidate remains the `match_kind` even if
installation fails; zero restored-state/avoided counts, tier `none`, and the
typed fallback record that cold outcome without erasing selection evidence.
Prompt text, token values,
identities, digests, paths, principals, keys, and model names are excluded.
Exact legacy and runtime-off responses do not gain this field.

## Qualification boundary

Hosted deterministic tests cover feature-off and no-authority cold results,
authenticated restart discovery, preseeded exact-prefix reuse for two distinct
suffixes, exact hit accounting, one-shot state application and wiping, failed
state application, authority/generation mismatch, canonical catalog-v1 bytes,
catalog-mutation custody with busy publication/lookup behavior, and terminal
corrupt/incomplete longer candidates. It also covers a same-generation
producer switch and the negative whole-cache sequence sentinel without a state
API call. The gated `llama-server` build and inherited exact/selector contracts
must remain green.

These tests do not execute a positive server hit because no trusted live-loader
authority provider exists. They do not prove slot insertion, model output
parity, residual-only model decode, automatic system-prefix capture, target
performance, or any two-rank behavior. A local WSL no-authority server smoke is
correctness evidence only and creates no speed claim.

The timing fields are intentionally narrow. `lookup_total_ns` covers the
product lookup path and `state_install_cleanup_ns` covers state apply plus
cleanup; neither is inclusive client latency, and the ordinary server
`prompt_ms` clock starts after this prelaunch cache transition. The response
does not expose a physical-I/O byte total. A later issue-#18 schema-v2 evidence
lane must use client-observed TTFT/wall time plus an inclusive cache-transition
measurement. The existing A/B v1 adapter drops streamed `halofpx_cache` and
requires uncached prompt accounting, so it is not qualification for this path.
Issue #18 must also account for request-scoped preprompt cache maintenance,
including selected-slot transition and synchronous idle-slot saves, with a
client-observed TTFT arbiter; that broader clock is outside this slice.

Issue #32 remains open until a trusted world-1 authority adapter, fresh-process
model-backed prefix and exact runs, deterministic cold/output parity, retained
phase evidence, and matched Strix Halo measurements exist. Issue #26 separately
owns atomic two-rank composition; this decision makes no dual-node claim.

## Relationship and rollback

This decision narrowly supersedes ADR-0051's no-product-link boundary only
when the new product build and runtime gates are both selected. ADR-0051
remains the selector contract; the existing exact path is unchanged. No stored
format migration is introduced.

Rollback removes the product option/library, runtime routing, manifest-only
catalog discovery extension, telemetry, hosted product tests, L10f record, and
this ADR. Existing exact records remain valid because catalog serialization is
unchanged.
