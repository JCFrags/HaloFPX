# Contributing

Changes should preserve protocol invariants and make assumptions explicit. A substantive change should update the relevant wiki pages, schemas or state machines, the decision log, and the append-only `log.md`.

Before submitting a change:

```bash
python3 scripts/lint-wiki.py
python3 scripts/deep-validate.py
```

Set `TLA2TOOLS_JAR=/path/to/tla2tools.jar` when running the deep validator to execute SANY and TLC; otherwise those checks are explicitly skipped.

For protocol changes, also add at least one positive trace, one rejected trace, and one fault or fuzz campaign. Formal-model changes should identify the invariant or liveness property affected and provide the smallest reproducing counterexample when a property fails.
