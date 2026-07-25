# L41 scheduler execution-authority adversarial review

Status: **NOT PASS**

Scope: the uncommitted runtime-default-off scheduler authority candidate built from
`53f414dfc5a8f9873ad9961f541eb41cf6dc2aae`. The reviewer made no source changes.

## Material findings

1. The candidate returned only counters, an opaque chain root, and a result tag.
   It did not expose a bounded authenticated event stream. Exact split ranges,
   copy maps, and source/destination digests therefore could not be independently
   inspected or verified from retained evidence.
2. Copy events bound the source canonical ID and destination backend/slot, but
   omitted complete destination tensor identity, buffer class and base-relative
   range, and explicit source/destination nested-view authority.
3. Focused qualification did not exercise unsupported layout/readback, unknown
   references, overlap, missing/duplicate/out-of-order/tampered events, or
   quantized/nested/non-contiguous/padding bounds.
4. The real expert partial-copy fixture proved that two partial transfers passed,
   but did not read and compare the expert graph output. The ordinary fixture
   checked counts rather than authenticated split/copy-map content.

## Verdict

These are contract-level omissions. The candidate is not a reusable scheduler
execution-authority foundation and must not be committed. It was removed before
the L41 closeout. Any future authorization would need a bounded caller-owned
authenticated event sink, complete source/destination allocation and view
authority, focused refusal fixtures, and deterministic expert output evidence.
