#!/usr/bin/env bash
set -euo pipefail
if [[ $# -lt 4 ]]; then
  echo "usage: $0 REPO_DIR BINARY BACKEND OUTPUT_JSON [DEVICE]" >&2
  exit 2
fi
repo=$1
binary=$2
backend=$3
output=$4
device=${5:-}
commit=$(git -C "$repo" rev-parse HEAD)
dirty=false
if ! git -C "$repo" diff --quiet || ! git -C "$repo" diff --cached --quiet; then dirty=true; fi
binary_sha=$(sha256sum "$binary" | awk '{print $1}')
compiler=$("$binary" --version 2>&1 | head -n 1 || true)
python3 - "$repo" "$binary" "$backend" "$output" "$device" "$commit" "$dirty" "$binary_sha" "$compiler" <<'PY'
import json, os, platform, sys
repo,binary,backend,output,device,commit,dirty,binary_sha,compiler=sys.argv[1:]
obj={
  "repository_path":os.path.abspath(repo),
  "source_commit":commit,
  "dirty":dirty=="true",
  "binary_path":os.path.abspath(binary),
  "binary_sha256":binary_sha,
  "binary_version_line":compiler,
  "os":platform.platform(),
  "arch":platform.machine(),
  "python":platform.python_version(),
  "backend":backend,
  "device":device or None,
}
os.makedirs(os.path.dirname(os.path.abspath(output)),exist_ok=True)
with open(output,"w",encoding="utf-8") as f:
  json.dump(obj,f,indent=2); f.write("\n")
PY
