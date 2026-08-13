from __future__ import annotations

import copy
import datetime as dt
import hashlib
import json
import tempfile
import threading
import time
from pathlib import Path
from typing import Any, Sequence


def _sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def _write_json(path: Path, value: Any) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, indent=2, sort_keys=True) + "\n", encoding="utf-8")


def _write_json_lf(path: Path, value: Any) -> None:
    """Write canonical LF JSON for contracts whose bytes are portable evidence."""
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(
        (json.dumps(value, indent=2, sort_keys=True) + "\n").encode("utf-8"))


def _canonicalize_core_ledger(root: Path) -> None:
    """Make the synthetic hosted tree match the target writer's LF ledger."""
    ledger = root / "SHA256SUMS"
    content = ledger.read_bytes().replace(b"\r\n", b"\n")
    ledger.write_bytes(content)


def _prometheus(values: dict[str, int], metrics: dict[str, str]) -> bytes:
    lines = ["# synthetic hosted adapter evidence; no target observations"]
    for name, wire_name in metrics.items():
        lines.extend((f"# TYPE {wire_name} counter", f"{wire_name} {values[name]}"))
    return ("\n".join(lines) + "\n").encode("utf-8")


def _freeze_sampling_sync_profile(
    evidence_root: Path, *, core: Any, sampling_sync: Any, plan: dict[str, Any],
) -> None:
    side_plan = {
        "schema": sampling_sync.PLAN_SCHEMA,
        "lane": sampling_sync.CONTRACT,
        "enabled": True,
        "issue": 28,
        "core_plan_sha256": core.plan_digest(plan),
        "endpoint": "/metrics",
        "completion_endpoint": "/completion",
        "metrics": sampling_sync.METRICS,
        "conditions": {"off": {"coalescing": False}, "on": {"coalescing": True}},
    }
    side_plan_path = evidence_root.parent / f"{evidence_root.name}-sampling-plan.json"
    core.write_json(side_plan_path, side_plan)
    sampling_sync.freeze_observability_plan(evidence_root, side_plan_path)
    side_plan_path.unlink()


def _add_sampling_sync_profile(
    evidence_root: Path, *, core: Any, sampling_sync: Any, plan: dict[str, Any],
    receipt_paths: list[Path],
) -> None:
    schedule = core.make_schedule(plan)
    for index, entry in enumerate(schedule["entries"]):
        receipt = core.read_json(receipt_paths[index])
        measured = receipt["cycles"][-1]
        identity = measured["identities"]["coordinator"]
        request = measured["request"]
        sample_root = evidence_root / "raw" / (
            f"pair-{entry['pair_id']:03d}-order-{entry['order_index']}-{entry['condition']}")
        sample = core.read_json(sample_root / "sample.json")
        raw = sample["raw"]
        base = (1 << 53) + index * 1000
        before = {name: base + ordinal for ordinal, name in enumerate(sampling_sync.METRICS)}
        output_tokens = plan["request"]["output_tokens"]
        delta = {
            "output_epochs": output_tokens + 1,
            "completed_barriers": output_tokens * (6 if entry["condition"] == "off" else 1),
            "reused_barriers": 0 if entry["condition"] == "off" else output_tokens * 5,
            "graph_submissions": output_tokens + 4,
            "output_transfers": output_tokens,
        }
        after = {name: before[name] + delta[name] for name in before}
        with tempfile.TemporaryDirectory(
                prefix="halofpx-sampling-sidecar-", dir=evidence_root.parent) as directory:
            temporary = Path(directory)
            before_path = temporary / "before.prom"
            after_path = temporary / "after.prom"
            capture_path = temporary / "capture.json"
            before_bytes = _prometheus(before, sampling_sync.METRICS)
            after_bytes = _prometheus(after, sampling_sync.METRICS)
            before_path.write_bytes(before_bytes)
            after_path.write_bytes(after_bytes)
            side_identity = {
                "pid": identity["pid"],
                "invocation_id": identity["invocation_id"],
                "process_start_ticks": identity["process_start_ticks"],
                "metrics_process_start_time_unix": 1_786_604_480 + index,
            }
            started = request["remote_started_monotonic_ns"]
            ended = request["remote_ended_monotonic_ns"]
            capture = {
                "schema": sampling_sync.CAPTURE_SCHEMA,
                "contract": sampling_sync.CONTRACT,
                "condition": entry["condition"],
                "endpoint": {
                    "scheme": "http", "host": "nimo-1", "port": 18080,
                    "metrics_path": "/metrics", "completion_path": "/completion",
                },
                "sequence": ["metrics_before", "request", "metrics_after"],
                "before": {
                    "captured_monotonic_ns": started - 1,
                    "identity": dict(side_identity),
                    "metrics_sha256": hashlib.sha256(before_bytes).hexdigest(),
                },
                "request": {
                    "started_monotonic_ns": started,
                    "ended_monotonic_ns": ended,
                    "identity": dict(side_identity),
                    "request_sha256": plan["request"]["sha256"],
                    "response_sha256": raw["response"]["sha256"],
                    "client_sha256": raw["client"]["sha256"],
                    "request_count": 1,
                },
                "after": {
                    "captured_monotonic_ns": ended + 1,
                    "identity": dict(side_identity),
                    "metrics_sha256": hashlib.sha256(after_bytes).hexdigest(),
                },
            }
            _write_json_lf(capture_path, capture)
            sampling_sync.record_observation(
                evidence_root, entry["pair_id"], entry["condition"],
                entry["order_index"], before_path, after_path, capture_path)
    report = sampling_sync.validate_frozen_run(evidence_root, plan)
    if report is None or report.get("evidence_complete") is not True:
        raise AssertionError("synthetic sampling-output-sync profile did not complete")


