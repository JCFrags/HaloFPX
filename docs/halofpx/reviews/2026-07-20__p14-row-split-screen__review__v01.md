# P14 row-split configuration screen independent review

Verdict: **accepted as an early rejection screen**

## Scope and evidence checked

This review is limited to the optional split-mode extension in
`scripts/run-halofpx-primary-block.sh`, the P14 synthesis and receipt, their
README routing, and the retained nimo-2 evidence. It does not promote row mode,
expand the performance matrix, or qualify final G9/G10 non-inferiority.

The nimo-2 evidence independently confirmed:

- executed harness SHA-256
  `933681f4a6be1569ff746859fe69b1a1295a95214fbf6a5ea75be7561c3cc775`;
- evidence-bundle SHA-256
  `e0cfc1dec5f06f719a0653f04a45feabed2ba78fc0e281adb3ef16563de4ad78`;
- pinned request SHA-256
  `f5b273f95a852d5e34252715bc953fa4eb101273d262b9d778c16138d7c00b7c`;
- two retained HTTP 200 responses, each with 1,129 prompt tokens and 128
  generated tokens;
- both exact decoded-content hashes equal
  `3c9bcc5624f4a1d6558c51be89e2443ffba4b2206d10b632c89b7ef64fdd026f`;
- recorded `row` mode and `1,1` tensor split; and
- raw prompt/generation samples exactly match the receipt.

Recomputing means and deltas produced `203.9061944554607` prompt tok/s,
`16.612244377383917` generation tok/s, `+0.04630202363316016%` prompt delta,
and `-0.09172512860812221%` generation delta. The synthesis rounding is
correct. The mixed noise-scale result supports rejection, but no speedup,
confidence-interval, universal-superiority, or greater-than-30-token/s claim.

## Contract and rollback assessment

The harness preserves `layer` as the omitted-argument default and validates
the admitted values as exactly `layer|row`. Shell syntax validation passed and
an invalid mode failed with exit 2. The change is confined to a measurement
harness; engine code, deployment defaults, persistence, and donor provenance
are untouched.

The production nimo-2 worker and nimo-1 coordinator were active with zero
recorded restarts after restoration. The coordinator was still loading and
returned HTTP 503 during the review's early readiness probe; this does not
contradict the narrowly worded restoration statement, which does not claim a
completed readiness probe. Operational readiness remains outside this P14
performance-screen verdict and should be confirmed by the milestone owner
before closeout.

No material blocker was found. README routing was extended to include this
review; no other correction was required.
