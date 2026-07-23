# L29 primary fresh-residency discriminator result

Date: 2026-07-23

Base: `09123048522281025afb532715f20457ac4b9918`

Outcome: **NOT PROMOTED — LIVE-APPLY BOUNDARY AND TOKEN MISMATCH**

## Result

[VERIFIED] The one authorized controller-managed transition used the exact
159,873,097,824-byte primary artifact with SHA-256
`96506ada918e60ca9a9cfde8a5437790e4453401a6a3e236e3f55e7bac3aaea6`.
It retained the frozen 1,129-token prompt, 1,128-token boundary, one generated
token, Q8_0 K/V, flash attention on, context 4096, batch and ubatch 512, seed
1234, temperature zero, explicit `RPC0,ROCm0`, layer split, tensor split
`1,1`, and diagnostics.

[MEASURED] Capture residency A used worker PID 2283443 / InvocationID
`58edc48d8f424a7b9886f7504b99d2c7` and coordinator PID 1453207. The
authenticated epoch sidecar bound those identities and worker object
`19f6335d78631ce7bf2a1b8320c405e8b959fff59c7086711a8b752288ec63e2`.
The reference token was 21549. Capture decoded the 1,129-token prompt in three
bounded chunks, maximum 512.

[MEASURED] Coordinator A terminated before worker A. Restore residency B used
the distinct worker PID 2283566 / InvocationID
`3439676f5ee14422b2beff2c98f772c5`, then loaded coordinator B PID 1453302 /
InvocationID `30f90c1e1eea44049579c6a16777fb43`. Exact HFXCAP2 and the current
worker/model-allocation epoch were revalidated before the capture receipt was
verified and staging began.

[MEASURED] Capture and stage each reported 64 components, 2,454,528 bytes, and
aggregate
`014a1024f13225a3f7bd7bba6be43dce1106a0354d68b5043f284263cce19bc9`.
The immediate live-post-apply aggregate was instead
`6e9418798c60dc8b6a51ec8c155148ef465a9bc51c7c458295e3666c2141b9a6`,
also across 64 components and 2,454,528 bytes. The captured and staged bytes
therefore agree, while the staged and live-applied boundaries do not.

[MEASURED] Coordinator capture and restore receipts were equal:

- control: 15,048 bytes,
  `2d614e8634f7f9defc4ed59f59b900490e021e382045c566109588bd288a0cbb`;
- local: 2,301,688 bytes,
  `7117319f7dc2b848d3ce3b35469aee3c62bc93a8262f88cb53949e7bbc5ceaca`;
- component manifest:
  `7ad364fb0a047ca8db745c439bf8ad3f6fe6b92ae56f7b9f632caa88e47c69b3`.

The restored token was 9283, not 21549. Capture token/text hashes were
`7d44cce76babad0e2459d2295ac8704af317509458dd9ea8bbfe2e6cc151e051`
and `c7d2216ad33210a1234c043b4074ff70c9a1b127590f612f813d904c597a9746`;
restore hashes were
`1d99e63f856e6c2a2903c515234d94ce863b30512beabe1a06e26cef831f9382`
and `0f6b1949b093e35266991fbea7a832212d428e874cc0666634faf37f51d4c63b`.

Strictly, this localizes the first retained unequal worker boundary to
stage-to-live-apply or restart layout. It does not establish the semantic root
cause. Equal coordinator receipt digests do not override that earlier worker
boundary difference.

## Allocation, transport, and timing

[MEASURED] Both residencies admitted `RPC0,ROCm0` with the intended 32 RPC
repeating layers, 30 ROCm repeating layers, and output ownership recorded by
the placement receipts. Each worker allocated an 80,950,550,528-byte material
model buffer. Capture timing was 5,487.380 ms prompt, 113.838 ms state, and
73.142 ms generation. Restore state and generation were 138.957 ms and
73.383 ms. These are diagnostic observations, not performance claims.

[VERIFIED] The capture and restore state windows each contained zero legacy
`GET_TENSOR` and zero `SET_TENSOR` operations. Ordinary model load and graph
RPC tensor operations outside those windows are not counted as state payload
transfer.

[VERIFIED] The controller retained 609 bounded transport records and the child
retained 224. No SSH operation timed out.

## Recovery, cleanup, and scope correction

[VERIFIED] Production recovery was worker-first. Nimo-2's exact system unit
`minimax-m27-rpc-worker.service` is active/running in its named cgroup,
PID 1454894, listener 50052, `NRestarts=0`. Nimo-1's exact system unit
`minimax-m27-q6-server.service` is active/running in its named cgroup,
PID 2283857, serves the standard UD-Q6 model on 8081, returns HTTP 200, and has
`NRestarts=0`.

A closeout probe initially queried `systemctl --user` and observed absent user
units while the exact system-unit processes and listeners were healthy. This
was a wrong-systemd-scope probe defect, not production inactivity. Cgroup proof
and explicit `systemctl --system` results are retained. No recovery or duplicate
start was attempted. Current controller production inspection and mutation
commands now bind system scope explicitly; disposable transient units remain
user scoped, with a focused test preventing regression.

[VERIFIED] All manifest-owned disposable units, port 50189, keys, state,
evidence staging, rendezvous, remote archives/build roots, and the local source
archive are absent.

The immutable evidence root is
`C:\Users\britt\Documents\Custom_Inference_Project\sources\halofpx\l29-primary-20260723`.
Its 25 files total 1,135,907 bytes and have canonical
relative-path-plus-NUL-plus-content SHA-256
`8c82cec295e5e5d07160abe93ebcbdf6068c2372ec87b1aaa3be471610234cf8`.

## Boundary

L29 is terminal NOT PROMOTED and was not retried. No cache semantic correction
was implemented. L29 does not authorize cache promotion, performance claims,
production enablement, another primary run, or L30.
