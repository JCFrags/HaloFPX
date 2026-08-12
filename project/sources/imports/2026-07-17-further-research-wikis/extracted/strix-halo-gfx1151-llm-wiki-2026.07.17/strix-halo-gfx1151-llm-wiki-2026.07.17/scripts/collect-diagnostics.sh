#!/usr/bin/env bash
set -uo pipefail

usage() {
    cat <<'USAGE'
Usage: collect-diagnostics.sh [--output DIR] [--no-root] [--llama-bin PATH]

Collects a non-destructive Strix Halo host snapshot and creates DIR.tar.gz.
Root-only commands are attempted with noninteractive sudo unless --no-root is used.
USAGE
}

OUT=""
ALLOW_ROOT=1
LLAMA_BIN=""
while [[ $# -gt 0 ]]; do
    case "$1" in
        --output) OUT="${2:?missing directory}"; shift 2 ;;
        --no-root) ALLOW_ROOT=0; shift ;;
        --llama-bin) LLAMA_BIN="${2:?missing path}"; shift 2 ;;
        -h|--help) usage; exit 0 ;;
        *) echo "Unknown argument: $1" >&2; usage >&2; exit 2 ;;
    esac
done

STAMP="$(date -u +%Y%m%dT%H%M%SZ)"
OUT="${OUT:-strix-halo-diagnostics-${STAMP}}"
mkdir -p "$OUT"

have() { command -v "$1" >/dev/null 2>&1; }

capture() {
    local name="$1"; shift
    {
        printf '# command:'
        printf ' %q' "$@"
        printf '\n# collected: %s\n\n' "$(date --iso-8601=seconds 2>/dev/null || date)"
        "$@"
    } >"$OUT/$name.txt" 2>&1 || true
}

capture_sh() {
    local name="$1" cmd="$2"
    {
        printf '# shell command: %s\n# collected: %s\n\n' "$cmd" "$(date --iso-8601=seconds 2>/dev/null || date)"
        bash -o pipefail -c "$cmd"
    } >"$OUT/$name.txt" 2>&1 || true
}

capture uname uname -a
capture_sh os-release 'cat /etc/os-release 2>/dev/null || true'
capture_sh proc-cmdline 'cat /proc/cmdline 2>/dev/null || true'
capture_sh cpu-memory 'lscpu 2>/dev/null; echo; free -h 2>/dev/null; echo; grep -E "MemTotal|HugePages|Hugepagesize" /proc/meminfo 2>/dev/null'
capture_sh pci 'lspci -nnk 2>/dev/null || true'
capture_sh modules 'lsmod 2>/dev/null | grep -E "^(amdgpu|thunderbolt|thunderbolt_net|thunderbolt_ibverbs|rdma|ib_)" || true'
capture_sh amdgpu-modinfo 'modinfo amdgpu 2>/dev/null || true'
capture_sh sysfs-memory 'for f in /sys/module/ttm/parameters/pages_limit /sys/module/ttm/parameters/page_pool_size /sys/module/amdgpu/parameters/gttsize /sys/module/amdgpu/parameters/cwsr_enable; do printf "%s=" "$f"; cat "$f" 2>/dev/null || echo unavailable; done'
capture_sh dri 'find /sys/class/drm -maxdepth 3 -type f \( -name uevent -o -name mem_info_vram_total -o -name mem_info_gtt_total -o -name gpu_busy_percent \) -print -exec cat {} \; 2>/dev/null'

capture_sh firmware-packages "if command -v rpm >/dev/null; then rpm -qa | grep -Ei '^(linux-firmware|amd-gpu-firmware|amdgpu)' | sort; elif command -v dpkg-query >/dev/null; then dpkg-query -W -f='\${Package} \${Version}\\n' 2>/dev/null | grep -Ei 'linux-firmware|amdgpu|rocm' | sort; else echo 'No rpm/dpkg-query'; fi"
capture_sh firmware-files 'find /lib/firmware/amdgpu -maxdepth 1 -type f -printf "%f %s bytes\n" 2>/dev/null | sort'
capture_sh firmware-sha256 'find /lib/firmware/amdgpu -maxdepth 1 -type f -print0 2>/dev/null | sort -z | xargs -0 -r sha256sum'

if [[ "$ALLOW_ROOT" -eq 1 ]] && have sudo && sudo -n true >/dev/null 2>&1; then
    capture_sh dmesg 'sudo -n dmesg -T 2>&1'
    capture_sh amdgpu-dmesg 'sudo -n dmesg -T 2>&1 | grep -Eai "amdgpu|MES|gfxhub|page fault|GPU reset|ring timeout|firmware|kfd" || true'
    capture_sh blocked-tasks "ps -eo pid,ppid,user,stat,wchan:40,comm,args | awk 'NR==1 || \$4 ~ /D/'; sudo -n cat /proc/locks 2>/dev/null || true"
