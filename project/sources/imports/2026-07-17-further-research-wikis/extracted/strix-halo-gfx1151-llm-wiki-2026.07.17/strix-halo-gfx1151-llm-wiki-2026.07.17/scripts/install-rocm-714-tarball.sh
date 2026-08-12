#!/usr/bin/env bash
set -euo pipefail

ROCM_VERSION="${ROCM_VERSION:-7.14.0}"
ROCM_TARBALL="${ROCM_TARBALL:-therock-dist-linux-gfx1151-${ROCM_VERSION}.tar.gz}"
ROCM_URL="${ROCM_URL:-https://repo.amd.com/rocm/tarball-multi-arch/${ROCM_TARBALL}}"
ROCM_PREFIX="${ROCM_PREFIX:-/opt/rocm-${ROCM_VERSION}}"
CACHE_DIR="${CACHE_DIR:-$HOME/.cache/strix-halo-wiki}"
EXPECTED="${ROCM_TARBALL_SHA256:-}"
ALLOW_UNVERIFIED="${ALLOW_UNVERIFIED:-0}"
FORCE="${FORCE:-0}"

for cmd in curl tar sha256sum; do command -v "$cmd" >/dev/null || { echo "Missing command: $cmd" >&2; exit 1; }; done
mkdir -p "$CACHE_DIR"
ARCHIVE="$CACHE_DIR/$ROCM_TARBALL"

if [[ -z "$EXPECTED" && "$ALLOW_UNVERIFIED" != 1 ]]; then
    cat >&2 <<'MSG'
ROCM_TARBALL_SHA256 is empty. Supply an independently recorded checksum, or set
ALLOW_UNVERIFIED=1 for a one-time provenance download. The latter is version-pinned
but not independently byte-verified.
MSG
    exit 2
fi

if [[ ! -f "$ARCHIVE" ]]; then
    curl --fail --location --retry 3 --continue-at - "$ROCM_URL" -o "$ARCHIVE"
fi
ACTUAL="$(sha256sum "$ARCHIVE" | awk '{print $1}')"
if [[ -n "$EXPECTED" && "$ACTUAL" != "$EXPECTED" ]]; then
    echo "Checksum mismatch: expected $EXPECTED, got $ACTUAL" >&2
    exit 1
fi
if [[ -d "$ROCM_PREFIX" && -n "$(find "$ROCM_PREFIX" -mindepth 1 -maxdepth 1 -print -quit 2>/dev/null)" ]]; then
    if [[ "$FORCE" != 1 ]]; then
        echo "Prefix is not empty: $ROCM_PREFIX (set FORCE=1 to replace)" >&2
        exit 1
    fi
    rm -rf "$ROCM_PREFIX"
fi
mkdir -p "$(dirname "$ROCM_PREFIX")"
STAGE="$(mktemp -d "${ROCM_PREFIX}.stage.XXXXXX")"
trap 'rm -rf "$STAGE"' EXIT
mkdir -p "$STAGE/extract" "$STAGE/install"
tar -xf "$ARCHIVE" -C "$STAGE/extract"
mapfile -d '' TOP < <(find "$STAGE/extract" -mindepth 1 -maxdepth 1 -print0)
if [[ ${#TOP[@]} -eq 1 && -d "${TOP[0]}" ]]; then
    cp -a "${TOP[0]}/." "$STAGE/install/"
else
    cp -a "$STAGE/extract/." "$STAGE/install/"
fi
if [[ ! -x "$STAGE/install/bin/hipconfig" ]]; then
    echo "Archive layout did not contain bin/hipconfig at the selected root" >&2
    find "$STAGE/install" -maxdepth 3 -type f -name hipconfig -print >&2
    exit 1
fi
mv "$STAGE/install" "$ROCM_PREFIX"
cat >"$ROCM_PREFIX/INSTALL-PROVENANCE.txt" <<EOF_PROV
version=$ROCM_VERSION
url=$ROCM_URL
archive=$ARCHIVE
sha256=$ACTUAL
independently_verified=$([[ -n "$EXPECTED" ]] && echo yes || echo no)
installed_at=$(date --iso-8601=seconds 2>/dev/null || date)
host=$(uname -a)
EOF_PROV
printf 'Installed ROCm %s to %s\nSHA256: %s\n' "$ROCM_VERSION" "$ROCM_PREFIX" "$ACTUAL"
printf 'Activate with:\n  export ROCM_PATH=%q\n  export PATH="$ROCM_PATH/bin:$PATH"\n  export LD_LIBRARY_PATH="$ROCM_PATH/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"\n' "$ROCM_PREFIX"
