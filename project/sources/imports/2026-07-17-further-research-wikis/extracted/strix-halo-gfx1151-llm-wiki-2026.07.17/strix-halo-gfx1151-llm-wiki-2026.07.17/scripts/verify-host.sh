#!/usr/bin/env bash
set -uo pipefail

STRICT=0
JSON=0
while [[ $# -gt 0 ]]; do
    case "$1" in
        --strict) STRICT=1; shift ;;
        --json) JSON=1; shift ;;
        -h|--help) echo "Usage: verify-host.sh [--strict] [--json]"; exit 0 ;;
        *) echo "Unknown argument: $1" >&2; exit 2 ;;
    esac
done

failures=()
warnings=()
passes=()

pass() { passes+=("$*"); }
warn() { warnings+=("$*"); }
fail() { failures+=("$*"); }
version_ge() { [[ "$(printf '%s\n%s\n' "$2" "$1" | sort -V | head -n1)" == "$2" ]]; }

ID=unknown
VERSION_ID=unknown
if [[ -r /etc/os-release ]]; then
    # shellcheck disable=SC1091
    . /etc/os-release
fi
KERNEL="$(uname -r)"
KBASE="${KERNEL%%-*}"

if [[ "$ID" == ubuntu && "$VERSION_ID" == 26.04* ]]; then
    if version_ge "$KBASE" 7.0; then pass "Ubuntu 26.04 kernel $KERNEL meets ROCm 7.14 GA kernel lane"; else fail "Ubuntu 26.04 kernel $KERNEL is below 7.0 GA lane"; fi
elif [[ "$ID" == ubuntu && "$VERSION_ID" == 24.04* ]]; then
    if [[ "$KERNEL" == *oem* ]]; then
        if version_ge "$KBASE" 6.14.0; then pass "Ubuntu OEM kernel $KERNEL is in the 6.14 family; verify package revision >=1018"; else fail "Ubuntu OEM kernel $KERNEL is below 6.14.0-1018 family"; fi
    elif version_ge "$KBASE" 6.17.0; then
        pass "Ubuntu HWE kernel $KERNEL meets the 6.17 family floor; verify package revision >=19"
    else
        fail "Ubuntu 24.04 non-OEM kernel $KERNEL is below the captured 6.17 HWE floor"
    fi
else
    if version_ge "$KBASE" 6.18.4; then pass "Kernel $KERNEL meets non-Ubuntu gfx1151 floor 6.18.4"; else fail "Kernel $KERNEL is below non-Ubuntu gfx1151 floor 6.18.4"; fi
fi

FW_INFO=""
if command -v rpm >/dev/null 2>&1; then
    FW_INFO="$(rpm -q --qf '%{NAME} %{VERSION}-%{RELEASE}\n' linux-firmware 2>/dev/null || true)"
elif command -v dpkg-query >/dev/null 2>&1; then
    FW_INFO="$(dpkg-query -W -f='${Package} ${Version}\n' linux-firmware 2>/dev/null || true)"
fi
if grep -q '20251125' <<<"$FW_INFO"; then
    fail "Known-bad linux-firmware 20251125 detected: $FW_INFO"
elif [[ -n "$FW_INFO" ]]; then
    pass "Firmware package detected: $FW_INFO"
    if [[ "$ID" == fedora ]] && grep -Eq '20260110|2026(0[2-9]|1[0-2])[0-9]{2}' <<<"$FW_INFO"; then
        pass "Fedora firmware is at or beyond the community-validated 20260110 line"
    else
        warn "Firmware package is not the exact Fedora 20260110 reference; inspect hashes and distribution advisories"
    fi
else
    warn "Could not determine linux-firmware package version"
fi

if [[ "${HSA_OVERRIDE_GFX_VERSION:-}" =~ ^11\.0\.(0|3)$ ]]; then
    fail "Unsupported generic architecture override is active: HSA_OVERRIDE_GFX_VERSION=$HSA_OVERRIDE_GFX_VERSION"