else
    capture_sh dmesg 'dmesg -T 2>&1 || true'
    capture_sh amdgpu-dmesg 'dmesg -T 2>&1 | grep -Eai "amdgpu|MES|gfxhub|page fault|GPU reset|ring timeout|firmware|kfd" || true'
    capture_sh blocked-tasks "ps -eo pid,ppid,user,stat,wchan:40,comm,args | awk 'NR==1 || \$4 ~ /D/'"
fi

capture_sh rocm-tools 'for c in hipconfig rocminfo rocm-smi amd-smi; do echo "===== $c ====="; command -v "$c" || true; "$c" --version 2>/dev/null || "$c" --full 2>/dev/null || "$c" 2>/dev/null || true; echo; done'
capture_sh compilers 'for c in cmake ninja clang clang++ gcc g++ meson; do echo "===== $c ====="; command -v "$c" || true; "$c" --version 2>/dev/null | head -n 5 || true; done; if [ -x /opt/rocm/llvm/bin/clang ]; then /opt/rocm/llvm/bin/clang --version; fi'
capture_sh rocm-libraries 'ldconfig -p 2>/dev/null | grep -E "libamdhip64|libhsa-runtime64|librocblas|libhipblas|libMIOpen" | sort || true'
capture_sh selected-env 'env | grep -E "^(ROCM_PATH|HIP_PATH|HIPCXX|HIP_VISIBLE_DEVICES|ROCR_VISIBLE_DEVICES|HSA_|GGML_|VK_|LD_LIBRARY_PATH|PATH)=" | sort || true'

capture_sh vulkan 'echo "===== ICD manifests ====="; find /usr/share/vulkan/icd.d /etc/vulkan/icd.d -maxdepth 1 -type f -name "*.json" -print -exec sed -n "1,160p" {} \; 2>/dev/null; echo "===== vulkaninfo ====="; vulkaninfo --summary 2>&1 || true'
capture_sh graphics-packages "if command -v rpm >/dev/null; then rpm -qa | grep -Ei 'mesa|vulkan|amdvlk' | sort; elif command -v dpkg-query >/dev/null; then dpkg-query -W -f='\${Package} \${Version}\\n' 2>/dev/null | grep -Ei 'mesa|vulkan|amdvlk' | sort; fi"

capture_sh usb4 'echo "===== boltctl ====="; boltctl list 2>&1 || true; echo "===== thunderbolt sysfs ====="; find /sys/bus/thunderbolt/devices -maxdepth 2 -type f -print -exec cat {} \; 2>/dev/null; echo "===== links ====="; ip -details link show 2>&1 || true; echo "===== routes ====="; ip route show table all 2>&1 || true'
capture_sh rdma 'ibv_devices 2>&1 || true; ibv_devinfo 2>&1 || true; rdma link 2>&1 || true; cat /sys/kernel/debug/thunderbolt_ibverbs/summary 2>/dev/null || true'

if [[ -n "$LLAMA_BIN" ]]; then
    if [[ -x "$LLAMA_BIN" ]]; then
        capture llama-version "$LLAMA_BIN" --version
        capture llama-devices "$LLAMA_BIN" --list-devices
        capture_sh llama-ldd "ldd $(printf '%q' "$LLAMA_BIN") | sort"
        capture_sh llama-sha256 "sha256sum $(printf '%q' "$LLAMA_BIN")"
    else
        printf 'Requested llama binary is not executable: %s\n' "$LLAMA_BIN" >"$OUT/llama-error.txt"
    fi
fi

cat >"$OUT/SUMMARY.md" <<EOF_SUMMARY
# Strix Halo diagnostics bundle

- Collected: ${STAMP}
- Host: $(hostname 2>/dev/null || echo unknown)
- Kernel: $(uname -r 2>/dev/null || echo unknown)
- Output: ${OUT}

Review first:

1. \`amdgpu-dmesg.txt\`
2. \`firmware-packages.txt\` and \`firmware-sha256.txt\`
3. \`rocm-tools.txt\`
4. \`vulkan.txt\`
5. \`selected-env.txt\`
6. \`usb4.txt\` / \`rdma.txt\` when networking is relevant

The bundle may contain hostnames, user names, device serials, paths, and network addresses. Review it before sharing.
EOF_SUMMARY

ARCHIVE="${OUT%/}.tar.gz"
tar -czf "$ARCHIVE" "$OUT"
printf 'Diagnostics: %s\nArchive: %s\n' "$OUT" "$ARCHIVE"
