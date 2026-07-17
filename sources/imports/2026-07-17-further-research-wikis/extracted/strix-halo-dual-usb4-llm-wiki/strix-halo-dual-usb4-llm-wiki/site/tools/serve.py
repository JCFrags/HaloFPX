#!/usr/bin/env python3
"""Serve the prebuilt wiki on localhost."""
from __future__ import annotations

import argparse
import http.server
import os
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SITE = ROOT / "site"


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=8000)
    args = parser.parse_args()
    if not (SITE / "index.html").exists():
        raise SystemExit("site/index.html is missing; run python tools/build_site.py")
    os.chdir(SITE)
    server = http.server.ThreadingHTTPServer((args.host, args.port), http.server.SimpleHTTPRequestHandler)
    print(f"Serving {SITE} at http://{args.host}:{args.port}/")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        server.server_close()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
