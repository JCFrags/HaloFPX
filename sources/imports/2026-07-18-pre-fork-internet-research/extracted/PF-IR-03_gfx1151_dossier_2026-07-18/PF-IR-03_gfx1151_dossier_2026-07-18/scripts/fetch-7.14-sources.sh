#!/usr/bin/env bash
set -euo pipefail
OUT="${1:-$PWD/TheRock-7.14-source}"
COMMIT="418cd5f63abb7a604bad5874cd7b2e29334e640f"
REPO="https://github.com/ROCm/TheRock.git"
ARTIFACT_MANIFEST="${ARTIFACT_MANIFEST:-}"

[[ ! -e "$OUT" ]] || { echo "Output exists: $OUT" >&2; exit 2; }
git clone --filter=blob:none --no-checkout "$REPO" "$OUT"
cd "$OUT"
git checkout --detach "$COMMIT"
[[ "$(git rev-parse HEAD)" == "$COMMIT" ]] || exit 3

# TheRock's source fetcher requires the pinned checkout's Python requirements.
python3 -m venv .venv-provenance
.venv-provenance/bin/python -m pip install --upgrade pip
.venv-provenance/bin/python -m pip install -r requirements.txt
.venv-provenance/bin/python ./build_tools/fetch_sources.py

git status --porcelain=v1 > provenance-git-status.txt
git submodule status --recursive > provenance-submodules.txt
git ls-tree HEAD | awk '$1=="160000" {print $4, $3}' > provenance-top-level-gitlinks.txt
find patches -type f -print0 2>/dev/null | LC_ALL=C sort -z | xargs -0 -r sha256sum > provenance-patches.sha256
printf '%s\n' "$REPO" > provenance-root-repo.txt
printf '%s\n' "$COMMIT" > provenance-root-commit.txt
printf '%s\n' "$(date -u +%FT%TZ)" > provenance-captured-at-utc.txt

# Generate the source manifest from the checkout for comparison with the artifact.
.venv-provenance/bin/python ./build_tools/generate_therock_manifest.py \
  --commit "$COMMIT" \
  --rocm-package-version 7.14.0 \
  --output provenance-source-manifest.json
.venv-provenance/bin/python -m json.tool provenance-source-manifest.json > provenance-source-manifest.pretty.json

if [[ -n "$ARTIFACT_MANIFEST" ]]; then
  [[ -f "$ARTIFACT_MANIFEST" ]] || { echo "Missing ARTIFACT_MANIFEST: $ARTIFACT_MANIFEST" >&2; exit 4; }
  cp "$ARTIFACT_MANIFEST" provenance-artifact-manifest.json
  python3 - "$ARTIFACT_MANIFEST" provenance-source-manifest.json <<'__JSONCHECK__'
import json, pathlib, sys
artifact = json.loads(pathlib.Path(sys.argv[1]).read_text())
source = json.loads(pathlib.Path(sys.argv[2]).read_text())
result = {
    "artifact_the_rock_commit": artifact.get("the_rock_commit"),
    "source_the_rock_commit": source.get("the_rock_commit"),
    "commit_match": artifact.get("the_rock_commit") == source.get("the_rock_commit"),
    "artifact_submodules": {x.get("submodule_path"): x.get("pin_sha") for x in artifact.get("submodules", [])},
    "source_submodules": {x.get("submodule_path"): x.get("pin_sha") for x in source.get("submodules", [])},
}
result["submodules_match"] = result["artifact_submodules"] == result["source_submodules"]
print(json.dumps(result, indent=2, sort_keys=True))
if not result["commit_match"] or not result["submodules_match"]:
    raise SystemExit(5)
__JSONCHECK__
fi

echo "Pinned TheRock source captured in $OUT"
