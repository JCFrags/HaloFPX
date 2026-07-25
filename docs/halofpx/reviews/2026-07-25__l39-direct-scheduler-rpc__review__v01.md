# L39 direct scheduler/RPC adversarial review

Verdict: `REJECT`.

The reviewer found that a PASS is not supportable. Although the candidate
compiled and the corrected combined fixture forced scheduler splits, copies,
RPC compute, and recompute with exact output, the execution authority remained
incomplete.

Material findings:

1. The client sent its own HMAC request but received no negotiated,
   independently generated authenticated server reconstruction receipt.
2. Mutable role admission depended on tensor names and only rejected unknown
   tensors carrying the input flag; unknown active unflagged inputs could be
   silently omitted.
3. Server reconstruction copied client wire allocation fields into its
   canonical comparison, making buffer ordinal/range agreement partly
   circular.
4. Scheduler authority was a raw callback rather than bounded HMAC-framed
   events. It omitted complete op parameters, flags, source/view chains,
   backend class, and independently authenticated content roots.
5. Flat range reads did not establish logical nested/non-contiguous view
   authority. The expert partial-copy hook was not exercised.
6. The tensor-update transcript lacked canonical graph identity, full view and
   buffer-relative authority, hard update bounds, and explicit SET versus
   SET_HASH coverage.
7. Focused HMAC, graph, transcript-order, unknown-role, copy, recompute,
   capability, and bound tamper/refusal cases were absent.
8. The combined graph did not execute Q8_0 flash attention, expert partial
   copies, nested views, or SET_HASH.
9. Recompute reused the graph UID rather than binding a newly generated opaque
   attempt/execution identity, and the receipt used native structure bytes
   rather than canonical little-endian encoding.

The review accepts only removal of the candidate and a narrow terminal
`NOT PROMOTED` closeout. It does not authorize a primary run or follow-on
implementation.

