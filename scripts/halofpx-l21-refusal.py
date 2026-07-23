#!/usr/bin/env python3
"""Exercise one real early RPC allocation refusal under the L21 collector."""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

from halofpx_l21_contract import (
    ContractError,
    EvidenceCollector,
    SshRunner,
    load_manifest,
    production_snapshot,
)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--manifest", required=True, type=Path)
    args = parser.parse_args()
    manifest = load_manifest(args.manifest)
    runner = SshRunner()
    collector = EvidenceCollector(runner, manifest)
    failure = ""
    archive = None
    cleanup_failure = ""
    worker_identity = None
    canary_identity = None
    try:
        if collector.path_exists(manifest.canary_host, manifest.retained_archive):
            raise ContractError("retained archive path already exists")
        # Bind production before the first disposable filesystem/unit/process mutation.
        before = production_snapshot(runner)
        evidence_root = manifest.paths[manifest.canary_host]["evidence_root"]
        collector.required(manifest.canary_host, ["install", "-d", "-m", "700", evidence_root])
        collector.write("production-before.json", (json.dumps(before, indent=2, sort_keys=True) + "\n").encode())
        collector.validate_executables()
        worker_unit = manifest.worker_units[0]
        worker_command = [
            manifest.executables["worker"], "--host", "10.44.0.1",
            "--port", str(manifest.worker_port), "--device", "ROCm0",
        ]
        worker_identity = collector.begin_unit(manifest.worker_host, worker_unit, worker_command)
        collector.required(manifest.canary_host, [
            "python3", manifest.executables["readiness"],
            "--endpoint", f"10.44.0.1:{manifest.worker_port}",
            "--timeout-seconds", "60", "--attempt-timeout-seconds", "2",
            "--expect-feature-off",
        ])
        canary_unit = manifest.canary_units[0]
        canary_command = [
            manifest.executables["canary"], "--endpoint",
            f"10.44.0.1:{manifest.worker_port}",
        ]
        canary_identity = collector.begin_unit(manifest.canary_host, canary_unit, canary_command)
        collector.wait_unit(canary_identity, 30)
        canary_exit = collector.finish_unit(canary_identity, require_refusal=True)
        if canary_exit["status"] != 23 or canary_exit["result"] != "exit-code":
            raise ContractError(f"allocation refusal child disposition mismatch: {canary_exit}")
        collector.finish_unit(worker_identity, require_refusal=True)
        after = production_snapshot(runner)
        collector.write("production-after.json", (json.dumps(after, indent=2, sort_keys=True) + "\n").encode())
        if before != after:
            raise ContractError("production before/after snapshot mismatch")
        collector.write("result.json", (json.dumps({
            "schema": "halofpx.l21.real-refusal.v1",
            "outcome": "pass",
            "worker": worker_identity.pid,
            "canary": canary_identity.pid,
            "canary_exit": canary_exit,
            "production_equal": True,
        }, indent=2, sort_keys=True) + "\n").encode())
    except Exception as exc:
        failure = str(exc)
        try:
            evidence_root = manifest.paths[manifest.canary_host]["evidence_root"]
            if collector.path_exists(manifest.canary_host, evidence_root):
                collector.write("failure.json", (json.dumps({
                    "schema": "halofpx.l21.failure.v1", "error": failure,
                }, sort_keys=True) + "\n").encode())
        except Exception as evidence_exc:
            failure += f"; failure-evidence={evidence_exc}"
    finally:
        runtime_cleanup = None
        try:
            runtime_cleanup = collector.cleanup_runtime()
            if runtime_cleanup["errors"]:
                raise ContractError(f"cleanup verification failed: {runtime_cleanup['errors']}")
        except Exception as cleanup_exc:
            cleanup_failure = str(cleanup_exc)
            failure = (failure + "; " if failure else "") + f"cleanup={cleanup_failure}"
        try:
            evidence_root = manifest.paths[manifest.canary_host]["evidence_root"]
            if collector.path_exists(manifest.canary_host, evidence_root):
                archive = collector.finalize_archive({
                    "outcome": "pass" if not failure else "non_promotable",
                    "error": failure,
                    "runtime_cleanup": runtime_cleanup,
                })
            else:
                raise ContractError("evidence root absent on finalization path")
        except Exception as archive_exc:
            failure = (failure + "; " if failure else "") + f"archive={archive_exc}"
    if archive:
        print(json.dumps({"archive": archive, "error": failure}, sort_keys=True))
    if failure:
        print(f"L21 refusal fixture failed: {failure}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
