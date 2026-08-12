#!/usr/bin/env bash
set -Eeuo pipefail
SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
# shellcheck source=common.sh
source "$SCRIPT_DIR/common.sh"

LABEL=${1:-capture}
STAMP=$(date -u +%Y%m%dT%H%M%SZ)
HOST=$(hostname -s 2>/dev/null || hostname)
OUT=${OUT:-topology-capture/${LABEL}-${HOST}-${STAMP}}
mkdir -p "$OUT/netdevs" "$OUT/sysfs"

capture() {
    local file=$1; shift
    {
        printf '$ '
        printf '%q ' "$@"
        printf '\n\n'
        "$@"
    } >"$OUT/$file" 2>&1 || true
}

capture_sh() {
    local file=$1; shift
    {
        printf '$ %s\n\n' "$*"
        bash -lc "$*"
    } >"$OUT/$file" 2>&1 || true
}

log "capturing topology into $OUT"
printf '%s\n' "$STAMP" > "$OUT/timestamp-utc.txt"
printf '%s\n' "$HOST" > "$OUT/hostname.txt"

capture uname.txt uname -a
capture os-release.txt cat /etc/os-release
capture cmdline.txt cat /proc/cmdline
capture modules.txt lsmod
capture dmesg-usb4.txt dmesg --ctime

if [[ -r /boot/config-$(uname -r) ]]; then
    cp "/boot/config-$(uname -r)" "$OUT/kernel-config.txt"
elif [[ -r /proc/config.gz ]]; then
    zcat /proc/config.gz > "$OUT/kernel-config.txt"
fi

capture lspci-nnk.txt lspci -Dnnk
capture lspci-tree.txt lspci -Dtv
capture lscpu.txt lscpu
capture lscpu-extended.txt lscpu -e=CPU,NODE,SOCKET,CORE,CACHE,ONLINE
command -v numactl >/dev/null && capture numactl.txt numactl --hardware
capture interrupts.txt cat /proc/interrupts
capture softirqs.txt cat /proc/softirqs
capture softnet-stat.txt cat /proc/net/softnet_stat
capture iommu-groups.txt find /sys/kernel/iommu_groups -maxdepth 2 -type l -ls

capture ip-link.txt ip -s -d link show
capture ip-address.txt ip -details address show
capture ip-rule.txt ip -4 rule show
capture ip-route-all.txt ip -4 route show table all
capture ip-neigh.txt ip neigh show
capture ss-listen.txt ss -lntup
capture ss-mptcp.txt ss -Mani
capture nstat.txt nstat -asz
capture tc-qdisc.txt tc -s qdisc show
capture sysctl-network.txt sysctl -a

if ip mptcp help >/dev/null 2>&1; then
    capture mptcp-endpoints.txt ip mptcp endpoint show
    capture mptcp-limits.txt ip mptcp limits show
fi

