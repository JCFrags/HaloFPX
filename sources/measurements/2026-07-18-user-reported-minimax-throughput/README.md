# User-reported MiniMax throughput observation

Captured: 2026-07-18

**[MEASURED, USER-REPORTED]** The user reports that the two-node ROCmFP4 MiniMax
deployment began generation at approximately 28 tokens/s, while the currently
loaded two-node MiniMax M2.7 UD-Q6_K_XL deployment begins around 17–18 tokens/s.

Interpretation: ROCmFP4 is the current operational speed incumbent for future
matched performance work. This is not an apples-to-apples benchmark: exact
prompt, model revision/content, quantization quality, context/cache state,
sampling, thermal/power state, run duration and raw logs were not supplied.
Do not infer a quality-adjusted speedup or a universal ratio from this note.

Related live Q6 smoke evidence:
`experiments/2026-07-17-minimax-m27-q6-lan-deployment/README.md`.

