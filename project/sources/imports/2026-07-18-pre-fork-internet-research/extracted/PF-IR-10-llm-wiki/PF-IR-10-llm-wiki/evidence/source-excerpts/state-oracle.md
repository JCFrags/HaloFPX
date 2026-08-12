# State oracle evidence

- **Sources:** pinned `test-save-load-state.cpp` and `test-recurrent-state-rollback.cpp`.
- **Claim:** `INFERENCE`.
- **Finding:** upstream tests establish semantic restore by equal continuation tokens and logits (recurrent rollback uses absolute epsilon `1e-5`), rather than cross-version equality of opaque state-file bytes.
- **Disposition:** state bytes are candidate artifacts; continuation/logit equivalence is the cross-fork oracle.
