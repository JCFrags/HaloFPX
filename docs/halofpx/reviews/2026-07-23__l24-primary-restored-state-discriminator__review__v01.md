# L24 primary restored-state discriminator review

Date: 2026-07-23

Verdict: **PASS — terminal NOT PROMOTED**

The independent adversarial review verified:

- the immutable evidence tree contains 17 files and 503,091 bytes with
  SHA-256
  `c333749cb92066ec671bd0f3d6b783250616d13cbf93a74dc6db4cb40af97940`;
- capture allocated 80,950,550,528 RPC bytes and stored 64 components,
  2,454,528 state bytes, with aggregate
  `014a1024f13225a3f7bd7bba6be43dce1106a0354d68b5043f284263cce19bc9`;
- the result makes no stage, apply, restored-token, root-cause, correctness,
  promotion, or performance claim;
- the evidence supports both controller-owned SSH hangs, explicit
  termination, registered recovery, and no retry;
- production recovered worker first, then coordinator, with exact commands,
  ports, models, HTTP 200, and `NRestarts=0`;
- disposable units and all admitted paths are absent;
- source identities match the receipt and diagnostics remain default-off; and
- the focused suite passes 66/66.

The review accepts the SSH transport deadline issue only as a retained
controller reliability inference. No controller correction, primary retry,
or subsequent milestone is opened by this review.
