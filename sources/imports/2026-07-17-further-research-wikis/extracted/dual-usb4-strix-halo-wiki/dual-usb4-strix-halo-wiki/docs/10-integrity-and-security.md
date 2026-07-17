# Integrity and security

## Threat model

A direct cable reduces exposure but does not make the stack authenticated. Consider:

- malicious or compromised peer host;
- unauthorized physical cable/device insertion;
- PCIe DMA exposure through USB4 security policy;
- malformed RPC requests or tensor graphs;
- passive capture where a link, dock, or debug environment is not physically trusted;
- accidental corruption, dropped frames, reordering, and partial writes.

## USB4 authorization and DMA protection

Linux exposes a security level per USB4 domain. Typical values include `none`, `user`, `secure`, `dponly`, `usbonly`, and `nopcie`. Read:

```bash
for d in /sys/bus/thunderbolt/devices/domain*; do
  echo "### $d"
  cat "$d/security" 2>/dev/null || true
  cat "$d/iommu_dma_protection" 2>/dev/null || true
done
```

The kernel documentation warns that a blanket udev rule authorizing every Thunderbolt device can expose the system to DMA attacks. If automatic authorization is required, scope it to known UUIDs or require `iommu_dma_protection=1`.

For dedicated host networking, prefer a BIOS mode that disables arbitrary PCIe tunneling while retaining host-to-host XDomain services, where the platform supports it.

## Link and transport integrity

| Layer | Detection | Authentication/encryption? |
|---|---|---|
| USB4 physical/link | Link-level CRC and driver error reporting | No |
| USB4NET | Frame validation and netdev CRC/error counters | No |
| IP/TCP | IPv4 header checksum and TCP checksum | No |
| MPTCP | TCP checksum per subflow; optional DSS checksum; HMAC-protected MP_JOIN/ADD_ADDR control | No payload encryption or application peer identity |
| USB4STREAM | Ring descriptor CRC error indication | No |
| Application framing | Optional cryptographic hash/MAC chosen by runtime | Only if implemented |

Zero CRC errors do not prove payload authenticity. TCP checksums are designed for accidental corruption, not an adversarial peer.

MPTCP v1 is more nuanced than “no authentication”: MP_JOIN and ADD_ADDR use key-derived HMACs and nonces to prove that an additional subflow belongs to the existing MPTCP connection and to resist replay/off-path address injection. That protection does **not** authenticate the original application peer, encrypt RPC payloads, authorize requests, or protect a compromised endpoint. TLS, WireGuard, or an authenticated application protocol is still required for those properties.

## llama.cpp RPC warning

The upstream llama.cpp RPC README calls the backend proof-of-concept, fragile, and insecure, and tells users not to run it on an open network or sensitive environment. Its security policy says not to use the RPC backend on untrusted networks and to encrypt network data.

The current RPC transport does not provide TLS, authentication, authorization, or request sandboxing. Bind only to the dedicated USB4 address or `0.0.0.0` inside an isolated network namespace, then enforce a firewall.

## nftables baseline

Node-specific samples are in `configs/nftables/dual-usb4-rpc-node-a.nft` and `configs/nftables/dual-usb4-rpc-node-b.nft`. Core server-side policy:

```nft
ip saddr { 10.44.0.1, 10.44.1.1 } iifname { "thunderbolt0", "thunderbolt1" } tcp dport 50052 accept
```

Do not expose port 50052 on Wi-Fi, Ethernet, a management bridge, or the public Internet.

## Encryption options

### WireGuard per link

Create one WireGuard interface per USB4 path, assign distinct tunnel subnets, and configure MPTCP over the tunnel addresses. This adds encryption, authentication, packet overhead, and CPU cost while preserving path identity.

### TLS in a proxy

Terminate TLS in a small authenticated proxy on each host and relay to the loopback-bound RPC server. One proxy stream must itself use MPTCP if both links should carry data.

### SSH tunnel

Suitable for administration and low-rate validation, but often not the best bulk-data path due to encryption and user-space relay overhead.

## Application integrity for a custom runtime

For striped or raw transports, include:

- protocol magic and version;
- session and message identifiers;
- global byte offset and total length;
- per-chunk length and sequence;
- strong per-chunk checksum such as BLAKE3 or SHA-256;
- final message hash;
- bounded allocation and length validation;
- replay/duplicate handling;
- authenticated key agreement and AEAD when peers are not physically trusted.

Do not rely on USB4STREAM's CLOSE packet as an authenticated session terminator.

## Security proof checklist

```bash
# Listener scope
ss -lntp | grep ':50052'

# Firewall path
sudo nft list ruleset

# IOMMU and domain security
find /sys/bus/thunderbolt/devices/domain* -maxdepth 1 -type f \
  \( -name security -o -name iommu_dma_protection \) -print -exec cat {} \;

# Link errors
ip -s link show thunderbolt0
ip -s link show thunderbolt1
ethtool -S thunderbolt0 2>/dev/null || true

# Transport retransmission/error counters
nstat -az | grep -Ei 'Tcp.*Retrans|MPTcp|InCsumErrors'
```

A secure deployment needs isolation, a current llama.cpp build, explicit peer filtering, and encryption when confidentiality matters. MPTCP changes path use, not the trust model.
