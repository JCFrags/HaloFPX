#!/usr/bin/env python3
"""One-shot L24 primary restored-state discriminator; controller child only."""

from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
import os
import re
import shlex
import signal
import subprocess
import sys
import threading
import time
from datetime import datetime, timezone
from pathlib import Path


NIMO1 = "nimo-1"
NIMO2 = "nimo-2"
PORT = 50184
WORKER_BIN = "/var/tmp/halofpx-l24-source-nimo1/build-l24/bin/rpc-server"
CANARY_BIN = "/var/tmp/halofpx-l24-source-nimo2/build-l24/bin/test-halofpx-distributed-state-canary"
READINESS_PROBE = "/var/tmp/halofpx-l24-source-nimo2/scripts/halofpx_rpc_readiness.py"
PLACEMENT_PROBE = "/var/tmp/halofpx-l24-source-nimo2/build-l24/bin/test-halofpx-placement-probe"
EPOCH_RECEIPT = "/var/tmp/halofpx-l24-source-nimo2/scripts/halofpx_epoch_receipt.py"
COMPONENT_DIAGNOSTICS = ""
PLACEMENT_PROBE_SHA = "8f4796e3f0912afa614ec35ecdc48322228a4b355e32544aa5a36c3cfbd6267e"
EPOCH_RECEIPT_SHA = ""
COMPONENT_DIAGNOSTICS_SHA = ""
READINESS_PROBE_SHA = "54b546c38ea8d0a4ec273ae33b961090589756788acc1b173f22f1dba3e070da"
MODEL = (
    "/opt/llm-usb4-cluster/models/rcmorano_saricles-minimax-m2.7-reap-172b-a10b-rocmfpx/"
    "dba517197f2854f3d362529e13abddcdcad6c10b/"
    "saricles-MiniMax-M2.7-REAP-172B-A10B-Q6_0_ROCMFPX_AGENT.gguf"
)
MODEL_SHA = "96506ada918e60ca9a9cfde8a5437790e4453401a6a3e236e3f55e7bac3aaea6"
MODEL_BYTES = 159873097824
CANARY_SHA = "acdcbd664cab4c7dacf076448c162b32c8854f23143520921713b16a714e6e11"
WORKER_SHA = "74323c3f5d6cf1682320da8e9e4889bedd955a21c59e85f13da47370e14461cd"
PROMPT_SHA = "f20c7c7a4137de98b991f1bfe6de27e194a93c7257d9496944e312085923143f"
PROMPT = "/var/tmp/halofpx-l13-primary-20260721/prompt.txt"
REMOTE_EVIDENCE = "/var/tmp/halofpx-l24-primary-evidence"
COORDINATOR_ROOT = "/var/tmp/halofpx-l24-primary-coordinator"
WORKER_ROOT = "/var/tmp/halofpx-l24-primary-worker"
RENDEZVOUS_ROOT = "/var/tmp/halofpx-l24-primary-rendezvous"
CONTROL = "/var/tmp/halofpx-l24-primary-control.key"
WORKER_CONTROL = CONTROL
CHANNEL_KEY_OWNER = "connorb"
CHANNEL_KEY_BYTES = 130
CHANNEL_KEY_DIGEST_ENV = "HALOFPX_CHANNEL_KEY_SHA256"
CHECKPOINT = "421016c41e1af022aa65feef9c7b9329fdc1b49ff0b1c4df4aaad10cf13bf816"
ARTIFACT_DIR = f"{COORDINATOR_ROOT}/{CHECKPOINT}"
CACHE_TYPE_K = "q8_0"
CACHE_TYPE_V = "q8_0"
FLASH_ATTN = "on"
FIXTURE_QUALIFICATION = False
DIAGNOSTIC_ONLY = True
UNIT_PREFIX = "halofpx-l24-primary"
LIVE_RECAPTURE_DIAGNOSTICS = False
SEMANTIC_DIAGNOSTICS_ONLY = False
SEMANTIC_PATTERN = re.compile(
    r"^\[halofpx-semantic-provenance\] "
    r"(phase=(capture|restore) replay_count=([012]) replay_token=(-?\d+) "
    r"position_before=(-?\d+) position_after=(-?\d+) logits_count=(\d+) "
    r"logits_sha256=([0-9a-f]{64}) argmax_token=(-?\d+) "
    r"sampled_token=(-?\d+) logits_invalidated=([01])) "
    r"auth_tag=([0-9a-f]{64})$")
SEMANTIC_VERIFIER = ""
SEMANTIC_VERIFIER_SHA = ""
REPLAY_AUTHORITY_VERIFIER = ""
REPLAY_AUTHORITY_VERIFIER_SHA = ""

MODEL_DIGEST = MODEL_SHA
COMPATIBILITY = "a8f921ae8742823eac2942004094d1d11f47962bae0607c4b2fce6ce5a81c36f"
PLAN = "0268cc6071a8d78983f6351fe45d510e767d8cd26618a8bdffc972b6655f7967"
TOPOLOGY = "09b71fe40ae05c841a5be563f6e2b27ad2529d893b9420412e5280541ae53e1f"
PLACEMENT = "d4aa0d3c14a3bec4ba5de733e00b6447f79f94d5dbeda6e3593be74ce84f917e"
SSH_TRANSPORT = None
SSH_TRANSPORT_MODULE = None


class CanaryError(RuntimeError):
    pass


def configure_l28_fixture() -> None:
    global PORT, WORKER_BIN, CANARY_BIN, READINESS_PROBE, PLACEMENT_PROBE
    global EPOCH_RECEIPT, EPOCH_RECEIPT_SHA
    global MODEL, MODEL_SHA, MODEL_BYTES, PROMPT, PROMPT_SHA
    global REMOTE_EVIDENCE, COORDINATOR_ROOT, WORKER_ROOT, RENDEZVOUS_ROOT
    global CONTROL, WORKER_CONTROL, CACHE_TYPE_K, CACHE_TYPE_V, FLASH_ATTN
    global FIXTURE_QUALIFICATION, CANARY_SHA, WORKER_SHA, READINESS_PROBE_SHA
    global PLACEMENT_PROBE_SHA, MODEL_DIGEST, ARTIFACT_DIR
    global UNIT_PREFIX
    PORT = 50188
    WORKER_BIN = "/var/tmp/halofpx-l28-source-nimo1/build-l28/bin/rpc-server"
    CANARY_BIN = (
        "/var/tmp/halofpx-l28-source-nimo2/build-l28/bin/"
        "test-halofpx-distributed-state-canary")
    READINESS_PROBE = (
        "/var/tmp/halofpx-l28-source-nimo2/scripts/halofpx_rpc_readiness.py")
    PLACEMENT_PROBE = (
        "/var/tmp/halofpx-l28-source-nimo2/build-l28/bin/"
        "test-halofpx-placement-probe")
    EPOCH_RECEIPT = (
        "/var/tmp/halofpx-l28-source-nimo2/scripts/halofpx_epoch_receipt.py")
    MODEL = (
        "/var/tmp/halofpx-qualification/l14q-t01-20260719-nimo2/"
        "build-cpu/tinyllamas/stories15M-q4_0.gguf")
    MODEL_SHA = "66967fbece6dbe97886593fdbb73589584927e29119ec31f08090732d1861739"
    MODEL_BYTES = 19077344
    MODEL_DIGEST = MODEL_SHA
    PROMPT = "/var/tmp/halofpx-l13-retry-a2-20260721/prompt-1129.txt"
    PROMPT_SHA = "326cb4971a99fe8588fffc8635f57144173a16295a2375c3e1eb28240182f81d"
    REMOTE_EVIDENCE = "/var/tmp/halofpx-l28-evidence"
    COORDINATOR_ROOT = "/var/tmp/halofpx-l28-coordinator"
    WORKER_ROOT = "/var/tmp/halofpx-l28-worker"
    RENDEZVOUS_ROOT = "/var/tmp/halofpx-l28-rendezvous"
    CONTROL = "/var/tmp/halofpx-l28-control.key"
    ARTIFACT_DIR = f"{COORDINATOR_ROOT}/{CHECKPOINT}"
    WORKER_CONTROL = CONTROL
    CACHE_TYPE_K = "f16"
    CACHE_TYPE_V = "f16"
    FLASH_ATTN = "off"
    FIXTURE_QUALIFICATION = True
    UNIT_PREFIX = "halofpx-l28"
    # Frozen after the exact L28 source is built on both hosts.
    CANARY_SHA = os.environ.get("HALOFPX_L28_CANARY_SHA256", "")
    WORKER_SHA = os.environ.get("HALOFPX_L28_WORKER_SHA256", "")
    READINESS_PROBE_SHA = os.environ.get("HALOFPX_L28_READINESS_SHA256", "")
    PLACEMENT_PROBE_SHA = os.environ.get("HALOFPX_L28_PLACEMENT_SHA256", "")
    EPOCH_RECEIPT_SHA = os.environ.get("HALOFPX_L28_EPOCH_RECEIPT_SHA256", "")


def configure_l29_primary() -> None:
    global PORT, WORKER_BIN, CANARY_BIN, READINESS_PROBE, PLACEMENT_PROBE
    global EPOCH_RECEIPT, EPOCH_RECEIPT_SHA, REMOTE_EVIDENCE
    global COORDINATOR_ROOT, WORKER_ROOT, RENDEZVOUS_ROOT, CONTROL
    global WORKER_CONTROL, ARTIFACT_DIR, CANARY_SHA, WORKER_SHA
    global READINESS_PROBE_SHA, PLACEMENT_PROBE_SHA, UNIT_PREFIX
    global CACHE_TYPE_K, CACHE_TYPE_V, FLASH_ATTN, FIXTURE_QUALIFICATION
    PORT = 50189
    WORKER_BIN = "/var/tmp/halofpx-l29-source-nimo1/build-l29/bin/rpc-server"
    CANARY_BIN = (
        "/var/tmp/halofpx-l29-source-nimo2/build-l29/bin/"
        "test-halofpx-distributed-state-canary")
    READINESS_PROBE = (
        "/var/tmp/halofpx-l29-source-nimo2/scripts/halofpx_rpc_readiness.py")
    PLACEMENT_PROBE = (
        "/var/tmp/halofpx-l29-source-nimo2/build-l29/bin/"
        "test-halofpx-placement-probe")
    EPOCH_RECEIPT = (
        "/var/tmp/halofpx-l29-source-nimo2/scripts/halofpx_epoch_receipt.py")
    REMOTE_EVIDENCE = "/var/tmp/halofpx-l29-evidence"
    COORDINATOR_ROOT = "/var/tmp/halofpx-l29-coordinator"
    WORKER_ROOT = "/var/tmp/halofpx-l29-worker"
    RENDEZVOUS_ROOT = "/var/tmp/halofpx-l29-rendezvous"
    CONTROL = "/var/tmp/halofpx-l29-control.key"
    WORKER_CONTROL = CONTROL
    ARTIFACT_DIR = f"{COORDINATOR_ROOT}/{CHECKPOINT}"
    UNIT_PREFIX = "halofpx-l29-primary"
    CACHE_TYPE_K = "q8_0"
    CACHE_TYPE_V = "q8_0"
    FLASH_ATTN = "on"
    FIXTURE_QUALIFICATION = False
    CANARY_SHA = os.environ.get("HALOFPX_L28_CANARY_SHA256", "")
    WORKER_SHA = os.environ.get("HALOFPX_L28_WORKER_SHA256", "")
    READINESS_PROBE_SHA = os.environ.get("HALOFPX_L28_READINESS_SHA256", "")
    PLACEMENT_PROBE_SHA = os.environ.get("HALOFPX_L28_PLACEMENT_SHA256", "")
    EPOCH_RECEIPT_SHA = os.environ.get("HALOFPX_L28_EPOCH_RECEIPT_SHA256", "")


