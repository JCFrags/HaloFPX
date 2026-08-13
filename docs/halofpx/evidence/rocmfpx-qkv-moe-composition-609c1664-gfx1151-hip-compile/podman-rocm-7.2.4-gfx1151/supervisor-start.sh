#!/usr/bin/env bash
set -Eeuo pipefail

root=/home/britt/halofpx-qkv-moe-compose-609c1664
evidence="${root}/evidence"
container=halofpx-qkv-moe-compose-609c1664
image=localhost/halofpx-rocm-7.2.4-builddeps:qkv-113e4117
expected_image_id=67427cc410efa89e605039547cc43780f400381874da97ec6d6e4cd3263757c2
expected_image_digest=localhost/halofpx-rocm-7.2.4-builddeps@sha256:9c7093f09068010487ad826e7677f9dc31b620db3a5544ef3c1da2c9d291dc80

mkdir -p "${evidence}"
actual_image_id=$(podman image inspect "${image}" --format '{{.Id}}')
actual_image_digest=$(podman image inspect "${image}" --format '{{index .RepoDigests 0}}')
test "${actual_image_id}" = "${expected_image_id}"
test "${actual_image_digest}" = "${expected_image_digest}"
podman image inspect "${image}" > "${evidence}/image-inspect.json"
{
    printf 'captured_utc='
    date --utc --iso-8601=seconds
    uname -a
    cat /etc/os-release
    podman version
    podman info
} > "${evidence}/podman-host.txt" 2>&1
{
    printf 'container=%s\n' "${container}"
    printf 'image=%s\n' "${image}"
    printf 'image_id=%s\n' "${actual_image_id}"
    printf 'image_digest=%s\n' "${actual_image_digest}"
    printf 'mount=%s:/workspace\n' "${root}"
    printf 'pull=never\n'
    printf 'device_mounts=none\n'
} > "${evidence}/launch-contract.txt"

cid=$(podman run -d \
    --pull=never \
    --name "${container}" \
    --security-opt label=disable \
    -v "${root}:/workspace" \
    "${image}" \
    bash /workspace/run.sh)
printf '%s\n' "${cid}" > "${evidence}/container-id.txt"
nohup setsid "${root}/wait.sh" \
    </dev/null > "${root}/wait-supervisor.log" 2>&1 &
printf '%s\n' "$!" > "${root}/wait-supervisor.pid"
