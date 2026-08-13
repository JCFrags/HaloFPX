#!/usr/bin/env bash
set -u

root=/home/britt/halofpx-qkv-moe-compose-609c1664
evidence="${root}/evidence"
container=halofpx-qkv-moe-compose-609c1664

mkdir -p "${evidence}"
rc=$(podman wait "${container}")
printf '%s\n' "${rc}" > "${evidence}/podman-exit-code.txt"
date --utc --iso-8601=seconds > "${evidence}/podman-finished-utc.txt"
podman logs "${container}" > "${evidence}/podman-console.txt" 2>&1
podman inspect "${container}" > "${evidence}/container-inspect-final.json" 2>&1
if [[ "${rc}" = 0 && -f "${evidence}/runner-PASS" && \
      "$(<"${evidence}/runner-exit-code.txt")" = 0 ]]; then
    touch "${evidence}/supervisor-PASS"
else
    touch "${evidence}/supervisor-FAIL"
fi
(
    cd "${evidence}"
    find . -type f ! -name retained-evidence.sha256 -print0 \
        | sort -z | xargs -0 sha256sum > retained-evidence.sha256
)