def configure_l31_primary() -> None:
    configure_l29_primary()
    global PORT, WORKER_BIN, CANARY_BIN, READINESS_PROBE, PLACEMENT_PROBE
    global EPOCH_RECEIPT, REMOTE_EVIDENCE, COORDINATOR_ROOT, WORKER_ROOT
    global RENDEZVOUS_ROOT, CONTROL, WORKER_CONTROL, ARTIFACT_DIR, UNIT_PREFIX
    global COMPONENT_DIAGNOSTICS, COMPONENT_DIAGNOSTICS_SHA
    PORT = 50191
    WORKER_BIN = "/var/tmp/halofpx-l31-source-nimo1/build-l31/bin/rpc-server"
    CANARY_BIN = (
        "/var/tmp/halofpx-l31-source-nimo2/build-l31/bin/"
        "test-halofpx-distributed-state-canary")
    READINESS_PROBE = (
        "/var/tmp/halofpx-l31-source-nimo2/scripts/halofpx_rpc_readiness.py")
    PLACEMENT_PROBE = (
        "/var/tmp/halofpx-l31-source-nimo2/build-l31/bin/"
        "test-halofpx-placement-probe")
    EPOCH_RECEIPT = (
        "/var/tmp/halofpx-l31-source-nimo2/scripts/halofpx_epoch_receipt.py")
    COMPONENT_DIAGNOSTICS = (
        "/var/tmp/halofpx-l31-source-nimo1/scripts/"
        "halofpx_state_component_diagnostics.py")
    COMPONENT_DIAGNOSTICS_SHA = os.environ.get(
        "HALOFPX_L31_COMPONENT_DIAGNOSTICS_SHA256", "")
    REMOTE_EVIDENCE = "/var/tmp/halofpx-l31-evidence"
    COORDINATOR_ROOT = "/var/tmp/halofpx-l31-coordinator"
    WORKER_ROOT = "/var/tmp/halofpx-l31-worker"
    RENDEZVOUS_ROOT = "/var/tmp/halofpx-l31-rendezvous"
    CONTROL = "/var/tmp/halofpx-l31-control.key"
    WORKER_CONTROL = CONTROL
    ARTIFACT_DIR = f"{COORDINATOR_ROOT}/{CHECKPOINT}"
    UNIT_PREFIX = "halofpx-l31-primary"


def configure_l32_fixture() -> None:
    configure_l28_fixture()
    global PORT, WORKER_BIN, CANARY_BIN, READINESS_PROBE, PLACEMENT_PROBE
    global EPOCH_RECEIPT, REMOTE_EVIDENCE, COORDINATOR_ROOT, WORKER_ROOT
    global RENDEZVOUS_ROOT, CONTROL, WORKER_CONTROL, ARTIFACT_DIR, UNIT_PREFIX
    global COMPONENT_DIAGNOSTICS, COMPONENT_DIAGNOSTICS_SHA
    global LIVE_RECAPTURE_DIAGNOSTICS
    PORT = 50232
    WORKER_BIN = "/var/tmp/halofpx-l32-build/bin/rpc-server"
    CANARY_BIN = (
        "/var/tmp/halofpx-l32-build/bin/"
        "test-halofpx-distributed-state-canary")
    READINESS_PROBE = (
        "/var/tmp/halofpx-l32-source/scripts/halofpx_rpc_readiness.py")
    PLACEMENT_PROBE = (
        "/var/tmp/halofpx-l32-build/bin/"
        "test-halofpx-placement-probe")
    EPOCH_RECEIPT = (
        "/var/tmp/halofpx-l32-source/scripts/halofpx_epoch_receipt.py")
    COMPONENT_DIAGNOSTICS = (
        "/var/tmp/halofpx-l32-source/scripts/"
        "halofpx_state_component_diagnostics.py")
    COMPONENT_DIAGNOSTICS_SHA = os.environ.get(
        "HALOFPX_L32_COMPONENT_DIAGNOSTICS_SHA256", "")
    REMOTE_EVIDENCE = "/var/tmp/halofpx-l32-evidence"
    COORDINATOR_ROOT = "/var/tmp/halofpx-l32-coordinator"
    WORKER_ROOT = "/var/tmp/halofpx-l32-worker"
    RENDEZVOUS_ROOT = "/var/tmp/halofpx-l32-rendezvous"
    CONTROL = "/var/tmp/halofpx-l32-control.key"
    WORKER_CONTROL = CONTROL
    ARTIFACT_DIR = f"{COORDINATOR_ROOT}/{CHECKPOINT}"
    UNIT_PREFIX = "halofpx-l32"
    LIVE_RECAPTURE_DIAGNOSTICS = True


def configure_l33_primary() -> None:
    configure_l31_primary()
    global PORT, WORKER_BIN, CANARY_BIN, READINESS_PROBE, PLACEMENT_PROBE
    global EPOCH_RECEIPT, REMOTE_EVIDENCE, COORDINATOR_ROOT, WORKER_ROOT
    global RENDEZVOUS_ROOT, CONTROL, WORKER_CONTROL, ARTIFACT_DIR, UNIT_PREFIX
    global COMPONENT_DIAGNOSTICS, COMPONENT_DIAGNOSTICS_SHA
    global LIVE_RECAPTURE_DIAGNOSTICS
    PORT = 50233
    WORKER_BIN = "/var/tmp/halofpx-l33-source-nimo1/build-l33/bin/rpc-server"
    CANARY_BIN = (
        "/var/tmp/halofpx-l33-source-nimo2/build-l33/bin/"
        "test-halofpx-distributed-state-canary")
    READINESS_PROBE = (
        "/var/tmp/halofpx-l33-source-nimo2/scripts/halofpx_rpc_readiness.py")
    PLACEMENT_PROBE = (
        "/var/tmp/halofpx-l33-source-nimo2/build-l33/bin/"
        "test-halofpx-placement-probe")
    EPOCH_RECEIPT = (
        "/var/tmp/halofpx-l33-source-nimo2/scripts/halofpx_epoch_receipt.py")
    COMPONENT_DIAGNOSTICS = (
        "/var/tmp/halofpx-l33-source-nimo1/scripts/"
        "halofpx_state_component_diagnostics.py")
    COMPONENT_DIAGNOSTICS_SHA = os.environ.get(
        "HALOFPX_L33_COMPONENT_DIAGNOSTICS_SHA256", "")
    REMOTE_EVIDENCE = "/var/tmp/halofpx-l33-evidence"
    COORDINATOR_ROOT = "/var/tmp/halofpx-l33-coordinator"
    WORKER_ROOT = "/var/tmp/halofpx-l33-worker"
    RENDEZVOUS_ROOT = "/var/tmp/halofpx-l33-rendezvous"
    CONTROL = "/var/tmp/halofpx-l33-control.key"
    WORKER_CONTROL = CONTROL
    ARTIFACT_DIR = f"{COORDINATOR_ROOT}/{CHECKPOINT}"
    UNIT_PREFIX = "halofpx-l33-primary"
    LIVE_RECAPTURE_DIAGNOSTICS = True


def configure_l34_fixture() -> None:
    configure_l28_fixture()
    global PORT, WORKER_BIN, CANARY_BIN, READINESS_PROBE, PLACEMENT_PROBE
    global EPOCH_RECEIPT, REMOTE_EVIDENCE, COORDINATOR_ROOT, WORKER_ROOT
    global RENDEZVOUS_ROOT, CONTROL, WORKER_CONTROL, ARTIFACT_DIR, UNIT_PREFIX
    global COMPONENT_DIAGNOSTICS, COMPONENT_DIAGNOSTICS_SHA
    global SEMANTIC_DIAGNOSTICS_ONLY
    global SEMANTIC_VERIFIER, SEMANTIC_VERIFIER_SHA
    PORT = 50234
    WORKER_BIN = "/var/tmp/halofpx-l34-final-build/bin/rpc-server"
    CANARY_BIN = (
        "/var/tmp/halofpx-l34-final-build/bin/"
        "test-halofpx-distributed-state-canary")
    READINESS_PROBE = (
        "/var/tmp/halofpx-l34-final-source/scripts/halofpx_rpc_readiness.py")
    PLACEMENT_PROBE = (
        "/var/tmp/halofpx-l34-final-build/bin/"
        "test-halofpx-placement-probe")
    EPOCH_RECEIPT = (
        "/var/tmp/halofpx-l34-final-source/scripts/halofpx_epoch_receipt.py")
    COMPONENT_DIAGNOSTICS = (
        "/var/tmp/halofpx-l34-final-source/scripts/"
        "halofpx_state_component_diagnostics.py")
    COMPONENT_DIAGNOSTICS_SHA = os.environ.get(
        "HALOFPX_L34_COMPONENT_DIAGNOSTICS_SHA256", "")
    SEMANTIC_VERIFIER = (
        "/var/tmp/halofpx-l34-final-source/scripts/"
        "halofpx_semantic_provenance.py")
    SEMANTIC_VERIFIER_SHA = os.environ.get(
        "HALOFPX_L34_SEMANTIC_VERIFIER_SHA256", "")
    REMOTE_EVIDENCE = "/var/tmp/halofpx-l34-evidence"
    COORDINATOR_ROOT = "/var/tmp/halofpx-l34-coordinator"
    WORKER_ROOT = "/var/tmp/halofpx-l34-worker"
    RENDEZVOUS_ROOT = "/var/tmp/halofpx-l34-rendezvous"
    CONTROL = "/var/tmp/halofpx-l34-control.key"
    WORKER_CONTROL = CONTROL
    ARTIFACT_DIR = f"{COORDINATOR_ROOT}/{CHECKPOINT}"
    UNIT_PREFIX = "halofpx-l34"
    SEMANTIC_DIAGNOSTICS_ONLY = True


def configure_l35_fixture() -> None:
    configure_l34_fixture()
    global PORT, WORKER_BIN, CANARY_BIN, READINESS_PROBE, PLACEMENT_PROBE
    global EPOCH_RECEIPT, REMOTE_EVIDENCE, COORDINATOR_ROOT, WORKER_ROOT
    global RENDEZVOUS_ROOT, CONTROL, WORKER_CONTROL, ARTIFACT_DIR, UNIT_PREFIX
    global COMPONENT_DIAGNOSTICS, COMPONENT_DIAGNOSTICS_SHA
    global SEMANTIC_VERIFIER, SEMANTIC_VERIFIER_SHA
    global REPLAY_AUTHORITY_VERIFIER, REPLAY_AUTHORITY_VERIFIER_SHA
    PORT = 50235
    WORKER_BIN = "/var/tmp/halofpx-l35-build/build/bin/rpc-server"
    CANARY_BIN = (
        "/var/tmp/halofpx-l35-build/build/bin/"
        "test-halofpx-distributed-state-canary")
    READINESS_PROBE = (
        "/var/tmp/halofpx-l35-build/scripts/halofpx_rpc_readiness.py")
    PLACEMENT_PROBE = (
        "/var/tmp/halofpx-l35-build/build/bin/"
        "test-halofpx-placement-probe")
    EPOCH_RECEIPT = (
        "/var/tmp/halofpx-l35-build/scripts/halofpx_epoch_receipt.py")
    COMPONENT_DIAGNOSTICS = (
        "/var/tmp/halofpx-l35-build/scripts/"
        "halofpx_state_component_diagnostics.py")
    COMPONENT_DIAGNOSTICS_SHA = os.environ.get(
        "HALOFPX_L35_COMPONENT_DIAGNOSTICS_SHA256", "")
    SEMANTIC_VERIFIER = (
        "/var/tmp/halofpx-l35-build/scripts/"
        "halofpx_semantic_provenance.py")
    SEMANTIC_VERIFIER_SHA = os.environ.get(
        "HALOFPX_L35_SEMANTIC_VERIFIER_SHA256", "")
    REPLAY_AUTHORITY_VERIFIER = (
        "/var/tmp/halofpx-l35-build/scripts/"
        "halofpx_replay_authority.py")
    REPLAY_AUTHORITY_VERIFIER_SHA = os.environ.get(
        "HALOFPX_L35_REPLAY_AUTHORITY_VERIFIER_SHA256", "")
    REMOTE_EVIDENCE = "/var/tmp/halofpx-l35-evidence"
    COORDINATOR_ROOT = "/var/tmp/halofpx-l35-coordinator"
    WORKER_ROOT = "/var/tmp/halofpx-l35-worker"
    RENDEZVOUS_ROOT = "/var/tmp/halofpx-l35-rendezvous"
    CONTROL = "/var/tmp/halofpx-l35-control.key"
    WORKER_CONTROL = CONTROL
    ARTIFACT_DIR = f"{COORDINATOR_ROOT}/{CHECKPOINT}"
    UNIT_PREFIX = "halofpx-l35"


