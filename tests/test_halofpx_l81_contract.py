import hashlib
import importlib.util
from pathlib import Path
import sys

import pytest


ROOT = Path(__file__).parents[1]
SCRIPT = ROOT / "scripts" / "halofpx-production-transition.py"
SPEC = importlib.util.spec_from_file_location("l81_transition", SCRIPT)
TRANSITION = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = TRANSITION
SPEC.loader.exec_module(TRANSITION)
HELPER = ROOT / "scripts" / "halofpx_server_authority_harvest.py"
WORKER_HELPER = (
    "/var/tmp/halofpx-l48-source-nimo1/scripts/"
    "halofpx_server_authority_harvest.py"
)


def manifest(path=WORKER_HELPER, digest=None):
    return {
        "worker_host": "nimo-1",
        "worker_units": [],
        "executables": {"server_authority_harvester": path},
        "executable_sha256": {
            "server_authority_harvester":
                digest or hashlib.sha256(HELPER.read_bytes()).hexdigest()
        },
        "key_paths": {"nimo-1": "/var/tmp/key"},
    }


@pytest.mark.parametrize("path,digest", [
    ("/var/tmp/halofpx-l48-source-nimo2/scripts/"
     "halofpx_server_authority_harvest.py", None),
    (WORKER_HELPER, "0" * 64),
])
def test_wrong_host_or_stale_manifest_refuses(tmp_path, path, digest):
    result = TRANSITION.harvest_server_authority_finally(
        manifest(path, digest), {}, tmp_path, object())
    assert result["status"] == "error"
    assert "manifest identity mismatch" in result["reason"]


def test_manifest_has_explicit_worker_helper_binding():
    import json
    value = json.loads(
        (ROOT / "scripts" / "halofpx-l77-primary-manifest.json").read_text())
    assert value["executables"]["server_authority_harvester"] == WORKER_HELPER
    assert value["executable_sha256"]["server_authority_harvester"] == (
        hashlib.sha256(HELPER.read_bytes()).hexdigest())


def test_production_emits_same_retained_result_before_abort():
    source = (ROOT / "src" / "llama-context.cpp").read_text()
    seam = source.index(
        "auto fail_composed =")
    recorder = source.index(
        "llama_halofpx_record_composed_failure_and_abort(", seam)
    emit = source.index("[halofpx-composed-failure]", recorder)
    abort = source.index("halofpx_execution_authority_abort();", emit)
    assert seam < recorder < emit < abort
    for branch in (
        "l44_mutable_commit_", "l42_scheduler_finalize",
        "l40_graph_result_reconcile", "l44_session_finalize",
        "rpc_execution_disarm",
    ):
        assert branch in source
