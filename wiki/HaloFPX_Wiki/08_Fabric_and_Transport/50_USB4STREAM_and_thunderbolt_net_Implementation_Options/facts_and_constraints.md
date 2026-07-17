---
section_id: "50"
title: "USB4STREAM and thunderbolt-net - Facts and Constraints"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: ["linux@fce2dfa773ced15f27dd27cd0b482a7473cdcf2a"]
  software_versions: ["Linux 7.2-rc3-era master"]
  hardware_revisions: []
related_sections: ["20", "49", "52", "53", "55"]
---

# Facts and constraints

## Direct USB4STREAM

**[VERIFIED]** At Linux commit `fce2dfa...`, the driver has these source-defined properties [S50-01]:

| Property | Pinned implementation |
|---|---|
| Device/API | configfs-created stream, `/dev/tbstreamX`, `open/read_iter/write_iter/poll/release` |
| Fabric | bidirectional XDomain tunnel; mandatory end-to-end flow control |
| Packet types | DATA (payload up to 4 KiB) and CLOSE (up to 256 B) |
| HopIDs | configured in/out; `-1` requests next available; effective IDs must be at least 8 |
| Ring size | default 256; accepted 32..4096; immutable while opened |
| Interrupt throttling | default 8,192 ns; accepted 0..16,776,960 ns; immutable while opened |
| Close behavior | CLOSE notifies peer; Linux reader sees EOF after preceding bytes |
| Poll | readiness and HUP/error are exposed; blocking open waits for peer unless nonblocking |
| Integrity indication | RX callback detects ring descriptor CRC error and buffer overrun |

**[VERIFIED]** User data is passed as-is. The driver does not define an application message header, replay protection, peer authentication, encryption, or delivery acknowledgment [S50-01].

**[VERIFIED]** Configfs path shape is `/sys/kernel/config/thunderbolt/stream/<xdomain>.<service>/<name>/`; stream names and HopIDs can be advertised through XDomain properties. Multiple named streams and concurrent `thunderbolt-net` are documented [S50-02].

## thunderbolt-net

**[VERIFIED]** Linux documents `thunderbolt-net` as its implementation of Apple ThunderboltIP. It creates virtual Ethernet interfaces (`thunderbolt0`, etc.); normal IP/TCP tooling operates above them [S50-02].

**[INFERENCE]** TCP provides mature framing, ordering, retransmission, congestion control, tools, and authentication options above IP, at the cost of networking-stack overhead. Direct stream removes the IP/TCP abstraction but HaloFPX must replace the missing semantics.

## Security boundary

**[VERIFIED]** Linux Thunderbolt security-level authorization primarily governs PCIe tunneling and warns that automatic authorization can expose DMA risk [S50-02].

**[RECOMMENDATION]** Do not infer that cable presence or a Thunderbolt authorization state authenticates a HaloFPX stream peer. Bind the protocol to expected peer identities and authenticate the application handshake; restrict device/configfs permissions.

## Version boundary

**[VERIFIED]** Mainline commit `6db21d817b43f8ce5654ccc7aff80d40e4dba4ac` (2026-05-19, "thunderbolt: Add support for USB4STREAM") introduced the driver; it is contained in the pinned 7.2-rc3-era tree [S50-06].

**[VERIFIED]** USB4STREAM is present in the pinned 7.2-rc3-era master source but absent from the versioned Linux 6.16 Thunderbolt documentation [S50-03].

**[INFERENCE]** Deployment will require a Linux 7.2-family kernel or an explicitly reviewed backport; distro packaging and Strix Halo support remain open.

## Target-machine state — 2026-07-17

- **[MEASURED]** `thunderbolt-net` exposed two active MTU-9000 interfaces on each node and MPTCP used both paths [S50-L01].
- **[MEASURED]** The running kernel config exposed `CONFIG_USB4=m` and `CONFIG_USB4_NET=m`, while `modinfo thunderbolt_stream` failed and `/dev/tbstream*` did not exist on either host [S50-L01].
- **[RECOMMENDATION]** Preserve USB4NET/MPTCP as the recovery and comparison lane. Qualify USB4STREAM only in a separate boot/kernel lane with rollback and matched correctness tests.
