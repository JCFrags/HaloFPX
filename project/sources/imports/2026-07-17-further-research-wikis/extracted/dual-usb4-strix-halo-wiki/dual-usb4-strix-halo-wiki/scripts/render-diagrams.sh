#!/usr/bin/env bash
set -Eeuo pipefail
SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
ROOT=$(cd -- "$SCRIPT_DIR/.." && pwd)
command -v dot >/dev/null || { echo 'Graphviz dot is required' >&2; exit 1; }
for f in "$ROOT"/diagrams/*.dot; do
    base=${f%.dot}
    dot -Tsvg "$f" -o "$base.svg"
    dot -Tpng -Gdpi=160 "$f" -o "$base.png"
    echo "rendered ${base##*/}.{svg,png}"
done