capture thunderbolt-devices.txt ls -la /sys/bus/thunderbolt/devices
capture_sh connection-manager-evidence.txt 'journalctl -k -b --no-pager 2>/dev/null | grep -Ei "thunderbolt|usb4|xdomain|ICM|native|connection manager" || dmesg 2>/dev/null | grep -Ei "thunderbolt|usb4|xdomain|ICM|native|connection manager" || true'
capture_sh thunderbolt-attributes.txt '
for d in /sys/bus/thunderbolt/devices/*; do
  [ -e "$d" ] || continue
  echo "### $d"
  for a in security iommu_dma_protection authorized deauthorization unique_id device_name vendor_name rx_speed tx_speed rx_lanes tx_lanes link_speed link_width; do
    [ -r "$d/$a" ] && printf "%s=" "$a" && cat "$d/$a"
  done
done'

capture_sh usb4-lane-speed-files.txt 'find /sys/bus/thunderbolt/devices -maxdepth 4 -type f \( -name rx_speed -o -name tx_speed -o -name rx_lanes -o -name tx_lanes -o -name link_speed -o -name link_width \) -print -exec cat {} \;'

printf 'interface\tdriver\tdevice_realpath\tdomain\tnhi_bdf\txdomain\tservice\tremote_uuid\tnuma_node\n' > "$OUT/netdev-map.tsv"
printf 'interface\tnhi_bdf\tirq\taffinity\n' > "$OUT/netdev-irqs.tsv"

for class in /sys/class/net/*; do
    [[ -e "$class" ]] || continue
    iface=${class##*/}
    devpath=$(readlink -f "$class/device" 2>/dev/null || true)
    driver=$(basename "$(readlink -f "$class/device/driver" 2>/dev/null || true)")
    if [[ $iface != thunderbolt* && $driver != thunderbolt-net && $driver != thunderbolt_net ]]; then
        continue
    fi

    domain=$(grep -oE 'domain[0-9]+' <<<"$devpath" | tail -1 || true)
    bdf=$(grep -oE '[0-9a-fA-F]{4}:[0-9a-fA-F]{2}:[0-9a-fA-F]{2}\.[0-7]' <<<"$devpath" | tail -1 || true)
    service=$(basename "$devpath")
    xdomain_path=$(dirname "$devpath")
    xdomain=$(basename "$xdomain_path")
    remote_uuid=''
    [[ -r $xdomain_path/unique_id ]] && remote_uuid=$(cat "$xdomain_path/unique_id")
    numa=''
    [[ -n $bdf && -r /sys/bus/pci/devices/$bdf/numa_node ]] && numa=$(cat /sys/bus/pci/devices/$bdf/numa_node)
    printf '%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n' "$iface" "$driver" "$devpath" "$domain" "$bdf" "$xdomain" "$service" "$remote_uuid" "$numa" >> "$OUT/netdev-map.tsv"

    if [[ -n $bdf && -d /sys/bus/pci/devices/$bdf/msi_irqs ]]; then
        for irq_path in /sys/bus/pci/devices/$bdf/msi_irqs/*; do
            [[ -e $irq_path ]] || continue
            irq=${irq_path##*/}
            affinity=$(cat "/proc/irq/$irq/smp_affinity_list" 2>/dev/null || true)
            printf '%s\t%s\t%s\t%s\n' "$iface" "$bdf" "$irq" "$affinity" >> "$OUT/netdev-irqs.tsv"
        done
    fi

    {
        echo "interface=$iface"
        echo "driver=$driver"
        echo "device_realpath=$devpath"
        echo "domain=$domain"
        echo "nhi_bdf=$bdf"
        echo "xdomain=$xdomain"
        echo "service=$service"
        echo "remote_uuid=$remote_uuid"
        echo
        ip -s -d link show dev "$iface"
        ip -details address show dev "$iface"
        ethtool -i "$iface" 2>&1 || true
        ethtool "$iface" 2>&1 || true
        ethtool -k "$iface" 2>&1 || true
        ethtool -l "$iface" 2>&1 || true
        ethtool -S "$iface" 2>&1 || true
        echo 'queues:'
        find "$class/queues" -maxdepth 2 -type f -print -exec cat {} \; 2>/dev/null || true
    } > "$OUT/netdevs/$iface.txt"
done

if [[ -d /sys/kernel/config/thunderbolt/stream ]]; then
    capture_sh usb4stream-configfs.txt 'find /sys/kernel/config/thunderbolt/stream -maxdepth 4 -printf "%y %p\n"'
fi

capture_sh pci-locality.txt '
for p in /sys/bus/pci/devices/*; do
  b=${p##*/}
  class=$(cat "$p/class" 2>/dev/null || true)
  drv=$(basename "$(readlink -f "$p/driver" 2>/dev/null || true)")
  if lspci -s "$b" 2>/dev/null | grep -Eqi "USB4|Thunderbolt|Host Router"; then
    echo "### $b class=$class driver=$drv"
    printf "numa_node="; cat "$p/numa_node" 2>/dev/null || true
    printf "local_cpulist="; cat "$p/local_cpulist" 2>/dev/null || true
    printf "iommu_group="; readlink -f "$p/iommu_group" 2>/dev/null || true
  fi
done'

cat > "$OUT/README.md" <<EOF
# Topology capture: $LABEL

- Host: $HOST
- UTC: $STAMP
- Netdev mapping: \`netdev-map.tsv\`
- Netdev/controller IRQ mapping: \`netdev-irqs.tsv\`
- From the wiki root, run: \`python3 tools/verify_report.py "$OUT"\`

This capture is observational evidence, not an automatic declaration of independence.
EOF

log "capture complete: $OUT"
printf '%s\n' "$OUT"
