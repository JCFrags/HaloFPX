# P06b MiniMax-M2 Q8 exact-shape kernel canary independent review

Status: **ACCEPT; no P1/P2 correction.**

The review confirmed that the gated roster is absent when
`HALOFPX_MINIMAX_M2_EXPERT_PARTITION_CANARY` remains at its default `OFF` and
does not alter or link into the ordinary `llama` or `llama-server` runtime.
The four admitted cases bind the pinned MiniMax-M2 geometry: 192 experts,
top-8 selection, 3072-to-1536 and 1536-to-3072 Q8_0_ROCMFPX projections, and
batch widths one and eight.

The local source hashes match both the receipt and retained nimo-1 sources.
All 13 selected evidence-manifest entries verify. The manifest, 89,012-byte
bundle, test binary, correctness CSV, three raw timing runs, reported means,
and sample standard deviations reconcile with the milestone report. The
correctness evidence records all four ROCm-versus-CPU cases supported without
error.

Claims are appropriately limited to inherited gfx1151 exact-shape kernel
support and microkernel timing. Expert partition equivalence, runtime expert
placement, local/remote overlap, end-to-end performance, and persistent writes
remain closed. The known-good worker and server are active with zero restarts,
and nimo-1 independently returned `200 {"status":"ok"}` on port 8081.

No donor, GPL llama-ai, or CachyLLama code, new dependency, deployment
replacement, model mutation, remote, WebUI, persistent write, or
reference-clone mutation entered this milestone.
