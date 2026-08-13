#!/usr/bin/env bash
set -Eeuo pipefail

root=/home/britt/halofpx-qkv-compose-b59f62e1
evidence="${root}/evidence"
container=halofpx-qkv-compose-b59f62e1
image=localhost/halofpx-rocm-7.2.4-builddeps:qkv-113e4117

mkdir -p "${evidence}"
podman image inspect "${image}" > "${evidence}/image-inspect.json"
{
    printf 'container=%s\n' "${container}"
    printf 'image=%s\n' "${image}"
    printf 'mount=%s:/workspace\n' "${root}"
    printf 'device_mounts=none\n'
} > "${evidence}/launch-contract.txt"

cid=$(podman run -d \
    --name "${container}" \
    --security-opt label=disable \
    -v "${root}:/workspace" \
    "${image}" \
    bash /workspace/run.sh)
printf '%s\n' "${cid}" > "${evidence}/container-id.txt"
nohup setsid "${root}/wait.sh" \
    </dev/null > "${root}/wait-supervisor.log" 2>&1 &
printf '%s\n' "$!" > "${root}/wait-supervisor.pid"
