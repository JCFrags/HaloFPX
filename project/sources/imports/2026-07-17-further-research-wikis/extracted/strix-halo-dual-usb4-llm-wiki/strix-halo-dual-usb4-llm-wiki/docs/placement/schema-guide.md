---
title: Placement schema guide
status: machine-readable contract
---

# Placement schema guide

The JSON Schema at [`schemas/placement.schema.json`](../../schemas/placement.schema.json) validates the placement YAML files under [`placements/`](../../placements/). The schema is intentionally descriptive rather than runtime-specific.

## Required top-level fields

| Field | Meaning |
|---|---|
| `schema_version` | Placement contract version. Current value: `1.0`. |
| `name` | Human-readable placement name. |
| `mode` | Enumerated execution family. |
| `viability` | `go_candidate`, `conditional`, `experimental`, or `no_go_without_new_evidence`. |
| `objective` | Capacity, latency, throughput, isolation, or other stated objective. |
| `topology` | Two nodes, two physical links, and the selected aggregation policy. |
| `coordinator_rank` | Rank 0, rank 1, or an external frontend. |
| `ranks` | Exactly two rank records with role and ownership. |
| `flows` | Every cross-rank or external flow, including formula and synchronization. |
| `hard_gates` | Conditions that must pass before GO. |

## Ownership record

Every rank declares strings for:

- `tokenizer`;
- `sampler`;
- `rng`;
- `model`;
- `experts`;
- `kv`;
- `sessions`.

The strings are deliberately explicit because a universal numeric layer/expert format would not capture runtime-specific shards. A deployment compiler can extend the schema with concrete tensor ranges, file names, or device maps.

## Flow record

A flow contains:

- `phase` — when it occurs;
- `from`, `to` — rank ID or external endpoint;
- `payload` — semantic content;
- `volume_formula` — symbolic bytes;
- `synchronization` — dependency/barrier semantics;
- `link_policy` — control, single bulk, validated striping, session-selected, or none.

A placement with a hidden flow is invalid analytically even if it passes JSON Schema. Examples of commonly omitted flows are full-logit gathers, token-ID feedback, proposal probability vectors, KV migration metadata, and failure heartbeats.

## Link policies

- `control` — low-volume ordered traffic; normally one reliable path.
- `bulk_single` — one selected path; no aggregation assumption.
- `bulk_striped_if_validated` — application-level striping is allowed only after the simultaneous-link gate.
- `session_selected` — independent sessions are assigned to paths/replicas; a single message is not striped.
- `none` — no cross-node model-path flow.

## Validation

Run:

```bash
python tools/validate_wiki.py
```

The validator checks:

- schema compliance;
- rank IDs 0 and 1 both present exactly once;
- required ownership terms are nonempty;
- flow endpoints and link policies;
- existence of every placement linked from the wiki.

## Extending a placement

For a concrete runtime, add an adjacent deployment file rather than editing away symbolic ownership. Useful extension fields include:

```yaml
runtime:
  engine: <name and commit>
  transport: <RPC/collective implementation>
  model_checkpoint: <immutable revision>
  quantization: <format and group size>
  layer_cut: 42
  interfaces:
    control: usb4net0
    bulk: [usb4net0, usb4net1]
  measured_profile: measurements/run-2026-...
```

Do not record credentials or private model access tokens in placement files.
