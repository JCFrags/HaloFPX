# L81 terminal result

Status: **NOT PROMOTED**

The single authorized primary attempt stopped at the warmup kill gate. The
pinned primary model loaded, then `llama_decode` returned `-3` before any
workload token, state capture, stage, commit, restore, or state comparison.

The expected `[halofpx-composed-failure]` diagnostic is absent from the complete
retained client journal. The independently authenticated server authority is
present and decodes to terminal branch 3
(`ADMISSION_ACCEPTED -> ABORT`) with reason 10
(`WRONG_ALLOCATION_EPOCH`), a zero execute receipt, and no physical
prepare/consume/backend-execute/receipt-publication record. Its immutable SHA-256 is
`553a22a4674cf87ef55311e0f4f2fa1d5773ebe9e5359a5c67bf8497c17e076d`.

This disproves the L80 inference that the observed failure necessarily followed
successful server execution. The narrowest retained boundary is now:
authenticated server admission accepted, then allocation-topology epoch
rollover before physical prepare, with the client returning
`GGML_STATUS_FAILED` before reaching the postcompute failure recorder. Current
source maps this to buffer allocation/free advancing `allocation_topology_epoch`
while the mutable session remains admitted
(`ggml/src/ggml-rpc/ggml-rpc.cpp:5763` and `:5843`).

The early kill gate held: no primary workload or cache action followed warmup
failure. Server authority custody completed before cleanup. All disposable
units, keys, source/build paths, and evidence paths were removed. Production
was recovered worker-first/coordinator-second with the same system units,
argv, listeners, and `NRestarts`; only expected PID/start-time fields changed.
The coordinator health endpoint returned HTTP 200.

No cache correctness or performance conclusion is admitted.
