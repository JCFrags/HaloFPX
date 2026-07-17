#!/usr/bin/env python3
"""Interpret a capture-topology output without overclaiming independence."""
from __future__ import annotations
import csv
import sys
from pathlib import Path


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: verify_report.py CAPTURE_DIR", file=sys.stderr)
        return 2
    root = Path(sys.argv[1])
    table = root / "netdev-map.tsv"
    if not table.exists():
        print(f"missing {table}", file=sys.stderr)
        return 1
    with table.open(newline="", encoding="utf-8") as fh:
        rows = list(csv.DictReader(fh, delimiter="\t"))

    print("# Dual-USB4 topology verifier\n")
    print(f"Capture: `{root}`\n")
    print("| Interface | Driver | Domain | NHI BDF | XDomain | Service | Remote UUID | NUMA |")
    print("|---|---|---|---|---|---|---|---:|")
    for r in rows:
        print(f"| {r['interface']} | {r['driver']} | {r['domain'] or '?'} | {r['nhi_bdf'] or '?'} | {r['xdomain'] or '?'} | {r['service'] or '?'} | {r.get('remote_uuid', '') or '?'} | {r['numa_node'] or '?'} |")

    domains = {r["domain"] for r in rows if r["domain"]}
    bdfs = {r["nhi_bdf"] for r in rows if r["nhi_bdf"]}
    xdomains = {r["xdomain"] for r in rows if r["xdomain"]}
    print("\n## Evidence assessment\n")
    if len(rows) < 2:
        print("- **Two-interface evidence: NOT PROVEN.** Fewer than two USB4NET interfaces were captured.")
    else:
        print(f"- **Two-interface evidence: PRESENT.** {len(rows)} candidate USB4NET interfaces captured.")
    if len(xdomains) >= 2:
        print("- **XDomain separation: PRESENT.** At least two distinct parent XDomain names were captured.")
    else:
        print("- **XDomain separation: NOT PROVEN.**")
    if len(domains) >= 2:
        print("- **USB4 domain separation: PRESENT.** This is strong host-controller evidence because Linux typically exposes one domain per controller. It also avoids same-domain firmware-CM UUID replacement.")
    else:
        print("- **USB4 domain separation: NOT PROVEN.** The paths may share a controller, or the capture may be incomplete. Firmware/ICM-managed same-domain links to one peer can replace one another by UUID.")
    if len(bdfs) >= 2:
        print("- **NHI PCI-function separation: PRESENT.** Distinct BDFs are the strongest topology evidence in this capture.")
    else:
        print("- **NHI PCI-function separation: NOT PROVEN.**")
    print("- **Failure independence: REQUIRES MANUAL UNPLUG/DEAUTHORIZE TEST.**")
    print("- **Capacity independence: REQUIRES ISOLATED AND CONCURRENT THROUGHPUT RESULTS.**")
    print("- **Application multipath: REQUIRES LIVE MPTCP SUBFLOW AND BYTE EVIDENCE.**")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
