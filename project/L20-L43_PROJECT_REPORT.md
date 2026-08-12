# HaloFPX L20–L43 Project Report

Date: 2026-07-25  
Implementation repository: `C:\Users\britt\Documents\HaloFPX`  
Branch: `codex/integration-base-61f2f2d`  
Current commit: `aba0f78d07c824c3bcdbcb5ffbdc26e174cda3bf`  
Locked ROCmFPX base: `61f2f2d7bc4955e9bca821095ef69125837133b5`

## Executive result

L20–L43 converted the original cache prototype into a much safer,
evidence-driven HaloFPX foundation. The project does **not** yet have a
correctly promoted persistent cache for the primary MiniMax model. It does now
have accepted production-transition safety, bounded SSH recovery, fresh RPC
residency control, corrected Q8 state application, authenticated RPC graph
reconstruction, and authenticated scheduler copy execution.

The remaining immediate blocker is narrow and known: mutable-input registration
must be scoped to an admitted execution session, and malformed/tampered update
cases must be injected through the real RPC server handlers. L43 attempted this
layer but was rejected and removed, leaving the accepted L40 and L42 source
unchanged.

Production was restored after every authorized transition and was healthy at
the final boundary:

- nimo-2 RPC worker: system service, port `50052`, PID `1535639`,
  `NRestarts=0`.
- nimo-1 coordinator: system service, port `8081`, PID `2356329`, HTTP `200`,
  `NRestarts=0`.

This production service is the preserved standard UD-Q6 deployment, not a claim
that HaloFPX cache behavior is enabled in production.

## Milestone record

| Lane | Result | Durable finding or accepted change |
|---|---|---|
| L20 | NOT PROMOTED | The first no-production execution contract did not close evidence and ownership authority. |
| L21 | PASS | Added a closed manifest/evidence contract controlling hosts, ports, units, binaries, hashes, argv, cleanup, and production equality. |
| L22 | NOT PROMOTED | The exact primary restart/restore produced the wrong first token; cache remained default-off. |
| L23 | BLOCKED diagnosis | Located the first observable divergence at the first generated token and added authenticated worker capture/stage/apply diagnostics. |
| L24 | NOT PROMOTED | The primary diagnostic was stopped by an unbounded controller SSH handoff; emergency recovery restored production. |
| L25 | PASS | Replaced controller/child SSH calls with bounded, recorded, kill-safe transport and durable capture-output authority. |
| L26 | NOT PROMOTED | Restarting the worker while retaining the original coordinator residency caused RPC context creation to abort. |
| L27 | PASS | Proved that RPC worker restart invalidates process-local buffers, tensor maps, graphs, sockets, and remote identifiers; same-residency restore now refuses. |
| L28 | PASS | Added a controller-managed two-fresh-residency lifecycle with authenticated epoch sidecar and fail-closed load ordering. |
| L29 | NOT PROMOTED | Fresh residencies avoided stale RPC handles, but primary restore still produced token `9283` instead of `21549`; stage-to-live worker authority differed. |
| L30 | PASS | Fixed Q8_0 live-apply geometry: the old code treated quantized blocks as scalar elements. Disposable Q8 and small-model restore passed. |
| L31 | NOT PROMOTED | With corrected Q8 application, all 64 worker components matched by content but primary first-token correctness still failed; physical layout topology differed. |
| L32 | PASS diagnostic | Added authenticated coordinator live recapture and four-phase worker capture/stage/apply/recapture evidence. |
| L33 | NOT PROMOTED | Primary coordinator and worker state matched across all four authenticated phases, yet first-token correctness still failed. This proved the serializer contract omitted or failed to represent some primary-specific semantic authority. |
| L34 | PASS diagnostic | Proved capture and restore replay the final prompt token exactly once and added authenticated logits/replay provenance. |
| L35 | PASS diagnostic | Added graph-history, backend, KV prepare/apply, tensor geometry, and replay authority. The small fixture did not support a generic graph-history or KV-index defect. |
| L36 | NOT PROMOTED | Two primary attempts were invalid because the canary read `llama_n_batch` after freeing the context. No model/cache conclusion was accepted. |
| L37 | PASS | Fixed the use-after-free result authority and added closed graph-input diagnostics with real lifetime-safe authenticated output. |
| L38 | NOT PROMOTED | A broad replay-exec candidate missed the real scheduler/RPC execution seams and was removed. Its bounded Q8 FA poison test demoted out-of-selected-span reads for that graph. |
| L39 | NOT PROMOTED | Direct combined scheduler/RPC instrumentation remained incomplete and was removed. The corrected fixture was deterministic only after replaying identical mutable inputs. |
| L40 | PASS | Accepted `halofpx.rpc-graph-authority.v1`: negotiated default-off capability, canonical client/server graph digests, server-owned authenticated reconstruction receipt, and compute/recompute lineage. |
| L41 | NOT PROMOTED | Reached real ordinary and expert-partial scheduler seams, but exported evidence and destination/view authority were insufficient; candidate removed. |
| L42 | PASS | Accepted `halofpx.scheduler-execution-authority.v2`: reconstructable HMAC transcripts for splits, copy maps, ordinary copies, expert partial writes, exact destination/view/range authority, logical hashing, padding, refusals, and deterministic outputs. |
| L43 | NOT PROMOTED | The mutable-input and SET/SET_HASH layer demonstrated useful behavior but lacked admitted-session isolation and real-handler negative injection. Candidate removed; L40/L42 remain intact. |

