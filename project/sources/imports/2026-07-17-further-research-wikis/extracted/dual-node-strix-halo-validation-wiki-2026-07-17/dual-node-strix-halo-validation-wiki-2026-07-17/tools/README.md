# Reference Tools

> **Wiki status:** Proposed · **Evidence state:** S0-capable tooling · **Last reviewed:** 2026-07-17  
> **Machine-validation status:** Not run. Tool execution on synthetic records is not machine evidence.

The tools are intentionally small, auditable scaffolds. Production harnesses may replace them, but must preserve schema fields, clock boundaries, raw attempts, provenance, and evidence-first gate behavior.

| Tool | Purpose | Key constraint |
|---|---|---|
| `check_bundle.py` | Offline files, links, YAML, schemas, source IDs, and fixtures | A pass means `DESIGN_COMPLETE`, not hardware validation |
| `validate_records.py` | Validate JSON/JSONL against a schema | Does not verify semantic state such as a truly cold cache |
| `audit_provenance.py` | Required fields, raw hashes, record coverage, and secret-key audit | Integrity/completeness is not performance validation |
| `token_stream_client.py` | Timestamp OpenAI-compatible SSE content events | True ITL requires one event/token or server token IDs |
| `aggregate_requests.py` | TTFT, ITL/event gap, TPOT, E2E, throughput, cache aggregates | Uses client monotonic timestamps; retains timing defects |
| `compare_runs.py` | Paired bootstrap candidate/baseline ratio | Input pairs must be genuinely matched |
| `sample_telemetry.py` | Linux host/AMDGPU/network/disk JSONL sampler | Supplement with AMD SMI/profilers and wall meters |
| `collect_provenance.sh` | Hardware/software/network/artifact snapshot | Deliberately does not dump environment secrets |
| `sample_usb4.sh` | USB4/Thunderbolt sysfs/interface snapshot | Capture before, after, and on link events |
| `run_iperf_matrix.sh` | Direction/stream-count link reference | Use the dedicated USB4 data interface only |
| `prepare_cache_state.sh` | Guarded C0–C3 action scaffold | Action record is not proof; counters must verify state |
| `fault_netem.sh` | Guarded delay/loss/rate helper | Never use on management; explicit confirmation required |
| `upstream_watch.py` | Poll/normalize upstream changes | Discovery only; never auto-adopts a change |
| `check_upstream_freshness.py` | Apply stale budgets and count unresolved P0/P1 events | Freshness is a prerequisite, not compatibility proof |
| `evaluate_gates.py` | Evidence-first release summary evaluation | Synthetic/missing/unmatched evidence is rejected |
| `build_inventory.py` | File inventory and bundle checksum manifest | Re-run after any bundle edit |

## Core commands

```bash
python -m pip install -r tools/requirements.txt
python tools/check_bundle.py --report BUNDLE-CHECK-REPORT.md
python tools/validate_records.py schemas/request-trace.schema.json raw-data/RUN/requests.jsonl --jsonl
python tools/audit_provenance.py raw-data/RUN/manifest.json --run-dir raw-data/RUN
python tools/aggregate_requests.py --requests raw-data/RUN/requests.jsonl --tokens raw-data/RUN/tokens.jsonl --output raw-data/RUN/derived/request-summary.json
python tools/evaluate_gates.py release/summary.json --stage G4 --output release/unsigned-evaluation.json
python tools/upstream_watch.py --dry-run
python tools/check_upstream_freshness.py --state raw-data/upstream/watch-state.json --ledger raw-data/upstream/events.jsonl
```

`evaluate_gates.py` exits 0 only for a pass. `INSUFFICIENT_EVIDENCE` and measured failures both return nonzero but remain distinct in the JSON result.

## Safety

Fault and cache scripts are deployment scaffolds, not unattended automation. Review exact interfaces, commands, temperature/storage aborts, and recovery access. Fault logs must travel over a management path that is not being disrupted.
