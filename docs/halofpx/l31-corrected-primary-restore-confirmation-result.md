# L31 corrected primary restore confirmation result

Date: 2026-07-23

Base: `b630c4f52c849af8cd8ebd30451a8c1268979ce3`

Outcome: **NOT PROMOTED — EXACT FIRST TOKEN MISMATCH**

## Result

[VERIFIED] The single authorized controller-managed transition used the exact
159,873,097,824-byte primary artifact with SHA-256
`96506ada918e60ca9a9cfde8a5437790e4453401a6a3e236e3f55e7bac3aaea6`.
It retained the frozen 1,129-token prompt, 1,128-token boundary, one generated
token, Q8_0 K/V, flash attention on, context 4096, batch and ubatch 512, seed
1234, temperature zero, explicit `RPC0,ROCm0`, layer split, tensor split
`1,1`, diagnostics, and two honest fresh model residencies.

[MEASURED] Residency A used coordinator PID 1467191 and worker PID 2303983 /
InvocationID `499a5d74b87e4958b7294ea98c288623`. It captured immutable object
`9e05567910ab88509cdde28a39d469543841de0339ed54b4c133a03af9c33b10`
and reference token 21549. Residency B used coordinator PID 1467285 /
InvocationID `bf6db1e065324ff89b3fdf5d186bd565` and distinct worker PID 2304106 /
InvocationID `763a8c4836f04bb9883c91508b39af54`. It restored token 9283.
Capture and restore token hashes were respectively
`7d44cce76babad0e2459d2295ac8704af317509458dd9ea8bbfe2e6cc151e051`
and `1d99e63f856e6c2a2903c515234d94ce863b30512beabe1a06e26cef831f9382`.
Exact correctness therefore failed at the first generated token.

## Authenticated component interpretation

[MEASURED] Capture, validated stage, and immediate live post-apply each contain
64 components and 2,454,528 bytes. All 64 are type 8 (Q8_0), with 32 kind-1
and 32 kind-2 records; every component is 38,352 bytes. Authenticated
identity-and-content comparison reports zero mismatches and no mismatch
ordinals. All three content aggregates equal
`014a1024f13225a3f7bd7bba6be43dce1106a0354d68b5043f284263cce19bc9`.

[MEASURED] Capture and stage Merkle roots equal
`0fc1f297514f5b7a38f9db179e80df0bb9b5ac6ae3865858d518669ef8a79884`.
The apply Merkle root is
`aac062cc10c560807b80d06bcf49ea79deb563f89d6924b7d933d278a66fe429`.
The Merkle difference records range topology: capture/stage ordinal `n` has
range `[38656*n,38656*n+38352)`, leaving 304 bytes of padding between
consecutive components; apply ordinals 0–31 have
`[8912896*n,8912896*n+38352)`, while apply ordinals 32–63 have
`[4456448+8912896*(n-32),4456448+8912896*(n-32)+38352)`. Ordinal 0's
normalized range is unchanged and ordinals 1–63 differ. No phase reports an
overlap, and each has one buffer group. Capture/stage topology SHA-256 is
`cc8fa3afa7634b563265ebcd96b0ba29b967d8e226b6ee90f6abe308c6346024`;
apply is
`18710186bc677ad618e79429990274ee8fc34fbaa1f40a67d3d24d185ae14f5c`.

This is a bounded observation only. The authenticated evidence establishes
equal component identity and content while recording different live range
topology; it does not establish a new root cause for the token mismatch.

[MEASURED] Coordinator capture and restore receipts are byte-identical:
control is 15,048 bytes /
`2d614e8634f7f9defc4ed59f59b900490e021e382045c566109588bd288a0cbb`,
local is 2,301,688 bytes /
`7117319f7dc2b848d3ce3b35469aee3c62bc93a8262f88cb53949e7bbc5ceaca`,
and the component-manifest digest is
`7ad364fb0a047ca8db745c439bf8ad3f6fe6b92ae56f7b9f632caa88e47c69b3`.

## Allocation, state transport, and timing

[MEASURED] Both residencies allocated 80,950,550,528 worker material bytes.
Capture timing was 5,502.264 ms prompt, 116.611 ms state, and 76.177 ms
generation. Restore state and generation were 137.849 ms and 74.243 ms.
These are diagnostic observations, not performance claims.

[VERIFIED] Capture and restore state windows each contain zero legacy
`GET_TENSOR` and zero `SET_TENSOR` operations. The controller retained 625
bounded transport records and the child retained 229; neither stream contains
a timeout.

## Recovery, cleanup, and evidence

[VERIFIED] Production recovered worker-first. Nimo-2's exact system unit
`minimax-m27-rpc-worker.service` is loaded active/running in
`/system.slice/minimax-m27-rpc-worker.service`, PID 1468887, listener 50052,
`NRestarts=0`. Nimo-1's exact system unit
`minimax-m27-q6-server.service` is loaded active/running in its system cgroup,
PID 2304428, serves the standard UD-Q6 model on 8081 with HTTP 200, and has
`NRestarts=0`.

[VERIFIED] All L31 disposable units, port 50191, keys, state, evidence staging,
rendezvous, remote archives/build roots, and the local staging archive are
absent.

The immutable evidence root is
`C:\Users\britt\Documents\Custom_Inference_Project\sources\halofpx\l31-primary-20260723`.
Its 41 files total 2,002,777 bytes and have canonical
relative-path-plus-NUL-plus-content SHA-256
`f8cbc28dace26dbbf25747b38316ba02b77679e49db808c9014aa2215f223de8`.

## Boundary

L31 is terminal NOT PROMOTED and was not retried. No semantic correction was
implemented. This result does not authorize cache promotion, production cache
enablement, a performance claim, another primary run, or L32.
