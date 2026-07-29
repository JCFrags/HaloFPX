# L100 independent offline review

Verdict: **PASS**

No P1 or P2 finding remains.

The reviewer independently verified:

- the q8_0 double block-scaling arithmetic in the device capture/prepare path;
- canonical K-then-V component mapping and fail-closed ordinal/kind checks;
- source-supported occupied-row reads in q8 K, V, and HIP tile paths;
- all 124 tensors, including exact coordinator-local blob framing;
- exact totals: 152,180,736 readable bytes, 4,755,648 serialized tensor
  payload bytes, and 147,425,088 unrepresented readable bytes;
- strided row data versus stride-padding classification;
- seven focused offline tests and byte-identical generated JSON, SHA-256
  `c354aecaa2511fa99e8ad335d8b6a0d23d3d6bb9dfa46690f2c9ea0f2b67a3d4`.

The causal claim is bounded to missing occupied ranges proven readable by
source. Padded rows remain conservative and noncausal.
