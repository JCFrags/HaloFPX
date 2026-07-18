#!/usr/bin/env bash
set -euo pipefail
OUT="${1:-$PWD/rocm-7.2.4-source-roots}"
[[ ! -e "$OUT" ]] || { echo "Output exists: $OUT" >&2; exit 2; }
mkdir -p "$OUT"

cat > "$OUT/pins.tsv" <<'EOF'
ROCm/ROCm	rocm-7.2.4	e0b62c25d8ea39473a7208c1c8995f1b5c2e277c
ROCm/rocm-systems	rocm-7.2.4	97f5574fe2fdc7bef44fb01545347912ee9f1779
ROCm/rocm-libraries	rocm-7.2.4	dabb6df2b988f8eabed1e2fecefaaf4e818bc7ef
ROCm/llvm-project	rocm-7.2.4	f58b06dce1f9c15707c5f808fd002e18c2accf7e
EOF

while IFS=$'\t' read -r repo ref commit; do
  name="${repo#*/}"
  dst="$OUT/$name"
  git clone --filter=blob:none --no-checkout "https://github.com/$repo.git" "$dst"
  git -C "$dst" checkout --detach "$commit"
  actual="$(git -C "$dst" rev-parse HEAD)"
  [[ "$actual" == "$commit" ]] || { echo "Commit mismatch for $repo" >&2; exit 3; }
  git -C "$dst" status --porcelain=v1 > "$dst/provenance-git-status.txt"
  git -C "$dst" ls-tree HEAD | awk '$1=="160000" {print $4, $3}' > "$dst/provenance-top-level-gitlinks.txt"
  git -C "$dst" submodule status --recursive > "$dst/provenance-submodules-before-init.txt" 2>&1 || true
  printf '%s\n' "$repo" > "$dst/provenance-repo.txt"
  printf '%s\n' "$ref" > "$dst/provenance-ref.txt"
  printf '%s\n' "$commit" > "$dst/provenance-commit.txt"
done < "$OUT/pins.tsv"

# Explicitly preserve the manifest caveat.
grep -n 'refs/tags/rocm-7.2.0' "$OUT/ROCm/default.xml" > "$OUT/default-xml-7.2.0-caveat.txt" || {
  echo 'Expected default.xml caveat not found' >&2
  exit 4
}
printf '%s\n' "$(date -u +%FT%TZ)" > "$OUT/provenance-captured-at-utc.txt"
echo "Pinned 7.2.4 source roots captured in $OUT"
