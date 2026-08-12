# Harness

Install locally:

```bash
python -m venv .venv
. .venv/bin/activate
pip install -e ./harness
pytest ./harness/tests
```

Examples:

```bash
llama-conformance select --area "GGUF parsing" --fork upstream
llama-conformance compare --kind tokens   --reference reports/upstream.json --reference-field result.outputs.tokens   --candidate reports/integration.json --candidate-field result.outputs.tokens
```

Numeric and distributional comparison commands reject `UNCALIBRATED` and `PROPOSED` profiles. This is intentional. Exact output, schema, error-class, language-membership, and invariant checks can run before numeric reference promotion.
