# ADR-0021 operations 1-4 execution-closure review

- Date: 2026-07-18
- Result: ACCEPT
- Scope: portable registry-lab fake-engine operations 1-4 only
- Reviewed decision SHA-256:
  `850a78e4b0b18241cc9f98a0eeca6cd662422a9560a2f6187afc17362ae929ea`

## Review outcome

The independent adversarial review required three revision rounds before
acceptance:

1. Ownership, not a scripted primitive code, now determines guard and writer-
   lock acquisition and busy behavior. Confirmed and latent-death semantics
   are exact.
2. Every invocation supplies one immutable four-entry script, validates its
   complete shape before entry, and consumes entries once through a monotonic
   pause/resume cursor.
3. The exception model is feasible: the admitted slice is allocation-free and
   `noexcept`, makes no exception-to-status claim, and cannot inject an
   exception through Ops.
4. Modeled death is process-wide. It invalidates every invocation in the dead
   process slot, removes every ordinary result, separately audits every secret
   owner, and cannot clear another process's writer lock.

The accepted review also verified the exact algebra counts (600 products, 55
admitted for operations 1-5, 43 admitted and 437 forbidden for operations
1-4, 11 lost-response and 16 death products), pre-entry rejection, cleanup and
result visibility, fieldwise secret-free restart images, and the independent
seven-archive audit design.

The subsequent status-only edit from `proposed` to `accepted` does not alter
the reviewed contract. Acceptance does not authorize operation 5 execution,
decoding, mutation, Linux I/O, persistent writes, runtime linkage, or public
authority.
