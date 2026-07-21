# Independent review: L17 device discovery and layer-placement authority

Date: 2026-07-21

Reviewer: independent adversarial agent

Verdict: **PASS — no P1/P2 finding.**

## Source and executable authority

The reviewed source extracts the loader's existing layer-device resolver
without changing its formula. Both `llama_model_base::load_tensors()` and the
placement probe call that resolver.

The primary runner explicitly freezes `--device RPC0,ROCm0` and runs the pinned
placement probe after HFXCAP2 readiness but before starting the model canary.
The pinned L17 placement probe and pinned L17 distributed-state canary were
built together from the same loader and resolver source. Their hashes and build
lineage are preserved in the protected evidence.

## Placement contract

The probe requires exactly two selected non-null devices followed by the parser
terminator, names and order `RPC0,ROCm0`, backends `RPC,ROCm`, the exact RPC
endpoint, sane nonzero memory, split mode `layer`, and exact tensor split `1,1`.

Using the shared resolver, the 62 repeating layers resolve 32 to RPC0 and 30 to
ROCm0, with the output layer on ROCm0, for total ownership 32/31. The retained
self-test passes the correct case and refuses omitted-device, reversed-order,
one-device, and all-RPC ownership cases.

## Real-host evidence

The disposable probe admitted the exact endpoint, device/backend order,
protocol identity, limits, memory, split, and ownership tuple.

Two retained small-model runs used exact one-argument-per-line invocation
records with `RPC0,ROCm0`, layer split, and `1,1`. They produced identical
24-token output. Runtime logs prove nonzero model, KV, and compute buffers on
both devices, while the worker records the corresponding bounded RPC
allocations. No worker-local state object or HaloFPX state operation occurred.

The Flash Attention fallback warning is preserved and correctly limited to the
tiny fixture.

## Review reconciliation

The initial review rejected two issues: the runner still pinned an L16 canary,
and v1 lacked exact argv and retained negative-test output. The correction
co-built and pinned the L17 canary with the probe, asserted its path in tests,
and created immutable v2 evidence with the missing records. Re-review verified
both corrections and found no remaining P1/P2 issue. V1 was preserved.

## Boundaries and operations

L17 did not access or allocate the 159.9 GB primary artifact and does not prove
its byte distribution, allocation shapes, or capacity fit. P01 and P11 are
consistent supporting evidence, while the missing L16 device argument remains
a leading explanation rather than a proven sole cause. Any primary retry still
requires Project Lead authorization.

Production remained continuously active at coordinator PID 2144857 and worker
PID 1305879, with HTTP 200 and both `NRestarts=0`. Disposable units, port 50190,
keys, roots, sources, builds, and staging were cleaned.

The reviewed v2 archive is mode 0600, 18,681 bytes, SHA-256
`e229ee0df08d368087dfee11c19ae24cb3e5249f4ac398f2dfd01b4a6d42ceeb`.
Its 34-file manifest and zstd integrity check pass. The original v1 archive
remains preserved as superseded evidence.