def semantic_env_args() -> list[str]:
    if os.environ.get("HALOFPX_SEMANTIC_DIAGNOSTICS") != "1":
        return []
    result = [
        "--setenv=HALOFPX_SEMANTIC_DIAGNOSTICS=1",
        "--setenv=HALOFPX_REPLAY_AUTHORITY_DIAGNOSTICS=1",
    ]
    replay = os.environ.get("HALOFPX_SEMANTIC_REPLAY_COUNT")
    if replay is not None:
        if replay not in {"0", "1", "2"}:
            raise CanaryError("semantic replay count must be exactly 0, 1, or 2")
        result.append(f"--setenv=HALOFPX_SEMANTIC_REPLAY_COUNT={replay}")
    invalidate = os.environ.get("HALOFPX_SEMANTIC_INVALIDATE_LOGITS")
    if invalidate is not None:
        if invalidate != "restore":
            raise CanaryError(
                "semantic logits invalidation is admitted only for restore")
        result.append("--setenv=HALOFPX_SEMANTIC_INVALIDATE_LOGITS=restore")
    graph_reset = os.environ.get("HALOFPX_REPLAY_CANONICAL_GRAPH_RESET")
    if graph_reset is not None:
        if graph_reset != "1":
            raise CanaryError("canonical replay graph reset must be exactly 1")
        result.append("--setenv=HALOFPX_REPLAY_CANONICAL_GRAPH_RESET=1")
    return result


def require_authenticated_semantic_provenance(
        capture_log: str, restore_log: str) -> dict[str, dict[str, object]]:
    if SSH_TRANSPORT is None:
        raise CanaryError("bounded SSH transport is unavailable")
    if not SEMANTIC_VERIFIER or not SEMANTIC_VERIFIER_SHA:
        raise CanaryError("semantic verifier authority is unavailable")
    actual = ssh(NIMO2, "sha256sum", SEMANTIC_VERIFIER).stdout.split()[0]
    if actual != SEMANTIC_VERIFIER_SHA:
        raise CanaryError("semantic verifier hash mismatch")
    result: dict[str, dict[str, object]] = {}
    for expected_phase, text_value in (
            ("capture", capture_log), ("restore", restore_log)):
        matches = [
            match for line in text_value.splitlines()
            if (match := SEMANTIC_PATTERN.fullmatch(line)) is not None
        ]
        if len(matches) != 1 or matches[0].group(2) != expected_phase:
            raise CanaryError(
                f"{expected_phase}: semantic provenance is missing or ambiguous")
        match = matches[0]
        canonical, auth_tag = match.group(1), match.group(12)
        verified = SSH_TRANSPORT.run_stdin(
            NIMO2,
            [
                "python3", SEMANTIC_VERIFIER,
                "--key-file", CONTROL,
                "--expected-tag", auth_tag,
            ],
            canonical.encode("ascii"),
            operation="evidence",
        )
        if (
            verified.returncode != 0
            or verified.stdout.strip() != auth_tag
        ):
            raise CanaryError(
                f"{expected_phase}: semantic provenance authentication failed")
        result[expected_phase] = {
            "replay_count": int(match.group(3)),
            "replay_token": int(match.group(4)),
            "position_before": int(match.group(5)),
            "position_after": int(match.group(6)),
            "logits_count": int(match.group(7)),
            "logits_sha256": match.group(8),
            "argmax_token": int(match.group(9)),
            "sampled_token": int(match.group(10)),
            "logits_invalidated": match.group(11) == "1",
            "auth_tag": auth_tag,
        }
    return result


def require_authenticated_replay_authority(
        capture_log: str, restore_log: str) -> dict[str, dict[str, object]]:
    if SSH_TRANSPORT is None:
        raise CanaryError("bounded SSH transport is unavailable")
    if not REPLAY_AUTHORITY_VERIFIER or not REPLAY_AUTHORITY_VERIFIER_SHA:
        raise CanaryError("replay-authority verifier is unavailable")
    actual = ssh(
        NIMO2, "sha256sum", REPLAY_AUTHORITY_VERIFIER).stdout.split()[0]
    if actual != REPLAY_AUTHORITY_VERIFIER_SHA:
        raise CanaryError("replay-authority verifier hash mismatch")
    result: dict[str, dict[str, object]] = {}
    prefix = "[halofpx-replay-authority] "
    for expected_phase, text_value in (
            ("capture", capture_log), ("restore", restore_log)):
        records = [
            line for line in text_value.splitlines() if line.startswith(prefix)
        ]
        if len(records) != 1:
            raise CanaryError(
                f"{expected_phase}: replay authority is missing or ambiguous")
        verified = SSH_TRANSPORT.run_stdin(
            NIMO2,
            [
                "python3", REPLAY_AUTHORITY_VERIFIER,
                "--key-file", CONTROL,
            ],
            records[0].encode("utf-8"),
            operation="evidence",
        )
        if verified.returncode != 0:
            raise CanaryError(
                f"{expected_phase}: replay authority verification failed")
        try:
            fields = json.loads(verified.stdout)
        except json.JSONDecodeError as exc:
            raise CanaryError(
                f"{expected_phase}: replay verifier output malformed") from exc
        if fields.get("phase") != expected_phase:
            raise CanaryError(
                f"{expected_phase}: replay verifier phase mismatch")
        result[expected_phase] = fields
    return result


def run(argv, *, timeout=900, check=True):
    result = subprocess.run(
        argv, text=True, encoding="utf-8", errors="replace",
        capture_output=True, timeout=timeout, check=False,
    )
    if check and result.returncode != 0:
        raise CanaryError(
            f"command failed ({result.returncode}): {argv!r}\n{result.stdout}\n{result.stderr}"
        )
    return result


def ssh(host, *argv, timeout=900, check=True):
    if SSH_TRANSPORT is None:
        raise CanaryError("bounded SSH transport is not initialized")
    remote_command = " ".join(shlex.quote(str(value)) for value in argv)
    operation = "model-session" if argv and argv[0] == "systemd-run" else (
        "hash" if argv and argv[0] == "sha256sum" else (
        "service-readiness" if argv and argv[0] in {"systemctl", "ss", "ps", "curl"} else
        "cleanup" if argv and argv[0] in {"rm", "stat", "find"} else
        "command"
    ))
    try:
        result = SSH_TRANSPORT.run(host, [remote_command], operation=operation)
    except Exception as exc:
        raise CanaryError(f"bounded SSH {operation} failure on {host}: {exc}") from exc
    completed = subprocess.CompletedProcess(
        ["ssh", host, remote_command], result.returncode, result.stdout, result.stderr)
    if check and completed.returncode != 0:
        raise CanaryError(
            f"command failed ({completed.returncode}): {argv!r}\n"
            f"{completed.stdout}\n{completed.stderr}")
    return completed


def initialize_ssh_transport(evidence_root: Path) -> None:
    global SSH_TRANSPORT, SSH_TRANSPORT_MODULE
    module_path = Path(__file__).with_name("halofpx-production-transition.py")
    spec = importlib.util.spec_from_file_location("halofpx_l25_transport", module_path)
    if spec is None or spec.loader is None:
        raise CanaryError("bounded SSH transport module is unavailable")
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    SSH_TRANSPORT_MODULE = module
    SSH_TRANSPORT = module.SshRunner(evidence_root)


def start_bounded_ssh_session(host: str, remote_command: str):
    if SSH_TRANSPORT_MODULE is None:
        raise CanaryError("bounded SSH transport is not initialized")
    creationflags = (
        subprocess.CREATE_NEW_PROCESS_GROUP if os.name == "nt" else 0)
    started_at = datetime.now(timezone.utc).isoformat()
    started_mono = time.monotonic()
    process = subprocess.Popen(
        [
            "ssh", "-o", "BatchMode=yes", "-o", "ConnectTimeout=10",
            "-o", "ConnectionAttempts=1", host, remote_command,
        ],
        text=True, encoding="utf-8", errors="replace",
        stdout=subprocess.PIPE, stderr=subprocess.PIPE,
        start_new_session=os.name != "nt",
        creationflags=creationflags,
    )
    try:
        job_handle = SSH_TRANSPORT_MODULE.SshRunner._create_windows_job(process)
    except Exception as exc:
        terminated, killed, detail = (
            SSH_TRANSPORT_MODULE.SshRunner._cleanup_setup_failure(process))
        SSH_TRANSPORT._record({
            "schema": "halofpx.ssh-operation.v1",
            "sequence": "model-session-setup",
            "host": host,
            "operation": "model-session",
            "argv": [remote_command],
            "started_at": started_at,
            "ended_at": datetime.now(timezone.utc).isoformat(),
            "duration_seconds": round(time.monotonic() - started_mono, 6),
            "deadline_seconds": SSH_TRANSPORT_MODULE.SSH_OPERATION_DEADLINES["model-session"],
            "pid": process.pid,
            "returncode": process.returncode,
            "timed_out": False,
            "term_sent": terminated,
            "kill_sent": killed,
            "failure_class": "process-group-setup",
            "stdout": "",
            "stderr": f"{type(exc).__name__}: {exc}; {detail}",
        })
        raise CanaryError(f"SSH session process-group setup failed: {exc}") from exc
    return process, job_handle


def terminate_bounded_ssh_session(process, job_handle) -> None:
    if SSH_TRANSPORT_MODULE is None:
        raise CanaryError("bounded SSH transport is not initialized")
    return SSH_TRANSPORT_MODULE.SshRunner._terminate_group(process, job_handle)


def write_log(root: Path, name: str, result) -> None:
    (root / name).write_text(result.stdout + result.stderr, encoding="utf-8", newline="\n")


def listener_pid(text: str, port: int) -> int:
    matches = []
    for line in text.splitlines():
        fields = line.split()
        if len(fields) >= 4 and fields[3].endswith(f":{port}"):
            matches.append(line)
    if len(matches) != 1:
        return 0
    match = re.search(r"pid=(\d+)", matches[0])
    return int(match.group(1)) if match else 0


def validate_provisioned_keys() -> str:
    expected = os.environ.get(CHANNEL_KEY_DIGEST_ENV, "")
    if not re.fullmatch(r"[0-9a-f]{64}", expected):
        raise CanaryError("controller channel key identity is absent or malformed")
    for host, path in ((NIMO1, WORKER_CONTROL), (NIMO2, CONTROL)):
        stat = ssh(host, "stat", "-c", "%F:%U:%a:%s", "--", path, check=False)
        if stat.returncode != 0:
            raise CanaryError(f"{host}: provisioned channel key is missing")
        fields = stat.stdout.strip().split(":")
        if fields != ["regular file", CHANNEL_KEY_OWNER, "600", str(CHANNEL_KEY_BYTES)]:
            raise CanaryError(f"{host}: provisioned channel key type/owner/mode/size mismatch")
        digest = ssh(host, "sha256sum", "--", path, check=False)
        actual = digest.stdout.split()[0] if digest.returncode == 0 and digest.stdout.split() else ""
        if actual != expected:
            raise CanaryError(f"{host}: provisioned channel key digest mismatch")
    return expected


