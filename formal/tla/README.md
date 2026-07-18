# HaloFPX publication model

Status: final P63-00 candidate. Persistent writes and server integration remain
disabled; see `docs/halofpx/l05-publication-model.md` for the gate result.

`HaloFPXPublication.tla` models accepted ADR-0004 as a finite target-owned
publication/recovery protocol. The exact protected identity contains lineage,
generation, manifest/predecessor digests, policy/key epochs, and authority
epoch. The model covers immutable object/manifest publication, a serialized
single writer, exact anchor selection, predecessor-chain validity, crashes,
writer fencing/restart/transfer, corruption/removal/replay, rejection,
recomputation, abandonment, and rank-consistent recovery.

It does not model bytes, cryptography, paths, actual filesystem calls, storage
hardware, real time, or implementation conformance.

## Pinned checkers

- TLA+ Tools `v1.7.4`, revision `5a47802`, JAR SHA-256
  `936a262061c914694dfd669a543be24573c45d5aa0ff20a8b96b23d01e050e88`
- Apalache `v0.57.0`, build `635865a`, checker-JAR SHA-256
  `1c2500ec2b014fcf41a7b0bd4c30fc3204b69377028fd689224eea9cf23f66f5`
- qualification runtime: Temurin OpenJDK `25.0.2+10-LTS`
- receipts remain under
  `C:\Users\britt\Documents\Custom_Inference_Project\sources\tools`

## Configurations

| Configuration | Bound and purpose |
|---|---|
| `PublicationCrashSafety.cfg` | two ranks, two writers, baseline + candidate, all modeled single-lineage faults |
| `GenerationChainSafety.cfg` | baseline + generations 1 and 2, exact predecessor chain |
| `TwoLineageIsolation.cfg` | independent lineage identities and interleaved publication/recovery |
| `PublicationProgress.cfg` | reduced fault-free liveness with weak fairness |
| `NegativeAckEarly.cfg` | premature acknowledgement counterexample |
| `NegativeMixedRecovery.cfg` | mixed-rank-generation counterexample |
| `NegativeRecoverNewest.cfg` | unanchored-newest counterexample |
| `NegativeReplayAnchor.cfg` | manifest-digest replay counterexample |
| `NegativeCrossLineageAnchor.cfg` | cross-lineage anchor replay counterexample |

## Evidence runners

Raw evidence must be outside the implementation repository:

```powershell
.\run-tlc.ps1 -EvidenceRoot <absolute-evidence-path> -SafetyRepetitions 3
.\run-apalache.ps1 -EvidenceRoot <absolute-evidence-path> -Length 5
```

The runners pin checker hashes, record exact commands and source/config/runtime
identity, recursively hash retained outputs, and enforce expected positive and
negative outcomes. A pass opens only the disabled offline writer/fault-harness
implementation. Actual filesystem crash, space/I/O, device, power-loss,
rollback, and machine qualification remain mandatory before any persistence or
server path can be enabled.
