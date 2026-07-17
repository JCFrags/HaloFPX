# Runbook

## 1. Validate this suite

```bash
python3 scripts/verify-suite.py

python3 -m venv .venv
. .venv/bin/activate
pip install -e ./harness pytest jsonschema
pytest ./harness/tests
```

## 2. Pin sources

Copy `references/source-inventory.yaml` into a run directory and replace research snapshot pins with the exact commits under test. Confirm the CachyLLama identity and add the integration repository. Record dirty state, submodules, merge bases, and binary hashes.

## 3. Materialize model fixtures

Download or generate only approved models, then create a local lock:

```bash
python3 scripts/build-fixture-manifest.py   --model model.tiny-stories15m-q4_0=/models/stories15M-q4_0.gguf   --output reports/locks/model-lock.json
```

Any required external model with a null manifest digest must be approved and locked before its cases can run.

## 4. Build each fork

Use equivalent build intent, but do not force flags a fork does not support. Preserve:

```text
cmake -S <source> -B <build> ...
cmake --build <build> --parallel
ctest --test-dir <build> --show-only=json-v1
```

Capture compiler, CMake cache, feature flags, backend libraries, and installed device/runtime data.

## 5. Probe capabilities

Record whether the binary supports each backend, server endpoint, GGUF type, state API, cache feature, speculative method, RPC transport, and cancellation hook. Conditional cases require this evidence.

## 6. Run native tests

Start with CTest and the fork's server pytest suite. Store native results separately from differential results. Do not hide tests that were removed or disabled by a fork.

## 7. Select matrix cases

```bash
python3 scripts/select-tests.py --area "GGUF parsing"
python3 scripts/select-tests.py --fork rocmfpx --backend rocm
python3 scripts/select-tests.py --fork cachyllama --include-failure-injection
```

The harness package also provides:

```bash
llama-conformance select --area "Cache rejection" --fork integration
```

## 8. Capture observations

Each adapter writes one JSON observation per case following `matrix/schemas/observation.schema.json`, plus raw logs and payloads. Validate:

```bash
llama-conformance validate-observation reports/raw/RUN/FORK/CASE/observation.json
```

## 9. Compare exact outputs

```bash
llama-conformance compare --kind tokens   --reference reports/raw/RUN/upstream/DET-006/observation.json   --reference-field result.outputs.tokens   --candidate reports/raw/RUN/integration/DET-006/observation.json   --candidate-field result.outputs.tokens
```

Use `text` or `json` only when the case declares that oracle and normalization.

## 10. Numeric evidence

Numeric cases remain `UNCALIBRATED` until references are approved. To inspect calibration evidence:

```bash
python3 scripts/calibrate-tolerances.py   --reference reports/calibration/reference-vector.json   --candidate reports/calibration/repeat-1.json   --candidate reports/calibration/repeat-2.json   --field result.outputs.logits.values   --profile-id example   --case-id LOGIT-002   --output reports/calibration/proposed.json
```

The output leaves normative metrics null. After protected approval, compare using an approved profile:

```bash
llama-conformance compare --kind numeric   --reference ... --reference-field result.outputs.logits.values   --candidate ... --candidate-field result.outputs.logits.values   --profile references/tolerances/APPROVED-PROFILE.json
```

## 11. Run failure lanes

Use isolated paths, watchdogs, sanitizer builds, and loopback RPC. Execute one mutation/fault at a time, retain digests, and run a post-failure correctness probe.

## 12. Aggregate

```bash
python3 scripts/generate-report.py   reports/raw/RUN/*/*/observation.json   --output reports/summary/RUN.json
```

Review errors and skips separately from semantic failures. A required case with no valid observation blocks release.