def start_worker(local_state: bool, unit: str, evidence_root: Path | None = None) -> tuple[int, str, dict[str, object]]:
    command = [
        "systemd-run", "--user", f"--unit={unit}", "--property=RuntimeMaxSec=90min",
        "--setenv=GGML_RPC_DEBUG=1", "--setenv=HALOFPX_STATE_DIAGNOSTICS=1", WORKER_BIN,
        "--host", "10.44.0.1", "--port", str(PORT), "--device", "ROCm0",
    ]
    if local_state:
        command.extend([
            "--halofpx-local-state", "--halofpx-state-root", WORKER_ROOT,
            "--halofpx-state-key-file", WORKER_CONTROL,
            "--halofpx-state-rank", "1", "--halofpx-state-world", "2",
            "--halofpx-state-key-generation", "7",
        ])
    ssh(NIMO1, *command)
    probe_command = [
        "python3", READINESS_PROBE,
        "--endpoint", f"10.44.0.1:{PORT}",
        "--timeout-seconds", "120",
        "--attempt-timeout-seconds", "2",
        "--initial-backoff-seconds", "0.1",
        "--maximum-backoff-seconds", "1",
    ]
    if local_state:
        probe_command.extend([
            "--logical-rank", "1",
            "--world-size", "2",
            "--key-generation", "7",
            "--expected-channel-key-file", CONTROL,
        ])
    else:
        probe_command.append("--expect-feature-off")
    readiness = ssh(NIMO2, *probe_command, timeout=130, check=False)
    if readiness.returncode != 0:
        raise CanaryError(f"worker {unit} failed HaloFPX CAPS readiness: {readiness.stdout}{readiness.stderr}")
    try:
        readiness_result = json.loads(readiness.stdout)
    except json.JSONDecodeError as exc:
        raise CanaryError(f"worker {unit} returned malformed readiness evidence") from exc
    expected_result = readiness_result.get("admitted") is True if local_state else (
        readiness_result.get("admitted") is False and readiness_result.get("feature_off_confirmed") is True
    )
    if not expected_result or readiness_result.get("endpoint") != f"10.44.0.1:{PORT}":
        raise CanaryError(f"worker {unit} returned mismatched readiness evidence")
    placement = ssh(
        NIMO2, PLACEMENT_PROBE,
        "--hfx-expected-rpc-endpoint", f"10.44.0.1:{PORT}",
        "--rpc", f"10.44.0.1:{PORT}",
        "--device", "RPC0,ROCm0",
        "--split-mode", "layer",
        "--tensor-split", "1,1",
        "--n-gpu-layers", "999",
        timeout=30, check=False,
    )
    if placement.returncode != 0:
        raise CanaryError(f"worker {unit} failed pre-allocation placement authority: {placement.stdout}{placement.stderr}")
    try:
        placement_result = json.loads(placement.stdout)
    except json.JSONDecodeError as exc:
        raise CanaryError(f"worker {unit} returned malformed placement evidence") from exc
    if placement_result.get("admitted") is not True or placement_result.get("endpoint") != f"10.44.0.1:{PORT}":
        raise CanaryError(f"worker {unit} returned mismatched placement evidence")
    if evidence_root is not None:
        (evidence_root / f"{unit}-readiness.json").write_text(
            json.dumps(readiness_result, indent=2, sort_keys=True) + "\n", encoding="utf-8", newline="\n"
        )
        (evidence_root / f"{unit}-placement.json").write_text(
            json.dumps(placement_result, indent=2, sort_keys=True) + "\n", encoding="utf-8", newline="\n"
        )
    active = ssh(NIMO1, "systemctl", "--user", "is-active", f"{unit}.service", check=False)
    if active.stdout.strip() != "active":
        raise CanaryError(f"worker {unit} left active state after CAPS readiness")
    pid = int(ssh(NIMO1, "systemctl", "--user", "show", f"{unit}.service", "-p", "MainPID", "--value").stdout.strip())
    invocation_id = ssh(
        NIMO1, "systemctl", "--user", "show", f"{unit}.service", "-p", "InvocationID", "--value"
    ).stdout.strip()
    if not re.fullmatch(r"[0-9a-fA-F]{32}", invocation_id):
        raise CanaryError(f"worker {unit} has invalid InvocationID")
    listeners = ssh(NIMO1, "ss", "-H", "-ltnp", check=False).stdout
    if listener_pid(listeners, PORT) != pid:
        raise CanaryError(f"worker {unit} listener no longer matches MainPID after CAPS readiness")
    return pid, invocation_id.lower(), readiness_result


def stop_worker(unit: str) -> None:
    def stopped(require_port_closed: bool) -> tuple[bool, str]:
        show = ssh(
            NIMO1, "systemctl", "--user", "show", f"{unit}.service",
            "-p", "LoadState", "-p", "ActiveState", "-p", "SubState", "-p", "MainPID",
            check=False,
        ).stdout
        listeners = ssh(NIMO1, "ss", "-H", "-ltnp", check=False).stdout
        props = dict(line.split("=", 1) for line in show.splitlines() if "=" in line)
        return (
            props.get("ActiveState") == "inactive"
            and props.get("SubState") == "dead"
            and props.get("MainPID") == "0"
            and (not require_port_closed or listener_pid(listeners, PORT) == 0),
            repr(props),
        )

    # An already-absent epoch must not claim the listener owned by the other
    # admitted epoch. If this call performs the stop, it must close the port.
    already_stopped, _ = stopped(require_port_closed=False)
    if already_stopped:
        return
    ssh(NIMO1, "systemctl", "--user", "stop", f"{unit}.service", check=False)
    ssh(NIMO1, "systemctl", "--user", "reset-failed", f"{unit}.service", check=False)
    deadline = time.monotonic() + 30
    last = ""
    while time.monotonic() < deadline:
        is_stopped, last = stopped(require_port_closed=True)
        if is_stopped:
            return
        time.sleep(1)
    raise CanaryError(f"disposable worker cleanup not verified for {unit}: {last}")


def stop_canary(unit: str) -> None:
    ssh(NIMO2, "systemctl", "--user", "stop", f"{unit}.service", check=False)
    ssh(NIMO2, "systemctl", "--user", "reset-failed", f"{unit}.service", check=False)
    deadline = time.monotonic() + 30
    while time.monotonic() < deadline:
        show = ssh(
            NIMO2, "systemctl", "--user", "show", f"{unit}.service",
            "-p", "ActiveState", "-p", "SubState", "-p", "MainPID",
            check=False).stdout
        props = dict(line.split("=", 1) for line in show.splitlines() if "=" in line)
        if (
            props.get("ActiveState") == "inactive"
            and props.get("SubState") == "dead"
            and props.get("MainPID") == "0"
        ):
            return
        time.sleep(1)
    raise CanaryError(f"disposable coordinator cleanup not verified for {unit}")


def canary_argv(sequence: str, *, restore_gate: bool = False) -> list[str]:
    canary_command = [
        CANARY_BIN,
        "--hfx-mode", "capture",
        "--hfx-sequence", sequence,
        "--hfx-rendezvous-root", RENDEZVOUS_ROOT,
        "--hfx-artifact-root", COORDINATOR_ROOT,
        "--hfx-model-digest", MODEL_DIGEST,
        "--hfx-compatibility-root", COMPATIBILITY,
        "--hfx-plan-digest", PLAN,
        "--hfx-topology-digest", TOPOLOGY,
        "--hfx-placement-digest", PLACEMENT,
        "--hfx-checkpoint-digest", CHECKPOINT,
        "--hfx-control-file", CONTROL,
        "--hfx-expected-prompt-tokens", "1129",
        "--model", MODEL,
        "--rpc", f"10.44.0.1:{PORT}",
        "--device", "RPC0,ROCm0",
        "--split-mode", "layer",
        "--tensor-split", "1,1",
        "--n-gpu-layers", "999",
        "--fit", "off",
        "--no-mmap",
        "--direct-io",
        "--flash-attn", FLASH_ATTN,
        "--ctx-size", "4096",
        "--batch-size", "512",
        "--ubatch-size", "512",
        "--cache-type-k", CACHE_TYPE_K,
        "--cache-type-v", CACHE_TYPE_V,
        "--parallel", "1",
        "--threads", "16",
        "--threads-batch", "16",
        "--file", PROMPT,
        "--n-predict", "1",
        "--seed", "1234",
        "--temp", "0",
    ]
    if restore_gate:
        canary_command.extend(["--hfx-restore-gate-root", RENDEZVOUS_ROOT])
    return canary_command


def canary_sequence(sequence: str, unit_label: str, rendezvous: bool = False):
    canary_command = canary_argv(sequence)
    unit = f"{UNIT_PREFIX}-canary-{unit_label}"
    command = [
        "systemd-run", "--user", f"--unit={unit}", "--property=RuntimeMaxSec=20min",
        *semantic_env_args(), "--wait", "--collect", "--pipe", *canary_command,
    ]
    invocation = "invocation=" + " ".join(canary_command) + "\ntransient_unit=" + unit + "\n"
    if rendezvous:
        raise CanaryError("legacy multi-case rendezvous is outside the closed L25 authority")
    result = ssh(NIMO2, *command, timeout=1800, check=False)
    result.stdout = invocation + result.stdout
    if result.returncode != 0:
        raise CanaryError(result.stdout + result.stderr)
    return result


def wait_remote_file(path: str, timeout_seconds: float) -> None:
    deadline = time.monotonic() + timeout_seconds
    while time.monotonic() < deadline:
        if ssh(NIMO2, "test", "-f", path, check=False).returncode == 0:
            return
        time.sleep(1)
    raise CanaryError(f"timed out waiting for residency rendezvous {path}")


def output_fields(text: str) -> dict[str, str]:
    line = next((line for line in reversed(text.splitlines()) if line.startswith("mode=")), "")
    if not line:
        raise CanaryError("canary output has no result line")
    fields = {}
    for match in re.finditer(r"(?:^| )([a-z0-9_]+)=([^ ]+)", line):
        fields[match.group(1)] = match.group(2)
    return fields


def output_sequence(text: str) -> dict[str, dict[str, str]]:
    result = {}
    for line in text.splitlines():
        if not line.startswith("mode="):
            continue
        fields = {}
        for match in re.finditer(r"(?:^| )([a-z0-9_]+)=([^ ]+)", line):
            fields[match.group(1)] = match.group(2)
        label = fields.get("label", "")
        if not label or label in result:
            raise CanaryError("sequence result labels are absent or duplicate")
        result[label] = fields
    return result


def fetch_suffix(root: Path, remote_name: str, local_name: str) -> tuple[str, str]:
    token_remote = f"{ARTIFACT_DIR}/{remote_name}-suffix.bin"
    text_remote = f"{ARTIFACT_DIR}/{remote_name}-suffix.txt"
    token_local = root / f"{local_name}-suffix.bin"
    text_local = root / f"{local_name}-suffix.txt"
    run(["scp", f"{NIMO2}:{token_remote}", str(token_local)])
    run(["scp", f"{NIMO2}:{text_remote}", str(text_local)])
    return (
        hashlib.sha256(token_local.read_bytes()).hexdigest(),
        hashlib.sha256(text_local.read_bytes()).hexdigest(),
    )


