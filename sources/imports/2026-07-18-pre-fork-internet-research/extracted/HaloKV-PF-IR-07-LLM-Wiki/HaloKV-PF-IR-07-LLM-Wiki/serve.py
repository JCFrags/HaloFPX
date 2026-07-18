#!/usr/bin/env python3
from http.server import ThreadingHTTPServer, SimpleHTTPRequestHandler
from pathlib import Path
import os

ROOT = Path(__file__).resolve().parent
HOST = "127.0.0.1"
PORT = int(os.environ.get("PORT", "8765"))
os.chdir(ROOT)
print(f"Serving HaloKV PF-IR-07 from {ROOT} at http://{HOST}:{PORT}/")
ThreadingHTTPServer((HOST, PORT), SimpleHTTPRequestHandler).serve_forever()
