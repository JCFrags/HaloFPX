---
type: evidence-import-receipt
status: preserved
created: 2026-07-17
source_project: C:/Users/britt/AgentRoot/10_projects/11_active/11.02_strix-halo-usb4-cluster
canonical_path: sources/measurements/2026-07-10_12-strix-halo-cluster
sensitivity: internal
retention: keep
---

# Strix Halo cluster evidence import receipt

Purpose: preserve the exact local evidence cited by HaloFPX Wiki sections 18-20 while leaving the AgentRoot originals unchanged.

## Provenance and claim boundary

- `redacted-audits/` contains reviewed, redacted synthesis reports created from read-only SSH inspection on 2026-07-12. The reports contain command lists and scoped observations, but their underlying raw command-output bundle was not present beside them. They support `[VERIFIED]` statements about what the reports recorded; they do not satisfy the HaloFPX `[MEASURED]` raw-data gate.
- `validation/` preserves the 2026-07-10 receipt and every file named in its SHA-256 table. `final-report.txt` contains the timestamped interface, MPTCP, service, and device-state capture used as environment metadata. The three `iperf3-*.log` files are raw transport outputs. Together they support only the scoped functional result that both rails carried traffic and an MPTCP socket used two subflows; they do not prove physical independence or additive capacity.
- The evidence is internal project material. No separate redistribution license was found or inferred.

## Preserved files

| Canonical file | Original path relative to source project | SHA-256 |
|---|---|---|
| `redacted-audits/2026-07-12__nimo-1__deep-system-audit__v01.md` | `01_discovery/output/2026-07-12__nimo-1__deep-system-audit__v01.md` | `03982946a2eb8fd18d6117861c5e4c75f43986fb366a1da5b57416f5ab2a50f2` |
| `redacted-audits/2026-07-12__nimo-2__deep-system-audit__v01.md` | `01_discovery/output/2026-07-12__nimo-2__deep-system-audit__v01.md` | `ecdc400942a1ed95615aeaddc83d2c78e2c38a9fcdcc0b56a68a77468b26e410` |
| `validation/validation-receipt.md` | `03_validation/output/validation-receipt.md` | `ec5cd2ffb12fe7b736310a6c7842ce154159b3fafbeb6272361d259da3a586e0` |
| `validation/api-chat-response.json` | `03_validation/output/api-chat-response.json` | `516f12f59956890c26f86a4ed9e381bbb365ac3448420e003c6eef89ec220ad1` |
| `validation/api-health.json` | `03_validation/output/api-health.json` | `a29ee2b15c494311c52521766e44af56a3ad2248e7a8ab465e5206463c13d288` |
| `validation/final-report.txt` | `03_validation/output/final-report.txt` | `ceefb3e3cc1dd67ea9a947b01c9139ed7c30f36bf9f9510815d12833c37e475a` |
| `validation/iperf3-links-after.log` | `03_validation/output/iperf3-links-after.log` | `129396b6ff0d1830d4d70d57969d01757f7e760f6a87d941657fc2772f1d915a` |
| `validation/iperf3-mptcp-socket.log` | `03_validation/output/iperf3-mptcp-socket.log` | `d6f041dacc1f3ba85104fb1376020119ce73b81bbd905499390bee97fd470728` |
| `validation/iperf3-primary.log` | `03_validation/output/iperf3-primary.log` | `47724f37de31323cebac3bcd7afd43fd6cb4649fcaf4a1005b773d8d476701ec` |

Verification on 2026-07-17: all nine destination hashes matched the originals. No source file was moved, renamed, or deleted.