def create_control_repository(
    repository_root: Path,
    *,
    core: Any,
    adapter: Any,
    incident_source: Path,
    response_content: str = "x" * 8,
) -> dict[str, Path]:
    """Create deterministic, local-only PR51 control inputs for controller tests."""
    fixture = repository_root / "fixture"
    fixture.mkdir(parents=True)
    incident = repository_root / adapter.ISSUE41_MANIFEST_RELATIVE
    incident.parent.mkdir(parents=True)
    incident.write_bytes(incident_source.read_bytes())

    artifacts: dict[str, Path] = {}
    for name in (
        "model", "off-server", "on-server", "off-worker", "on-worker",
    ):
        path = fixture / name
        path.write_bytes((name + "\n").encode("utf-8"))
        artifacts[name] = path

    request = fixture / "request.json"
    request.write_bytes(json.dumps({
        "prompt": "frozen synthetic adapter-evidence prompt",
        "n_predict": 8,
        "stream": True,
        "cache_prompt": False,
        "ignore_eos": True,
        "seed": 1234,
        "temperature": 0,
    }, separators=(",", ":")).encode("utf-8"))
    artifacts["request"] = request

    scripts_root = Path(adapter.__file__).resolve().parent
    hmm_paths: dict[str, Path] = {}
    for kind in ("snapshot", "policy", "result"):
        source = scripts_root / f"halofpx-strix-hmm-admission-{kind}.example.json"
        if not source.is_file():
            raise AssertionError(f"required ADR0064 example is missing: {source}")
        destination = fixture / f"hmm-admission-{kind}.json"
        destination.write_bytes(source.read_bytes())
        hmm_paths[kind] = destination
    hmm_result_path = hmm_paths["result"]
    authority_paths: dict[str, Path] = {}
    for role, host, unit, port in (
        ("coordinator", "nimo-1", adapter.PROTECTED_UNITS["coordinator"], 8081),
        ("worker", "nimo-2", adapter.PROTECTED_UNITS["worker"], 50052),
    ):
        authority = fixture / f"authority-{role}.json"
        _write_json(authority, {
            "schema": "halofpx.strix-ab-machine-authority.v2",
            "role": role,
            "host": host,
            "service": {
                "unit": unit,
                "port": port,
                "active": False,
                "pid": 0,
                "control_group": None,
                "listener_pids": [],
            },
            "process_baseline": None,
            "hmm_admission_result_sha256": _sha(hmm_result_path),
        })
        authority_paths[role] = authority

    plan = {
        "schema": core.PLAN_SCHEMA,
        "experiment_id": "adr0062-complete-adapter-evidence",
        "issues": [15, 16, 28],
        "source": {
            "repository": "https://github.com/JCFrags/HaloFPX.git",
            "off_commit": "0" * 40,
            "on_commit": "0" * 40,
        },
        "model": {
            "path": str(artifacts["model"]),
            "sha256": _sha(artifacts["model"]),
            "size_bytes": artifacts["model"].stat().st_size,
            "format_family": "rocmfpx",
            "architecture": "synthetic-model-general",
        },
        "request": {
            "path": str(request),
            "sha256": _sha(request),
            "prompt_tokens": 512,
            "output_tokens": 8,
            "require_content_parity": True,
            "expected_content_sha256": hashlib.sha256(response_content.encode("utf-8")).hexdigest(),
        },
        "topology": {
            "world_size": 2,
            "rpc_endpoint": "nimo-2:50252",
            "coordinator": {
                "host": "nimo-1",
                "device": "ROCm0",
                "authority_receipt": {
                    "path": str(authority_paths["coordinator"]),
                    "sha256": _sha(authority_paths["coordinator"]),
                },
            },
            "worker": {
                "host": "nimo-2",
                "device": "ROCm0",
                "authority_receipt": {
                    "path": str(authority_paths["worker"]),
                    "sha256": _sha(authority_paths["worker"]),
                },
            },
        },
        "runtime": {
            "lane": "cold_prompt_generation",
            "cache_class": "cold_cache_off",
            "context": 1024,
            "batch": 512,
            "ubatch": 512,
            "flash_attention": True,
            "kv_k": "q8_0",
            "kv_v": "q8_0",
            "common_environment": {"HSA_ENABLE_SDMA": "0"},
            "common_worker_args": [
                "--host", "0.0.0.0", "--port", "50252", "--device", "ROCm0",
            ],
            "common_coordinator_args": [
                "--host", "127.0.0.1", "--port", "18080",
                "--model", str(artifacts["model"]),
                "--rpc", "nimo-2:50252", "--device", "RPC0,ROCm0",
                "--ctx-size", "1024", "--batch-size", "512",
                "--ubatch-size", "512", "--cache-type-k", "q8_0",
                "--cache-type-v", "q8_0", "--flash-attn", "on",
                "--seed", "1234", "--temp", "0", "--metrics", "--offline",
                "--parallel", "1", "--no-cont-batching", "--no-warmup",
            ],
        },
        "execution": {
            "pairs": 1,
            "order_seed": 20260813,
            "warmups_per_condition": 1,
            "retained_per_condition_per_pair": 1,
            "profiling_separate": True,
        },
        "conditions": {
            "off": {
                "source_commit": "0" * 40,
                "coordinator_binary": {
                    "path": str(artifacts["off-server"]),
                    "sha256": _sha(artifacts["off-server"]),
                },
                "worker_binary": {
                    "path": str(artifacts["off-worker"]),
                    "sha256": _sha(artifacts["off-worker"]),
                },
                "coordinator_args": [],
                "worker_args": [],
            },
            "on": {
                "source_commit": "0" * 40,
                "coordinator_binary": {
                    "path": str(artifacts["on-server"]),
                    "sha256": _sha(artifacts["on-server"]),
                },
                "worker_binary": {
                    "path": str(artifacts["on-worker"]),
                    "sha256": _sha(artifacts["on-worker"]),
                },
                "coordinator_args": [],
                "worker_args": [],
            },
        },
    }
    plan_path = fixture / "plan.json"
    _write_json(plan_path, plan)

    policy_path = fixture / "adapter-policy.json"
    _write_json(policy_path, {
        "schema": adapter.POLICY_SCHEMA,
        "issue": 37,
        "controller_host": "nimo-1",
        "protected": {
            "coordinator": {
                "host": "nimo-1",
                "unit": adapter.PROTECTED_UNITS["coordinator"],
                "ports": [8081],
                "health_url": "http://127.0.0.1:8081/health",
            },
            "worker": {
                "host": "nimo-2",
                "unit": adapter.PROTECTED_UNITS["worker"],
                "ports": [50052],
                "health_url": None,
            },
        },
        "disposable": {
            "unit_prefix": "halofpx-ab-",
            "coordinator_port": 18080,
            "worker_port": 50252,
            "runtime_max_seconds": 3600,
        },
        "timeouts": {
            "identity_seconds": 10,
            "readiness_seconds": 60,
            "request_seconds": 60,
            "stop_seconds": 10,
            "telemetry_interval_seconds": 0.1,
        },
        "require_no_foreign_gpu_clients": True,
    })
    return {
        "plan": plan_path,
        "policy": policy_path,
        "incident": incident,
        "request": request,
        "hmm_snapshot": hmm_paths["snapshot"],
        "hmm_policy": hmm_paths["policy"],
        "hmm_result": hmm_result_path,
    }


