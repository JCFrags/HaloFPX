# L65 independent adversarial review

Verdict: **FAIL / NOT PROMOTED**.

The independent reviewer rejected the L65 candidate. The candidate source was
removed in full and is not present in the terminal commit.

## Seven-blocker assessment

1. **Negotiated/versioned capabilities: not closed.** The candidate carried an
   exact capability mask and protocol-major fields, but retained qualification
   did not prove real client/server rejection of unsupported combinations
   before mutation.
2. **Connection/allocation epochs: not closed.** Separate fields and allocation
   rollover existed, but reconnect, teardown, cross-session,
   cross-connection, and stale-allocation invalidation were not all retained as
   real refusal evidence.
3. **L42 prepared admission: not closed.** The admission was independently
   HMAC-authenticated, lifetime-checked, and non-consuming to observation, but
   independent verification did not accept and compare scheduler
   session/generation, prepared-handle identity, backend ordinal, split count,
   and split-mapping root against independent expectations. L44 also marked
   begin consumed before admission authentication completed.
4. **Recorder/grammar/concurrency: not closed.** Recorders preceded L44 begin
   and two attempts genuinely overlapped, but the terminal grammar admitted
   ranges (`begin 1..2`, register/exclude allowance ranges, and abort upper
   bounds) rather than exact productions. The candidate verifier also accepted
   unterminated streams.
5. **Observed transport: not closed.** The candidate implemented ordered
   byte-counted transport stages and actual socket-path injections, but the
   retained terminal evidence package did not preserve the complete injection
   matrix and command/exit provenance.
6. **Evidence preservation/publication: not closed.** Publication failure did
   not advance in-memory chain state, but shared publication used only an
   in-process mutex. It lacked interprocess locking or exclusive unique
   per-attempt paths with collision/refusal authority.
7. **Full refusal and qualification coverage: not closed.** The source fixture
   covered successful execute/recompute, exact output, multiple UIDs,
   allocation rollover, overlap, cross-connection tensor, unknown role,
   duplicates, and post-abort reuse. The retained package nevertheless lacked
   the complete manifest, logs, exit statuses, refusal matrix, compile/runtime
   off receipts, verifier report, harvesting receipt, and proof of evidence-set
   completeness required for promotion.

Disposition: **reject L65 candidate; retain terminal evidence and review only.**

