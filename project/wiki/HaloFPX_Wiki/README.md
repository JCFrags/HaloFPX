# HaloFPX Wiki

Use the root [`WORKER_START_HERE.md`](../../WORKER_START_HERE.md) before you change Wiki material.
Keep completed research in the exact numbered category and section paths.

## Wiki navigation

- [Current project state](../../CURRENT_STATE.md)
- [Architecture overview](architecture-overview.md)
- [Evidence map](evidence-map.md)
- [Decision map](decision-map.md)
- [Glossary](glossary.md)
- [Archive index](archive-index.md)

## Categories

1. [Wiki Governance](01_Wiki_Governance/README.md)
2. [Project Definition](02_Project_Definition/README.md)
3. [Repository and Engineering](03_Repository_and_Engineering/README.md)
4. [Hardware and Operating System Platform](04_Hardware_and_OS_Platform/README.md)
5. [Performance Software and Tools](05_Performance_Software_and_Tools/README.md)
6. [Models, Quantization, and Inference](06_Models_Quantization_and_Inference/README.md)
7. [Distributed Runtime](07_Distributed_Runtime/README.md)
8. [Fabric and Transport](08_Fabric_and_Transport/README.md)
9. [HaloKV Persistent Cache](09_HaloKV_Persistent_Cache/README.md)
10. [Product, Server, and Operations](10_Product_Server_and_Operations/README.md)
11. [Verification and Performance](11_Verification_and_Performance/README.md)
12. [Project Execution and Governance](12_Project_Execution_and_Governance/README.md)

## Authority model

Pages label verified facts, measurements, inferences, assumptions, recommendations, and open questions.
Applicability is versioned.
[`manifest.yaml`](manifest.yaml) is authoritative only for canonical section paths.
The root manifest also records structural artifact state.
[`manifest.schema.json`](manifest.schema.json) validates the root manifest.
Each `section.yaml` is authoritative for its declared status and applicability.
Structural completeness does not approve a software baseline or claim.
An applicability entry does not approve a software baseline or claim.
A newer page is not automatically authoritative.

The generator uses `research/prompts/section_index.yaml` and the present section manifests.
It generates the manifest deterministically:

```powershell
$env:PYTHONUTF8 = '1'
$env:PYTHONIOENCODING = 'utf-8'
python3.12 -X utf8 -m venv .venv
./.venv/bin/python -m pip install --requirement requirements/requirements-halofpx-validation.txt
./.venv/bin/python -X utf8 project/research/prompts/tools/generate_wiki_manifest.py project/wiki/HaloFPX_Wiki
./.venv/bin/python -X utf8 project/research/prompts/tools/generate_wiki_manifest.py project/wiki/HaloFPX_Wiki --check
```

The UTF-8 environment is part of the Windows control-PC contract. It prevents
legacy CP1252 standard streams from failing on Unicode Wiki diagnostics.

Validate both required artifacts and the permissive-core `section.yaml` contract with:

```powershell
./.venv/bin/python -X utf8 -B project/research/prompts/tools/validate_wiki.py project/wiki/HaloFPX_Wiki
./.venv/bin/python -X utf8 -B -m unittest discover -s project/research/prompts/tools -p "test_*.py"
./.venv/bin/python -X utf8 -B -m unittest tests/test_halofpx_strix_ab.py tests/test_halofpx_strix_ab_cachyos.py -v
./.venv/bin/python -X utf8 -B tests/test_materialize_rocmfpx_fixture.py -v
```

On Windows, substitute `py -3.12` for environment creation and
`.\.venv\Scripts\python.exe -X utf8` for `./.venv/bin/python -X utf8`.

The discovery pattern covers both Wiki validator and manifest-generator tests.
The model-general Strix A/B, CachyOS adapter, and fixture-materialization suites
are offline contract checks: they do not contact either Strix Halo target,
download a model, or establish recovery or performance evidence. The fixture
suite currently contains 12 tests.

The validator enforces registry identity, category, and allowed status values.
The validator requires real International Organization for Standardization (ISO) dates.
The validator requires non-negative source and open-question counts.
The counts must match the content.
The validator checks experiment and related-section shapes.
The validator requires a non-empty applicability mapping.
Section-specific extension keys remain allowed.
Richer applicability values remain allowed.

## Required ledgers

- glossary and naming;
- sources;
- assumptions;
- open questions;
- decisions and architecture decision records (ADRs);
- experiments and measured results;
- compatibility matrices;
- risks;
- implementation status.

Use [`evidence-map.md`](evidence-map.md) for evidence routing.
Use [`decision-map.md`](decision-map.md) for accepted decisions and milestone history.