class CompleteEvidenceRunner:
    """Side-effect-free runner that emits the real PR51 nested evidence profile."""

    def __init__(self, adapter: Any, request_bytes: bytes, response_content: str = "x" * 8):
        self.adapter = adapter
        self.request_bytes = request_bytes
        if len(response_content) != 8:
            raise AssertionError("synthetic response content must contain exactly eight tokens")
        self.response_content = response_content
        self.units: dict[tuple[str, str], Any] = {}
        self.ports: dict[tuple[str, int], int] = {}
        self.next_pid = 10000
        self.identity_counter = 0
        self.clock_lock = threading.Lock()
        self.clock_ns = {
            "nimo-1": 1_000_010_000,
            "nimo-2": 2_000_011_000,
        }

    def _tick(self, host: str, increment_ns: int = 1_000_000) -> int:
        with self.clock_lock:
            self.clock_ns[host] += increment_ns
            return self.clock_ns[host]

    @staticmethod
    def _boot_id(host: str) -> str:
        return (
            "11111111-1111-4111-8111-111111111111"
            if host == "nimo-1" else "22222222-2222-4222-8222-222222222222")

    def snapshot_production(self, protected: dict[str, dict[str, Any]]) -> dict[str, Any]:
        return {
            role: {
                "host": spec["host"],
                "unit": spec["unit"],
                "active": False,
                "pid": 0,
                "invocation_id": "",
                "nrestarts": 0,
                "start_monotonic": "0",
                "control_group": "",
                "process": None,
                "listeners": {str(port): [] for port in spec["ports"]},
                "health": None,
            }
            for role, spec in protected.items()
        }

    def artifact(self, host: str, path: str) -> dict[str, Any]:
        artifact = Path(path)
        return {"path": path, "size_bytes": artifact.stat().st_size, "sha256": _sha(artifact)}

    def gpu_clients(self, host: str) -> dict[str, Any]:
        return {
            "complete": True,
            "pids": sorted(identity.pid for identity in self.units.values() if identity.host == host),
            "errors": [],
        }

    def port_owners(self, host: str, port: int) -> list[int]:
        owner = self.ports.get((host, port))
        return [] if owner is None else [owner]

    def ensure_unit_absent(self, host: str, unit: str) -> None:
        if (host, unit) in self.units:
            raise self.adapter.AdapterError("synthetic unit is not absent")

    def start_unit(
        self, role: str, host: str, unit: str, argv: Sequence[str],
        environment: dict[str, str], executable_sha256: str, port: int,
        runtime_max_seconds: int, identity_timeout_seconds: int,
        stop_timeout_seconds: int,
    ) -> Any:
        self.identity_counter += 1
        pid = self.next_pid
        self.next_pid += 1
        started_us = self._tick(host) // 1000
        identity = self.adapter.UnitIdentity(
            role=role,
            host=host,
            unit=unit,
            pid=pid,
            invocation_id=f"{self.identity_counter:032x}",
            process_start_ticks=100000 + self.identity_counter,
            # Same synthetic boot as the retained ADR-0064 capture, and
            # strictly later than both retained node capture clocks.
            start_monotonic_us=started_us,
            boot_id=self._boot_id(host),
            cursor_before=f"s=fixture-{self.identity_counter}",
            argv=tuple(argv),
            environment=dict(environment),
            executable_sha256=executable_sha256,
            port=port,
            control_group=(
                f"/user.slice/user-1000.slice/user@1000.service/app.slice/{unit}"
            ),
        )
        self.units[(host, unit)] = identity
        self.ports[(host, port)] = pid
        return identity

    def _systemd(self, identity: Any) -> dict[str, str]:
        return {
            "ExecMainPID": str(identity.pid),
            "InvocationID": identity.invocation_id,
            "ActiveState": "active",
            "SubState": "running",
            "ControlGroup": identity.control_group,
            "FragmentPath": f"/run/user/1000/systemd/transient/{identity.unit}",
        }

    def _process(self, identity: Any) -> dict[str, Any]:
        return {
            "pid": identity.pid,
            "exe": identity.argv[0],
            "exe_sha256": identity.executable_sha256,
            "argv": list(identity.argv),
            "environment": dict(identity.environment),
            "cgroup": f"0::{identity.control_group}\n",
            "process_start_ticks": identity.process_start_ticks,
        }

    def prove_live(self, identity: Any, require_listener: bool = True) -> dict[str, Any]:
        if self.units.get((identity.host, identity.unit)) != identity:
            raise self.adapter.AdapterError("synthetic identity is not live")
        listeners = self.port_owners(identity.host, identity.port)
        return {
            "systemd": self._systemd(identity),
            "process": self._process(identity),
            "listener_pids": listeners,
            "boot_id": identity.boot_id,
            "observed_monotonic_ns": self._tick(identity.host),
        }

    def wait_ready(self, identity: Any, timeout_seconds: int) -> dict[str, Any]:
        self.prove_live(identity)
        common = {
            "identity": {
                "role": identity.role, "host": identity.host, "unit": identity.unit,
                "pid": identity.pid, "invocation_id": identity.invocation_id,
                "process_start_ticks": identity.process_start_ticks,
            },
            "boot_id": identity.boot_id,
            "observed_monotonic_ns": self._tick(identity.host),
        }
        if identity.role == "worker":
            return {
                "kind": "pid-owned-rpc-hello",
                "ready": True,
                "rpc_protocol": "4.0.1",
                "connection_caps_sha256": "a" * 64,
                **common,
            }
        return {
            "kind": "pid-owned-http-health",
            "ready": True,
            "body_sha256": "b" * 64,
            "body_bytes": 16,
            **common,
        }

    def request(
        self, host: str, port: int, body: bytes, output_tokens: int,
        timeout_seconds: int,
    ) -> Any:
        if body != self.request_bytes:
            raise self.adapter.AdapterError("synthetic request bytes changed")
        started = self._tick(host)
        time.sleep(0.30)
        event_step = 20_000_000
        stamps = [started + 110_000_000 + index * event_step for index in range(output_tokens)]
        ended = started + 270_000_000
        self.clock_ns[host] = ended
        if stamps[-1] >= ended:
            raise AssertionError("synthetic emitted-event stamps escape request")
        # Keep synthetic server work within the same retained ~250 ms client
        # request. These are hosted fixture values, never performance evidence.
        prompt_tps = 5120.0
        generation_tps = 80.0
        timings = {
            "cache_n": 0,
            "prompt_n": 512,
            "predicted_n": output_tokens,
            "prompt_ms": 512 / prompt_tps * 1000.0,
            "predicted_ms": output_tokens / generation_tps * 1000.0,
            "prompt_per_token_ms": (512 / prompt_tps * 1000.0) / 512,
            "prompt_per_second": prompt_tps,
            "predicted_per_token_ms": (output_tokens / generation_tps * 1000.0) / output_tokens,
            "predicted_per_second": generation_tps,
        }
        events = []
        for index in range(output_tokens):
            event: dict[str, Any] = {
                "index": 0, "content": self.response_content[index], "tokens": [index], "stop": False,
                "id_slot": -1, "tokens_predicted": index + 1, "tokens_evaluated": 512,
            }
            events.append(b"data: " + json.dumps(event, separators=(",", ":")).encode("utf-8") + b"\n\n")
        terminal = {
            "index": 0, "content": "", "tokens": [], "id_slot": 0, "stop": True,
            "model": "synthetic-offline.gguf", "tokens_predicted": output_tokens,
            "tokens_evaluated": 512, "generation_settings": {
                "seed": 1234, "temperature": 0, "dynatemp_range": 0,
                "dynatemp_exponent": 1, "top_k": 40, "top_p": 0.95,
                "min_p": 0.05, "top_n_sigma": -1, "xtc_probability": 0,
                "xtc_threshold": 0.1, "typical_p": 1, "repeat_last_n": 64,
                "repeat_penalty": 1, "presence_penalty": 0, "frequency_penalty": 0,
                "dry_multiplier": 0, "dry_base": 1.75, "dry_allowed_length": 2,
                "dry_penalty_last_n": 4096, "dry_sequence_breakers": ["\n", ":", '"', "*"],
                "mirostat": 0, "mirostat_tau": 5, "mirostat_eta": 0.1,
                "stop": [], "max_tokens": output_tokens, "n_predict": output_tokens,
                "n_keep": 0, "n_discard": 0, "ignore_eos": True, "stream": True,
                "logit_bias": [], "n_probs": 0, "min_keep": 0, "grammar": "",
                "grammar_lazy": False, "grammar_triggers": [], "preserved_tokens": [],
                "chat_format": "Content-only", "reasoning_format": "none",
                "reasoning_in_content": False, "generation_prompt": "", "samplers": [
                    "penalties", "dry", "top_n_sigma", "top_k", "typ_p", "top_p",
                    "min_p", "xtc", "temperature"],
                "speculative.types": "none", "speculative.n_min": 0,
                "speculative.n_max": 0, "timings_per_token": False,
                "post_sampling_probs": False, "backend_sampling": False, "lora": [],
            },
            "prompt": "frozen synthetic adapter-evidence prompt",
            "has_new_line": "\n" in self.response_content,
            "truncated": False, "stop_type": "limit", "stopping_word": "",
            "tokens_cached": 512 + output_tokens - 1, "timings": timings,
        }
        raw = b"".join(events) + b"data: " + json.dumps(
            terminal, separators=(",", ":")).encode("utf-8") + b"\n\n"
        now = dt.datetime.now(dt.timezone.utc)
        elapsed_ms = (ended - started) / 1_000_000
        response = {"content": self.response_content, "timings": timings}
        client = {
            "schema": "halofpx.client-timing.v2",
            "started_at": now.isoformat(),
            "ended_at": (now + dt.timedelta(milliseconds=elapsed_ms)).isoformat(),
            "http_status": 200,
            "wall_ms": elapsed_ms,
            "ttft_ms": (stamps[0] - started) / 1_000_000,
            "itl_ms": [
                (right - left) / 1_000_000 for left, right in zip(stamps, stamps[1:])],
            "remote_started_monotonic_ns": started,
            "remote_ended_monotonic_ns": ended,
            "event_monotonic_ns": stamps,
        }
        return self.adapter.RequestCapture(
            response=response,
            client=client,
            raw_http=raw,
            sent_body_sha256=hashlib.sha256(body).hexdigest(),
            started_monotonic_ns=started,
            ended_monotonic_ns=ended,
        )

    def telemetry(self, host: str) -> dict[str, Any]:
        return {
            "monotonic_ns": self._tick(host, 10_000_000),
            "boot_id": self._boot_id(host),
            "loadavg": "0.00 0.01 0.05 1/100 1234",
            "meminfo": "MemTotal:       131072000 kB\nMemAvailable:  120000000 kB\n",
            "gpu": {
                "/sys/class/drm/card1/device/gpu_busy_percent": "0",
                "/sys/class/drm/card1/device/hwmon/hwmon1/temp1_input": "40000",
            },
        }

    def stop_unit(self, identity: Any, timeout_seconds: int) -> tuple[dict[str, Any], bytes]:
        if self.units.get((identity.host, identity.unit)) != identity:
            raise self.adapter.AdapterError("synthetic terminal identity changed")
        properties = {
            "ExecMainPID": "0",
            "InvocationID": identity.invocation_id,
            "ExecMainStatus": "0",
            "Result": "success",
            "ActiveState": "inactive",
            "ControlGroup": identity.control_group,
        }
        journal = (
            f"[fixture] role={identity.role} invocation={identity.invocation_id} stopped\n"
        ).encode("utf-8")
        return {"properties": properties}, journal

    def cleanup_unit(
        self, host: str, unit: str, port: int, timeout_seconds: int,
        identity: Any | None = None,
    ) -> dict[str, Any]:
        observed = self.units.pop((host, unit), None)
        self.ports.pop((host, port), None)
        captured = identity or observed
        if captured is None:
            raise self.adapter.AdapterError("synthetic cleanup lacks an identity")
        return {
            "unit_absent": True,
            "port_closed": True,
            "stop_returncode": 0,
            "reset_returncode": 0,
            "captured_pid_absent": True,
            "captured_cgroup_absent": True,
            "captured_pid": captured.pid,
            "captured_control_group": captured.control_group,
            "identity_source": "provided",
            "boot_id": captured.boot_id,
            "completed_monotonic_ns": self._tick(host),
            "pre_state": {
                "LoadState": "loaded",
                "ActiveState": "inactive",
                "MainPID": "0",
                "InvocationID": captured.invocation_id,
                "ControlGroup": captured.control_group,
                "FragmentPath": f"/run/user/1000/systemd/transient/{captured.unit}",
            },
        }


