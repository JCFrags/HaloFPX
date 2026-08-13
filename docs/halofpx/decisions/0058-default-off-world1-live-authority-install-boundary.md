# ADR-0058: default-off world-1 live-authority install boundary

- Status: accepted for the fail-closed install-boundary slice only
- Date: 2026-08-12
- Scope: host implementation and documentation; no positive authority source

## Context

ADR-0054 leaves the world-size-1 prefix product cold until one trusted source
can bind the complete loaded model, resolved runtime, request profile, topology,
and model lifetime. ADR-0052 proves deterministic construction for selected
typed families, but its standalone builder is fixed to world size two, returns
only a compatibility expectation, and intentionally has no product link.

An audit at exact repository commit
`9bfccf25d43af0c446df591035e9cdac0b74d6c0` confirmed that no current server
lifecycle point owns all of the required facts. In particular, the stack-local
model loader owns the opened shards and typed GGUF contexts, effective chat
templates are finalized after current context-store initialization, build
information is not a complete immutable build/state-ABI receipt, and the
allocated model/context expose no canonical complete authority snapshot. The
exact table is retained in
[`world1-live-authority-install-v1.md`](../world1-live-authority-install-v1.md).

## Decision

Add `HALOFPX_CONTEXT_STORE_WORLD1_LIVE_AUTHORITY_INSTALL`, literal default
`OFF`, as a Linux-only sub-gate of
`HALOFPX_CONTEXT_STORE_WORLD1_PREFIX_PRODUCT`. It compiles an install boundary,
not a digest builder.

The boundary accepts only a separately trusted live
loader/context/lifecycle source capability. That source must synchronously
return:

1. a structurally valid ADR-0054 world-one authority whose compatibility root
   recomputes from all 16 nonzero components;
2. distinct nonzero producer, global-plan, rank-ownership, and rank-placement
   identities, nonzero topology epoch and model generation, world size `1`,
   and rank `0`; and
3. custody of every registered fact domain, including exact model artifacts,
   typed metadata/tokenizer/template, all closed runtime/request components,
   build and state ABI, plan/ownership/placement/topology, producer, security
   scope, and model generation.

The fact mask records custody asserted by that trusted source. It is not a
replacement for deriving and validating the closed preimages. The installer
does not accept component digests, CLI values, file paths, the standalone
world-two expectation, or a partial fact set. It also compares the candidate's
model generation with a separately supplied nonzero expected generation before
owning the capability. This scalar equality is not yet an independent
lifecycle lease; a positive source must add that ownership and race closure.

At this commit the server deliberately supplies `source = nullptr` and
`expected_model_generation = 0`. Installation therefore returns no capability,
the store remains unopened, and inference follows ADR-0054's cold
`live-authority-unavailable` behavior. This explicit refusal is the only
production result of enabling the new gate today.

## Required work before a positive source

A later decision and review must implement all of the following as one
lifecycle-owned source:

- freeze exact opened-shard byte receipts, typed GGUF arrays, tokenizer facts,
  and tensor file/order/offset records while loader contexts are alive;
- finalize the effective template and renderer before store opening;
- freeze resolved context, RoPE/window, K/V, backend/device, and state-ABI
  semantics after all automatic fallbacks;
- retain exact immutable source-tree, dirty-tree, executable/library,
  toolchain, and build-option evidence;
- canonicalize actual global plan, ownership, placement, and stable topology
  from the live allocation rather than operator intent;
- bind explicit closed “none” profiles for every excluded feature and compose
  request-resolved system/tool, sampler, parser, RNG, and scope semantics; and
- own a nonzero monotonic model/context generation across load, destroy,
  sleep, resume, recreation, and plan change, closing the old store before
  replacement.

No subset authorizes installation. There is no operator-digest fallback.

## Qualification and limits

Deterministic CPU tests cover feature-off, missing source, zero or mismatched
generation, every individually missing fact, operator-origin and world-two
refusal, structurally invalid capability refusal, and ownership of a complete
test-only trusted fixture. A source-boundary test proves the production server
passes null/zero, prohibits the ADR-0052 builder and operator inputs in this
module, and checks the generated feature-off graph contains neither install
source nor compile definition.

The tests do not prove a positive server hit, loader capture, fresh-process
reuse, model output parity, target compatibility, cache speed, or two-rank
behavior. No target machine is used.

## Relationship and rollback

ADR-0052 remains standalone and world-two. ADR-0054 remains the product and
fail-closed authority contract. This decision adds a narrower future-source
install boundary without weakening either record.

Rollback removes the install option/library/tests, the null-source server
call, this record, and its audit page. Persisted formats and the existing cold
product behavior are unchanged.
