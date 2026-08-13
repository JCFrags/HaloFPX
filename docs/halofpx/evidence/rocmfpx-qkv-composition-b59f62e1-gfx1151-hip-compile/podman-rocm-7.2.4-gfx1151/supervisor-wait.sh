#!/usr/bin/env bash
set -u

root=/home/britt/halofpx-qkv-compose-b59f62e1
evidence="${root}/evidence"
container=halofpx-qkv-compose-b59f62e1

mkdir -p "${evidence}"
rc=$(podman wait "${container}")
printf '%s\n' "${rc}" > "${evidence}/podman-exit-code.txt"
date --utc --iso-8601=seconds > "${evidence}/podman-finished-utc.txt"
podman logs "${container}" > "${evidence}/podman-console.txt" 2>&1
podman inspect "${container}" > "${evidence}/container-inspect-final.json" 2>&1
if [[ "${rc}" = 0 && -f "${evidence}/runner-PASS" ]]; then
    touch "${evidence}/supervisor-PASS"
else
    touch "${evidence}/supervisor-FAIL"
fi
