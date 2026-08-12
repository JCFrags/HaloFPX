#!/usr/bin/env bash
# PF-IR-11 read-only substrate probe for AMD XDNA2 / npu5.
#
# Default behavior is non-invasive: no sudo, package changes, module changes,
# device reset, sysfs writes, suspend, compilation, or model execution.
set -u
umask 077

PROBE_VERSION="1.0"
OUTPUT=""
RUN_XRT_QUERY=0

usage() {
  cat <<'EOF'
Usage:
  xdna2_readonly_probe.sh [--output DIR] [--xrt-query]
  xdna2_readonly_probe.sh DIR

Options:
  --output DIR   Write results to DIR. Default: ./xdna2-readonly-probe-<UTC>.
  --xrt-query    If xrt-smi is already installed, run "xrt-smi examine".
                 Disabled by default because it opens the userspace runtime path.
  -h, --help     Show this help.

The script does not use sudo and does not modify the host.
EOF
}

while (($#)); do
  case "$1" in
    --output)
      if (($# < 2)); then echo "ERROR: --output requires a directory" >&2; exit 2; fi
      OUTPUT="$2"; shift 2 ;;
    --xrt-query)
      RUN_XRT_QUERY=1; shift ;;
    -h|--help)
      usage; exit 0 ;;
    --)
      shift; break ;;
    -*)
      echo "ERROR: unknown option: $1" >&2; usage >&2; exit 2 ;;
    *)
      if [[ -n "$OUTPUT" ]]; then
        echo "ERROR: multiple output directories specified" >&2; exit 2
      fi
      OUTPUT="$1"; shift ;;
  esac
done

STAMP="$(date -u +%Y%m%dT%H%M%SZ 2>/dev/null || date +%Y%m%dT%H%M%S)"
OUTPUT="${OUTPUT:-./xdna2-readonly-probe-${STAMP}}"
if [[ -e "$OUTPUT" && ! -d "$OUTPUT" ]]; then
  echo "ERROR: output exists and is not a directory: $OUTPUT" >&2
  exit 2
fi
mkdir -p "$OUTPUT"/{host,kernel,pci,device,firmware,packages,runtime,logs}

note() { printf '%s\n' "$*" >&2; }

capture() {
  local rel="$1"; shift
  local out="$OUTPUT/$rel"
  mkdir -p "$(dirname "$out")"
  {
    printf '$'
    printf ' %q' "$@"
    printf '\n'
    "$@"
    rc=$?
    printf '\n[exit_status=%s]\n' "$rc"
  } >"$out" 2>&1
  return 0
}

capture_shell() {
  local rel="$1"; shift
  local script="$1"
  local out="$OUTPUT/$rel"
  mkdir -p "$(dirname "$out")"
  {
    printf '$ %s\n' "$script"
    bash -o pipefail -c "$script"
    rc=$?
    printf '\n[exit_status=%s]\n' "$rc"
  } >"$out" 2>&1
  return 0
}

{
  echo "PF-IR-11 XDNA2 read-only probe"
  echo "probe_version=$PROBE_VERSION"
  echo "started_utc=$(date -u +%Y-%m-%dT%H:%M:%SZ 2>/dev/null || true)"
  echo "effective_uid=$(id -u)"
  echo "effective_user=$(id -un 2>/dev/null || true)"
  echo "xrt_query_requested=$RUN_XRT_QUERY"
  echo "safety=no sudo; no install; no module load/unload; no bind/unbind; no writes; no reset; no suspend; no model"
} >"$OUTPUT/PROBE-METADATA.txt"

capture host/uname.txt uname -a
capture host/os-release.txt sh -c 'cat /etc/os-release 2>/dev/null || true'
capture host/hostname.txt sh -c 'hostnamectl 2>/dev/null || hostname 2>/dev/null || true'
capture host/cpu-summary.txt sh -c 'lscpu 2>/dev/null || cat /proc/cpuinfo 2>/dev/null || true'
capture host/limits.txt sh -c 'ulimit -a; printf "\n/proc/self/limits:\n"; cat /proc/self/limits 2>/dev/null || true'

