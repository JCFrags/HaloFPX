# L29 primary fresh-residency discriminator independent review

Date: 2026-07-23

Reviewer: independent adversarial agent `l29_adversarial_review`

Verdict: **PASS terminal NOT PROMOTED**

No material findings remain.

The reviewer independently verified:

- the exact artifact, runtime tuple, two-residency ordering, worker epoch
  authority, and authenticated capture receipt;
- capture and stage agreement at 64 components, 2,454,528 bytes, and aggregate
  `014a1024f13225a3f7bd7bba6be43dce1106a0354d68b5043f284263cce19bc9`;
- the unequal live-apply aggregate
  `6e9418798c60dc8b6a51ec8c155148ef465a9bc51c7c458295e3666c2141b9a6`,
  with interpretation limited to the stage-to-live-apply or restart-layout
  boundary and no semantic root-cause claim;
- equal coordinator control, local, and component-manifest receipts across
  residencies;
- capture token 21549 versus restore token 9283 and all retained suffix hashes;
- zero legacy `GET_TENSOR` and `SET_TENSOR` operations in both exact state
  windows;
- the 80,950,550,528-byte worker material allocation and expected placement in
  both residencies;
- 609 controller plus 224 child bounded transport records with zero timeouts;
- exact system-unit/cgroup/command/PID/listener authority, HTTP 200, and zero
  restart counters after worker-first recovery;
- correct classification of the contradictory user-scope query as a probe
  defect, explicit system scope in current production probes/mutations, and
  continued user scope only for disposable transient units;
- complete disposable cleanup;
- 76 passing focused tests and a clean diff check.

The reviewer independently recomputed the immutable raw evidence as 25 files,
1,135,907 bytes, canonical relative-path-plus-NUL-plus-content SHA-256
`8c82cec295e5e5d07160abe93ebcbdf6068c2372ec87b1aaa3be471610234cf8`.

The review accepts L29 only as a coherent, fail-closed terminal diagnostic
result. It does not authorize promotion, retry, a semantic root-cause claim,
or L30.

