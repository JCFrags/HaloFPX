# L111 lead reconciliation

Reconciled: 2026-08-12

This note is additive. It does not modify the immutable L111 source, build,
review, or delivery receipts.

## Terminal source identity

- Accepted pre-change base:
  `6c88472bf5f567a1064f27f4d8a90fc8e2b47a02`.
- Terminal retained source:
  `620ef60aa446990335ef46c7d76738f797e62f8f`.
- The terminal source is exactly one commit after, and a direct child of, the
  accepted base.
- The SHA-256 of the binary Git diff for the seven tracked candidate source
  files is
  `4f0dd8080d24b1a0401eae60ac561f0c605b27076b0bcef9534935a29c8b2551`.
  It matches `source-receipt.txt`.
- The committed focused test Git blob has SHA-256
  `5f9b68ce557fa74df47e811fe606458518cdc7b2938bc44b33a780d067e3c2de`.
  It matches `source-receipt.txt`.
- `git diff --check` from the accepted base to the terminal source passes.

The candidate-file hashes in `source-receipt.txt` are byte hashes from the
original Windows working checkout. They were rechecked against that preserved
checkout and all eight match. Seven files used mixed or Windows checkout line
endings, so their checkout-byte hashes intentionally differ from the normalized
Git blob hashes. The focused test was already LF and matches as both checkout
bytes and a Git blob. The exact commit, parent, source-diff digest, and focused
test blob digest provide repository-portable identity.

The receipt's 715-line focused-test length used PowerShell
`Measure-Object -Line`, which excludes blank records. Enumerating the same
committed content yields 771 records. The SHA-256 match proves that this is a
line-count-method difference, not different test content.

## Receipt interpretation

The immutable source receipt says
`accepted_head=6c88472bf5f567a1064f27f4d8a90fc8e2b47a02`. That value identifies the
accepted base on which the candidate was built. It does not identify the
terminal retained source, because the receipt was captured before the clean
L111 PASS commit. The terminal retained source is `620ef60...`.

The immutable `project-lead-report.txt` says that direct delivery was rejected
because its requested task identifier was not present in the reachable thread
tree. That is a control-plane delivery failure. It is not a technical review
failure. The retained independent review says `PASS / RETAIN` with no P0 or P1
finding.

The 2026-08-12
[Project Lead decision](../../../../project/project-management/lead/DECISIONS.md#2026-08-12--accept-the-bounded-l111-loader-foundation)
supplies the missing terminal lead disposition.

## Accepted boundary

L111 is accepted only as a loader foundation. The result covers the
loader-internal, generation-bound atomic two-rank partition transaction and
its focused accounting, rollback, lookup, and obsolete-mode-refusal checks.

It does not accept or promote:

- MiniMax graph execution or rank-owned expert routing;
- asynchronous RPC or scheduler behavior;
- model or runtime execution;
- production state;
- cache-product behavior;
- user-facing product readiness; or
- correctness or performance for the primary model.

Feature-off evidence is compile-level. No real MiniMax model was loaded in
L111. Any later graph or RPC milestone needs a separate decision and separate
qualification.

## Evidence limitations retained

- The focused fixture is a tiny contiguous F32 GGUF on two mock GPU devices.
  It does not qualify quantized MiniMax tensors, ROCm, or RPC devices.
- No production caller uses `create_axis2_partition_pair`; the accepted
  constructor remains a foundation awaiting separate graph integration.
- Public lookup behavior, stale and cross-loader refusal, negative cases, and
  deterministic destruction are focused unit-test checks. Some are narrower or
  indirect compared with the literal worker specification. They are not an
  end-to-end model-loader or runtime result.
- The transaction records are process-global and pointer-bound. The evidence
  assumes the current synchronous, serial loader use and does not qualify
  concurrent construction.
- The transaction functions are absent from public headers but remain
  link-visible. The test-only failure-injection environment hook also remains
  in normal source.
- Raw build logs, binaries, the disposable build tree, reviewer transcript,
  and reviewer identity were not retained. Therefore the recorded CTest,
  feature-off build, review, and binary hashes cannot be independently rerun
  from repository artifacts alone. This reconciliation verifies their
  receipts, source identity, and retained review text; it does not promote them
  into broader runtime evidence.