# Sanitize common boot-volume identifiers before recording the command line.
if [[ -r /proc/cmdline ]]; then
  if command -v python3 >/dev/null 2>&1; then
    python3 - "$OUTPUT/host/proc-cmdline-sanitized.txt" <<'PY'
import re, sys
text = open("/proc/cmdline", "r", encoding="utf-8", errors="replace").read().strip()
patterns = [
    (r'(?<!\S)(root|resume)=\S+', r'\1=<redacted>'),
    (r'(?<!\S)(cryptdevice|rd\.luks\.uuid|rd\.luks\.name)=\S+', r'\1=<redacted>'),
]
for pat, repl in patterns:
    text = re.sub(pat, repl, text)
open(sys.argv[1], "w", encoding="utf-8").write(text + "\n")
PY
  else
    sed -E \
      -e 's/(^|[[:space:]])(root|resume)=[^[:space:]]+/\1\2=<redacted>/g' \
      -e 's/(^|[[:space:]])(cryptdevice|rd\.luks\.uuid|rd\.luks\.name)=[^[:space:]]+/\1\2=<redacted>/g' \
      /proc/cmdline >"$OUTPUT/host/proc-cmdline-sanitized.txt"
  fi
fi

capture kernel/modinfo-amdxdna.txt sh -c 'command -v modinfo >/dev/null && modinfo amdxdna || true'
capture kernel/loaded-modules.txt sh -c 'lsmod 2>/dev/null | grep -Ei "^(amdxdna|amd_iommu|iommu)" || true'
capture kernel/module-sysfs.txt sh -c '
if [ -d /sys/module/amdxdna ]; then
  find /sys/module/amdxdna -maxdepth 2 \( -type f -o -type l \) -print 2>/dev/null | sort
  printf "\nparameters:\n"
  for f in /sys/module/amdxdna/parameters/*; do
    [ -e "$f" ] || continue
    printf "%s=" "$f"
    cat "$f" 2>/dev/null || printf "<unreadable>\n"
  done
else
  echo "amdxdna module not present in /sys/module"
fi'
capture kernel/iommu-groups.txt sh -c '
if [ -d /sys/kernel/iommu_groups ]; then
  find /sys/kernel/iommu_groups -maxdepth 2 -type l -printf "%p -> %l\n" 2>/dev/null | sort
else
  echo "no /sys/kernel/iommu_groups"
fi'

CONFIG_OUT="$OUTPUT/kernel/relevant-config.txt"
{
  echo "# Relevant kernel configuration"
  cfg=""
  if [[ -r "/boot/config-$(uname -r)" ]]; then
    cfg="/boot/config-$(uname -r)"
    echo "# source=$cfg"
    grep -E '^(CONFIG_(DRM_ACCEL_AMDXDNA|AMD_IOMMU|DRM_ACCEL|IOMMU_SVA|PCI_PASID|HMM_MIRROR|FW_LOADER|DRM_SCHED))=' "$cfg" || true
    grep -E '^# CONFIG_(DRM_ACCEL_AMDXDNA|AMD_IOMMU|DRM_ACCEL|IOMMU_SVA|PCI_PASID|HMM_MIRROR|FW_LOADER|DRM_SCHED) is not set' "$cfg" || true
  elif [[ -r /proc/config.gz ]] && command -v zcat >/dev/null 2>&1; then
    echo "# source=/proc/config.gz"
    zcat /proc/config.gz 2>/dev/null | grep -E '^(CONFIG_(DRM_ACCEL_AMDXDNA|AMD_IOMMU|DRM_ACCEL|IOMMU_SVA|PCI_PASID|HMM_MIRROR|FW_LOADER|DRM_SCHED))=|^# CONFIG_(DRM_ACCEL_AMDXDNA|AMD_IOMMU|DRM_ACCEL|IOMMU_SVA|PCI_PASID|HMM_MIRROR|FW_LOADER|DRM_SCHED) is not set' || true
  else
    echo "# no readable kernel config found"
  fi
} >"$CONFIG_OUT" 2>&1

capture pci/lspci-all.txt sh -c 'command -v lspci >/dev/null && lspci -nn -D || true'
capture pci/lspci-kernel-drivers.txt sh -c 'command -v lspci >/dev/null && lspci -nnk -D || true'
capture_shell pci/lspci-xdna-filtered.txt 'if command -v lspci >/dev/null; then lspci -nnk -D | grep -iE -B3 -A6 "1022:17f0|neural|signal processing controller|coprocessor"; else echo "lspci not installed"; fi'

FOUND=0
for devpath in /sys/bus/pci/devices/*; do
  [[ -r "$devpath/vendor" && -r "$devpath/device" ]] || continue
  vendor="$(cat "$devpath/vendor" 2>/dev/null || true)"
  device="$(cat "$devpath/device" 2>/dev/null || true)"
  [[ "$vendor" == "0x1022" && "$device" == "0x17f0" ]] || continue
  FOUND=1
  bdf="$(basename "$devpath")"
  ddir="$OUTPUT/device/$bdf"
  mkdir -p "$ddir"
  for attr in vendor device revision class subsystem_vendor subsystem_device numa_node current_link_speed current_link_width max_link_speed max_link_width; do
    if [[ -e "$devpath/$attr" ]]; then
      { cat "$devpath/$attr" 2>/dev/null || echo "<unreadable>"; } >"$ddir/$attr.txt"
    fi
  done
  {
    echo "bdf=$bdf"
    echo "driver=$(readlink -f "$devpath/driver" 2>/dev/null || true)"
    echo "driver_name=$(basename "$(readlink -f "$devpath/driver" 2>/dev/null)" 2>/dev/null || true)"
    echo "iommu_group=$(readlink -f "$devpath/iommu_group" 2>/dev/null || true)"
    echo "firmware_node=$(readlink -f "$devpath/firmware_node" 2>/dev/null || true)"
  } >"$ddir/links.txt"
  if command -v udevadm >/dev/null 2>&1; then
    udevadm info --query=property --path="$devpath" >"$ddir/udev-properties.txt" 2>&1 || true
  fi
done
echo "matching_sysfs_devices=$FOUND" >"$OUTPUT/device/summary.txt"

capture device/dev-accel.txt sh -c '
if [ -e /dev/accel ]; then
  ls -ld /dev/accel
  find /dev/accel -maxdepth 1 -printf "%M %u %g %p -> %l\n" 2>/dev/null | sort
else
  echo "/dev/accel absent"
fi'
capture device/dri-render-nodes.txt sh -c '
if [ -d /dev/dri ]; then
  find /dev/dri -maxdepth 1 -printf "%M %u %g %p -> %l\n" 2>/dev/null | sort
else
  echo "/dev/dri absent"
fi'

FW_REPORT="$OUTPUT/firmware/amdnpu-17f0_11.txt"
: >"$FW_REPORT"
declare -A FW_SEEN=()
for root in /lib/firmware /usr/lib/firmware; do
  dir="$root/amdnpu/17f0_11"
  [[ -e "$dir" ]] || continue
  {
    echo "directory=$dir"
    ls -ld "$dir" 2>&1 || true
    find "$dir" -maxdepth 1 -printf '%y %M %u %g %s %p -> %l\n' 2>/dev/null | sort
  } >>"$FW_REPORT"
  while IFS= read -r -d '' f; do
    resolved="$(readlink -f "$f" 2>/dev/null || true)"
    key="${resolved:-$f}"
    [[ -n "${FW_SEEN[$key]+x}" ]] && continue
    FW_SEEN["$key"]=1
    {
      echo
      echo "logical_path=$f"
      echo "resolved_path=$resolved"
      if [[ -n "$resolved" && -r "$resolved" && -f "$resolved" ]]; then
        stat "$resolved" 2>&1 || true
        sha256sum "$resolved" 2>&1 || true
        case "$resolved" in
          *.zst)
            if command -v zstdcat >/dev/null 2>&1; then
              printf 'decompressed_sha256='
              zstdcat "$resolved" 2>/dev/null | sha256sum | awk '{print $1}'
            else
              echo "decompressed_sha256=<zstdcat unavailable>"
            fi ;;
        esac
      else
        echo "resolved payload missing or unreadable"
      fi
    } >>"$FW_REPORT"
  done < <(find "$dir" -maxdepth 1 \( -type f -o -type l \) -print0 2>/dev/null)
done
if [[ ! -s "$FW_REPORT" ]]; then echo "amdnpu/17f0_11 firmware directory not found" >"$FW_REPORT"; fi

capture packages/dpkg.txt sh -c 'if command -v dpkg-query >/dev/null; then dpkg-query -W -f="${binary:Package}\t${Version}\t${Architecture}\n" 2>/dev/null | grep -Ei "(^|[-_:])(xrt|amdxdna|ryzen-ai|ryzenai|linux-firmware|firmware-amd)" || true; else echo "dpkg-query unavailable"; fi'
capture packages/rpm.txt sh -c 'if command -v rpm >/dev/null; then rpm -qa --qf "%{NAME}\t%{VERSION}-%{RELEASE}\t%{ARCH}\n" 2>/dev/null | grep -Ei "(^|[-_:])(xrt|amdxdna|ryzen-ai|ryzenai|linux-firmware|firmware-amd)" || true; else echo "rpm unavailable"; fi'
capture packages/pacman.txt sh -c 'if command -v pacman >/dev/null; then pacman -Q 2>/dev/null | grep -Ei "(^|[-_:])(xrt|amdxdna|ryzen-ai|ryzenai|linux-firmware|firmware-amd)" || true; else echo "pacman unavailable"; fi'
capture runtime/ldconfig.txt sh -c 'if command -v ldconfig >/dev/null; then ldconfig -p 2>/dev/null | grep -Ei "(xrt|xilinx|amdxdna|onnxruntime|vitis|vaip|ryzen)" || true; else echo "ldconfig unavailable"; fi'
capture runtime/path-tools.txt sh -c '
for c in xrt-smi xbutil python3 conda; do
  printf "%s=" "$c"
  command -v "$c" 2>/dev/null || echo "<not found>"
done
if command -v python3 >/dev/null; then python3 --version 2>&1; fi'
capture runtime/python-packages.txt sh -c '
if command -v python3 >/dev/null; then
  python3 -m pip list --format=freeze 2>/dev/null | grep -Ei "^(onnx|onnxruntime|ryzen|vitis|vaip|xrt|quark|model-generate|transformers)" || true
else
  echo "python3 unavailable"
fi'

capture_shell logs/dmesg-filtered.txt 'dmesg --color=never 2>&1 | grep -iE "amdxdna|amdnpu|17f0|npu|iommu|pasid|sva|mailbox|psp|smu" || true'
capture_shell logs/journal-kernel-filtered.txt 'if command -v journalctl >/dev/null; then journalctl -k -b --no-pager 2>&1 | grep -iE "amdxdna|amdnpu|17f0|npu|iommu|pasid|sva|mailbox|psp|smu" || true; else echo "journalctl unavailable"; fi'

if ((RUN_XRT_QUERY)); then
  if command -v xrt-smi >/dev/null 2>&1; then
    capture runtime/xrt-smi-examine.txt xrt-smi examine
  else
    echo "xrt-smi not installed" >"$OUTPUT/runtime/xrt-smi-examine.txt"
  fi
else
  echo "not run; use --xrt-query to opt in" >"$OUTPUT/runtime/xrt-smi-examine.txt"
fi

# Generate a compact summary from files already collected. This reads probe
# outputs only and does not touch the device.
if command -v python3 >/dev/null 2>&1; then
  python3 - "$OUTPUT" "$PROBE_VERSION" <<'PY'
import json, os, pathlib, re, sys
out = pathlib.Path(sys.argv[1])
def text(rel):
    p = out / rel
    try:
        return p.read_text(encoding="utf-8", errors="replace")
    except Exception:
        return ""
def config_value(name):
    t = text("kernel/relevant-config.txt")
    m = re.search(rf"^{re.escape(name)}=(.*)$", t, re.M)
    if m: return m.group(1)
    if re.search(rf"^# {re.escape(name)} is not set$", t, re.M): return "not set"
    return None
devices=[]
devroot=out/"device"
if devroot.exists():
    for p in sorted(devroot.iterdir()):
        if not p.is_dir(): continue
        def one(name):
            q=p/f"{name}.txt"
            return q.read_text(errors="replace").strip() if q.exists() else None
        links=text(str(pathlib.Path("device")/p.name/"links.txt"))
        vals=dict(re.findall(r"^([^=]+)=(.*)$", links, re.M))
        devices.append({
            "bdf": p.name,
            "vendor": one("vendor"),
            "device": one("device"),
            "revision": one("revision"),
            "driver": vals.get("driver_name") or None,
            "iommu_group": vals.get("iommu_group") or None,
        })
fw=text("firmware/amdnpu-17f0_11.txt")
pkgs="\n".join([text("packages/dpkg.txt"),text("packages/rpm.txt"),text("packages/pacman.txt")])
summary={
  "schema_version":"1.0",
  "probe_version":sys.argv[2],
  "generated_utc":__import__("datetime").datetime.now(__import__("datetime").timezone.utc).isoformat(),
  "expected":{"vendor_device":"1022:17f0","revision":"0x11","kernel_device":"npu5"},
  "devices":devices,
  "expected_device_match":any(d.get("vendor")=="0x1022" and d.get("device")=="0x17f0" and d.get("revision") in ("0x11","11") for d in devices),
  "kernel_config":{
    k:config_value(k) for k in [
      "CONFIG_DRM_ACCEL_AMDXDNA","CONFIG_AMD_IOMMU","CONFIG_DRM_ACCEL",
      "CONFIG_IOMMU_SVA","CONFIG_PCI_PASID","CONFIG_HMM_MIRROR",
      "CONFIG_FW_LOADER","CONFIG_DRM_SCHED"
    ]
  },
  "firmware_directory_found":"directory=" in fw,
  "firmware_report":"firmware/amdnpu-17f0_11.txt",
  "amdxdna_modinfo_available":"filename:" in text("kernel/modinfo-amdxdna.txt"),
  "dev_accel_present":"/dev/accel absent" not in text("device/dev-accel.txt"),
  "candidate_packages_present":bool(re.search(r"xrt|amdxdna|ryzen.?ai",pkgs,re.I)),
  "xrt_query_requested":"not run" not in text("runtime/xrt-smi-examine.txt"),
  "limitations":[
    "read-only inventory; no model compilation or execution",
    "dmesg or journal may be restricted without privilege",
    "package name filtering can miss custom installations",
    "firmware presence does not prove version compatibility"
  ]
}
(out/"summary.json").write_text(json.dumps(summary,indent=2,sort_keys=True)+"\n",encoding="utf-8")
PY
else
  printf '{"schema_version":"1.0","error":"python3 unavailable; inspect text captures"}\n' >"$OUTPUT/summary.json"
fi

{
  echo "completed_utc=$(date -u +%Y-%m-%dT%H:%M:%SZ 2>/dev/null || true)"
  echo "output=$(cd "$OUTPUT" 2>/dev/null && pwd -P || printf '%s' "$OUTPUT")"
} >>"$OUTPUT/PROBE-METADATA.txt"

(
  cd "$OUTPUT" || exit 0
  if find . -type f ! -name SHA256SUMS -print0 >/dev/null 2>&1 && sort -z </dev/null >/dev/null 2>&1; then
    find . -type f ! -name SHA256SUMS -print0 | sort -z | xargs -0 sha256sum >SHA256SUMS
  else
    find . -type f ! -name SHA256SUMS -print | LC_ALL=C sort | while IFS= read -r f; do sha256sum "$f"; done >SHA256SUMS
  fi
)

note "Read-only probe complete: $OUTPUT"
note "Review summary.json and SHA256SUMS before sharing."
