#!/usr/bin/env bash
set -Eeuo pipefail
SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
ROOT=$(cd -- "$SCRIPT_DIR/.." && pwd)
TMP=$(mktemp -d -t dual-usb4-wiki-validation.XXXXXX)
trap 'rm -rf "$TMP"' EXIT

for f in "$ROOT"/scripts/*.sh "$ROOT"/examples/*.sh; do
    [[ -f $f ]] || continue
    bash -n "$f"
done

# Keep Python bytecode outside the packaged tree.
export PYTHONPYCACHEPREFIX="$TMP/pycache"
python3 -m compileall -q "$ROOT/tools"
python3 "$ROOT/tools/selftest.py"
python3 "$ROOT/tools/build_citations.py" --check
python3 "$ROOT/tools/build_llm_corpus.py" --check
python3 "$ROOT/tools/check_links.py" "$ROOT"
python3 "$ROOT/tools/check_llama_patch.py"

for f in "$ROOT/llms.txt" "$ROOT/llms-full.txt" "$ROOT/CITATIONS.md" "$ROOT/SOURCE-SNAPSHOT.md"; do
    [[ -s $f ]] || { echo "missing or empty: $f" >&2; exit 1; }
done
"$ROOT/scripts/render-diagrams.sh" >/dev/null

if command -v gcc >/dev/null; then
    gcc -Wall -Wextra -Werror -O2 "$ROOT/tools/mptcp_smoke.c" -o "$TMP/mptcp_smoke"
fi
if command -v mkdocs >/dev/null; then
    (cd "$ROOT" && mkdocs build --strict --site-dir "$TMP/site")
else
    echo 'mkdocs not installed; Markdown/link/diagram validation completed' >&2
fi

echo 'validation passed'
