# Model-general dual-Strix A/B harness

Status: **implementation and offline qualification only; no target measurement**

This bounded harness supports the prompt/generation measurement lanes from
GitHub issues #15 and #16. The isolated CachyOS process adapter is tracked by
GitHub issue #37. The harness replaces the
MiniMax-specific assumptions in `run-halofpx-primary-block.sh` and
`analyze-halofpx-primary-blocks.py` with a strict manifest, deterministic
paired order, exact artifact preflight, raw-sample retention, and paired
analysis. It does not change or control the always-on services.

The harness has two versioned comparison contracts. Plan v1 retains its exact
feature-build behavior and serialization. Plan v2 declares either
`feature_build` or `runtime_n_batch`; it does not reinterpret or migrate an
initialized v1 run. Both versions admit only cold-cache-off prompt/generation
work. Persistent cache reuse keeps its separate lifecycle qualification.
MiniMax remains an optional capacity fixture; no model name, architecture,
prompt length, or golden output is embedded in the implementation.

The plan-v2 `runtime_n_batch` comparison is deliberately closed: OFF is
`n_batch/n_ubatch=512/512`, ON is `2048/512`, and both conditions use the same
source commit plus identical coordinator and worker binary paths and SHA-256
identities. Condition-specific arguments remain empty. The common coordinator
argv must omit every batch alias, including llama.cpp's underscore-normalized
long spellings; the harness emits the only `--batch-size` pair from the typed
condition map. Worker argv is identical and coordinator
argv differs only in that generated integer. `LLAMA_ARG_BATCH` and
`LLAMA_ARG_UBATCH` are refused. This makes outer batch size the only declared
independent variable.

## Scope and safety boundary

