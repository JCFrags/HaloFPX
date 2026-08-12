# TLA+/TLC research model

`HaloKV.tla` is a deliberately small safety model for the two-rank checkpoint protocol. It models delayed prepare messages across epoch bumps, durable and prepared rank sets, commit/cancel terminal states, rank crashes, topology incompatibility, corruption, repair, and read rejection.

Run with a local TLA+ tools distribution:

```bash
java -cp tla2tools.jar tlc2.TLC -config HaloKV.cfg HaloKV.tla
```

## Recorded finite-model run

On 2026-07-17, SANY and TLC build `2026.03.02.213938` (revision `54e73ad`) parsed the model and exhaustively checked the included `HaloKV.cfg` configuration:

```text
2,800,241 states generated
242,384 distinct states found
0 states left on queue
maximum depth 34
no error found
```

See `../../validation/tlc-output.txt` and `../../wiki/Validation-Evidence.md`. This is evidence for the configured finite abstraction, not a machine-checked proof of a production implementation. Extend the model according to `wiki/Formal-Modeling.md` before protocol release.
