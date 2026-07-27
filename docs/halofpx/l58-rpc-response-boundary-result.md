# L58 RPC response-boundary discriminator

Status: **NOT PROMOTED**
Date: 2026-07-27
Base: `0026d5243c6108659fa53ce9185af9de0d6ec857`

## Qualified instrumentation

L58 added a runtime-default-off, HMAC-authenticated response-boundary record at
the real RPC graph execute seam. The bounded client record distinguishes
request publication, response-header EOF/error, response-size mismatch,
response-body EOF/error, decode refusal, and authenticated receipt refusal.
The independently generated server record distinguishes handler admission,
backend completion, receipt construction, handler exit, and response
header/body publication. Feature-off continues through the original transport
methods without traversal, evidence allocation, or wire changes.

Focused qualification passed 59 Python tests and 11 subtests. The verifier
requires exact client/server cardinality, consecutive event ordering, closed
phase alternatives, bounded files and records, HMAC validity, and cross-side
attempt, connection epoch, split UID, execution sequence, backend ordinal, and
opcode agreement. An independent pre-runtime review returned **GO** after the
receipt-construction failure path was made terminal and directly tested.

The first controller attempt, retained separately under `l58-raw`, refused
before model launch because the runner still compared binary provenance
against stale compile-time constants. The narrow runner/controller binding was
changed to consume the same closed manifest provenance instance, all hashes
and binaries were rebuilt, and the final dry-run passed. No disposable model
runtime occurred in that rejected preflight.

## Sole stories15M discriminator

The one admitted runtime (`l58-raw-run2`) passed key, exact source/binary,
ROCm device, HFXCAP2 readiness, and placement admission. It launched:

- worker unit `halofpx-l48-worker-capture.service`, invocation
  `9f7304f4e77b4636abb84d5d2f230f87`, PID `2513812`;
- canary unit `halofpx-l48-canary-first-chunk.service`, invocation
  `91a232f71c2640fc9e4c3389171c5291`, PID `1770973`.

The worker journal records an ordinary RPC graph execution with 144 nodes and
193 tensors followed by tensor reads and client connection closure. The
coordinator records scheduler status `-1`, `llama_decode` status `-3`, the
authenticated L55 branch `scheduler_graph_compute_failed_authority_0` for
execution sequence 1, and then `ggml-rpc.cpp:1997`:
`Remote RPC server crashed or returned malformed response`.

The coordinator subsequently aborted while freeing the RPC buffer. Systemd
records `Result=core-dump`, `ExecMainCode=3`, and `ExecMainStatus=6`; its
backtrace reaches `ggml_backend_rpc_buffer_free_buffer`. This teardown failure
is secondary evidence and does not classify the original response failure.

Neither authenticated response JSONL stream was retained. The runner copied
the streams only after a successful canary unit result, so the unit-exit
exception bypassed harvesting and controller cleanup then removed the remote
roots. Consequently L58 cannot distinguish server handler crash/exit, absent
publication, truncated or malformed response, response-size mismatch, socket
EOF/error, client decode/receipt refusal, or a teardown-only failure. The
ordinary worker graph log is not an authenticated L58 completion receipt.

The independent final review therefore accepted only this terminal
**NOT PROMOTED / ambiguous response boundary** classification. The smallest
future prerequisite is failure-path harvesting before worker stop and cleanup:
independently stat/copy/fsync both streams, retain explicit missing/error
status, then authenticate every available closed prefix. L58 does not
authorize that correction or another run.

## Production and cleanup

Production was never stopped or reconfigured. Run2 before/final snapshots are
byte-identical with SHA-256
`511f05d2b638277e19ae7af44573eb9e244d829458f24cdefc46e3f88ffd6ded`:

- nimo-2 system worker PID `1535639`, port `50052`, `NRestarts=0`;
- nimo-1 system coordinator PID `2356329`, standard UD-Q6 command, port
  `8081`, HTTP `200`, `NRestarts=0`.

All disposable units, ports, keys, roots, staged source/build trees, and
controller-owned evidence publication paths were absent at closeout. The
rejected preflight evidence contains 7 files with canonical tree hash
`803718b821b961fafcd5cb185e75955780cdc94c9c57e9f9ebf7be2e9b7fb821`.
The sole runtime evidence contains 19 files with canonical tree hash
`245ebcb240af6cb36fc6a4261d6d5d67e9c298a5dde0d5a10e1d8fa0471f32c5`.

L58 is terminal **NOT PROMOTED**. It makes no primary-readiness, cache
correctness, or performance claim and authorizes no retry or L59.
