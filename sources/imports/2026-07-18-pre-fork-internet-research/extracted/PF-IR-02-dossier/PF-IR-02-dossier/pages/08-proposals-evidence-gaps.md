# Proposals, unsupported features and evidence gaps

## Kept separate from released behavior

- `[PROPOSED]` V1 custom major and thunderbolt-net parameter design: rejected/reworked before integration.
- `[PROPOSED]` 2026 thunderbolt-net ring/NAPI enlargement: not part of v7.2-rc3 behavior.
- `[BACKPORTED]` accepted USB4STREAM unmap-size patch: in fixes/linux-next, not in v7.2-rc3.
- No mailing-list proposal was found that establishes registered buffers, driver mmap, dma-buf, GPU-direct, directional preference or a non-Linux USB4STREAM implementation.

## Unsupported versus unknown

`[UNSUPPORTED]` is used only when the released source lacks the relevant interface: registered buffers, mmap fop, splice fops, dma-buf/GPUDirect registration, native zero-copy UAPI and directional preference.

`[EVIDENCE-GAP]` is used where absence of evidence is not proof of impossibility: non-Linux USB4STREAM peers, exact restricted standard clauses, exact CachyOS build source, hardware reconnect behavior, poll readiness after teardown, controller/cable quirks and application-level performance.

## Access constraints

- USB-IF normative packages are downloadable under limited specification terms and are not redistributed here.
- PCI-SIG PCIe 7.0 normative text is member/license controlled.
- A standalone public normative USB4NET/ThunderboltIP protocol specification was not captured; Linux source and Microsoft platform documentation provide implementation evidence.
- Thunderbolt connection-manager guidance and some interoperability material may be license- or membership-constrained.

## Claim discipline

No link-rate, PHY generation, advertised ethtool speed, ring depth, MTU, source-file presence or package version is used as proof of application goodput, latency, GPU-direct, lossless reconnect or cross-vendor conformance.


## Sources

- **S011** — USB4STREAM v1 cover letter (Message-ID 20260428072209.3084930-1-mika.westerberg@linux.intel.com)
- **S012** — USB4STREAM v2 cover letter (Message-ID 20260511102744.1867485-1-mika.westerberg@linux.intel.com)
- **S013** — USB4STREAM DMA unmap-size fix proposal and acceptance (blob delta c1f5c55583d0..4cc86d8d6491; fixes branch head observed db79679595326fd3f6bd1e6fd0cefc3ba016039a)
- **S014** — thunderbolt-net implementation source audit capture (v7.2-rc3 blob 02a91650561ab1556d28c4945cae7f62a704b1f3)
- **S021** — USB4 Specification v2.0 public download metadata (USB4 Specification November 2025 package; page updated 2026-04-02)
- **S024** — PCI Express Base Specification public overview/access record (PCI Express Base Specification Revision 7.0)
- **S031** — Public USB4 interoperability and compatibility material index (USB4 interoperability procedures v1.06 (2025-10-09); Thunderbolt 3 compatibility requirements rev 1.0 (2021-08-18))