def materialize_complete_adapter_tree(
    evidence_root: Path,
    *,
    core: Any,
    adapter: Any,
    plan_path: Path,
    policy_path: Path,
    incident_bytes: bytes,
    hmm_snapshot_bytes: bytes,
    hmm_policy_bytes: bytes,
    hmm_result_bytes: bytes,
    selected_schedule_index: int,
    sampling_sync: Any | None = None,
    response_content: str = "x" * 8,
) -> bytes:
    core.init_run(plan_path, evidence_root)
    plan = core.load_plan(plan_path)
    for role, host in (("coordinator", "nimo-1"), ("worker", "nimo-2")):
        receipt = core.collect_preflight(plan, role, observed_hostname=host)
        receipt_path = evidence_root.parent / f"{evidence_root.name}-{role}-preflight.json"
        core.write_json(receipt_path, receipt)
        core.import_preflight(evidence_root, receipt_path)
        receipt_path.unlink()
    (evidence_root / "incident.raw").write_bytes(incident_bytes)
    (evidence_root / "hmm-admission-snapshot.raw.json").write_bytes(hmm_snapshot_bytes)
    (evidence_root / "hmm-admission-policy.raw.json").write_bytes(hmm_policy_bytes)
    (evidence_root / "hmm-admission-result.raw.json").write_bytes(hmm_result_bytes)
    runner = CompleteEvidenceRunner(
        adapter, (evidence_root / "inputs" / "request.raw").read_bytes(), response_content)
    schedule = core.make_schedule(plan)
    receipt_paths = []
    if sampling_sync is not None:
        _freeze_sampling_sync_profile(
            evidence_root, core=core, sampling_sync=sampling_sync, plan=plan)
    real_datetime = dt.datetime

    class _FixtureDateTime(real_datetime):
        calls = 0

        @classmethod
        def now(cls, tz: dt.tzinfo | None = None) -> dt.datetime:
            value = real_datetime(2026, 8, 13, 7, 1, 30, tzinfo=dt.timezone.utc) + \
                dt.timedelta(seconds=cls.calls)
            cls.calls += 1
            return value if tz is not None else value.replace(tzinfo=None)

    # The checked-in ADR-0064 synthetic evidence has a deliberately narrow
    # validity/age window. Freeze hosted fixture wall time inside that window;
    # monotonic request timing remains real and is still cross-checked.
    dt.datetime = _FixtureDateTime
    try:
        for _entry in schedule["entries"]:
            receipt_paths.append(adapter.execute_next(evidence_root, policy_path, runner))
    finally:
        dt.datetime = real_datetime
    if sampling_sync is not None:
        _add_sampling_sync_profile(
            evidence_root, core=core, sampling_sync=sampling_sync, plan=plan,
            receipt_paths=receipt_paths)
    core.analyze_run(evidence_root)
    _canonicalize_core_ledger(evidence_root)
    return receipt_paths[selected_schedule_index].read_bytes()
