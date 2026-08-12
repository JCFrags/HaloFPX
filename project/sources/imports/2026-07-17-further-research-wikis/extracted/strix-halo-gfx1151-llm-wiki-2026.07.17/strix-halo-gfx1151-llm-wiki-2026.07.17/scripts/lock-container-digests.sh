#!/usr/bin/env bash
set -euo pipefail

images=(
  "rocm/dev-ubuntu-24.04:7.2.1-complete"
  "ubuntu:24.04"
)
if [[ $# -gt 0 ]]; then images=("$@"); fi

if command -v docker >/dev/null 2>&1 && docker buildx version >/dev/null 2>&1; then
    for image in "${images[@]}"; do
        echo "===== $image ====="
        docker buildx imagetools inspect "$image" || true
    done
elif command -v skopeo >/dev/null 2>&1; then
    for image in "${images[@]}"; do
        echo "===== $image ====="
        skopeo inspect "docker://$image" || true
    done
elif command -v podman >/dev/null 2>&1; then
    for image in "${images[@]}"; do
        echo "===== $image ====="
        podman pull "$image" >/dev/null
        podman image inspect "$image" --format '{{json .Digest}}' || true
    done
else
    echo "Install docker buildx, skopeo, or podman to resolve OCI digests" >&2
    exit 1
fi
