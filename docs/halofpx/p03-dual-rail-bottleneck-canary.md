# P03 dual-rail bottleneck canary

Status: **two-subflow transport verified; no obvious generation signal in the
canary; expanded transport trial deprioritized**

P03 tests the concrete hypothesis raised by P02: whether its one-subflow
experimental launch hid useful throughput available from the second USB4 rail.
This is a bounded canary, not a speedup claim, transport promotion, or final
G9/G10 non-inferiority trial.

The experiment reused the exact P02 feature-off HaloFPX candidate at commit
`0cc67d1790fe2fee23ea8ffb0dc87ff48bb1be68`, tree
`2bacfc47b152a3b7fdfe0bea76743c3ede8acaff`. The RPC and server SHA-256 values
remained `76c575b5b8463013764253e9a3f8898a0196b3a5fe96174010a2729b6d104dc7`
and `6618dc53dc9fbecd492632fedb428af067a61f524313e0d16d2f4a81a148cf9b`.
Both L14Q candidates and all persistence gates were off.

The model was the pinned 160 GB ROCmFPX artifact at revision
`dba517197f2854f3d362529e13abddcdcad6c10b`, size `159873097824`, retained
SHA-256 `96506ada918e60ca9a9cfde8a5437790e4453401a6a3e236e3f55e7bac3aaea6`.
The request and runtime tuple were exactly P02: 1129 prompt tokens, 128 forced
generation tokens, Q8_0 K/V, context 4096, batch/ubatch 512, layer split `1,1`,
`RPC0,ROCm0`, fixed seed 1234, temperature zero, direct I/O, no mmap, offline,
and no WebUI.

The only intentional launch change was the nimo-1 RPC listener. P02 bound it
to `10.44.0.1`, which prevented a peer subflow addressed to `10.44.0.5` from
reaching the same port. P03 bound the disposable worker to `0.0.0.0`, matching
the known-good MPTCP deployment pattern. Existing firewall and private-link
controls remained authoritative; no persistent MPTCP, firewall, or system
transport configuration was changed.

After one excluded warmup, the retained request returned HTTP 200 with the
same decoded-content SHA-256 as P01/P02:
`3c9bcc5624f4a1d6558c51be89e2443ffba4b2206d10b632c89b7ef64fdd026f`.
It measured 203.7112 prompt tok/s and 16.6550 generation tok/s. MPTCP reported
`subflows_total:2` before and after the request. Nimo-1 rail deltas were:

| Rail | Receive bytes | Transmit bytes |
| --- | ---: | ---: |
| thunderbolt0 | 16,600,645 | 7,897,210 |
| thunderbolt1 | 23,699,753 | 10,310,910 |
| Total | 40,300,398 | 18,208,120 |

The total RPC traffic closely matches P02, but it is now visibly split across
both rails. This one retained result is only 0.0100% below P02's 16.6567 tok/s
candidate mean and agrees with its unprofiled 16.65 tok/s check. The second rail
is working, and the canary found no obvious generation signal. Because P03 did
not run a contemporaneous single-rail control, it does not establish a causal
zero benefit. It does justify deprioritizing a larger dual-rail-only matrix until
a new transport or scheduling hypothesis predicts a measurable change.

The disposable RPC worker stopped cleanly. Because the worker was stopped
before the coordinator, the experimental nimo-2 server exited with SIGABRT and
retained a 584.3 MB compressed core; future teardown must stop the coordinator
first. Stopping the pre-existing nimo-1 server before P03 also reproduced its
already-recorded SIGABRT/core-dump defect. Both abort dispositions and exact
transient commands are preserved from journald. Memory released, rollback was
not impeded, and the known-good binaries restarted in their original topology
with HTTP-200 health, zero restarts, and two MPTCP subflows.

The next performance gate is a matched feature-off build of current HaloFPX
HEAD versus the locked ROCmFPX control on the now-proven two-subflow topology.
After that control closes, candidate work should target compute/communication
overlap, transfer aggregation, placement, or a separately justified L14Q path,
not claim that dual rail alone is an optimization.