- Any future execution adapter must apply the P0 target-ownership predicate in
  [issue #41](https://github.com/JCFrags/HaloFPX/issues/41) before adapter
  launch. It must refuse work when a protected production service or
  an unaccounted KFD/render/HMM owner is active. `MemAvailable`, free RAM,
  swap, and conventional RSS cannot override this rule; see the
  [2026-08-12 HMM/global-OOM incident](evidence/2026-08-12-target-hmm-oom-incident/README.md).
- Bind admission to an authorized maintenance window, exact before-state
  service identities, a clean kernel-OOM baseline, and an empty foreign
  GPU-owner census on both hosts. Preserve those observations in the run
  evidence before starting either condition.
- Treat a worker PID, InvocationID, or restart-count change as invalidation of
  coordinator RPC readiness. A health route is not sufficient recovery proof;
  require both-rank identity reconciliation and a real minimal inference.
- Do not run this draft on the target CachyOS nodes. Windows or another control
  host may validate plans and offline fixtures, but cannot create a Strix Halo
  performance result. A separately reviewed change must enable target
  execution after every issue-#41 custody gate closes.
- The [offline maintenance admission controller](strix-maintenance-admission-controller.md)
  exercises shutdown/cleanup/recovery ordering only through deterministic
  fakes. It is not the missing target executor. Owner-signed authorization,
  atomic two-node receipt consumption, and an independent recovery watchdog
  remain mandatory future promotion gates.
- Use fresh disposable worker and coordinator processes for every condition.
  A machine-specific adapter owns start, readiness, telemetry, stop, cleanup,
  and rollback. The checked-in tool does not claim that frozen argv was run.
- Never point an experiment adapter at the names, ports, roots, or process IDs
  of the always-on services.
- Do not store secrets in a plan or raw bundle. The harness records only the
  explicit environment allowlist; it never dumps the ambient environment.
- A mismatch among the plan, preflight, raw bundle, binary/model/request hashes,
  command, topology, schedule, token counts, or output prevents evidence-core
  completeness.
- Evidence-core completeness is not execution qualification. Until the target
  adapter proves live InvocationIDs, executable hashes, argv/environment,
  order, warmups, cache-off state, and cleanup, `analysis.json` keeps both
  `execution_qualified` and `measurement_ready` false.
- Preflight v2 carries byte-for-byte Base64 copies of the exact request and
  both sanitized machine-authority receipts. Import materializes them at
  `inputs/request.raw`, `inputs/authority-coordinator.raw`, and
  `inputs/authority-worker.raw`; validation rehashes those copies even if the
  original machine paths no longer exist. Do not place secrets in an authority
  receipt.
- The exact hashed request JSON must contain only the seven documented
  deterministic fields, set `"stream": true` and `"cache_prompt": false`, use
  `"ignore_eos": true` for a fixed output length, use a fixed non-sentinel seed
  and zero temperature, and bind `n_predict` to the
  plan. Duplicate or extra keys are refused. Every retained
  request must run in a newly isolated process after warmup; do not warm the
  process that supplies a measured sample. The final server timing object must
  contain integer `cache_n: 0`. Missing, Boolean, floating-point, or nonzero
  cache counts are rejected.
- Three pairs are a preliminary direction screen. Five pairs meet only the
  project's minimum count before a `[MEASURED]` review; the analyzer never
  emits a performance claim by itself.

## Small workflow

Start from the historical plan-v1
[`halofpx-strix-ab-plan.example.json`](../../scripts/halofpx-strix-ab-plan.example.json)
for a feature-build comparison. Start from
[`halofpx-strix-ab-runtime-n-batch-plan.example.json`](../../scripts/halofpx-strix-ab-runtime-n-batch-plan.example.json)
for the exact plan-v2 512/512 versus 2048/512 screen. Replace every placeholder
with an exact identity. Use one ordinary ROCmFPX model for daily work and keep
`cache_class=cold_cache_off` for issues #15/#16.
The request file identified by `request.path` and `request.sha256` should have
this shape (replace the prompt and keep its exact token count in the plan):

```json
{
  "prompt": "replace-with-the-exact-frozen-prompt",
  "n_predict": 128,
  "stream": true,
  "cache_prompt": false,
  "ignore_eos": true,
  "seed": 1234,
  "temperature": 0
}
```

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

Execute `schedule.json` in order. For each entry, the adapter must run the
declared warmup outside the measured process, then start fresh disposable
worker and coordinator processes using exactly the condition arrays in
`commands.json`. Retain every attempted measured request, collect both nodes'
telemetry/journals/link counters, and stop both processes. Keep profiling runs
outside this schedule.

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
For plan v2 it also binds the comparison kind, control/candidate labels,
condition batch map, ubatch, and both complete condition-command hashes.
It does not use requests within a long-lived block as independent replicates.
`samples.jsonl` and `SHA256SUMS` make the exact raw bundle portable to a new PC.
The core may mark only `evidence_core_complete`; it never marks a measurement
ready without the separate target execution adapter.

## Optional sampling-output synchronization observability

[`halofpx_strix_ab_sampling_sync.py`](../../scripts/halofpx_strix_ab_sampling_sync.py)
adds an offline-only, independently versioned evidence sidecar for issue #28.
Its lane name is `sampling_output_sync_prometheus_v1`. The sidecar is absent by
default, and the checked-in
[`example`](../../scripts/halofpx-strix-ab-sampling-output-sync-prometheus.example.json)
sets `enabled=false`. Absence or explicit disablement leaves plan v1, its
canonical digest, schedule, commands, sample schema, and analysis schema
unchanged. It does not allocate another Strix plan version.

Enabling the sidecar requires an issue-#28 feature-build comparison and the
following exact controls in both generated coordinator commands:

- `--metrics`;
- `--parallel 1`;
- `--no-cont-batching`; and
- `--no-warmup`.

OFF and ON must bind the same exact source commit. The core feature-build
contract still requires distinct condition binary identity, so the frozen
comparison cannot substitute unrelated source revisions for the canary toggle.

Aliases, equals forms, duplicates, contradictory flags, and environment
overrides of those controls are refused. The harness-declared warmup still
uses a separate disposable process; `--no-warmup` prevents hidden server
warmup in the fresh process that supplies the retained sample. A runtime-batch
comparison is outside this lane because it would add another independent
variable.

The example's `core_plan_sha256` binds the unchanged checked-in v1 example.
For a real frozen plan, replace it with the exact digest printed by the core
`validate` command, set `enabled=true`, and freeze the sidecar before any raw
sample exists:

```bash
python scripts/halofpx_strix_ab.py validate plan.json
python scripts/halofpx_strix_ab.py init plan.json /var/tmp/halofpx-ab-my-run
python scripts/halofpx_strix_ab_sampling_sync.py validate-plan \
  plan.json sampling-output-sync-plan.json
python scripts/halofpx_strix_ab_sampling_sync.py freeze \
  /var/tmp/halofpx-ab-my-run sampling-output-sync-plan.json
```

For each fresh measured process, a separately reviewed capture layer must
perform exactly this sequence with no other traffic: raw `GET /metrics`, one
already-frozen completion request, raw `GET /metrics`. The capture receipt
binds the planned coordinator host, one canonical listener port, metrics and
completion paths, the two raw hashes, strict monotonic order, request,
response, and client hashes, request count `1`, and the same PID, 32-hex
systemd InvocationID, `/proc` process-start ticks, and metrics process-start
header across all three steps. The endpoint header is corroboration; PID,
InvocationID, and process-start ticks are the authoritative process identity.
This slice provides no target capture implementation and does not enable the
blocked CachyOS adapter.

After recording the ordinary successful sample, import its two raw Prometheus
documents and closed capture receipt:

```bash
python scripts/halofpx_strix_ab_sampling_sync.py record \
  /var/tmp/halofpx-ab-my-run --pair 1 --condition off --order-index 0 \
  --before metrics-before.prom --after metrics-after.prom \
  --capture capture.json

python scripts/halofpx_strix_ab.py analyze /var/tmp/halofpx-ab-my-run
```

The parser requires exactly one `counter` TYPE and one unlabeled canonical
unsigned-64-bit decimal sample for each of the five issue-#28 metrics. Missing,
duplicate, labeled, timestamped, malformed, overflowing, or decreasing values
fail closed. Every scheduled sample has exactly one sidecar directory containing
only `before.prom`, `after.prom`, `capture.json`, and `summary.json` as regular,
non-symlink files; orphan or unreferenced artifacts fail closed. Raw values are
parsed as integers without floating point; derived JSON stores decimal strings
so values above `2^53` remain portable.

A run without a frozen sidecar plan must contain neither those evidence
directories nor the reserved `sampling-output-sync-analysis.json` output. The
reserved plan and analysis paths must be regular, non-symlink files whenever
present. With a valid frozen plan, analysis deterministically rewrites a
preexisting regular derived report after reparsing every retained raw artifact.

A complete PR #51 adapter evidence verifier **MUST** detect the versioned
`sampling_output_sync_prometheus_v1` profile whenever any reserved sidecar root
file or evidence directory is present. Support for that profile **MUST** bind
and validate both `sampling-output-sync-plan.json` and
`sampling-output-sync-analysis.json`, require exactly one
`sampling-output-sync/` directory for every frozen scheduled sample, require
each such directory to contain only the four regular non-symlink files
`before.prom`, `after.prom`, `capture.json`, and `summary.json`, and invoke the
authoritative `validate_frozen_run` reparse before accepting the bundle. A
verifier that does not implement this versioned profile **MUST** refuse any
bundle containing a reserved sidecar path. Neither core
`evidence_core_complete` nor sidecar `evidence_complete` alone establishes
execution qualification or evidence acceptance. This sidecar slice does not
implement the PR #51 adapter integration.

Each retained window requires positive output-epoch, completed-barrier,
graph-submission, and output-transfer work. OFF requires zero reused barriers;
ON requires positive reuse. Within a pair, ON must complete fewer barriers,
and completed plus reused synchronization decisions must equal the matched OFF
total. OFF and ON must also have equal output-epoch, graph-submission, and
output-transfer deltas. The ordinary A/B core separately enforces the planned
emitted-token count and exact output parity. Output epochs are reserve/reset
lifecycles, not generated-token counts, and one request may submit more than one
graph, so the lane deliberately does not equate epochs, graphs, transfers, and
tokens within a sample.

The derived field is named `single_process_window_delta`. These cumulative
context counters are not completion, SSE, or general request counters. The
window is admissible only because a fresh `--parallel 1 --no-cont-batching
--no-warmup` process sees exactly one completion between snapshots. It must
never be used to attribute work in a long-lived or generally batched server.
Offline fake-adapter tests establish parser and contract behavior only; they
provide no CachyOS, ROCm, `gfx1151`, dual-node, or performance evidence.

## CachyOS isolated-process adapter — issue #37

[`halofpx_strix_ab_cachyos.py`](../../scripts/halofpx_strix_ab_cachyos.py)
consumes the frozen plan, commands, schedule, imported preflights, and exact
retained input bytes. Its closed policy example is
[`halofpx-strix-ab-cachyos-policy.example.json`](../../scripts/halofpx-strix-ab-cachyos-policy.example.json).
The adapter is a candidate execution layer with offline qualification only; it
has not run on the targets and creates no performance claim. Its real SSH
`execute-next` path is hard-disabled. Only the local `validate` command is
available in this draft.

Local validation binds the draft to the immutable issue-#41 incident manifest
at
`docs/halofpx/evidence/2026-08-12-target-hmm-oom-incident/manifest.json`,
SHA-256
`331634016681b57183aedbea3550f95d86486ce21d1baf8e7e3e3e5c6f35d815`.
The incident bundle's own `validate.ps1` and optional Windows read-only
`collect-read-only.ps1` remain the canonical evidence tools. The adapter does
not copy, call, or replace that collector. Binding the historical incident is
not live admission.

The following custody remains unresolved and therefore keeps target execution
blocked:

- an authorized maintenance-window receipt;
- exact before-state service identities and a clean kernel-OOM baseline;
- a complete empty foreign KFD, render-node, and HMM-owner census;
- independently reviewed disposable-process and cleanup custody; and
- a two-rank recovery contract that requires exact identities plus a real
  minimal inference after either identity changes.

The policy permanently protects the current system-unit names and ports:

- `minimax-m27-q6-server.service` and port 8081 on nimo-1;
- `minimax-m27-rpc-worker.service` and port 50052 on nimo-2.

The example uses disposable ports 18080 and 50252. The older A/B example's use
of 50052 was unsafe because that is the production worker port; it is corrected
in this slice. At runtime, fresh system-unit snapshots add their live PIDs,
InvocationIDs, executable hashes, argv, listener ownership, restart counts,
and coordinator health to the protected boundary. The adapter never calls a
system-unit stop or restart operation and refuses a disposable unit, port, or
PID collision.

The unreachable candidate lifecycle is deliberately one entry at a time. It is
retained for offline fake-runner review; the statements below describe intended
behavior, not an executable or target-qualified workflow:

1. `validate` revalidates the frozen core, preflight-v2 inputs, and closed
   issue-#37 policy without target mutation.
2. A future reviewed `execute-next` would be accepted only on the frozen
   coordinator host. Before any process launch it would require a root-visible
   `/dev/kfd` and render-node client
   census (`sudo -n fuser`) and zero existing GPU clients on both nodes. The
   measured pair repeats this census immediately before and after its request,
   allowing only its captured disposable PIDs; any late foreign client consumes
   the slot as a retained failure. The always-on comparison deployment is
   therefore expected to make the adapter refuse until an independently
   authorized maintenance window had already made the GPUs idle. This draft
   neither creates nor accepts evidence for that window, and its source gate
   refuses before SSH.
3. The adapter records and fsyncs a next-only intent. From that point the
   schedule slot is consumed. A crash, timeout, or ambiguous request becomes a
   retained failure and is never retried under the same run.
4. Each declared warmup uses a fresh worker/coordinator user-unit pair and is
   fully stopped and cleaned before the measured pair starts. User transient
   units start without `--collect`; the adapter binds PID, fresh 32-hex
   InvocationID, process start time, journal cursor, executable SHA-256, exact
   NUL-decoded argv, exact `env -i` allowlisted environment, cgroup, and
   PID-owned listener.
5. The worker starts first. Its PID-owned RPC listener must be ready before the
   coordinator starts; coordinator HTTP health plus a still-live worker proves
   both-node readiness. The adapter posts `inputs/request.raw` as the body
   without JSON reserialization.
6. A measured request retains raw streamed HTTP bytes, the assembled response,
   monotonic TTFT/inter-token events, and overlapping telemetry from both
   nodes. It refuses an HTTP/token-event mismatch or any final timing record
   whose integer `cache_n` is not zero. SSH has bounded liveness probes and a
   local process group; an independent remote `/usr/bin/timeout` watchdog owns
   each remote child if the connection is lost.
7. Shutdown revalidates identities, stops coordinator then worker, captures
   InvocationID/cursor-bound journals before collection, attempts cleanup for
   both authorized roles even after an error, and proves units absent, ports
   closed, captured PIDs absent, and captured transient cgroups removed.
   The fresh production snapshot must equal the pre-run snapshot exactly.
8. A success or failure receipt is copied into the ordinary evidence-core raw
   sample. Failed cycle records retain all reached identities, readiness,
   request hashes, exact journal bytes, GPU censuses, cleanup proofs, and every
   primary/secondary error. The adapter leaves `execution_qualified`,
   `measurement_ready`, and `performance_claim` false; promotion requires
   separate target evidence and review.

Run only local validation from a checkout containing a prepared frozen run:

```bash
python scripts/halofpx_strix_ab_cachyos.py validate \
  /var/tmp/halofpx-ab-my-run policy.json
```

`validate` accepts the versioned runtime comparison and reports
`target_execution_state=blocked`, the incident-manifest
identity, and the unresolved custody list. The CLI `execute-next` command exits
before host inspection or SSH. Offline fake-runner tests cover the proposed
happy lifecycle, exact 512/2048 generated argv and equal-binary custody,
exact body custody, global schedule order, protected
unit/port/PID collision, pre-intent and late foreign GPU clients, argv
mismatch, reused-cache response, production drift, multi-target cleanup
failure, failed-cycle evidence retention, and SSH watchdog construction. They
do not establish CachyOS, ROCm, `gfx1151`, dual-node, or performance behavior.
