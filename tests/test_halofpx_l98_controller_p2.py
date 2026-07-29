import importlib.util
import json
import sys
from pathlib import Path

import pytest


SCRIPT = Path(__file__).parents[1] / "scripts" / "halofpx-l13-primary-retry.py"
SPEC = importlib.util.spec_from_file_location("halofpx_l98_primary_retry", SCRIPT)
assert SPEC and SPEC.loader
retry = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = retry
SPEC.loader.exec_module(retry)


def test_total_result_parser_accepts_empty_and_nonempty_values():
    assert retry.parse_canonical_result_line(
        "mode=restore prompt_chunk_sizes= tokens=9283,"
    ) == {
        "mode": "restore",
        "prompt_chunk_sizes": "",
        "tokens": "9283,",
    }
    assert retry.parse_canonical_result_line(
        "mode=capture prompt_chunk_sizes=512,512 tokens=21549,"
    )["prompt_chunk_sizes"] == "512,512"


@pytest.mark.parametrize("line", [
    "",
    " mode=restore",
    "mode=restore ",
    "mode=restore  tokens=1,",
    "mode=restore\tokens=1,",
    "mode=restore\n",
    "mode",
    "=restore",
    "Mode=restore",
    "mode=restore mode=capture",
    "mode=restore bad-key=value",
    "mode=restore value=has\tcontrol",
])
def test_total_result_parser_refuses_noncanonical_input(line):
    with pytest.raises(retry.CanaryError):
        retry.parse_canonical_result_line(line)


def test_retained_l97_line_matches_independently_verified_json():
    operations = [
        json.loads(line)
        for line in (
            Path(__file__).parents[1]
            / "docs/halofpx/evidence/l97-attempt/child/ssh-operations.jsonl"
        ).read_text(encoding="utf-8").splitlines()
    ]
    emitted = next(item["stdout"] for item in operations if item["sequence"] == 505)
    durable = json.loads(
        next(item["stdout"] for item in operations if item["sequence"] == 507))
    assert retry.output_fields(emitted) == durable
    assert durable["prompt_chunk_sizes"] == ""


def test_exact_map_comparison_detects_extra_and_missing_fields():
    expected = {"mode": "restore", "prompt_chunk_sizes": "", "tokens": "1,"}
    assert retry.parse_canonical_result_line(
        "mode=restore prompt_chunk_sizes= tokens=1,") == expected
    assert retry.parse_canonical_result_line(
        "mode=restore tokens=1,") != expected
    assert retry.parse_canonical_result_line(
        "mode=restore prompt_chunk_sizes= tokens=1, extra=x") != expected


@pytest.mark.parametrize("text", [
    "mode=restore tokens=1,\nmode=restore tokens=1,\n",
    "mode=restore tokens=1,\nmode=capture tokens=2,\n",
    "mode restore tokens=1,\nmode=restore tokens=1,\n",
])
def test_output_fields_refuses_duplicate_or_malformed_result_lines(text):
    with pytest.raises(retry.CanaryError):
        retry.output_fields(text)


def running_props(**changes):
    props = {
        "InvocationID": "a" * 32,
        "MainPID": "42",
        "ExecMainPID": "42",
        "ExecMainCode": "0",
        "ExecMainStatus": "0",
        "Result": "success",
        "ActiveState": "active",
        "SubState": "running",
    }
    props.update(changes)
    return props


def success_props(**changes):
    props = running_props(
        MainPID="0", ExecMainCode="1", ActiveState="active",
        SubState="exited")
    props.update(changes)
    return props


def test_restore_terminal_status_accepts_only_exact_success():
    assert retry.restore_terminal_status(running_props(), "a" * 32, 42) is None
    assert retry.restore_terminal_status(success_props(), "a" * 32, 42) == 0


@pytest.mark.parametrize("changes", [
    {"InvocationID": "b" * 32},
    {"MainPID": "41"},
    {"ExecMainPID": "41"},
    {"ExecMainCode": ""},
    {"ExecMainCode": "0"},
    {"ExecMainStatus": "1"},
    {"Result": "exit-code"},
    {"ActiveState": "inactive"},
    {"SubState": "dead"},
])
def test_restore_terminal_status_refuses_changed_missing_or_contradictory(changes):
    with pytest.raises(retry.CanaryError):
        retry.restore_terminal_status(success_props(**changes), "a" * 32, 42)


def test_restore_terminal_status_accepts_exact_failure_for_collection():
    props = success_props(
        ActiveState="failed", SubState="failed", ExecMainCode="1",
        ExecMainStatus="2", Result="exit-code")
    assert retry.restore_terminal_status(props, "a" * 32, 42) == 2
    for changes in (
        {"SubState": "exited"},
        {"Result": "success"},
        {"ExecMainStatus": "0"},
    ):
        with pytest.raises(retry.CanaryError):
            retry.restore_terminal_status(
                {**props, **changes}, "a" * 32, 42)


def test_restore_launch_and_collection_keep_terminal_identity():
    launch = retry.restore_canary_launch_argv("halofpx-l48-canary-restore")
    assert "--property=RemainAfterExit=yes" in launch
    source = SCRIPT.read_text(encoding="utf-8")
    terminal = source[source.index("restore_status = None"):source.index(
        "if restore_status != 0:")]
    assert "restore_terminal_status(" in terminal
    assert "ExecMainCode" in terminal
    assert "ExecMainStatus" in terminal
    assert "Result" in terminal
    assert "collect_disposable_unit_evidence(" in terminal
    assert terminal.index("collect_disposable_unit_evidence(") < source[
        source.index("restore_status = None"):].index(
            "stop_canary(restore_canary_unit)")


def test_ordinary_canary_and_restore_share_remain_after_exit_contract():
    assert "--property=RemainAfterExit=yes" in retry.canary_launch_argv(
        "capture-only", "halofpx-l48-canary-capture")
    assert "--property=RemainAfterExit=yes" in retry.restore_canary_launch_argv(
        "halofpx-l48-canary-restore")
