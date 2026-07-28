# L67 final ADR-0049 foundation result

Result: **PASS**

L67 recovered the exact rejected L66 candidate and made only the four bounded
corrections authorized by Project Lead:

1. An independently constructed scheduler-source expected admission now binds
   and verifies the complete admission tuple. The execute request and signed
   receipt carry the same admission object ID and expected-object digest.
2. Runtime and offline verification use mirrored finite, versioned, exact
   terminal-production tables and reject incomplete or structurally invalid
   streams.
3. Evidence publication uses unique per-attempt files, `O_EXCL` temporary
   creation, a `0400` transition, file fsync, byte-exact reopen verification,
   atomic no-replace `linkat`, directory fsync, and final reopen verification.
4. The focused manifest validates case completeness and semantics, HMAC
   authority records, artifact identity, process/InvocationID provenance, live
   source diff identity, the pinned key-file digest, and an HMAC-authenticated
   binary/source receipt.

Focused observed results:

- composed fixture: `real_composed=1 recompute=1 concurrent=1 exact=1`,
  UIDs `2/5/1`, allocation epochs `2/4/2`;
- authenticated authority verification: three terminal successes, 87 records,
  six real transport commands;
- admission refusals: expired and aborted both refused; partial binding,
  self-comparison, reuse, and cross-side receipt mismatches refused;
- transport refusal: actual `send_error:1` path refused and retained as exact
  branch 3;
- publication: two separate processes raced the same identity; one published,
  one failed with `EEXIST`, and the retained file was mode `0400`;
- structural verifier: unterminated, extra, missing, reordered, unknown, and
  post-terminal variants all refused;
- runtime-off inert and compile-off `rpc-server` build passed.

Independent adversarial review initially rejected exact-grammar, publication,
and manifest details. After source-proven corrections, the same reviewer
returned final PASS with no P1/P2 and recommended promotion and retention.

No stories, model, cache, performance, or production work was performed.