## Accepted technical foundation

The following are accepted source changes, not merely diagnostic hypotheses:

1. Closed controller transition and evidence ownership.
2. Bounded SSH subprocess execution, cleanup, and worker-first recovery.
3. RPC worker epoch/model-residency binding and two-fresh-residency lifecycle.
4. Correct Q8_0 worker-state application geometry.
5. Lifetime-safe, authenticated diagnostic result publication.
6. L40 authenticated RPC graph reconstruction and compute/recompute lineage.
7. L42 authenticated scheduler split/copy execution, including expert partial
   writes and quantized/strided/view-aware logical hashing.

Rejected candidates were removed before their closeout commits. Their evidence
is retained under `C:\Users\britt\Documents\HaloFPX\docs\halofpx\evidence\`.

## Primary workload and established failure

The exact validation artifact is:

- File:
  `saricles-MiniMax-M2.7-REAP-172B-A10B-Q6_0_ROCMFPX_AGENT.gguf`
- Size: `159873097824` bytes.
- SHA-256:
  `96506ada918e60ca9a9cfde8a5437790e4453401a6a3e236e3f55e7bac3aaea6`
- nimo path:
  `/opt/llm-usb4-cluster/models/rcmorano_saricles-minimax-m2.7-reap-172b-a10b-rocmfpx/dba517197f2854f3d362529e13abddcdcad6c10b/saricles-MiniMax-M2.7-REAP-172B-A10B-Q6_0_ROCMFPX_AGENT.gguf`

The frozen one-token discriminator repeatedly produced:

- Cold/capture reference token: `21549`.
- Restored token: `9283`.

After L30, worker component bytes matched. By L33, coordinator state and worker
capture/stage/apply/recapture content also matched. By L36, replay graph/KV
metadata and the retained primary logits still diverged. The project therefore
needs execution-input completeness authority, not more blind cache retries.

## Current status and remaining work

The project is paused after L43. The next milestone should be L44 and should
address only the two L43 blockers:

1. Replace process-global pointer registration with a lifetime-safe admitted
   session handle scoped to one RPC attempt/execution, including concurrency,
   teardown, stale-handle, and cross-session refusal.
2. Exercise malformed/tampered updates, duplicate/out-of-order mutation
   sequences, out-of-bounds ranges, wrong views, and omitted reconstructed
   leaves through the actual RPC server handlers.

If L44 passes independent review, the next step is one exact-primary,
two-fresh-residency, one-token discriminator combining the accepted L40, L42,
and L44 authorities. It should run once, not as a matrix. If the token matches,
proceed to a guarded cache-product canary and then matched performance work. If
it fails, use the authenticated transcript to identify the first unequal
execution-input or server-applied boundary before authorizing any new fix.

The broader end state remains:

1. Correct, default-off SSD-backed persistent context state with corruption as
   a miss/recompute.
2. Production-grade lifecycle, quota, eviction, rollback, and cold fallback.
3. Matched performance qualification where HaloFPX is not slower than accepted
   baselines.
4. Optimize the pinned 160 GB ROCmFPX MiniMax workload across nimo-1/nimo-2.
5. After fork stability, optimize 200–230 GB model inference over dual USB4.

Generation above 30 tokens/s is a stretch objective, not a correctness gate or
an established result.

