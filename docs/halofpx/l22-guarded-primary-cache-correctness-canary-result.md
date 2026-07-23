# L22 guarded primary cache correctness canary result

Date: 2026-07-23

Base: `851dc6f1af55c856532a5908516ebed9a5679891`

Outcome: **NOT PROMOTED — RESTART-RESTORE CORRECTNESS FAILURE**

## Result

[VERIFIED] The single authorized controller-managed transition used the pinned
159,873,097,824-byte ROCmFPX artifact with SHA-256
`96506ada918e60ca9a9cfde8a5437790e4453401a6a3e236e3f55e7bac3aaea6`.
The invocation retained the exact 1,129-token prompt, 1,128-token capture
boundary, 128 generated tokens, context 4096, batch/ubatch 512, Q8_0 K/V,
seed 1234, temperature zero, `--device RPC0,ROCm0`, layer split, and tensor
split `1,1`.

[MEASURED] Capture and the separate clean-cold context were exact. Both emitted
token SHA-256 `d8c8822f2ad7951dc363056b4c165e6696ad6066a41d68b1a99097526012b6d9`
and decoded-text SHA-256
`3c9bcc5624f4a1d6558c51be89e2443ffba4b2206d10b632c89b7ef64fdd026f`.
The missing-object and plan-mismatch cases cold-recomputed to those same
hashes, as did the matched feature-off cold control.

[MEASURED] The true worker-restart restore did not reproduce the continuation.
It emitted token SHA-256
`d32acac3fc1d2ac80ee33ad8cf66192112200b7b93ed88e520c7c56660a71470`
and decoded-text SHA-256
`b51fb7b571004d81d625dd392a2d97c8ff97b7bfce8f7926bd162cd793eedf1e`.
The runner therefore failed closed before producing a PASS result. L22 is not
promoted and no retry is authorized.

## State and allocation evidence

[MEASURED] Capture created a rank-local immutable worker object and reported
2,454,528 worker state bytes across 64 components. Coordinator control and
local bytes were 15,048 and 2,301,688 respectively. The restored path reported
the same worker bytes and component count. The bounded capture and restore
state windows contained zero legacy `GET_TENSOR` or `SET_TENSOR` operations.

[MEASURED] The admitted placement probe reported ordered devices RPC0 then
ROCm0, 32 repeating layers on RPC and 30 on ROCm, with 31 total ROCm-owned
material groups including output ownership. The worker journal recorded an
80,950,550,528-byte maximum RPC allocation and 81,712,377,856 aggregate RPC
allocations during the capture residency, consistent with the accepted L18
material plan. The harness did not retain an equally direct local ROCm
allocation counter; L18's loader plan and successful load remain supporting
evidence, not a substitute for a measured local allocation total.

The observed timings were retained only as milestone evidence:

- capture prompt/state/generation: 5,476.825 / 118.389 / 7,708.124 ms;
- cold prompt/generation: 5,444.101 / 7,753.920 ms;
- restore state/generation: 128.709 / 7,701.232 ms;
- missing fallback prompt/state/generation: 5,434.682 / 5,635.614 / 7,721.800 ms;
- plan mismatch prompt/state/generation: 5,413.797 / 5,512.868 / 7,699.092 ms;
- mode-off prompt/generation: 5,451.287 / 7,785.766 ms.

These values are not a final performance claim.

## Controller and production closeout

[VERIFIED] Before the transition, an independent adversarial review accepted
the repaired manifest-to-`Popen` binding. The exact normalized interpreter,
child, argv order, and controller-derived child evidence directory were bound
before key provisioning or shutdown. Sixty-two focused tests passed.

[VERIFIED] The controller stopped the coordinator before the worker and
recovered the worker before the coordinator after the correctness failure.
The final authority is nimo-2 worker PID 1396163 on port 50052, then nimo-1
coordinator PID 2213675 on port 8081 serving HTTP 200. Both exact production
commands/models reconcile with the preflight snapshot and both have
`NRestarts=0`. Disposable units, port 50180, protected keys, roots, and staged
build paths were removed by the closed cleanup contract.

The immutable external evidence root is
`C:\Users\britt\Documents\Custom_Inference_Project\sources\halofpx\l22-primary-20260723\primary-run-20260723T113115`.
Its 31 files total 2,892,349 bytes; the canonical relative-path-plus-content
tree digest is
`b56d3bd617e3eb43f58354d74f299833204e270cc246ce813686178411735e5c`.

## Terminal boundary

The cache remains compile- and runtime-default-off and is not enabled in
production. This milestone makes no eviction, shared/prefix reuse, broad fault
matrix, cable-fault, tuning, or final-performance claim. Diagnosing or changing
the restored-state correctness path requires a separately authorized
milestone; L22 does not open L23.
