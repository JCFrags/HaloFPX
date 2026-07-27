#!/usr/bin/env python3
"""Emit a bounded authenticated no-model ROCm device inventory receipt."""

import argparse
import hashlib
import hmac
import json
import os
from pathlib import Path
import re
import subprocess
import sys

SCHEMA = "halofpx.l50.device-admission.v1"
DOMAIN = b"halofpx.l50.device-admission.v1\0"


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--binary", required=True)
    parser.add_argument("--key-file", required=True)
    parser.add_argument("--output", required=True)
    parser.add_argument("--verify", action="store_true")
    args = parser.parse_args()
    binary = Path(args.binary)
    key_path = Path(args.key_file)
    output = Path(args.output)
    if not binary.is_absolute() or not key_path.is_absolute() or not output.is_absolute():
        raise SystemExit("all paths must be absolute")
    key_stat = key_path.stat()
    if not key_path.is_file() or key_stat.st_mode & 0o777 != 0o600 or key_stat.st_size != 130:
        raise SystemExit("key authority mismatch")
    if args.verify:
        record = json.loads(output.read_text(encoding="utf-8"))
        tag = record.pop("hmac_sha256", "")
        canonical = json.dumps(record, sort_keys=True, separators=(",", ":")).encode()
        expected = hmac.new(
            key_path.read_bytes(), DOMAIN + canonical, hashlib.sha256).hexdigest()
        if not hmac.compare_digest(tag, expected):
            raise SystemExit("device receipt authentication failed")
        record["hmac_sha256"] = tag
        print(json.dumps(record, sort_keys=True))
        return 0
    result = subprocess.run(
        [str(binary), "--help"], text=True, encoding="utf-8",
        errors="strict", capture_output=True, timeout=30, check=False)
    inventory = result.stdout + result.stderr
    if result.returncode != 0 or len(inventory.encode("utf-8")) > 65536:
        raise SystemExit("device inventory command failed or exceeded bound")
    device_lines = [
        line.strip() for line in inventory.splitlines()
        if line.strip() and ("ROCm" in line or "gfx" in line or "CPU" in line)
    ]
    if "found 1 ROCm devices" not in inventory:
        raise SystemExit("exact single ROCm device inventory is absent")
    if not any("gfx1151" in line for line in device_lines):
        raise SystemExit("gfx1151 is absent")
    record = {
        "schema": SCHEMA,
        "binary_sha256": hashlib.sha256(binary.read_bytes()).hexdigest(),
        "backend": "ROCm",
        "device": "ROCm0",
        "gfx": "gfx1151",
        "inventory_sha256": hashlib.sha256(inventory.encode("utf-8")).hexdigest(),
        "inventory_lines": device_lines[:32],
    }
    canonical = json.dumps(record, sort_keys=True, separators=(",", ":")).encode()
    record["hmac_sha256"] = hmac.new(
        key_path.read_bytes(), DOMAIN + canonical, hashlib.sha256).hexdigest()
    output.parent.mkdir(mode=0o700, parents=True, exist_ok=True)
    flags = os.O_WRONLY | os.O_CREAT | os.O_EXCL
    fd = os.open(output, flags, 0o600)
    with os.fdopen(fd, "w", encoding="utf-8", newline="\n") as handle:
        json.dump(record, handle, sort_keys=True)
        handle.write("\n")
        handle.flush()
        os.fsync(handle.fileno())
    print(json.dumps(record, sort_keys=True))
    return 0


if __name__ == "__main__":
    sys.exit(main())
