---
title: "Validation evidence"
tags: ["validation", "tlc", "schemas", "tests"]
created: 2026-07-17
updated: 2026-07-17
status: validated-research-artifact
sources: ["FORMAL-01", "FUZZ-02"]
related: ["Formal-Modeling", "Fuzzing-and-Fault-Injection", "Message-Schemas"]
---

# Validation evidence

Validation establishes that this research package is internally consistent and that its finite protocol abstraction satisfies the configured safety invariants. It is not a proof that an implementation, filesystem, object store, GPU runtime, or deployment is correct.

## Build recorded on 2026-07-17

| Check | Tool/configuration | Result |
|---|---|---|
| Wiki structure and links | `scripts/lint-wiki.py` | Pass |
| JSON syntax and Draft 2020-12 examples | Python `json` + `jsonschema` | Pass |
| CSV shape and required columns | Python `csv` | Pass |
| Protocol Buffer schema | `libprotoc 35.0`, descriptor-set generation | Pass |
| TLA+ syntax and semantics | SANY2 from TLC build `2026.03.02.213938`, revision `54e73ad` | Pass |
| TLA+/TLC safety model | `Ranks={r0,r1}`, `MaxEpoch=2`, 8 workers | Pass: no invariant violation |
| Python reference model | `unittest`, four regression tests | Pass |
| Stateful trace corpus | four JSON traces | Pass |
| Fault-campaign YAML | safe YAML parse and structural checks | Pass |

## TLC result

The included model and `HaloKV.cfg` completed exhaustive explicit-state exploration for the configured finite state space:

```text
2,800,241 states generated
242,384 distinct states found
0 states left on queue
maximum graph depth 34
no error found
```

The checked invariants are `TypeOK`, `CommitHasAllRanks`, `CommitNotCancelled`, `PreparedRanksAreCurrent`, `EpochFence`, and `AcceptedReadIsValid`. The complete console record and tool/JAR digest are in `validation/tlc-output.txt` and `validation/build-report.json`.

## Scope boundary

The finite model abstracts cryptography, byte ranges, storage APIs, quotas, and GPU tensors into validated actions. The result therefore supports the protocol control-state design only. The implementation release gate must still execute the larger modeling matrix in [[Formal-Modeling]], compile and test the P model after adaptation, run storage-specific crash consistency tests, and complete the fuzzing campaigns in [[Fuzzing-and-Fault-Injection]].

## Known unexecuted item

`formal/p/HaloKV.p` is an implementation-near research sketch and was not compiled in this build. Its README states the required adaptation and extension work; no P verification claim is made.
