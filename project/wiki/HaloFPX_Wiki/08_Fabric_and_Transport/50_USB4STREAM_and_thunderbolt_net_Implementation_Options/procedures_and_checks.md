---
section_id: "50"
title: "USB4STREAM and thunderbolt-net - Procedures and Checks"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: ["linux@fce2dfa"]
  software_versions: ["Linux 7.2 candidate or reviewed backport"]
  hardware_revisions: ["two target nodes"]
related_sections: ["20", "49", "52", "55"]
---

# Procedures and checks

## Internet/source-code research completed

Pinned and inspected Linux `stream.c` plus the official Thunderbolt guide. Source constants are reported in [facts_and_constraints.md](facts_and_constraints.md); no secondary throughput claim is used.

## FT-50-E1 - non-destructive capability inventory

Root: not required except where distro permissions restrict files.

```bash
uname -r
grep -E '^(CONFIG_THUNDERBOLT(_NET|_STREAM)?|CONFIG_CONFIGFS_FS)=' /boot/config-"$(uname -r)"
modinfo thunderbolt-stream 2>&1 || true
modinfo thunderbolt-net 2>&1 || true
find /sys/bus/thunderbolt/devices -maxdepth 2 -type f -print
mount | grep configfs || true
```

Record exact kernel package/commit and module hashes. Do not patch the kernel during this inventory.

## FT-50-E2 - controlled stream setup

Prerequisites: confirmed XDomain names on both nodes; root; no production traffic. Substitute observed names; do not copy example IDs blindly. Before mutation, resolve and record the exact ConfigFS directories, capture existing stream/module/interface state, preserve out-of-band management, set a bounded test-data/resource ceiling, and prepare exact-path teardown plus a post-test USB4NET smoke. Never remove a pre-existing stream or operate on a production service.

```bash
sudo modprobe thunderbolt-stream
sudo mount -t configfs none /sys/kernel/config 2>/dev/null || true
sudo mkdir -p /sys/kernel/config/thunderbolt/stream/<XD1>/data
printf '%s\n' -1 | sudo tee /sys/kernel/config/thunderbolt/stream/<XD1>/data/in_hopid
printf '%s\n' -1 | sudo tee /sys/kernel/config/thunderbolt/stream/<XD1>/data/out_hopid
grep . /sys/kernel/config/thunderbolt/stream/<XD1>/data/{index,in_hopid,out_hopid,ring_size,throttling}
```

Configure the matching peer directory, open the reader first, transfer a known generated file, and compare SHA-256 plus byte count. Remove only the exact created stream directory after both file descriptors close.

## FT-50-E3 - matched carrier benchmark

Compare TCP/IPv4 and TCP/IPv6 over `thunderbolt-net` with framed USB4STREAM at identical payloads, concurrency, CPU affinity, and direction. Capture p50/p99/p99.9, goodput, CPU cycles/time, syscalls, context switches, IRQs, and errors. Test simultaneous `net` + `stream` traffic.

## FT-50-E4 - faults and security

Use an unprivileged disposable codec harness for malformed frames, identity failures, and short-I/O cases. Peer restart must target a dedicated test worker. Cable removal or interface mutation requires separate Section 80 authorization, exact resolved targets, a preserved recovery path, explicit stop conditions, and cleanup verification; no production traffic, model/cache store, boot/storage path, workspace, or sole evidence copy may be exposed.

Test peer-before-local open, nonblocking open, close with unread bytes, authorized cable removal during read/write, peer restart, malformed frame, unprivileged device/configfs access, and wrong peer identity. Expected outcome is bounded error/EOF, a fresh global epoch after either rail fails, and no application of incomplete messages.
