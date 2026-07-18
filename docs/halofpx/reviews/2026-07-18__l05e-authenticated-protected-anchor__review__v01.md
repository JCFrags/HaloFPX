# L05e authenticated protected-anchor review v01

- Date: 2026-07-18
- Scope: offline protected-anchor wire, encoder, parser, authentication, and tests
- Final verdict: **ACCEPT**

## Independent review

The first adversarial pass returned REVISE for four blockers: anchor key IDs
were wider than the accepted ASCII registered-ID policy; the local tag compare
did not use the established volatile accumulator; the golden receipt omitted
the exact envelope and an automated independent-encoder check; and the parser
mutation matrix did not yet exercise enough closed/canonical CBOR failures.

The implementation was revised to:

1. accept only 1--128 ASCII non-NUL anchor key-ID bytes and reject multibyte and
   embedded-NUL inputs;
2. use fixed-trip volatile accumulators for key-ID and 32-byte tag comparison;
3. retain the exact 229-byte envelope, tag, digest, generator SHA-256, and
   reference runtime, and run the independent Python encoder as a CTest; and
4. reject wrong/duplicate/reordered map shapes, indefinite forms, non-shortest
   nested integers, wrong versions/algorithm, malformed byte/text lengths,
   invalid key text, truncation, trailing input, and oversize input while
   exposing no unauthenticated carrier.

The independent re-review returned ACCEPT. It found no filesystem, provider,
server-runtime, or persistence linkage and confirmed the target remains
`EXCLUDE_FROM_ALL`.

## Verification

| Gate | Result |
|---|---|
| Clean Windows CPU/WebUI-off Release build including `llama-server` | Pass |
| Full HaloFPX-labeled CTests | Pass, 16/16 |
| Focused inherited CTests | Pass, 7/7 |
| C++ anchor process repetitions | Pass, 100/100 |
| Independent Python golden repetitions | Pass, 100/100 |
| Independent adversarial review | ACCEPT after revisions |
| Reference clone status/head checks | Four repositories clean and unchanged |

The build emitted inherited compiler conversion warnings and the expected
OpenSSL-not-found/HTTPS-disabled configuration warning; neither is introduced
by or linked to the anchor codec.

## Promotion boundary

L05e is accepted only as an offline memory codec. The publication coordinator's
provisional 32-byte store ID and non-optional predecessor representation remain
incompatible with the frozen wire's 16-byte UUID and generation-one null
predecessor. Bootstrap, protected key authority, cross-process exact-envelope
CAS, filesystem durability, persistent writes, server integration, and nodes
remain closed.
