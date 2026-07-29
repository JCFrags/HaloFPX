# Independent adversarial review

Verdict: **CONFIRMED DISTINCT P1 SEMANTIC BLOCKER**

The reviewer independently verified that coordinator “prepare only” applies
live KV metadata while deferring only tensor bytes. A fresh-residency candidate
therefore changes the state and generation against which a request placement
preview was frozen.

The reviewer rejected all three shortcuts:

- commit the pre-restore preview: slots and graph-facing `n_kv` can be wrong;
- rebuild after candidate preparation: violates the authenticated single-handle
  key/execution binding;
- treat cleanup as rollback: current failure paths do not restore exact prior
  cells, heads, metadata, or partial distributed outcomes.

Minimum safe design confirmed by review: shadow candidate metadata/data,
preview against that shadow, authenticated remote stage, atomic validation and
installation of shadow state plus the previewed placement, coordinated commit
recovery, and execution with the same handle.

Classification: P1 correctness/state integrity if bypassed. This candidate-state
shadow/swap transaction is distinct from L106's authorized KV placement-preview
seam.

Review performed against source base
`a35816e52f4bb2510936fa1a29e623c3b9249521`; reviewer made no source edits.

