"""Offline-only fake evidence producer for sampling-sync sidecar tests."""

from __future__ import annotations

import hashlib
import json
from pathlib import Path
from typing import Any


METRICS = {
    "output_epochs": "llamacpp:halofpx_sampling_sync_output_epochs_total",
    "completed_barriers": "llamacpp:halofpx_sampling_sync_completed_barriers_total",
    "reused_barriers": "llamacpp:halofpx_sampling_sync_reused_barriers_total",
    "graph_submissions": "llamacpp:halofpx_sampling_sync_graph_submissions_total",
    "output_transfers": "llamacpp:halofpx_sampling_sync_output_transfers_total",
}


class FakeSamplingOutputSyncAdapter:
    """Writes deterministic raw files; it has no network or process methods."""

    def __init__(self) -> None:
        self.ordinal = 0

    @staticmethod
    def prometheus(values: dict[str, int]) -> bytes:
        lines = ["# fake adapter: offline evidence only", "unrelated_metric 7"]
        for name, wire_name in METRICS.items():
            lines.append(f"# TYPE {wire_name} counter")
            lines.append(f"{wire_name} {values[name]}")
        return ("\n".join(lines) + "\n").encode("utf-8")

    def emit(
        self,
        directory: Path,
        condition: str,
        request_sha256: str,
        output_tokens: int,
        *,
        response_sha256: str = "a" * 64,
        client_sha256: str = "b" * 64,
        endpoint: dict[str, Any] | None = None,
        before_overrides: dict[str, int] | None = None,
        delta_overrides: dict[str, int] | None = None,
        request_count: int = 1,
        identity_overrides: dict[str, Any] | None = None,
    ) -> tuple[Path, Path, Path]:
        self.ordinal += 1
        directory.mkdir(parents=True, exist_ok=True)
        base = (1 << 53) + self.ordinal * 1000
        before = {name: base + index for index, name in enumerate(METRICS)}
        before.update(before_overrides or {})
        if condition == "off":
            delta = {
                "output_epochs": output_tokens + 1,
                "completed_barriers": output_tokens * 6,
                "reused_barriers": 0,
                "graph_submissions": output_tokens + 4,
                "output_transfers": output_tokens,
            }
        else:
            delta = {
                "output_epochs": output_tokens + 1,
                "completed_barriers": output_tokens,
                "reused_barriers": output_tokens * 5,
                "graph_submissions": output_tokens + 4,
                "output_transfers": output_tokens,
            }
        delta.update(delta_overrides or {})
        after = {name: before[name] + delta[name] for name in METRICS}
        before_path = directory / "before.prom"
        after_path = directory / "after.prom"
        capture_path = directory / "capture.json"
        before_bytes = self.prometheus(before)
        after_bytes = self.prometheus(after)
        before_path.write_bytes(before_bytes)
        after_path.write_bytes(after_bytes)
        identity = {
            "pid": 4100 + self.ordinal,
            "invocation_id": f"{self.ordinal:032x}",
            "process_start_ticks": 10000 + self.ordinal,
            "metrics_process_start_time_unix": 1_786_579_200 + self.ordinal,
        }
        identity.update(identity_overrides or {})
        start = 1_000_000 * self.ordinal
        capture = {
            "schema": "halofpx.sampling-output-sync-capture.v1",
            "contract": "sampling_output_sync_prometheus_v1",
            "condition": condition,
            "endpoint": endpoint or {
                "scheme": "http",
                "host": "nimo-1",
                "port": 18080,
                "metrics_path": "/metrics",
                "completion_path": "/completion",
            },
            "sequence": ["metrics_before", "request", "metrics_after"],
            "before": {
                "captured_monotonic_ns": start + 1,
                "identity": dict(identity),
                "metrics_sha256": hashlib.sha256(before_bytes).hexdigest(),
            },
            "request": {
                "started_monotonic_ns": start + 2,
                "ended_monotonic_ns": start + 3,
                "identity": dict(identity),
                "request_sha256": request_sha256,
                "response_sha256": response_sha256,
                "client_sha256": client_sha256,
                "request_count": request_count,
            },
            "after": {
                "captured_monotonic_ns": start + 4,
                "identity": dict(identity),
                "metrics_sha256": hashlib.sha256(after_bytes).hexdigest(),
            },
        }
        capture_path.write_text(json.dumps(capture), encoding="utf-8")
        return before_path, after_path, capture_path
