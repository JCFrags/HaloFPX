# Reproducibility and non-fabrication audit

## Deliberately absent

- No claimed USB4 payload throughput.
- No claimed USB4 latency.
- No claimed Strix Halo tokens/s, time-to-first-token, inter-token latency, or speedup.
- No claim that two USB4 links automatically bond.
- No claim that a model fits based only on nominal parameter count.
- No claim that a given inference runtime implements every placement described.

## Included numerical values

Numerical values fall into one of three auditable classes:

1. **Sourced platform/model fields**, linked to official product pages, official model repositories, or primary papers.
2. **Calculated byte volumes**, generated from equations in `tools/cost_model.py` and architecture fields in `data/model_configs.csv`.
3. **Nominal-line-rate payload floors**, calculated at 40 Gb/s per USB4 link and explicitly excluding encoding/protocol/software overhead and latency. These are lower bounds on transfer time, not expected measurements.

The illustrative MoE remote fraction `rho = 0.5` is a scenario assumption used only to show sensitivity. It is not a claim about routing behavior.

## Validation trail

- `python tools/cost_model.py --demo` regenerates worked-example JSON.
- `python tools/validate_wiki.py` checks required files, internal links, YAML placements, evidence labels, and structured data.
- `python -m pytest -q` checks key arithmetic identities and break-even edge cases.