elif [[ -n "${HSA_OVERRIDE_GFX_VERSION:-}" ]]; then
    warn "HSA_OVERRIDE_GFX_VERSION is active ($HSA_OVERRIDE_GFX_VERSION); scope it to a pinned project only"
else
    pass "No HSA architecture override active"
fi

if grep -qw 'amdgpu.cwsr_enable=0' /proc/cmdline 2>/dev/null; then
    warn "Legacy amdgpu.cwsr_enable=0 workaround is active; current supported stacks should be tested without it"
fi
if grep -qw 'amd_iommu=off' /proc/cmdline 2>/dev/null; then
    warn "amd_iommu=off is active; DMA isolation is reduced, especially relevant to USB4/RDMA"
fi

if command -v rocminfo >/dev/null 2>&1; then
    ROCMINFO="$(rocminfo 2>&1 || true)"
    if grep -q 'gfx1151' <<<"$ROCMINFO"; then pass "rocminfo enumerates gfx1151"; else warn "rocminfo did not show gfx1151"; fi
else
    warn "rocminfo not installed or not on PATH"
fi

if command -v hipconfig >/dev/null 2>&1; then
    HIPROOT="$(hipconfig -R 2>/dev/null || true)"
    pass "hipconfig active root: ${HIPROOT:-unknown}"
    if [[ -n "${ROCM_PATH:-}" && -n "$HIPROOT" && "${ROCM_PATH%/}" != "${HIPROOT%/}" ]]; then
        warn "ROCM_PATH ($ROCM_PATH) differs from hipconfig root ($HIPROOT)"
    fi
else
    warn "hipconfig not installed or not on PATH"
fi

if command -v cmake >/dev/null 2>&1; then
    CMAKE_VERSION="$(cmake --version | awk 'NR==1{print $3}')"
    if version_ge "$CMAKE_VERSION" 3.21; then pass "CMake $CMAKE_VERSION meets HIP-language minimum 3.21"; else fail "CMake $CMAKE_VERSION is below HIP-language minimum 3.21"; fi
else
    warn "CMake not installed"
fi

if command -v vulkaninfo >/dev/null 2>&1; then
    if vulkaninfo --summary >/tmp/strix-vulkan-summary.$$ 2>&1; then
        if grep -Eqi 'RADV|AMD.*RADV' /tmp/strix-vulkan-summary.$$; then pass "Vulkan RADV device detected"; else warn "Vulkan works but RADV was not identified; inspect ICD selection"; fi
    else
        warn "vulkaninfo --summary failed"
    fi
    rm -f /tmp/strix-vulkan-summary.$$
else
    warn "vulkaninfo not installed"
fi

if [[ "$JSON" -eq 1 ]]; then
    python3 - "$KERNEL" "$ID" "$VERSION_ID" "$STRICT" "${#failures[@]}" "${#warnings[@]}" "${#passes[@]}" <<'PY'
import json, sys
print(json.dumps({
  "kernel": sys.argv[1], "os_id": sys.argv[2], "os_version": sys.argv[3],
  "strict": bool(int(sys.argv[4])), "failure_count": int(sys.argv[5]),
  "warning_count": int(sys.argv[6]), "pass_count": int(sys.argv[7])
}, indent=2))
PY
else
    printf 'Strix Halo host verification\nOS: %s %s\nKernel: %s\n\n' "$ID" "$VERSION_ID" "$KERNEL"
    for x in "${passes[@]}"; do printf '[PASS] %s\n' "$x"; done
    for x in "${warnings[@]}"; do printf '[WARN] %s\n' "$x"; done
    for x in "${failures[@]}"; do printf '[FAIL] %s\n' "$x"; done
    printf '\nSummary: %d pass, %d warning, %d failure\n' "${#passes[@]}" "${#warnings[@]}" "${#failures[@]}"
fi

if (( ${#failures[@]} > 0 )); then exit 1; fi
if [[ "$STRICT" -eq 1 ]] && (( ${#warnings[@]} > 0 )); then exit 1; fi
exit 0
