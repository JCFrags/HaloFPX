# L41 — scheduler execution-authority foundation

Result: **NOT PROMOTED**

Base: `53f414dfc5a8f9873ad9961f541eb41cf6dc2aae` (accepted L40 PASS)

L41 was restricted to a no-primary, no-production implementation of authenticated
authority at the real ggml scheduler split and copy seams. Production was never
mutated and the primary artifact was not accessed.

## Candidate evidence

The candidate compiled on nimo-2 and a disposable ROCm RPC worker listened only
on `127.0.0.1:50241`. The focused executable observed:

- feature-off output preservation;
- a genuine two-split graph with one ordinary RPC-to-CPU copy;
- exact ordinary output equality;
- one real `MUL_MAT_ID` expert execution-seam fixture with two authenticated
  partial-range transfers.

Raw output and binary/log hashes are retained under
`docs/halofpx/evidence/l41-raw/`. The disposable process and listener were
removed after the run.

## Why L41 did not pass

Independent adversarial review found that the candidate could not support an
independently verifiable exact contract:

- its authenticated event stream was internal and only an opaque final
  root/counters were returned;
- destination allocation, buffer-relative range, and full nested-view authority
  were incomplete;
- essential malformed, tamper, ordering, overlap, quantized, strided, and
  padding refusal cases were not qualified;
- the expert fixture proved branch execution but not deterministic graph output.

The findings are material to L41's objective. The rejected source and test
candidate were removed, leaving only this closeout, raw evidence, and review.
L40 is unchanged and remains the accepted RPC graph-authority foundation.

## Production and cleanup

At closeout:

- nimo-2 system unit `minimax-m27-rpc-worker.service` was active/running,
  MainPID `1535639`, listening on `50052`, `NRestarts=0`;
- nimo-1 system unit `minimax-m27-q6-server.service` was active/running,
  MainPID `2356329`, listening on `8081`, HTTP `200`, `NRestarts=0`;
- disposable port `50241` was closed;
- no L41 candidate source remained.

No L42 is opened.
