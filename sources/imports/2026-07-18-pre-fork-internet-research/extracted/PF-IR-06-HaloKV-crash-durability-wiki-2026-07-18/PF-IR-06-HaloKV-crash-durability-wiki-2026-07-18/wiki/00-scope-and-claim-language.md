# Scope and literal claim language

**PF-IR-06** is a research and decision-support bundle. It does not claim that HaloKV, the RPC tensor cache, the host kernel, the filesystem, or the storage device currently satisfy a durable-write contract.

## Target

The target is a **local Linux filesystem** on a block stack that is under deployment control. The deployment filesystem was not supplied. ext4 is therefore only a provisional baseline for decision analysis, followed by XFS and conditionally Btrfs.

## Claim labels

<span class="badge g">A-GUARANTEE</span> upstream guarantee or implementation fact
<span class="badge c">A-CONSTRAINT</span> limit that the contract must respect
<span class="badge i">INFERENCE</span> source-backed reasoning, not directly stated upstream
<span class="badge r">RECOMMENDATION</span> proposed design choice
<span class="badge t">TEST-REQUIRED</span> cannot be accepted without local evidence
<span class="badge u">UNKNOWN-DEPLOYMENT</span> missing production fact

Every normative sentence in the claim ledger has a stable ID. The table at `../matrices/guarantee-source-test.md` maps each required guarantee to authoritative evidence and a local fault test.

## Evidence classes

1. **Normalized source excerpt from an immutable revision.** The upstream commit/version, path and blob hash are exact; the compact local excerpt is explicitly not represented as the complete byte-for-byte file.
2. **Official web/standards receipt.** Canonical URL, version/date and a short excerpt or normalized fact. It is not labelled byte-for-byte HTML.
3. **Analyst synthesis.** Recommendations and inferences in the wiki and matrices. These carry claim labels and sources.

## Explicit exclusions

NFS, SMB/CIFS, clustered filesystems, object stores, overlayfs, FUSE, virtiofs and cross-host writer coordination are not supported by the minimum profile. Windows and macOS notes are portability warnings only.
