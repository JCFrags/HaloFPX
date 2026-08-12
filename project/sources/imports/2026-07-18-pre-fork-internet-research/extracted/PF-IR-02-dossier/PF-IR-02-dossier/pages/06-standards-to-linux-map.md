# Standards-to-Linux interface map

## Selected normative/public baselines

| Domain | Selected revision | Access | Linux mapping posture |
|---|---|---|---|
| USB4 | USB4 Specification v2.0, November 2025 package | public download under USB-IF limited terms; package not redistributed | Linux connection manager, tunnels, XDomain services, HopIDs, rings |
| USB Type-C | Cable and Connector Release 2.5, 2026-04-08 | public download under USB-IF limited terms | cable identity, orientation/capability discovery below service drivers |
| USB4 interoperability | USB4 interoperability procedures v1.06; TB3 compatibility requirements rev 1.0 | public metadata/mixed access | conformance evidence class, not a Linux API contract |
| USB4NET | Linux protocol source; Microsoft platform documentation | public implementation evidence; no standalone normative text captured | `thunderbolt-net` netdevice/control/data framing |
| PCI Express | Base Specification 7.0, 2025-06-11 | member/license controlled normative text | PCIe tunneling payload semantics beneath USB4 connection management |
| MPTCP | RFC 8684 + verified erratum 6609 | public normative | Linux MPTCP socket, PM, scheduler, fallback and DSS handling |

## Topic map

| Topic | Linux interface/behavior | Normative status and unresolved gap |
|---|---|---|
| directionality / full duplex | USB4STREAM allocates independent TX and RX rings and separate out/in HopIDs; thunderbolt-net is bidirectional | physical/link-layer clause mapping is restricted; Linux source proves driver directionality, not cable goodput |
| tunnel resource allocation | ConfigFS chooses/allocates HopIDs; `tb_xdomain_enable_paths()` creates both paths | exact USB4 arbitration/resource clauses not redistributed |
| flow control | USB4STREAM sets E2E ring flags mandatorily; USB4NET negotiates optional E2E | normative capability and credit semantics remain in USB4 material |
| reset / retrain | service detach, unplug validity, suspend stop/resume restart and controller-level callbacks | exact reset/retrain state-machine clauses and cross-controller timing are an evidence gap |
| error reporting | stream logs CRC/overrun and returns file-operation errnos; net exposes netdev counters; MPTCP has socket/netlink diagnostics | mapping to physical/link error taxonomies is incomplete without restricted standards and hardware traces |
| simultaneous services/paths | stream and net coexist on separate rings; MPTCP can run multiple TCP subflows | resource contention and scheduling are implementation/platform dependent; no throughput inference |
| peer trust | service binding uses discovered XDomain properties; stream driver adds no encryption, application authentication or peer-attestation UAPI | authorization/security policy is handled elsewhere in the Thunderbolt connection manager/platform; exact proprietary behavior unresolved |
| cable/controller discovery | Thunderbolt sysfs/XDomain service names and controller data identify peers/services; tbtools can inspect adapters | Type-C/USB4 normative discovery clauses are not redistributed; cable firmware/controller quirks require local capture |
| non-Linux interop | Windows USB4NET documentation supplies implementation evidence | USB4STREAM has no captured non-Linux implementation; `[EVIDENCE-GAP]` |
| PCIe tunneling | core Thunderbolt/USB4 code manages tunnels independently of stream/net services | PCIe 7.0 normative text is member-only; stream source does not imply PCIe peer-memory or GPU-direct |

## Trust and confidentiality boundary

Neither USB4STREAM nor USB4NET provides an application encryption or peer-authentication API in the audited service driver. Platform security levels, device authorization, IOMMU policy, DMA protection and connection-manager trust decisions must be audited on the selected machine. Cable presence and a matching service UUID are not an application trust proof.


## Sources

- **S005** — USB4STREAM implementation source (v7.2-rc3 blob c1f5c55583d069c811d25df95f4e90136255d585)
- **S014** — thunderbolt-net implementation source audit capture (v7.2-rc3 blob 02a91650561ab1556d28c4945cae7f62a704b1f3)
- **S019** — RFC 8684 — TCP Extensions for Multipath Operation (RFC 8684; obsoletes RFC 6824)
- **S020** — RFC 8684 verified technical erratum (Errata ID 6609)
- **S021** — USB4 Specification v2.0 public download metadata (USB4 Specification November 2025 package; page updated 2026-04-02)
- **S022** — USB Type-C Cable and Connector Specification (Release 2.5)
- **S023** — Windows USB4 connection-manager and USB4NET documentation (current at access)
- **S024** — PCI Express Base Specification public overview/access record (PCI Express Base Specification Revision 7.0)
- **S031** — Public USB4 interoperability and compatibility material index (USB4 interoperability procedures v1.06 (2025-10-09); Thunderbolt 3 compatibility requirements rev 1.0 (2021-08-18))
