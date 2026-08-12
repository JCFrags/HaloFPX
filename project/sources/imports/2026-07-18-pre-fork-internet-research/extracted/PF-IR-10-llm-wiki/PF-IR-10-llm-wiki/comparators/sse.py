#!/usr/bin/env python3
"""Normalize and compare OpenAI-compatible Server-Sent Events traces.

SPDX-License-Identifier: CC0-1.0

Policy is explicit: blank-line-delimited records, ``data:`` fields only,
malformed JSON records skipped when requested, and records after ``[DONE]``
ignored.  Byte chunks may split UTF-8 code points.
"""
from __future__ import annotations
import argparse, codecs, json
from pathlib import Path
from typing import Any


def parse_chunks(raw: bytes, ranges: list[list[int]] | None) -> tuple[list[Any], dict[str, int | bool]]:
    if ranges is None:
        pieces = [raw]
    else:
        pieces = [raw[a:b] for a, b in ranges]
    decoder = codecs.getincrementaldecoder("utf-8")("strict")
    text = ""
    for piece in pieces:
        text += decoder.decode(piece, final=False)
    text += decoder.decode(b"", final=True)
    events: list[Any] = []
    malformed = 0; post_done = 0; done = False
    for record in text.replace("\r\n", "\n").split("\n\n"):
        if not record:
            continue
        data_lines = [line[5:].lstrip(" ") for line in record.split("\n") if line.startswith("data:")]
        if not data_lines:
            continue
        payload = "\n".join(data_lines)
        if done:
            post_done += 1
            continue
        if payload == "[DONE]":
            done = True
            continue
        try:
            events.append(json.loads(payload))
        except json.JSONDecodeError:
            malformed += 1
    return events, {"done": done, "malformed_records_skipped": malformed, "post_done_ignored": post_done}


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("trace", type=Path)
    ap.add_argument("--chunks", type=Path)
    ap.add_argument("--expected-row", type=Path)
    ns = ap.parse_args()
    ranges = None
    if ns.chunks:
        ranges = json.loads(ns.chunks.read_text(encoding="utf-8"))["chunks"]
    events, stats = parse_chunks(ns.trace.read_bytes(), ranges)
    result = {"events": events, **stats}
    ok = True
    if ns.expected_row:
        expected = json.loads(ns.expected_row.read_text(encoding="utf-8"))
        expected = {k: expected[k] for k in ("events", "done", "malformed_records_skipped", "post_done_ignored")}
        ok = result == expected
        result = {"match": ok, "expected": expected, "actual": result}
    print(json.dumps(result, ensure_ascii=False, sort_keys=True, indent=2))
    return 0 if ok else 1
if __name__ == "__main__": raise SystemExit(main())