def worker_journal(unit: str, invocation_id: str, pid: int) -> str:
    journal = ssh(
        NIMO1,
        "journalctl", "--user", "-u", f"{unit}.service",
        f"_SYSTEMD_INVOCATION_ID={invocation_id}", "--no-pager", "-o", "short",
    ).stdout
    state_lines = [line for line in journal.splitlines() if "[halofpx-state]" in line]
    if any(re.search(rf"\[{pid}\]:", line) is None for line in state_lines):
        raise CanaryError(f"worker {unit} state journal is not bound to admitted PID {pid}")
    return journal


def require_result(
    fields: dict[str, str],
    mode: str,
    *,
    fallback_reason: str | None = None,
    require_worker_state: bool = False,
) -> None:
    exact = {
        "mode": mode,
        "prompt_tokens": "1129",
        "saved_boundary": "1128",
        "n_batch": (
            "0" if SEMANTIC_DIAGNOSTICS_ONLY and mode == "restore" else "512"
        ),
    }
    for name, expected in exact.items():
        if fields.get(name) != expected:
            raise CanaryError(f"{mode} result {name} mismatch: {fields.get(name)!r}")
    if fallback_reason is None:
        if "fallback" in fields or "reason" in fields:
            raise CanaryError(f"{mode} unexpectedly cold-fell back: {fields}")
    elif fields.get("fallback") != "cold" or fields.get("reason") != fallback_reason:
        raise CanaryError(f"{mode} fallback mismatch: expected {fallback_reason}, got {fields}")
    if require_worker_state and (
        int(fields.get("worker_bytes", "0")) <= 0
        or int(fields.get("worker_components", "0")) <= 0
    ):
        raise CanaryError(f"{mode} did not report positive worker state")


def state_windows(capture: str, restore: str) -> tuple[list[str], list[str]]:
    def is_state_allocation(line: str, payload_bytes: int) -> bool:
        match = re.search(r"\[alloc_buffer\].* size: (\d+)", line)
        if not match:
            return False
        allocated = int(match.group(1))
        return payload_bytes <= allocated < payload_bytes + 65536

    capture_lines = capture.splitlines()
    stored = next(i for i, line in enumerate(capture_lines) if "[halofpx-state] stored" in line)
    stored_bytes = re.search(r"bytes=(\d+)", capture_lines[stored])
    if not stored_bytes:
        raise CanaryError("capture state byte count is absent")
    capture_payload_bytes = int(stored_bytes.group(1))
    capture_start = next(
        i for i in range(stored - 1, -1, -1)
        if is_state_allocation(capture_lines[i], capture_payload_bytes)
    )
    capture_window = capture_lines[capture_start:stored + 1]

    restore_lines = restore.splitlines()
    ready = next(i for i, line in enumerate(restore_lines) if "[halofpx-state] ready" in line)
    applied = next(i for i, line in enumerate(restore_lines[ready + 1:], ready + 1) if "[halofpx-state] apply" in line)
    ready_bytes = re.search(r"bytes=(\d+)", restore_lines[ready])
    if not ready_bytes:
        raise CanaryError("restore state byte count is absent")
    restore_payload_bytes = int(ready_bytes.group(1))
    restore_start = next(
        i for i in range(ready - 1, -1, -1)
        if is_state_allocation(restore_lines[i], restore_payload_bytes)
    )
    restore_end = next(
        i for i in range(applied + 1, len(restore_lines)) if "[free_buffer]" in restore_lines[i]
    )
    restore_window = restore_lines[restore_start:restore_end + 1]
    combined = "\n".join(capture_window + restore_window).lower()
    if "[get_tensor]" in combined or "[set_tensor]" in combined:
        raise CanaryError("state window contains GET_TENSOR/SET_TENSOR")
    return capture_window, restore_window


def require_diagnostic_agreement(
    capture_journal: str,
    restore_journal: str,
    capture_result: dict[str, str],
    restore_result: dict[str, str],
) -> dict[str, object]:
    pattern = re.compile(
        r"\[halofpx-state-diag\] phase=(capture|stage|apply) "
        r"components=(\d+) bytes=(\d+) descriptor_content_sha256=([0-9a-f]{64}) "
        r"merkle_sha256=([0-9a-f]{64}) auth_tag=([0-9a-f]{64})"
    )
    diagnostic_lines = [
        line for line in (capture_journal + "\n" + restore_journal).splitlines()
        if "[halofpx-state-diag]" in line
    ]
    records = []
    for line in diagnostic_lines:
        match = pattern.search(line)
        if match is None or line[match.end():].strip():
            raise CanaryError(f"malformed worker diagnostic line: {line}")
        records.append(match.groups())
    by_phase: dict[str, tuple[int, int, str, str, str]] = {}
    for phase, components, byte_count, digest, merkle, tag in records:
        if phase in by_phase:
            raise CanaryError(f"duplicate worker diagnostic phase: {phase}")
        by_phase[phase] = (int(components), int(byte_count), digest, merkle, tag)
    if set(by_phase) != {"capture", "stage", "apply"}:
        raise CanaryError(f"incomplete worker diagnostic phases: {sorted(by_phase)}")
    if any(
        count <= 0 or byte_count <= 0 or any(value == "0" * 64 for value in (digest, merkle, tag))
        for count, byte_count, digest, merkle, tag in by_phase.values()
    ):
        raise CanaryError("worker diagnostic count/digest is invalid")
    if len({(value[0], value[1], value[2]) for value in by_phase.values()}) != 1:
        raise CanaryError(f"worker diagnostic mismatch: {by_phase}")

    capture_components = int(capture_result.get("worker_components", "0"))
    restore_components = int(restore_result.get("worker_components", "0"))
    capture_bytes = int(capture_result.get("worker_bytes", "0"))
    restore_bytes = int(restore_result.get("worker_bytes", "0"))
    if (
        capture_components != restore_components
        or capture_components != by_phase["capture"][0]
        or capture_bytes <= 0
        or capture_bytes != restore_bytes
        or capture_bytes != by_phase["capture"][1]
    ):
        raise CanaryError("worker result bytes/components disagree across capture/restore diagnostics")

    state_patterns = {
        "stored": re.compile(
            r"\[halofpx-state\] stored rank=\d+ generation=\d+ "
            r"components=(\d+) bytes=(\d+)$"),
        "ready": re.compile(
            r"\[halofpx-state\] ready rank=\d+ generation=\d+ "
            r"components=(\d+) bytes=(\d+)$"),
        "apply": re.compile(
            r"\[halofpx-state\] apply rank=\d+ generation=\d+ status=3 "
            r"components=(\d+) bytes=(\d+)$"),
    }
    state_metadata: dict[str, tuple[int, int]] = {}
    combined_lines = (capture_journal + "\n" + restore_journal).splitlines()
    for phase, state_pattern in state_patterns.items():
        matches = []
        marker = f"[halofpx-state] {phase}"
        for line in combined_lines:
            if marker not in line:
                continue
            match = state_pattern.search(line)
            if match is None:
                raise CanaryError(f"malformed worker state metadata: {phase}")
            matches.append((int(match.group(1)), int(match.group(2))))
        if len(matches) != 1:
            raise CanaryError(f"worker state metadata count mismatch: {phase}={len(matches)}")
        state_metadata[phase] = matches[0]
    expected_metadata = (capture_components, capture_bytes)
    if any(value != expected_metadata for value in state_metadata.values()):
        raise CanaryError(f"worker state metadata mismatch: {state_metadata}")

    coordinator: dict[str, str] = {}
    for name in ("control_sha256", "local_sha256", "component_manifest_sha256"):
        captured = capture_result.get(name, "")
        restored = restore_result.get(name, "")
        if (
            not re.fullmatch(r"[0-9a-f]{64}", captured)
            or captured == "0" * 64
            or restored != captured
        ):
            raise CanaryError(f"coordinator diagnostic mismatch: {name}")
        coordinator[name] = captured
    return {
        "worker_components": capture_components,
        "worker_bytes": capture_bytes,
        "worker_descriptor_content_sha256": by_phase["capture"][2],
        "worker_diagnostic_roots": {
            phase: {"merkle_sha256": value[3], "auth_tag": value[4]}
            for phase, value in by_phase.items()
        },
        "worker_state_phases": {
            phase: {"components": value[0], "bytes": value[1]}
            for phase, value in state_metadata.items()
        },
        "coordinator": coordinator,
    }


def require_authenticated_component_diagnostics(
        capture_journal: str, restore_journal: str, root: Path) -> dict[str, object]:
    if not COMPONENT_DIAGNOSTICS or not COMPONENT_DIAGNOSTICS_SHA:
        raise CanaryError("component diagnostic analyzer authority is unavailable")
    if SSH_TRANSPORT is None:
        raise CanaryError("bounded SSH transport is unavailable")
    actual = ssh(NIMO1, "sha256sum", COMPONENT_DIAGNOSTICS).stdout.split()[0]
    if actual != COMPONENT_DIAGNOSTICS_SHA:
        raise CanaryError("component diagnostic analyzer hash mismatch")
    combined = (capture_journal + "\n" + restore_journal).encode("utf-8")
    command = ["python3", COMPONENT_DIAGNOSTICS, "--log", "/dev/stdin", "--key-file", CONTROL]
    receipt_stage = ""
    if LIVE_RECAPTURE_DIAGNOSTICS:
        receipt_local = root / "live-recapture-receipt.bin"
        run(["scp", f"{NIMO2}:{ARTIFACT_DIR}/live-recapture-receipt.bin", str(receipt_local)])
        receipt_stage = f"{WORKER_ROOT}/live-recapture-receipt.bin"
        run(["scp", str(receipt_local), f"{NIMO1}:{receipt_stage}"])
        command.extend([
            "--require-recapture",
            "--coordinator-receipt", receipt_stage,
        ])
    result = SSH_TRANSPORT.run_stdin(
        NIMO1,
        command,
        combined,
        operation="evidence",
    )
    if receipt_stage:
        ssh(NIMO1, "rm", "-f", "--", receipt_stage)
    if result.returncode != 0:
        raise CanaryError(
            "authenticated component diagnostics refused: "
            + (result.stderr.strip() or "unknown analyzer failure"))
    try:
        report = json.loads(result.stdout)
    except json.JSONDecodeError as exc:
        raise CanaryError("component diagnostic report is malformed") from exc
    if (
        not isinstance(report, dict)
        or report.get("schema") != (
            "halofpx.state-component-live-recapture-diagnostic.v1"
            if LIVE_RECAPTURE_DIAGNOSTICS else "halofpx.state-component-diagnostic.v1"
        )
        or report.get("mismatches") != []
        or not re.fullmatch(r"[0-9a-f]{64}", str(report.get("report_auth_tag", "")))
        or set(report.get("phases", {})) != (
            {"capture", "stage", "apply", "recapture"}
            if LIVE_RECAPTURE_DIAGNOSTICS else {"capture", "stage", "apply"}
        )
        or (LIVE_RECAPTURE_DIAGNOSTICS and not isinstance(report.get("coordinator"), dict))
    ):
        raise CanaryError("component diagnostic report did not prove exact agreement")
    path = root / "authenticated-component-report.json"
    path.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    return report


def require_flushed_capture_evidence(text: str) -> dict[str, str]:
    if not text.endswith("\n"):
        raise CanaryError("capture evidence line is not durably delimited")
    parsed = output_sequence(text)
    capture = parsed.get("capture")
    if capture is None:
        raise CanaryError("capture-ready preceded flushed authenticated capture evidence")
    require_result(capture, "capture", require_worker_state=True)
    for digest_name in (
        "control_sha256", "local_sha256", "component_manifest_sha256",
    ):
        digest = capture.get(digest_name, "")
        if not re.fullmatch(r"[0-9a-f]{64}", digest) or digest == "0" * 64:
            raise CanaryError(f"capture evidence lacks authenticated {digest_name}")
    first_token = capture.get("tokens", "").split(",", 1)[0]
    if first_token != "21549":
        raise CanaryError(f"unexpected flushed reference token: {first_token!r}")
    return capture


