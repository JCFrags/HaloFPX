#!/usr/bin/env python3

import argparse
import json
import math
import pathlib
import re
import statistics


def percentile(values, fraction):
    if not values:
        return None
    ordered = sorted(values)
    index = min(len(ordered) - 1, math.ceil(fraction * len(ordered)) - 1)
    return ordered[max(index, 0)]


def read_json_stream(path):
    text = path.read_text(encoding="utf-8")
    decoder = json.JSONDecoder()
    result = []
    offset = 0
    while offset < len(text):
        while offset < len(text) and text[offset].isspace():
            offset += 1
        if offset == len(text):
            break
        value, offset = decoder.raw_decode(text, offset)
        result.extend(value)
    return {item["ifname"]: item for item in result}


def network_delta(root):
    before = read_json_stream(root / "net-before.json")
    after = read_json_stream(root / "net-after.json")
    result = {}
    for name in sorted(before.keys() & after.keys()):
        result[name] = {
            "rx_bytes": after[name]["stats64"]["rx"]["bytes"] - before[name]["stats64"]["rx"]["bytes"],
            "tx_bytes": after[name]["stats64"]["tx"]["bytes"] - before[name]["stats64"]["tx"]["bytes"],
        }
    return result


def gpu_summary(root):
    rows = []
    for line in (root / "gpu-samples.csv").read_text(encoding="utf-8").splitlines():
        fields = line.split(",")
        if len(fields) >= 4:
            rows.append((int(fields[0]), int(fields[1]), int(fields[2]), int(fields[3])))

    request_start = root / "request-start-realtime-ns.txt"
    request_end = root / "request-end-realtime-ns.txt"
    prompt_rows = []
    generation_rows = []
    if request_start.exists() and request_end.exists():
        start = int(request_start.read_text().strip())
        end = int(request_end.read_text().strip())
        request_rows = [row for row in rows if start <= row[0] <= end]
        response = root / "retained-response.json"
        if response.exists():
            payload = json.loads(response.read_text(encoding="utf-8"))
            prompt_end = start + int(payload["timings"]["prompt_ms"] * 1_000_000)
            prompt_rows = [row for row in rows if start <= row[0] < prompt_end]
            generation_rows = [row for row in rows if prompt_end <= row[0] <= end]
    else:
        request_rows = []

    def summarize(selected):
        busy = [row[1] for row in selected]
        return {
            "samples": len(selected),
            "busy_mean_percent": statistics.fmean(busy) if busy else None,
            "busy_p50_percent": percentile(busy, 0.50),
            "busy_p95_percent": percentile(busy, 0.95),
            "busy_max_percent": max(busy) if busy else None,
            "busy_nonzero_fraction": sum(value > 0 for value in busy) / len(busy) if busy else None,
            "vram_used_min_bytes": min((row[2] for row in selected), default=None),
            "vram_used_max_bytes": max((row[2] for row in selected), default=None),
            "gtt_used_min_bytes": min((row[3] for row in selected), default=None),
            "gtt_used_max_bytes": max((row[3] for row in selected), default=None),
        }

    return {
        "monitor_window": summarize(rows),
        "request_window": summarize(request_rows),
        "prompt_window": summarize(prompt_rows),
        "generation_window": summarize(generation_rows),
    }


def perf_trace_summary(root):
    pattern = re.compile(r"\((\d+(?:\.\d+)?) ms\): .*?\b(recvfrom|sendto|poll|ppoll|epoll_wait|futex)\(")
    values = {}
    for line in (root / "perf-trace.txt").read_text(encoding="utf-8", errors="replace").splitlines():
        match = pattern.search(line)
        if match:
            values.setdefault(match.group(2), []).append(float(match.group(1)))
    return {
        name: {
            "count": len(durations),
            "observed_duration_sum_ms": sum(durations),
            "duration_p50_ms": percentile(durations, 0.50),
            "duration_p95_ms": percentile(durations, 0.95),
            "duration_max_ms": max(durations),
        }
        for name, durations in sorted(values.items())
    }


def pidstat_summary(root, pid):
    cpu = []
    voluntary = []
    involuntary = []
    for line in (root / "pidstat.txt").read_text(encoding="utf-8", errors="replace").splitlines():
        fields = line.split()
        if len(fields) >= 14 and fields[3] == str(pid) and fields[4] == "-":
            try:
                cpu.append(float(fields[9]))
                voluntary.append(float(fields[11]))
                involuntary.append(float(fields[12]))
            except ValueError:
                pass
    return {
        "samples": len(cpu),
        "cpu_mean_percent": statistics.fmean(cpu) if cpu else None,
        "cpu_max_percent": max(cpu) if cpu else None,
        "voluntary_context_switches_per_second_mean": statistics.fmean(voluntary) if voluntary else None,
        "involuntary_context_switches_per_second_mean": statistics.fmean(involuntary) if involuntary else None,
    }


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("pid", type=int)
    parser.add_argument("root", type=pathlib.Path)
    args = parser.parse_args()
    result = {
        "schema": "halofpx.rank-profile-summary.v1",
        "pid": args.pid,
        "gpu": gpu_summary(args.root),
        "network_monitor_window": network_delta(args.root),
        "syscalls": perf_trace_summary(args.root),
        "process": pidstat_summary(args.root, args.pid),
        "limitations": [
            "profilers perturb execution; this is diagnostic, not promotion-grade throughput evidence",
            "syscall duration sums can overlap across threads and are not wall-clock fractions",
            "GPU busy is a coarse sysfs sample, not kernel occupancy",
        ],
    }
    print(json.dumps(result, indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
