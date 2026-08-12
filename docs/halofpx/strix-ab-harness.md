# Model-general dual-Strix A/B harness

Status: **implementation and offline qualification only; no target measurement**

This bounded harness supports GitHub issues #15 and #16. It replaces the
MiniMax-specific assumptions in `run-halofpx-primary-block.sh` and
`analyze-halofpx-primary-blocks.py` with a strict manifest, deterministic
paired order, exact artifact preflight, raw-sample retention, and paired
analysis. It does not change or control the always-on services.

The harness is feature-agnostic. An OFF/ON pair may compare different commits
or different builds of one commit. Runtime arguments must match across the two
conditions, and the typed runtime contract must agree with the exact common
argv. Version 1 admits only cold-cache-off prompt/generation work; persistent
cache reuse keeps its separate lifecycle qualification. MiniMax remains an
optional capacity fixture; no model name, architecture, prompt length, or
golden output is embedded in the implementation.

## Scope and safety boundary

- Run the tool on the target CachyOS nodes. Windows may validate plans and
  fixtures but cannot create a Strix Halo performance result.
- Use fresh disposable worker and coordinator processes for every condition.
  A machine-specific adapter owns start, readiness, telemetry, stop, cleanup,
  and rollback. The checked-in tool does not claim that frozen argv was run.
- Never point an experiment adapter at the names, ports, roots, or process IDs
  of the always-on services.
- Do not store secrets in a plan or raw bundle. The harness records only the
  explicit environment allowlist; it never dumps the ambient environment.
- A changed binary, model, request, node authority receipt, command, topology,
  schedule, token count, output, raw-file hash, failure, or missing sample
  prevents a complete result.
- Three pairs are a preliminary direction screen. Five pairs meet only the
  project's minimum count before a `[MEASURED]` review; the analyzer never
  emits a performance claim by itself.

## Small workflow

Start from
[`halofpx-strix-ab-plan.example.json`](../../scripts/halofpx-strix-ab-plan.example.json).
Replace every placeholder with an exact identity. Use one ordinary ROCmFPX
model for daily work and keep `cache_class=cold_cache_off` for issues #15/#16.

```bash
python scripts/halofpx_strix_ab.py validate plan.json
python scripts/halofpx_strix_ab.py init plan.json /var/tmp/halofpx-ab-my-run
```

Copy the frozen plan to each target node. On nimo-1 and nimo-2, run the role
that the plan assigns. Preflight hashes both OFF and ON binaries. Coordinator
preflight also hashes the exact model and request. Each role hashes its current
machine-authority receipt, which should contain the captured CachyOS, kernel,
ROCm, Mesa, firmware, GPU, power, and topology tuple.

```bash
python scripts/halofpx_strix_ab.py preflight plan.json --role coordinator --output coordinator.json
python scripts/halofpx_strix_ab.py preflight plan.json --role worker --output worker.json
python scripts/halofpx_strix_ab.py import-preflight /var/tmp/halofpx-ab-my-run coordinator.json
python scripts/halofpx_strix_ab.py import-preflight /var/tmp/halofpx-ab-my-run worker.json
```

Execute `schedule.json` in order. For each entry, the adapter must start fresh
disposable processes using exactly the condition arrays in `commands.json`,
exclude the declared warmup, retain every attempted request, collect both
nodes' telemetry/journals/link counters, and stop both processes. Keep profiling
runs outside this schedule.

For a successful streamed `/completion` request, retain an assembled raw server
response with its final timing object and a monotonic client-event record. This
small record illustrates a three-token fixture, so it contains two ITLs:

```json
{
  "schema": "halofpx.client-timing.v1",
  "started_at": "2026-08-12T20:00:00Z",
  "ended_at": "2026-08-12T20:00:10Z",
  "http_status": 200,
  "wall_ms": 10000.0,
  "ttft_ms": 312.5,
  "itl_ms": [21.4, 20.9]
}
```

The harness requires one positive interval for every generated token after the
first, valid zoned timestamps, and a TTFT-plus-ITL span consistent with
monotonic wall time.

Import the response, client record, and any raw logs. The `pair`, `condition`,
and `order-index` values must match the frozen schedule. Record failed requests
with `--status failure --failure-code NAME`; failures remain evidence and make
the performance result incomplete.

```bash
python scripts/halofpx_strix_ab.py record /var/tmp/halofpx-ab-my-run \
  --pair 1 --condition off --order-index 0 --status success \
  --response response.json --client client.json \
  --extra coordinator.log --extra worker.log --extra telemetry.jsonl

python scripts/halofpx_strix_ab.py analyze /var/tmp/halofpx-ab-my-run
```

`analysis.json` uses the pair as the comparison unit. It reports OFF/ON means,
paired deltas, and paired improvement percentages separately for prompt rate,
generation rate, client wall time, TTFT, and mean inter-token latency.
It does not use requests within a long-lived block as independent replicates.
`samples.jsonl` and `SHA256SUMS` make the exact raw bundle portable to a new PC.

## Follow-up execution adapter

The next small slice is a CachyOS adapter that starts isolated user units on
the two nodes, proves their InvocationIDs and executable hashes, captures
streaming monotonic TTFT/inter-token events plus both-node telemetry, and calls
`record`. Keep that adapter separate from this evidence core so process control
can be reviewed against the current target service authority without weakening
the model-general plan or paired analyzer.
