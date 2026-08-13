#!/usr/bin/env bash
set -Eeuo pipefail

root=/home/britt/halofpx-qkv-moe-compose-609c1664
bundle=/mnt/c/Users/britt/Documents/HaloFPX_Source_609c1664.bundle
source_dir=/mnt/c/Users/britt/Documents/HaloFPX_NextPrefill_Worktree/docs/halofpx/evidence/rocmfpx-qkv-moe-composition-609c1664-gfx1151-hip-compile/podman-rocm-7.2.4-gfx1151
expected_head=609c166421ecf3eecaa67340e4f40fcb750a0f48
expected_parent=3d9a0c3cc52168f696d600099742c7caf964161f

test ! -e "${root}"
test -f "${bundle}"
mkdir -p "${root}/evidence"
cp "${bundle}" "${root}/source.bundle"
cp "${source_dir}/capture-driver.sh" "${root}/run.sh"
cp "${source_dir}/supervisor-start.sh" "${root}/start.sh"
cp "${source_dir}/supervisor-wait.sh" "${root}/wait.sh"
cp "${source_dir}/capture-driver.sh" "${root}/evidence/capture-driver.sh"
cp "${source_dir}/supervisor-start.sh" "${root}/evidence/supervisor-start.sh"
cp "${source_dir}/supervisor-wait.sh" "${root}/evidence/supervisor-wait.sh"
cp "${source_dir}/setup-workspace.sh" "${root}/evidence/setup-workspace.sh"
sha256sum "${root}/source.bundle" > "${root}/evidence/source-bundle.sha256"
git clone --branch codex/prefill-next-kernel-candidate \
    "${root}/source.bundle" "${root}/src"
test "$(git -C "${root}/src" rev-parse HEAD)" = "${expected_head}"
test "$(git -C "${root}/src" rev-parse HEAD^)" = "${expected_parent}"
test -z "$(git -C "${root}/src" status --porcelain)"
git -C "${root}/src" bundle verify "${root}/source.bundle" \
    > "${root}/evidence/source-bundle-verify.txt" 2>&1
bash -n "${root}/run.sh"
bash -n "${root}/start.sh"
bash -n "${root}/wait.sh"
