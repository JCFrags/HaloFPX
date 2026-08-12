# Dual-Node Strix Halo Validation Wiki

> **Wiki status:** Proposed · **Evidence state:** D0 — design only · **Last reviewed:** 2026-07-17  
> **Machine-validation status:** Not run. Missing measurements are `INSUFFICIENT_EVIDENCE`, never an implicit pass.


This repository-shaped bundle is the validation program for moving a two-node, Strix Halo-class LLM system from integration to stable operation. The default adapter assumes a coordinator plus a remote accelerator exposed through `llama.cpp` RPC over a dedicated USB4/Thunderbolt host-to-host network. The measurement model, data schemas, and release logic are engine-neutral unless a page explicitly says otherwise.

## Start here

1. Open [Home.md](Home.md) and [wiki/Program-Charter.md](wiki/Program-Charter.md).
2. Instantiate [config/sut.example.yaml](config/sut.example.yaml) as `config/sut.yaml`; replace every `null` release SLO.
3. Execute matched single-node trials before dual-node trials.
4. Store immutable raw evidence using the schemas under [schemas/](schemas/).
5. Evaluate the result with `python tools/evaluate_gates.py`.

## Evidence boundary

This bundle contains research, protocols, schemas, automation scaffolding, and **synthetic non-machine examples**. Its local checks prove only that the bundle is structurally coherent. They do not prove that a node, link, model, driver, or runtime works. See [EVIDENCE-STATUS.md](EVIDENCE-STATUS.md).

## Repository map

| Path | Purpose |
|---|---|
| `Home.md`, `_Sidebar.md`, `_Footer.md` | Standard GitHub Wiki entry files |
| `wiki/` | Program, methodology, gates, provenance, roadmap, and operating pages |
| `experiments/` | Executable experiment cards with acceptance criteria |
| `schemas/` | Raw and derived evidence contracts |
| `config/` | SUT, matrix, gate, threshold, fault, and upstream-watch configuration |
| `tools/` | Collectors, aggregators, gate evaluator, and upstream watcher |
| `templates/` | Run, release, waiver, incident, and watch-triage forms |
| `examples/synthetic-non-machine/` | Parser fixtures only; never release evidence |
| `references/` | Research snapshot, source registry, and claim map |

## Local verification

```bash
python -m pip install -r tools/requirements.txt
make check
make synthetic
```

`make synthetic` is expected to return `INSUFFICIENT_EVIDENCE` because the input is synthetic.
