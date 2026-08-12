# Wiki operation log

## [2026-07-17] ingest | Distributed HaloKV design research

Created the initial source catalog and synthesized the two-node persistent-cache protocol, fault model, security model, formal-modeling plan, fuzzing plan, and degraded-mode behavior.

## [2026-07-17] lint | Initial package validation

Generated machine-readable schemas and tables; structural lint results are recorded by the build process and should be rerun after edits.

## [2026-07-17] validate | Schema and finite-model checks

Compiled `protocol/halokv.proto`, executed the Python reference traces/tests, parsed the TLA+ model with SANY, and exhaustively checked the included TLC configuration. TLC generated 2,800,241 states, found 242,384 distinct states, emptied the queue, and reported no invariant violation.

## [2026-07-17] repair | Read-observation and certificate model typing

During validation, reset stale read-observation state after corruption/topology mutation and replaced the mixed string/record null certificate representation with a typed certificate record. Both changes remove modeling artifacts without relaxing protocol safety.
