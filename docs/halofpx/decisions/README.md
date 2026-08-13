# HaloFPX implementation decisions

These records specialize the canonical Wiki for the writable implementation
fork. They may narrow a recommendation conservatively but may not silently
replace verified evidence or advance the locked ROCmFPX base.

- [ADR-0001](0001-complete-state-admission.md): complete-state admission
- [ADR-0002](0002-scope-and-authority.md): scope and authority
- [ADR-0003](0003-target-owned-storage-format.md): storage format
- [ADR-0004](0004-publication-and-failure.md): publication and failure
- [ADR-0005](0005-distributed-ownership-and-threat-model.md): distributed ownership and threat model
- [ADR-0006](0006-anchor-cas-and-attempt-identity.md): anchor compare-and-swap and publication attempt identity
- [ADR-0007](0007-publication-attempt-lifecycle.md): publication attempt lifecycle and operation fencing
- [ADR-0008](0008-authenticated-protected-anchor.md): authenticated protected anchor
- [ADR-0009](0009-authenticated-anchor-carrier-and-ordinary-transition.md): authenticated carrier and ordinary transition
- [ADR-0010](0010-offline-bootstrap-authority-plan.md): offline bootstrap-authority plan
- [ADR-0011](0011-authority-admitted-bootstrap-manifest.md): authority-admitted bootstrap manifest
- [ADR-0012](0012-authenticated-bootstrap-admin-token.md): authenticated bootstrap-admin token
- [ADR-0013](0013-authenticated-protected-registry-snapshot.md): authenticated protected-registry snapshot
- [ADR-0014](0014-bootstrap-authorization-consumption-transition.md): bootstrap-authorization consumption transition
- [ADR-0015](0015-bootstrap-consumption-reconciliation.md): bootstrap-consumption ambiguity reconciliation
- [ADR-0016](0016-offline-bootstrap-material-preparation.md): offline bootstrap-material preparation seam
- [ADR-0017](0017-synthetic-protected-anchor-bootstrap-create.md): synthetic protected-anchor bootstrap create and reconciliation
- [ADR-0018](0018-linux-concrete-protected-registry-lab-substrate.md): Linux concrete protected-registry lab substrate
- [ADR-0019](0019-portable-registry-lab-engine-before-linux-mutation.md): portable registry-lab engine before Linux mutation
- [ADR-0020](0020-portable-registry-lab-read-only-prologue.md): portable registry-lab read-only prologue
- [ADR-0021](0021-portable-registry-lab-operations-1-4-execution-closure.md): portable registry-lab operations 1-4 execution closure
- [ADR-0022](0022-portable-registry-lab-operation-5-decoder-classification.md): portable registry-lab operation-5 decoder and classification closure
- [ADR-0023](0023-portable-registry-lab-recovery-terminalization.md): portable registry-lab recovery mutation admission and terminalization
- [ADR-0024](0024-portable-registry-lab-sticky-quarantine-publication.md): portable registry-lab sticky-quarantine publication
- [ADR-0025](0025-linux-registry-lab-preinitialization-primitives.md): Linux registry-lab pre-initialization primitives
- [ADR-0026](0026-linux-registry-lab-initialization-discard-only.md): Linux registry-lab initialization with discard-only recovery
- [ADR-0027](0027-default-off-direct-session-canary.md): default-off private direct-session persistent canary
- [ADR-0028](0028-generation-one-protected-session-canary.md): generation-one protected direct-session canary
- [ADR-0029](0029-excluded-full-v1-read-composition.md): excluded authenticated full-v1 read composition
- [ADR-0030](0030-excluded-linux-full-v1-snapshot-reader.md): excluded Linux full-v1 snapshot reader
- [ADR-0031](0031-excluded-synthetic-full-v1-materialization-lab.md): excluded synthetic full-v1 materialization lab
- [ADR-0032](0032-excluded-full-v1-transformer-codec.md): excluded full-v1 transformer snapshot codec
- [ADR-0033](0033-generation-one-attempt-wire.md): generation-one publication-attempt wire
- [ADR-0034](0034-generation-one-protected-full-v1-authority.md): generation-one protected full-v1 publication authority
- [ADR-0035](0035-default-off-explicit-handle-full-v1-server-canary.md): default-off explicit-handle full-v1 server canary
- [ADR-0036](0036-generation-one-lifecycle-guard-and-redacted-inspection.md): generation-one lifecycle guard and redacted inspection
- [ADR-0037](0037-default-off-exact-key-operational-canary.md): default-off exact-key operational cache canary
- [ADR-0038](0038-bounded-authenticated-exact-key-catalog.md): bounded authenticated exact-key catalog
- [ADR-0039](0039-rpc-tensor-split-distributed-restore-blocker.md): current RPC tensor-split distributed restore blocker
- [ADR-0040](0040-worker-local-rpc-state-protocol-canary.md): worker-local RPC state protocol for the disposable two-rank canary
- [ADR-0041](0041-preallocation-device-placement-authority.md): pre-allocation device and layer-placement authority
- [ADR-0042](0042-read-only-primary-allocation-authority.md): read-only exact-primary allocation authority
- [ADR-0043](0043-guarded-primary-correctness-cache-canary.md): guarded exact-primary correctness and cache canary
- [ADR-0044](0044-no-production-execution-contract.md): no-production primary-canary execution contract
- [ADR-0045](0045-closed-disposable-execution-evidence-contract.md): closed disposable execution and evidence contract
- [ADR-0045b](0045-rpc-worker-epoch-model-residency.md): RPC worker epoch and model-residency authority
- [ADR-0046](0046-fresh-rpc-residency-restore-authority.md): fresh RPC model residency precedes restore staging
- [ADR-0047](0047-quantized-state-apply-block-geometry.md): quantized state application uses scalar block geometry
- [ADR-0048](0048-composed-scheduler-rpc-execution-authority.md): composed scheduler and RPC execution authority
- [ADR-0049](0049-l63-real-lifecycle-preexecute-authority.md): real-lifecycle pre-execute authority
- [Decision 0050](0050-l67-retained-adr0049-foundation.md): retained ADR-0049 foundation at L67
- [ADR-0051](0051-default-off-exact-longest-prefix-selector.md): default-off exact longest-prefix selector
- [ADR-0052](0052-standalone-live-cache-authority.md): standalone live-derived cache authority
- [ADR-0054](0054-default-off-world1-prefix-product-shell.md): default-off world-1 authenticated-prefix product shell
