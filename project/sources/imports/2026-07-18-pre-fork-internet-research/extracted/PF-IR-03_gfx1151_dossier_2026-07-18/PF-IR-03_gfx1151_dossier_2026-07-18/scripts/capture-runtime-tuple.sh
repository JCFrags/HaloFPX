#!/usr/bin/env bash
set -euo pipefail
OUT="${1:-$PWD/runtime-tuple-$(hostname)-$(date -u +%Y%m%dT%H%M%SZ)}"
mkdir -p "$OUT"

run() {
  local name="$1"; shift
  { echo "+ $*"; "$@"; } > "$OUT/$name.txt" 2>&1 || true
}

printf '%s\n' "$(date -u +%FT%TZ)" > "$OUT/captured-at-utc.txt"
printf '%s\n' "$(hostname -f 2>/dev/null || hostname)" > "$OUT/hostname.txt"
run os-release cat /etc/os-release
run uname uname -a
run kernel-version uname -r
run cmdline cat /proc/cmdline
run lscpu lscpu
run lspci-gpu lspci -nnk
run modules lsmod
run amdgpu-modinfo modinfo amdgpu
run amdkfd-modinfo modinfo amdkfd
run dmesg-amdgpu bash -lc 'dmesg --color=never | grep -Ei "amdgpu|amdkfd|kfd|firmware"'
run iommu bash -lc 'dmesg --color=never | grep -Ei "IOMMU|AMD-Vi|DMAR"'
run rocminfo rocminfo
run amd-smi amd-smi static --all
run rocm-smi rocm-smi --showallinfo
run hipconfig hipconfig --full
run hipconfig-path hipconfig --path
run hipconfig-root hipconfig -R
run hipconfig-lib hipconfig -l
run amdclang-version amdclang --version
run amdclang-resource amdclang --print-resource-dir
run clang-resource clang --print-resource-dir
run ldconfig ldconfig -p
run env bash -lc 'env | LC_ALL=C sort'
run rocm-symlinks bash -lc 'for p in /opt/rocm /opt/rocm/core; do echo "$p -> $(readlink -f "$p" 2>/dev/null || true)"; done'
run compiler-device-dirs bash -lc 'find /opt/rocm /opt/rocm/core -type d \( -path "*/amdgcn/bitcode" -o -path "*/lib/clang/*" \) -print 2>/dev/null | sort -u'
run kfd-nodes bash -lc 'find /sys/class/kfd -maxdepth 5 -type f -print -exec sh -c "printf \"  \"; cat \"$1\" 2>/dev/null" sh {} \; 2>/dev/null'
run drm-nodes bash -lc 'find /sys/class/drm -maxdepth 4 -type f -print -exec sh -c "printf \"  \"; cat \"$1\" 2>/dev/null" sh {} \; 2>/dev/null'

if command -v dpkg-query >/dev/null; then
  dpkg-query -W -f='${binary:Package}\t${Version}\t${Architecture}\n' | LC_ALL=C sort > "$OUT/dpkg-packages.tsv"
  grep -Ei 'rocm|amd|mesa|llvm|clang|linux-(image|modules|firmware)' "$OUT/dpkg-packages.tsv" > "$OUT/dpkg-relevant.tsv" || true
  run apt-policy apt-cache policy
  if command -v apt-get >/dev/null; then
    apt-get indextargets --format '$(IDENTIFIER)\t$(SITE)\t$(RELEASE)\t$(COMPONENT)\t$(ARCHITECTURE)\t$(FILENAME)' > "$OUT/apt-index-targets.tsv" 2>&1 || true
  fi
fi
if command -v rpm >/dev/null; then
  rpm -qa --qf '%{NAME}\t%{EPOCHNUM}:%{VERSION}-%{RELEASE}\t%{ARCH}\t%{SIGPGP:pgpsig}\n' | LC_ALL=C sort > "$OUT/rpm-packages.tsv"
  grep -Ei 'rocm|amd|mesa|llvm|clang|kernel|firmware' "$OUT/rpm-packages.tsv" > "$OUT/rpm-relevant.tsv" || true
fi

# Capture exact AMD firmware bytes referenced/present. The operator should correlate
# this list with dmesg and retain the distro package's WHENCE/license metadata.
if [[ -d /lib/firmware/amdgpu ]]; then
  find /lib/firmware/amdgpu -type f -print0 | LC_ALL=C sort -z | xargs -0 sha256sum > "$OUT/amdgpu-firmware.sha256"
  find /lib/firmware/amdgpu -type l -printf '%p -> %l\n' | LC_ALL=C sort > "$OUT/amdgpu-firmware-symlinks.txt"
fi
for whence in /lib/firmware/WHENCE /usr/lib/firmware/WHENCE; do
  [[ -f "$whence" ]] && cp "$whence" "$OUT/$(echo "$whence" | tr '/' '_')"
done

if [[ -r /proc/config.gz ]]; then
  zcat /proc/config.gz > "$OUT/kernel-config.txt"
elif [[ -r "/boot/config-$(uname -r)" ]]; then
  cp "/boot/config-$(uname -r)" "$OUT/kernel-config.txt"
fi
if [[ -f "$OUT/kernel-config.txt" ]]; then
  grep -E '^(CONFIG_DRM_AMDGPU|CONFIG_HSA_AMD|CONFIG_USB4|CONFIG_USB4_CONFIGFS|CONFIG_USB4_STREAM|CONFIG_IOMMU|CONFIG_AMD_IOMMU)=' "$OUT/kernel-config.txt" > "$OUT/kernel-config-relevant.txt" || true
fi

(
  cd "$OUT"
  find . -type f ! -name 'files.sha256' -print0 | LC_ALL=C sort -z | xargs -0 sha256sum
) > "$OUT/files.sha256"
echo "Runtime tuple captured in $OUT"
