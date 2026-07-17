# HaloKV fuzzing and fault-injection package

Read `wiki/Fuzzing-and-Fault-Injection.md` first. This directory contains campaign definitions, valid seeds, stateful traces, a small executable reference oracle, and a protocol dictionary.

```bash
python3 fuzz/reference_model.py fuzz/traces/normal-commit.json
python3 -m unittest fuzz/test_reference_model.py
```

The reference model is intentionally abstract. It is useful as a stateful-fuzzer oracle and trace minimization target, not as production code.
