# L27 RPC worker epoch/residency adversarial review

Date: 2026-07-23

Verdict: **PASS after correction**

The reviewer independently inspected the RPC client/server lifetime source,
the L20/L22/L23 versus L24/L26 process boundaries, the disposable
discriminator, the fail-closed diagnostic change, and the focused tests.

The source-backed result is accepted: server buffer/tensor identifiers and
maps are process-local; live client model buffers retain their creating
socket; a separate HELLO/HFXCAP2 probe does not refresh those objects. The
same-residency abort and fresh-process exact restore support the stated
lifetime dependency. The reviewer independently reproduced the 71/71 focused
test pass.

Two material findings were corrected before acceptance:

1. The epoch/PID/load-order validator is standalone and unit-tested, not
   consumed by a runnable fresh-residency path. The ADR, result, and receipt
   now state that boundary; operational protection today is the unconditional
   refusal of the legacy same-residency diagnostic.
2. Closeout production and cleanup claims lacked retained raw evidence.
   Read-only snapshots now preserve exact production unit/PID/listener/HTTP
   authority plus exact disposable unit, port, source, state, and key absence.
   The result no longer claims unverified rendezvous cleanup.

With those corrections, no material review issue remains. This review does
not authorize a primary experiment, transparent RPC recovery, or L28.
