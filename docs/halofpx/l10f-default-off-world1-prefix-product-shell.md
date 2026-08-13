# L10f default-off world-1 authenticated-prefix product shell

Status: **implemented and locally hosted-CPU-qualified as a fail-closed server
shell; positive model-backed reuse remains blocked**

The accepted boundary is recorded in
[ADR-0054](decisions/0054-default-off-world1-prefix-product-shell.md). This is
the first explicit server composition of the exact authenticated selector. It
does not claim that the current server can produce a positive product hit.

## Implemented

**[VERIFIED]** `HALOFPX_CONTEXT_STORE_WORLD1_PREFIX_PRODUCT` defaults to `OFF`,
requires the existing longest-prefix selector, and adds the explicit
`full-v1-world1-prefix-product` runtime mode only to an opted-in Linux server
build. Runtime-off and existing exact modes remain separate.

**[VERIFIED]** The server product path has no operator compatibility fallback.
It requires a typed immutable world-1 capability whose canonical compatibility
root is recomputed from all 16 components and whose producer, global-plan,
rank-ownership, rank-placement, topology-epoch, generation, world, and rank
fields are explicit. No trusted live-loader provider currently installs that
capability. The selected runtime mode therefore starts normally, does not open
or publish storage, computes cold, and reports
`fallback_reason=live-authority-unavailable`.

**[VERIFIED]** Manifest-only catalog discovery authenticates each relevant
child manifest and validates the exact generation-one world-1 transformer
metadata/descriptor roster before exposing a token count. It preserves the
existing 496-byte catalog-v1 record and canonical zero reserved bytes. Any
relevant corruption, incomplete publication, pending record, ambiguity,
descriptor mismatch, or configured-limit violation yields no partial
candidate list.

**[VERIFIED]** A selected child failure is terminal cold. It never falls
through to a shorter prefix. The coordinator rederives the selected exact
identity, applies state only while authority generation remains current,
consumes the snapshot once, wipes state and token material, and reports typed
logical prefix/residual counts. Server installation discards stale live slot
checkpoints and inserts only the restored prefix tokens so the ordinary prompt
path can evaluate the suffix. A rejected post-restore task launch rolls the
slot back to cold. Exact hits preserve mandatory one-token logits replay;
server telemetry reports that effective replay as one residual token while the
selector retains logical residual zero internally.

**[VERIFIED]** Eligibility rejects recurrent/hybrid memory, encoder and
encoder-decoder models, distributed ranks, draft/speculative/MTP, multimodal
state, adapters, grammar/tool/sampler state, control vectors, tensor overrides,
streaming/multiple completions, and non-greedy sampling. A nonempty live slot
is not searched and reports `live-slot-state-present`. Publication is
full-request-only after a clean miss or shorter-prefix extension. Automatic
chat/system boundary capture is not implemented; the two-suffix hosted test
uses a deliberately preseeded canonical prefix.

## Local correctness evidence

**[MEASURED] (off-target WSL2, 2026-08-12):** A Release CPU build on Ubuntu
WSL2 compiled `llama-server` with the product gate and repository-required RPC
local-state support. Seven focused inherited and new CTests passed: feature
contract, exact session, exact-session contract, exact-key runtime contract,
catalog/product, prefix selector, and selector contract. The catalog/product
test additionally passed after later hardening for one-shot install,
compatibility-root recomputation, configured manifest/frame limits, stale
checkpoint cleanup, and authenticated incomplete-record handling.

**[MEASURED] (off-target WSL2, 2026-08-12):** A manual server smoke using the
local tiny GGUF fixture with SHA-256
`3e184de6d7bbe7e16fdf33b35b086b3df426f8557b166933415b59479dd021ec`
returned cold telemetry with zero selected/restored tokens and
`live-authority-unavailable`. This validates the reachable fallback only. The
fixture is not added by this slice, and this is not retained target or
performance evidence.

## Open product proof

**[OPEN]** The positive server path is structurally dormant until issue #33
delivers a separately reviewed trusted world-1 live-loader authority adapter.
After that dependency lands, issue #32 still requires fresh-process model runs
that prove:

1. a preseeded canonical prefix serves distinct suffixes with output equality;
2. normal prompt processing evaluates only the residual suffix;
3. an exact hit retains logical selector residual zero while server telemetry
   reports the one-token logits replay as effective residual work;
4. corrupt or incomplete longer state is terminal cold without rewrite;
5. coherent model/plan/topology mismatch is cold; and
6. cache-off and cache-on results match on the Strix Halo target.

**[OPEN]** Full-request publication cannot bootstrap system-only reuse when two
requests diverge immediately after their system prefix. Trusted
template-derived boundary capture is a separate follow-up. Issue #26 also
remains the sole owner of atomic two-rank state restoration; no dual-node claim
is made here.

The local tests establish fail-closed mechanics, not avoided model work or a
speed improvement. Target evidence must attribute lookup, validation,
state-apply-and-cleanup, actual prompt tokens, TTFT, prompt rate, and generation
rate under issue #18.

The portable hosted qualification receipt, including the compile-time-off
control and no-authority server smoke, is
[retained here](evidence/l10f-default-off-world1-prefix-product-shell-receipt.json).

## Rollback

Disable the product build option or remove the product coordinator/server
routing. Because catalog-v1 serialization is unchanged, existing exact cache
roots require no migration.
