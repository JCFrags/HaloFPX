#!/usr/bin/env bash
set -euo pipefail
OUT="${1:-$PWD/rocm-repo-signing-capture}"
DIST="${DIST:-ubuntu2404}"
BASE="https://repo.amd.com/rocm/packages-multi-arch"
REPO="$BASE/$DIST"
KEY_URL="$BASE/gpg/rocm.gpg"
mkdir -p "$OUT"
command -v curl >/dev/null || exit 2
command -v gpg >/dev/null || exit 2

curl --fail --location --proto '=https' --tlsv1.2 -o "$OUT/rocm.gpg" "$KEY_URL"
gpg --show-keys --with-colons --fingerprint "$OUT/rocm.gpg" > "$OUT/key-fingerprint.colons.txt"
gpg --show-keys --with-fingerprint "$OUT/rocm.gpg" > "$OUT/key-fingerprint.txt"

# Capture all available signed release forms. At least InRelease or Release.gpg
# must verify. Package-manager tools remain the preferred resolver for indexes.
for f in dists/stable/InRelease dists/stable/Release dists/stable/Release.gpg; do
  mkdir -p "$OUT/$(dirname "$f")"
  curl --fail --location --proto '=https' --tlsv1.2 -o "$OUT/$f" "$REPO/$f" || true
done

GNUPGHOME="$OUT/gnupg"
mkdir -m 700 "$GNUPGHOME"
gpg --homedir "$GNUPGHOME" --import "$OUT/rocm.gpg" > "$OUT/key-import.txt" 2>&1
verified=0
if [[ -s "$OUT/dists/stable/InRelease" ]]; then
  gpg --homedir "$GNUPGHOME" --status-fd 1 --verify "$OUT/dists/stable/InRelease" > "$OUT/inrelease-verify.txt" 2>&1 && verified=1
fi
if [[ -s "$OUT/dists/stable/Release" && -s "$OUT/dists/stable/Release.gpg" ]]; then
  gpg --homedir "$GNUPGHOME" --status-fd 1 --verify "$OUT/dists/stable/Release.gpg" "$OUT/dists/stable/Release" > "$OUT/release-verify.txt" 2>&1 && verified=1
fi
[[ "$verified" == 1 ]] || { echo 'No repository release signature verified' >&2; exit 3; }

printf '%s\n' "$REPO" > "$OUT/repository-url.txt"
printf '%s\n' "$KEY_URL" > "$OUT/key-url.txt"
printf '%s\n' "$(date -u +%FT%TZ)" > "$OUT/captured-at-utc.txt"
(
 cd "$OUT"
 find . -type f ! -name files.sha256 -print0 | LC_ALL=C sort -z | xargs -0 sha256sum
) > "$OUT/files.sha256"
echo "Repository signing evidence captured in $OUT"