def require_fresh_rpc_model_residency(
        capture_worker_invocation: str,
        restore_worker_invocation: str,
        capture_coordinator_pid: int,
        restore_coordinator_pid: int,
        restore_model_loaded_after_worker: bool,
        capture_coordinator_stopped_before_worker: bool = True,
        capture_worker_stopped_before_restore: bool = True,
        restore_worker_pid: int = 1,
        current_worker_pid: int = 1,
        current_worker_invocation: str | None = None) -> None:
    """Refuse restore unless both sides have fresh post-restart lifetime authority."""
    invocation = re.compile(r"[0-9a-f]{32}")
    if (
        not invocation.fullmatch(capture_worker_invocation)
        or not invocation.fullmatch(restore_worker_invocation)
        or capture_worker_invocation == restore_worker_invocation
    ):
        raise CanaryError("restore requires an exact changed worker InvocationID")
    if (
        capture_coordinator_pid <= 0
        or restore_coordinator_pid <= 0
        or capture_coordinator_pid == restore_coordinator_pid
    ):
        raise CanaryError("restore requires a fresh coordinator process/model residency")
    if not restore_model_loaded_after_worker:
        raise CanaryError("restore model residency must load after the restarted worker")
    if not capture_coordinator_stopped_before_worker:
        raise CanaryError("capture coordinator must terminate before worker A")
    if not capture_worker_stopped_before_restore:
        raise CanaryError("worker A must stop before worker B/model residency B")
    if current_worker_invocation is None:
        current_worker_invocation = restore_worker_invocation
    if (
        restore_worker_pid <= 0
        or current_worker_pid <= 0
        or restore_worker_pid != current_worker_pid
        or current_worker_invocation != restore_worker_invocation
    ):
        raise CanaryError("worker epoch changed after restore model load")


def run_diagnostic(root: Path, local_units: list[str]) -> dict[str, object]:
    capture_unit = f"{UNIT_PREFIX}-worker-capture"
    restore_unit = f"{UNIT_PREFIX}-worker-restore"
    restore_canary_unit = f"{UNIT_PREFIX}-canary-restore"
    # Reversed cleanup order stops capture first if capture fails before the
    # explicit A->B transition, avoiding attribution of A's listener to B.
    local_units.extend([restore_unit, capture_unit])

    capture_worker_pid, capture_invocation, capture_readiness = start_worker(
        True, capture_unit, root)
    capture_result = canary_sequence("capture-only", "capture")
    write_log(root, "capture.log", capture_result)
    capture_fields = output_fields(capture_result.stdout)
    require_result(capture_fields, "capture", require_worker_state=True)
    capture_coordinator_pid = int(capture_fields.get("coordinator_pid", "0"))
    if capture_coordinator_pid <= 0:
        raise CanaryError("capture coordinator PID evidence is absent")
    if ssh(NIMO2, "kill", "-0", str(capture_coordinator_pid), check=False).returncode == 0:
        raise CanaryError("capture coordinator A survived capture completion")
    capture_suffix = fetch_suffix(root, "capture", "capture")
    capture_journal = worker_journal(
        capture_unit, capture_invocation, capture_worker_pid)
    (root / "worker-capture.log").write_text(capture_journal, encoding="utf-8")

    capture_epoch = {
        "schema": "halofpx.l28.capture-epoch-audit.v1",
        "worker_pid": capture_worker_pid,
        "worker_invocation_id": capture_invocation,
        "coordinator_pid": capture_coordinator_pid,
        "coordinator_terminated_before_worker_stop": True,
    }
    capture_epoch_path = root / "capture-epoch-audit.json"
    with capture_epoch_path.open("x", encoding="utf-8", newline="\n") as output:
        json.dump(capture_epoch, output, indent=2, sort_keys=True)
        output.write("\n")
        output.flush()
        os.fsync(output.fileno())
    capture_epoch_sha = hashlib.sha256(capture_epoch_path.read_bytes()).hexdigest()
    object_digest = capture_fields.get("object", "")
    epoch_receipt_remote = f"{ARTIFACT_DIR}/capture-epoch-auth.json"
    epoch_args = [
        "--key", CONTROL,
        "--receipt", epoch_receipt_remote,
        "--object-digest", object_digest,
        "--worker-pid", str(capture_worker_pid),
        "--worker-invocation-id", capture_invocation,
        "--coordinator-pid", str(capture_coordinator_pid),
    ]
    ssh(NIMO2, "python3", EPOCH_RECEIPT, "create", *epoch_args)
    epoch_receipt_local = root / "capture-epoch-auth.json"
    run(["scp", f"{NIMO2}:{epoch_receipt_remote}", str(epoch_receipt_local)])
    epoch_receipt_sha = hashlib.sha256(epoch_receipt_local.read_bytes()).hexdigest()

    stop_worker(capture_unit)
    restore_worker_pid, restore_invocation, restore_readiness = start_worker(
        True, restore_unit, root)
    if (
        restore_worker_pid == capture_worker_pid
        or restore_invocation == capture_invocation
    ):
        raise CanaryError("worker B reused worker A PID or InvocationID")

    restore_command = canary_argv("restore-guarded", restore_gate=True)
    ssh(
        NIMO2, "systemd-run", "--user", f"--unit={restore_canary_unit}",
        "--property=RuntimeMaxSec=20min",
        *(["--setenv=HALOFPX_STATE_DIAGNOSTICS=1"] if LIVE_RECAPTURE_DIAGNOSTICS else []),
        *semantic_env_args(),
        *restore_command)
    wait_remote_file(f"{RENDEZVOUS_ROOT}/model-ready", 1200)
    restore_show = ssh(
        NIMO2, "systemctl", "--user", "show", f"{restore_canary_unit}.service",
        "-p", "ActiveState", "-p", "SubState", "-p", "MainPID",
        "-p", "InvocationID").stdout
    restore_props = dict(
        line.split("=", 1) for line in restore_show.splitlines() if "=" in line)
    if (
        restore_props.get("ActiveState") != "active"
        or restore_props.get("SubState") != "running"
    ):
        raise CanaryError("restore coordinator is not active at model-ready")
    restore_coordinator_pid = int(restore_props.get("MainPID", "0"))
    restore_coordinator_invocation = restore_props.get("InvocationID", "").lower()
    if not re.fullmatch(r"[0-9a-f]{32}", restore_coordinator_invocation):
        raise CanaryError("restore coordinator InvocationID is absent or malformed")

    current_worker_pid = int(ssh(
        NIMO1, "systemctl", "--user", "show", f"{restore_unit}.service",
        "-p", "MainPID", "--value").stdout.strip())
    current_worker_invocation = ssh(
        NIMO1, "systemctl", "--user", "show", f"{restore_unit}.service",
        "-p", "InvocationID", "--value").stdout.strip().lower()
    # This authenticated object/epoch binding is checked after model B is
    # ready and immediately before the only staging authorization.
    ssh(NIMO2, "python3", EPOCH_RECEIPT, "verify", *epoch_args)
    require_fresh_rpc_model_residency(
        capture_invocation, restore_invocation,
        capture_coordinator_pid, restore_coordinator_pid, True,
        capture_coordinator_stopped_before_worker=True,
        capture_worker_stopped_before_restore=True,
        restore_worker_pid=restore_worker_pid,
        current_worker_pid=current_worker_pid,
        current_worker_invocation=current_worker_invocation)
    ssh(NIMO2, "touch", f"{RENDEZVOUS_ROOT}/restore-authorized")

    deadline = time.monotonic() + 1200
    restore_status = None
    while time.monotonic() < deadline:
        show = ssh(
            NIMO2, "systemctl", "--user", "show", f"{restore_canary_unit}.service",
            "-p", "ActiveState", "-p", "SubState", "-p", "ExecMainStatus",
            check=False).stdout
        props = dict(line.split("=", 1) for line in show.splitlines() if "=" in line)
        if props.get("ActiveState") in {"inactive", "failed"}:
            restore_status = int(props.get("ExecMainStatus", "-1"))
            break
        time.sleep(1)
    if restore_status is None:
        raise CanaryError("timed out waiting for guarded restore coordinator")
    restore_log = ssh(
        NIMO2, "journalctl", "--user", "-u", f"{restore_canary_unit}.service",
        f"_SYSTEMD_INVOCATION_ID={restore_coordinator_invocation}",
        "--no-pager", "-o", "cat").stdout
    (root / "restore.log").write_text(restore_log, encoding="utf-8")
    if restore_status != 0:
        raise CanaryError(f"guarded restore failed with {restore_status}: {restore_log}")
    restore_fields = output_fields(restore_log)
    require_result(restore_fields, "restore", require_worker_state=True)
    if int(restore_fields.get("coordinator_pid", "0")) != restore_coordinator_pid:
        raise CanaryError("restore result PID does not match admitted model residency")
    restore_suffix = fetch_suffix(root, "restore", "restore")

    restore_journal = worker_journal(
        restore_unit, restore_invocation, restore_worker_pid)
    (root / "worker-restore.log").write_text(restore_journal, encoding="utf-8")
    semantic_provenance = None
    replay_authority = None
    if SEMANTIC_DIAGNOSTICS_ONLY:
        semantic_provenance = require_authenticated_semantic_provenance(
            capture_result.stdout, restore_log)
        capture_semantic = semantic_provenance["capture"]
        restore_semantic = semantic_provenance["restore"]
        if (
            capture_semantic["replay_count"] != 1
            or restore_semantic["replay_count"] != 1
            or capture_semantic["replay_token"] != restore_semantic["replay_token"]
            or capture_semantic["position_before"] != restore_semantic["position_before"]
            or capture_semantic["position_after"] != restore_semantic["position_after"]
            or capture_semantic["logits_count"] != restore_semantic["logits_count"]
            or capture_semantic["logits_sha256"] != restore_semantic["logits_sha256"]
            or capture_semantic["argmax_token"] != restore_semantic["argmax_token"]
            or capture_semantic["sampled_token"] != restore_semantic["sampled_token"]
            or capture_semantic["argmax_token"] != capture_semantic["sampled_token"]
            or capture_semantic["logits_invalidated"]
            or restore_semantic["logits_invalidated"]
        ):
            raise CanaryError("normal semantic provenance did not prove exact agreement")
        replay_authority = require_authenticated_replay_authority(
            capture_result.stdout, restore_log)
    component_diagnostics = None
    if not SEMANTIC_DIAGNOSTICS_ONLY:
        component_diagnostics = require_authenticated_component_diagnostics(
            capture_journal, restore_journal, root)
    if LIVE_RECAPTURE_DIAGNOSTICS:
        assert component_diagnostics is not None
        agreement = {
            "worker_components": component_diagnostics["phases"]["recapture"]["components"],
            "worker_bytes": component_diagnostics["phases"]["recapture"]["bytes"],
            "worker_descriptor_content_sha256": (
                component_diagnostics["phases"]["recapture"]["aggregate_sha256"]),
            "worker_diagnostic_roots": component_diagnostics["phases"],
            "coordinator": component_diagnostics["coordinator"],
            "live_recap_replaces_receipt_input_only_check": True,
        }
    else:
        agreement = require_diagnostic_agreement(
            capture_journal, restore_journal, capture_fields, restore_fields)
    capture_window, restore_window = state_windows(capture_journal, restore_journal)
    (root / "capture-state-window.log").write_text(
        "\n".join(capture_window) + "\n", encoding="utf-8")
    (root / "restore-state-window.log").write_text(
        "\n".join(restore_window) + "\n", encoding="utf-8")
    if capture_suffix != restore_suffix:
        raise CanaryError(
            f"fresh-residency suffix mismatch: capture={capture_suffix} restore={restore_suffix}")
    return {
        "schema": "halofpx.l28.fresh-residency-diagnostic-result.v1",
        "material_model_residencies": 2,
        "capture_epoch_audit": {
            **capture_epoch,
            "sha256": capture_epoch_sha,
            "authenticated_object_receipt_sha256": epoch_receipt_sha,
            "object_sha256": object_digest,
        },
        "restore_epoch": {
            "worker_pid": restore_worker_pid,
            "worker_invocation_id": restore_invocation,
            "coordinator_pid": restore_coordinator_pid,
            "coordinator_invocation_id": restore_coordinator_invocation,
            "model_ready_before_restore_authorization": True,
        },
        "readiness": {
            "capture": capture_readiness,
            "restore": restore_readiness,
        },
        "results": {"capture": capture_fields, "restore": restore_fields},
        "suffix_hashes": {
            "capture": {"tokens": capture_suffix[0], "text": capture_suffix[1]},
            "restore": {"tokens": restore_suffix[0], "text": restore_suffix[1]},
        },
        "diagnostic_agreement": agreement,
        "authenticated_component_diagnostics": component_diagnostics,
        "authenticated_semantic_provenance": semantic_provenance,
        "authenticated_replay_authority": replay_authority,
        "state_window_get_set": 0,
    }


