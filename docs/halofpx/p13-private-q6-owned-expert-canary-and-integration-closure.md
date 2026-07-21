# P13 private Q6 owned-expert canary and integration closure

Status: **direct HIP canary passed; RPC/model integration closed for insufficient end-to-end leverage**

P13 converts the P12 compute-only opportunity into the smallest direct
implementation that can measure real ownership preparation overhead. The
result is correct and locally faster, but its absolute saving is too small to
justify another RPC/model-graph integration after P09 and P10 produced
multi-percent exact-model regressions.

## Authority and default-off boundary

The implementation parent is HaloFPX commit
`45ae40532391ccc2ff121624b865ea366fda1036`, tree
`decc9fc72530172bd821deba06a08bc6615260c5`. A new
`HALOFPX_MINIMAX_M2_Q6_PRIVATE_HIP_CANARY` option defaults off and is rejected
unless HIP is enabled. Only an ON build registers the private backend proc
`halofpx_minimax_m2_q6_owned_private_v1`.

The proc is not a public ggml operation or ABI. It has no RPC command and no
model-graph caller. Caller-owned, contiguous ROCm tensors make buffer and
stride authority explicit. Host validation accepts only Q6 physical weights
with 96 experts, exact 3072/1536 projection pairs, eight global IDs, four
compact slots, base 0 or 96, and exact output/trace shapes.

On the device, one ordered validator rejects IDs outside `[0,192)`, duplicate
IDs, and ownership counts other than four. Valid input is compacted in original
slot order, evaluated through the inherited tuned MMVQ path, and scattered
into an initially zero eight-slot partial. Invalid input uses zeroed safe
scratch, returns a nonzero trace status, and leaves the published partial all
zero; it cannot address weights with an untrusted ID.

## Direct gfx1151 qualification

The Release candidate was built and run on nimo-2 with the pinned ROCm compiler,
gfx1151, forced MMQ, no VMM, HIP on, Vulkan off, and `HSA_ENABLE_SDMA=0`.
The ON `libggml-hip.so` SHA-256 is
`9753aabb1dbe89e50873f5d7fab98bf91f82a4c95c2b0513ae901e4a93725b8d`;
the standalone test SHA-256 is
`d800d0b6d9737858ee26ed0e4ce6f3ffb63619dc520c78dfc1612b831f52da7e`.

Across five fresh-process runs, both bases returned bit-exact scattered output
against the inherited direct MMVQ oracle (`NMSE=0`). The oracle independently
derives local IDs, compact activations, and destination slots on the host, and
the selected local experts use distinguishable quantized weights. The test
also compares the helper's compact IDs, gathered activations, and trace with
those independent values. Invalid base was rejected
on the host. Out-of-range, duplicate, and wrong-owned-count inputs returned
statuses 1, 2, and 3 respectively, each with an all-zero partial.

| Run | Private, us | Padded eight-slot, us | Ratio |
| ---: | ---: | ---: | ---: |
| 1 | 87.844 | 110.536 | 0.794712 |
| 2 | 87.949 | 109.588 | 0.802538 |
| 3 | 87.945 | 110.265 | 0.797571 |
| 4 | 88.198 | 107.346 | 0.821625 |
| 5 | 86.695 | 109.410 | 0.792388 |

The means are 87.7262 and 109.4290 microseconds. The ratio of means is
0.801672, a 21.7028 microsecond saving per projection. Warmup preceded both
timed paths; every timed iteration synchronized the backend. This is a direct
microbenchmark under an otherwise loaded but idle production GPU, not an
end-to-end or isolated-power performance claim.

## Feature-off control

The same source was reconfigured with the private option off and rebuilt. The
OFF `libggml-hip.so` SHA-256 is
`2b21d28c24cf7c70173755fec026dd7ffc86743575acf3172abd88ff048c10ea`.
The private proc string is absent from that binary. No test target, helper
kernel, registry branch, RPC behavior, model behavior, persistence behavior,
or runtime default exists in the feature-off build.

## Decision

Even granting the 21.7028 microsecond mean saving to all three expert
projections, the optimistic saving is about 65.1 microseconds, or 0.109% of
the approximately 60 ms/token critical path observed in P08. Integration
would add RPC protocol, graph scheduling, dynamic ownership, and cross-rank
join risk that this direct canary deliberately excludes. That attainable
benefit is far below the multi-percent regressions already observed in P09 and
P10 and below a robust exact-model detection margin.

P13 therefore retains the private default-off canary as reusable evidence but
does not open RPC or MiniMax model integration. A future design must affect
multiple layers or scheduler-wide rank serialization before this lane can be
reconsidered. P13 makes no exact-model speedup, non-inferiority, or greater-than
30 tok/s claim.

No donor or GPL code is introduced. Production services were not interrupted
for P13 and remained the rollback control. Raw builds, excluded compile
diagnostics, five-run output, feature-off proof, binary hashes, and manifests
are retained on nimo-2 under `/var/tmp/halofpx-p13-private-evidence`. The bundle
`/var/tmp/halofpx-p13-private-evidence-nimo2-20260720-v2.tar.zst` has SHA-256
`9b2b52ef32342e0f8bbe70e67261db8ec46491f5764e9a5f473a294bc486003a`.
