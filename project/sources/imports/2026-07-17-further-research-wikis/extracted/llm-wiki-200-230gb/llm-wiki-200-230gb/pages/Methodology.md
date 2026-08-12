# Methodology

## Inclusion rule

A candidate is included when a **single runnable representation**—possibly sharded—has a publisher-displayed total of approximately 200–230 decimal GB. A repository whose unrelated files sum to the band is not included.

## Unit policy

- Artifact listings: decimal GB, matching publisher UI.
- Runtime capacity: binary GiB.
- Exact byte formulas: integer bytes.
- Displayed capacity values: rounded only after calculation.

## Weight planning

```text
W_payload_GiB = display_GB × 10^9 / 2^30
W_plan_GiB    = ceil(W_payload_GiB) + 1
```

This converts a rounded display value into a conservative deterministic allocation. Once exact LFS sizes are refreshed, replace the display input with the exact byte sum and retain at least the loader/alignment margin.

## Cache planning

Standard GQA is derived from layer count, KV heads, K/V head dimensions, context, and cache encoding. MiMo/Step hybrid attention use the current interleaved SWA allocation. DeepSeek uses the current MLA absorption graph. MiniMax MSA is intentionally left as a measurement gate.

## Runtime reserves

No universal graph-buffer equation is asserted. Each profile declares fixed OS, runtime/graph, and skew/safety reserves. They make procurement arithmetic reproducible, but they must be replaced by measured allocator logs for production approval.

## Ranking method

The score is a deployment-readiness heuristic for this capacity target. It does not blend incompatible official benchmark tables into a false universal quality number.

## Research limitations

- Hugging Face UI display sizes can be rounded.
- Community quant repositories can update or move `main`.
- Runtime support can change daily; commits are pinned.
- Exact backend allocation depends on build flags and device topology.
- No target-size Strix Halo benchmark was available.