def run_legacy_same_residency_diagnostic(root: Path, local_units: list[str]) -> dict[str, object]:
    capture_unit = "halofpx-l24-primary-worker-capture"
    restore_unit = "halofpx-l24-primary-worker-restore"
    local_units.extend([capture_unit, restore_unit])
    capture_pid, capture_invocation, capture_readiness = start_worker(True, capture_unit, root)

    canary_command = [
        CANARY_BIN,
        "--hfx-mode", "capture",
        "--hfx-sequence", "diagnostic",
        "--hfx-rendezvous-root", RENDEZVOUS_ROOT,
        "--hfx-artifact-root", COORDINATOR_ROOT,
        "--hfx-model-digest", MODEL_DIGEST,
        "--hfx-compatibility-root", COMPATIBILITY,
        "--hfx-plan-digest", PLAN,
        "--hfx-topology-digest", TOPOLOGY,
        "--hfx-placement-digest", PLACEMENT,
        "--hfx-checkpoint-digest", CHECKPOINT,
        "--hfx-control-file", CONTROL,
        "--hfx-expected-prompt-tokens", "1129",
        "--model", MODEL,
        "--rpc", f"10.44.0.1:{PORT}",
        "--device", "RPC0,ROCm0",
        "--split-mode", "layer",
        "--tensor-split", "1,1",
        "--n-gpu-layers", "999",
        "--fit", "off",
        "--no-mmap",
        "--direct-io",
        "--flash-attn", "on",
        "--ctx-size", "4096",
        "--batch-size", "512",
        "--ubatch-size", "512",
        "--cache-type-k", "q8_0",
        "--cache-type-v", "q8_0",
        "--parallel", "1",
        "--threads", "16",
        "--threads-batch", "16",
        "--file", PROMPT,
        "--n-predict", "1",
        "--seed", "1234",
        "--temp", "0",
    ]
    canary_unit = "halofpx-l24-primary-canary-diagnostic"
    command = [
        "systemd-run", "--user", f"--unit={canary_unit}",
        "--property=RuntimeMaxSec=30min", "--wait", "--collect", "--pipe",
        *canary_command,
    ]
    session_started_wall = datetime.now(timezone.utc).isoformat()
    session_started_mono = time.monotonic()
    session_timed_out = False
    session_terminated = False
    session_killed = False
    process, session_job = start_bounded_ssh_session(
        NIMO2, " ".join(shlex.quote(str(value)) for value in command))
    stream_lines: list[str] = []
    stream_errors: list[str] = []
    stream_lock = threading.Lock()
    stream_path = root / "diagnostic-stream.log"
    stream_error_path = root / "diagnostic-stream.stderr.log"

    def drain(pipe, destination: Path, lines: list[str]) -> None:
        assert pipe is not None
        with destination.open("x", encoding="utf-8", newline="\n") as output:
            for line in pipe:
                output.write(line)
                output.flush()
                os.fsync(output.fileno())
                # Publish only after the controller-owned evidence file is
                # durable, so capture-ready can never outrun the receipt.
                with stream_lock:
                    lines.append(line)

    stdout_thread = threading.Thread(
        target=drain, args=(process.stdout, stream_path, stream_lines), daemon=False)
    stderr_thread = threading.Thread(
        target=drain, args=(process.stderr, stream_error_path, stream_errors), daemon=False)
    stdout_thread.start()
    stderr_thread.start()
    try:
        wait_remote_file(f"{RENDEZVOUS_ROOT}/capture-ready", 1800)
        capture_deadline = time.monotonic() + 30
        capture_fields = None
        while time.monotonic() < capture_deadline:
            try:
                with stream_lock:
                    durable_text = "".join(stream_lines)
                capture_fields = require_flushed_capture_evidence(durable_text)
                break
            except CanaryError:
                pass
            time.sleep(0.05)
        if capture_fields is None:
            raise CanaryError("capture-ready preceded flushed authenticated capture evidence")
        reference_suffix = fetch_suffix(root, "capture", "capture-pre-restart")
        capture_journal = worker_journal(capture_unit, capture_invocation, capture_pid)
        (root / "worker-capture.log").write_text(capture_journal, encoding="utf-8")
        stop_worker(capture_unit)
        restore_pid, restore_invocation, restore_readiness = start_worker(True, restore_unit, root)
        ssh(NIMO2, "touch", f"{RENDEZVOUS_ROOT}/worker-restarted")
        process.wait(timeout=SSH_TRANSPORT_MODULE.SSH_OPERATION_DEADLINES["model-session"])
    except BaseException as exc:
        session_timed_out = isinstance(exc, subprocess.TimeoutExpired)
        session_terminated, session_killed = terminate_bounded_ssh_session(
            process, session_job)
        raise
    finally:
        stdout_thread.join(timeout=10)
        stderr_thread.join(timeout=10)
        if stdout_thread.is_alive() or stderr_thread.is_alive():
            raise CanaryError("diagnostic evidence drain thread did not terminate")
        SSH_TRANSPORT._record({
            "schema": "halofpx.ssh-operation.v1",
            "sequence": "model-session",
            "host": NIMO2,
            "operation": "model-session",
            "argv": command,
            "started_at": session_started_wall,
            "ended_at": datetime.now(timezone.utc).isoformat(),
            "duration_seconds": round(time.monotonic() - session_started_mono, 6),
            "deadline_seconds": SSH_TRANSPORT_MODULE.SSH_OPERATION_DEADLINES["model-session"],
            "pid": process.pid,
            "returncode": process.returncode,
            "timed_out": session_timed_out,
            "term_sent": session_terminated,
            "kill_sent": session_killed,
            "failure_class": "timeout" if session_timed_out else (
                None if process.returncode == 0 else "command"),
            "stdout": "".join(stream_lines)[-SSH_TRANSPORT_MODULE.SSH_EVIDENCE_LIMIT:],
            "stderr": "".join(stream_errors)[-SSH_TRANSPORT_MODULE.SSH_EVIDENCE_LIMIT:],
        })
    SSH_TRANSPORT_MODULE.SshRunner._close_windows_job(session_job)
    stdout = "".join(stream_lines)
    stderr = "".join(stream_errors)
    result = subprocess.CompletedProcess(command, process.returncode, stdout, stderr)
    write_log(root, "diagnostic.log", result)
    if result.returncode != 0:
        raise CanaryError(f"diagnostic canary failed: {stdout}{stderr}")
    parsed = output_sequence(stdout)
    if set(parsed) != {"capture", "restore"}:
        raise CanaryError(f"diagnostic result set mismatch: {parsed.keys()}")
    require_result(parsed["capture"], "capture", require_worker_state=True)
    require_result(parsed["restore"], "restore", require_worker_state=True)
    capture_suffix = fetch_suffix(root, "capture", "capture")
    restore_suffix = fetch_suffix(root, "restore", "restore")
    if capture_suffix != restore_suffix:
        raise CanaryError(f"first-token mismatch: capture={capture_suffix} restore={restore_suffix}")
    capture_tokens = parsed["capture"].get("tokens", "").split(",", 1)[0]
    restore_tokens = parsed["restore"].get("tokens", "").split(",", 1)[0]
    if capture_tokens != "21549" or restore_tokens != "21549":
        raise CanaryError(
            f"unexpected first token: capture={capture_tokens!r} restore={restore_tokens!r}")

    restore_journal = worker_journal(restore_unit, restore_invocation, restore_pid)
    (root / "worker-restore.log").write_text(restore_journal, encoding="utf-8")
    diagnostic_agreement = require_diagnostic_agreement(
        capture_journal, restore_journal, parsed["capture"], parsed["restore"])
    capture_window, restore_window = state_windows(capture_journal, restore_journal)
    (root / "capture-state-window.log").write_text("\n".join(capture_window) + "\n", encoding="utf-8")
    (root / "restore-state-window.log").write_text("\n".join(restore_window) + "\n", encoding="utf-8")
    objects = ssh(NIMO1, "find", WORKER_ROOT + "/objects", "-type", "f", "-name", "*.hfx").stdout.splitlines()
    if len(objects) != 1:
        raise CanaryError(f"expected one worker object, found {len(objects)}")
    object_path = objects[0]
    return {
        "schema": "halofpx.l24.primary-diagnostic-result.v1",
        "model_sha256": MODEL_SHA,
        "model_bytes": MODEL_BYTES,
        "request": {
            "prompt_tokens": 1129, "saved_boundary": 1128, "generated_tokens": 1,
            "ctx": 4096, "batch": 512, "ubatch": 512, "cache_k": "q8_0",
            "cache_v": "q8_0", "flash_attention": True, "seed": 1234,
            "temperature": 0, "device_order": ["RPC0", "ROCm0"],
        },
        "pids": {"capture": capture_pid, "restore": restore_pid},
        "invocation_ids": {"capture": capture_invocation, "restore": restore_invocation},
        "readiness": {"capture": capture_readiness, "restore": restore_readiness},
        "results": parsed,
        "suffix_hashes": {
            "capture": {"tokens": capture_suffix[0], "text": capture_suffix[1]},
            "restore": {"tokens": restore_suffix[0], "text": restore_suffix[1]},
        },
        "pre_restart_reference": {
            "first_token": 21549,
            "tokens_sha256": reference_suffix[0],
            "text_sha256": reference_suffix[1],
            "stream_path": str(stream_path),
        },
        "worker_object": {
            "path": object_path,
            "bytes": int(ssh(NIMO1, "stat", "-c", "%s", object_path).stdout.strip()),
            "sha256": ssh(NIMO1, "sha256sum", object_path).stdout.split()[0],
        },
        "diagnostic_agreement": diagnostic_agreement,
        "state_window_get_set": 0,
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--evidence-dir", required=True, type=Path)
    parser.add_argument("--l28-fixture", action="store_true")
    parser.add_argument("--l29-primary", action="store_true")
    parser.add_argument("--l31-primary", action="store_true")
    parser.add_argument("--l32-fixture", action="store_true")
    parser.add_argument("--l33-primary", action="store_true")
    parser.add_argument("--l34-fixture", action="store_true")
    parser.add_argument("--l35-fixture", action="store_true")
    args = parser.parse_args()
    if sum((
        args.l28_fixture, args.l29_primary, args.l31_primary,
        args.l32_fixture, args.l33_primary, args.l34_fixture, args.l35_fixture,
    )) > 1:
        parser.error("fixture and primary modes are mutually exclusive")
    if args.l28_fixture:
        configure_l28_fixture()
    if args.l29_primary:
        configure_l29_primary()
    if args.l31_primary:
        configure_l31_primary()
    if args.l32_fixture:
        configure_l32_fixture()
    if args.l33_primary:
        configure_l33_primary()
    if args.l34_fixture:
        configure_l34_fixture()
    if args.l35_fixture:
        configure_l35_fixture()
    root = args.evidence_dir.resolve()
    root.mkdir(mode=0o700, parents=True, exist_ok=False)
    initialize_ssh_transport(root)
    local_units = []
    results = {}
    suffixes = {}
    try:
        if FIXTURE_QUALIFICATION:
            if ssh(NIMO1, "systemctl", "is-active", "minimax-m27-q6-server.service", check=False).stdout.strip() != "active":
                raise CanaryError("fixture qualification requires the production coordinator to remain active")
            if ssh(NIMO2, "systemctl", "is-active", "minimax-m27-rpc-worker.service", check=False).stdout.strip() != "active":
                raise CanaryError("fixture qualification requires the production worker to remain active")
            health = ssh(NIMO1, "curl", "-fsS", "-o", "/dev/null", "-w", "%{http_code}", "http://127.0.0.1:8081/health")
            if health.stdout.strip() != "200":
                raise CanaryError("fixture qualification requires production HTTP 200")
        else:
            if ssh(NIMO1, "systemctl", "is-active", "minimax-m27-q6-server.service", check=False).stdout.strip() != "inactive":
                raise CanaryError("production coordinator is not inactive")
            if ssh(NIMO2, "systemctl", "is-active", "minimax-m27-rpc-worker.service", check=False).stdout.strip() != "inactive":
                raise CanaryError("production worker is not inactive")
            if f":8081" in ssh(NIMO1, "ss", "-H", "-ltnp").stdout or f":50052" in ssh(NIMO2, "ss", "-H", "-ltnp").stdout:
                raise CanaryError("production listener remains open")
        channel_key_sha = validate_provisioned_keys()
        model_stat = ssh(NIMO2, "stat", "-c", "%s", MODEL).stdout.strip()
        if int(model_stat) != MODEL_BYTES:
            raise CanaryError("model size mismatch")
        if ssh(NIMO2, "sha256sum", MODEL, timeout=300).stdout.split()[0] != MODEL_SHA:
            raise CanaryError("model SHA-256 mismatch")
        if ssh(NIMO2, "sha256sum", CANARY_BIN).stdout.split()[0] != CANARY_SHA:
            raise CanaryError("coordinator canary binary mismatch")
        if ssh(NIMO1, "sha256sum", WORKER_BIN).stdout.split()[0] != WORKER_SHA:
            raise CanaryError("worker binary mismatch")
        if ssh(NIMO2, "sha256sum", READINESS_PROBE).stdout.split()[0] != READINESS_PROBE_SHA:
            raise CanaryError("readiness probe mismatch")
        if ssh(NIMO2, "sha256sum", PLACEMENT_PROBE).stdout.split()[0] != PLACEMENT_PROBE_SHA:
            raise CanaryError("placement probe mismatch")
        if ssh(NIMO2, "sha256sum", EPOCH_RECEIPT).stdout.split()[0] != EPOCH_RECEIPT_SHA:
            raise CanaryError("epoch receipt helper mismatch")
        if ssh(NIMO2, "sha256sum", PROMPT).stdout.split()[0] != PROMPT_SHA:
            raise CanaryError("prompt SHA-256 mismatch")
        free_worker = int(ssh(NIMO1, "df", "-B1", "--output=avail", "/var/tmp").stdout.splitlines()[-1])
        if free_worker < 2_000_000_000:
            raise CanaryError("worker free space below 2 GB gate")
        ssh(NIMO2, "rm", "-rf", "--", REMOTE_EVIDENCE, COORDINATOR_ROOT, RENDEZVOUS_ROOT)
        ssh(NIMO2, "install", "-d", "-m", "700", REMOTE_EVIDENCE, COORDINATOR_ROOT, RENDEZVOUS_ROOT)
        ssh(NIMO1, "rm", "-rf", "--", WORKER_ROOT)
        ssh(NIMO1, "install", "-d", "-m", "700", WORKER_ROOT)

        (root / "diskstats-nimo1-before.txt").write_text(ssh(NIMO1, "cat", "/proc/diskstats").stdout, encoding="utf-8")
        (root / "diskstats-nimo2-before.txt").write_text(ssh(NIMO2, "cat", "/proc/diskstats").stdout, encoding="utf-8")

        if DIAGNOSTIC_ONLY:
            summary = run_diagnostic(root, local_units)
            summary["channel_key_sha256"] = channel_key_sha
            (root / "result.json").write_text(
                json.dumps(summary, indent=2, sort_keys=True) + "\n", encoding="utf-8")
            (root / "diskstats-nimo1-after.txt").write_text(
                ssh(NIMO1, "cat", "/proc/diskstats").stdout, encoding="utf-8")
            (root / "diskstats-nimo2-after.txt").write_text(
                ssh(NIMO2, "cat", "/proc/diskstats").stdout, encoding="utf-8")
            return 0

        unit1 = "halofpx-l22-primary-worker-capture"
        local_units.append(unit1)
        pid_capture, invocation_capture, readiness_capture = start_worker(True, unit1, root)
        residency1 = canary_sequence("residency1", "residency1")
        write_log(root, "residency1.log", residency1)
        residency1_results = output_sequence(residency1.stdout)
        if set(residency1_results) != {"capture", "cold"}:
            raise CanaryError(f"residency1 result set mismatch: {residency1_results.keys()}")
        results.update(residency1_results)
        require_result(results["capture"], "capture", require_worker_state=True)
        require_result(results["cold"], "cold")
        suffixes["capture"] = fetch_suffix(root, "capture", "capture")
        suffixes["cold"] = fetch_suffix(root, "cold", "cold")
        capture_journal = worker_journal(unit1, invocation_capture, pid_capture)
        (root / "worker-capture.log").write_text(capture_journal, encoding="utf-8")
        stop_worker(unit1)

        unit2 = "halofpx-l22-primary-worker-restore"
        local_units.append(unit2)
        pid_restore, invocation_restore, readiness_restore = start_worker(True, unit2, root)

        objects = ssh(NIMO1, "find", WORKER_ROOT + "/objects", "-type", "f", "-name", "*.hfx").stdout.splitlines()
        if len(objects) != 1:
            raise CanaryError(f"expected one worker object after capture, found {len(objects)}")
        object_path = objects[0]
        object_bytes = int(ssh(NIMO1, "stat", "-c", "%s", object_path).stdout.strip())
        object_sha = ssh(NIMO1, "sha256sum", object_path).stdout.split()[0]

        residency2 = canary_sequence("residency2", "residency2", rendezvous=True)
        write_log(root, "residency2.log", residency2)
        residency2_results = output_sequence(residency2.stdout)
        if set(residency2_results) != {"restore", "missing", "plan-mismatch"}:
            raise CanaryError(f"residency2 result set mismatch: {residency2_results.keys()}")
        results["restore"] = residency2_results["restore"]
        results["missing_object"] = residency2_results["missing"]
        results["plan_mismatch"] = residency2_results["plan-mismatch"]
        require_result(results["restore"], "restore", require_worker_state=True)
        suffixes["restore"] = fetch_suffix(root, "restore", "restore")
        suffixes["missing_object"] = fetch_suffix(root, "missing", "missing-object")
        require_result(results["missing_object"], "restore", fallback_reason="worker-stage")
        suffixes["plan_mismatch"] = fetch_suffix(root, "plan-mismatch", "plan-mismatch")
        require_result(results["plan_mismatch"], "restore", fallback_reason="coordinator-artifact")

        restore_journal = worker_journal(unit2, invocation_restore, pid_restore)
        (root / "worker-restore.log").write_text(restore_journal, encoding="utf-8")
        capture_window, restore_window = state_windows(capture_journal, restore_journal)
        (root / "capture-state-window.log").write_text("\n".join(capture_window) + "\n", encoding="utf-8")
        (root / "restore-state-window.log").write_text("\n".join(restore_window) + "\n", encoding="utf-8")
        stop_worker(unit2)

        unit3 = "halofpx-l22-primary-worker-runtime-off"
        local_units.append(unit3)
        pid_runtime_off, invocation_runtime_off, readiness_runtime_off = start_worker(False, unit3, root)
        residency3 = canary_sequence("residency3", "residency3")
        write_log(root, "residency3.log", residency3)
        residency3_results = output_sequence(residency3.stdout)
        if set(residency3_results) != {"runtime-off"}:
            raise CanaryError(f"residency3 result set mismatch: {residency3_results.keys()}")
        results["runtime_off"] = residency3_results["runtime-off"]
        require_result(results["runtime_off"], "cold")
        suffixes["runtime_off"] = fetch_suffix(root, "runtime-off", "runtime-off-cold")

        if len(set(suffixes.values())) != 1:
            raise CanaryError(f"suffix mismatch: {suffixes}")
        cold_ms = float(results["cold"]["prompt_ms"]) + float(results["cold"]["generation_ms"])
        off_ms = float(results["runtime_off"]["prompt_ms"]) + float(results["runtime_off"]["generation_ms"])
        if cold_ms > max(off_ms * 1.5, off_ms + 50.0):
            raise CanaryError(f"obvious retained cold slowdown: enabled={cold_ms} off={off_ms}")

        summary = {
            "schema": "halofpx.l22.primary-result.v1",
            "fixture_qualification": FIXTURE_QUALIFICATION,
            "fixture_nonrepresentative_primary_kv_kernel": FIXTURE_QUALIFICATION,
            "channel_key_sha256": channel_key_sha,
            "model_sha256": MODEL_SHA,
            "model_bytes": MODEL_BYTES,
            "pids": {"capture": pid_capture, "restore": pid_restore, "runtime_off": pid_runtime_off},
            "invocation_ids": {
                "capture": invocation_capture,
                "restore": invocation_restore,
                "runtime_off": invocation_runtime_off,
            },
            "readiness": {
                "capture": readiness_capture,
                "restore": readiness_restore,
                "runtime_off": readiness_runtime_off,
            },
            "results": results,
            "suffix_hashes": {name: {"tokens": value[0], "text": value[1]} for name, value in suffixes.items()},
            "worker_object": {"path": object_path, "bytes": object_bytes, "sha256": object_sha},
            "state_window_get_set": 0,
            "cold_enabled_ms": cold_ms,
            "cold_runtime_off_ms": off_ms,
            "cold_ratio": cold_ms / off_ms,
        }
        (root / "result.json").write_text(json.dumps(summary, indent=2, sort_keys=True) + "\n", encoding="utf-8")
        (root / "diskstats-nimo1-after.txt").write_text(ssh(NIMO1, "cat", "/proc/diskstats").stdout, encoding="utf-8")
        (root / "diskstats-nimo2-after.txt").write_text(ssh(NIMO2, "cat", "/proc/diskstats").stdout, encoding="utf-8")
        return 0
    except Exception as exc:
        (root / "failure.txt").write_text(str(exc) + "\n", encoding="utf-8")
        print(f"L22 primary canary failed: {exc}", file=sys.stderr)
        return 1
    finally:
        cleanup_errors = []
        for unit in reversed(local_units):
            try:
                stop_worker(unit)
            except Exception as exc:
                cleanup_errors.append(f"{unit}: {exc}")
        if DIAGNOSTIC_ONLY:
            try:
                stop_canary(f"{UNIT_PREFIX}-canary-restore")
            except Exception as exc:
                cleanup_errors.append(f"halofpx-l28-canary-restore: {exc}")
        if cleanup_errors:
            raise CanaryError("; ".join(cleanup_errors))


if __name__ == "__main__":
    raise SystemExit(main())
