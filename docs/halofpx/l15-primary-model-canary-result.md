# L15 primary-model canary result

Date: 2026-07-21

Reviewed L14 closeout: `a496492c590570e88ac83b511f37e66c52197816`

L15 executable guards: `09fe45f82dc91be87142d47a27348788a1ac7c03`

Outcome: **NOT PROMOTED — TERMINAL PROTECTED-KEY PREFLIGHT BLOCKER**

## Decision

[MEASURED] The repository-tracked controller performed the one authorized L15
production maintenance transition. The disposable nimo-1 worker started once,
but the exact readiness probe rejected its expected-channel key locally because
the nimo-2 copy had mode `0644`. This happened before the probe opened an RPC
connection, before any HELLO or `HFXCAP2` exchange, and before any nimo-2 canary
process or primary-model load. The controller stopped the disposable worker and
restored the known-good production orientation. No retry was performed.

L15 therefore does not admit capture, cold, restore, missing-object,
plan-mismatch, runtime-off, exact-continuation, zero-state-transfer, object,
latency, throughput, or cold-regression results. The single authorized attempt
is consumed.

## Frozen authority and guards

[VERIFIED] The C++ binaries were built from exact source
`a496492c590570e88ac83b511f37e66c52197816`, which contains L14 implementation
`b688680e8b2027f095a8414c18995009fb433451`. The final execution identities were:

| Item | SHA-256 |
|---|---|
| worker binary | `919cb4c6a3144da20d1bb8d8a4b09d5a9bd2d4c9da256495079f187e066339ea` |
| coordinator canary | `5ecf44d64006e1c00f0a9a86d44395d4a27ad5a3e3648d70ff4d617a91913759` |
| readiness probe | `f2db27e26567b33a4d4e69c5cb248cf61b63dfa3765aa218d09668225905c980` |
| transition controller | `d99c15a745cba082c01c0a7a49ebaab1574e872a2b6d7d07c8c1c82ce783fd59` |
| one-shot runner | `7a24884d399acfdbde5628cef55ad30642a0b5a5e91807d55be9490c4c1d547fd` |

The exact pinned model remained
`rcmorano/saricles-MiniMax-M2.7-REAP-172B-A10B-ROCMFPX` revision
`dba517197f2854f3d362529e13abddcdcad6c10b`, file
`saricles-MiniMax-M2.7-REAP-172B-A10B-Q6_0_ROCMFPX_AGENT.gguf`, size
159,873,097,824 bytes, SHA-256
`96506ada918e60ca9a9cfde8a5437790e4453401a6a3e236e3f55e7bac3aaea6`.
It was fully re-hashed before mutation.

The frozen request was 1,129 prompt tokens, saved boundary 1,128, 128 generated
tokens, context 4096, batch/ubatch 512, Q8_0 K/V, balanced `1,1` layer split,
seed 1234, and temperature zero. Prompt SHA-256 was
`f20c7c7a4137de98b991f1bfe6de27e194a93c7257d9496944e312085923143f`.
No unrelated inference setting was changed.

[VERIFIED] Independent pre-mutation review initially found recovery and result
acceptance gaps. The committed guard correction made each nimo-2 canary an
exact named transient unit, made controller recovery prove all nimo-1 worker
and nimo-2 canary units stopped plus port 50179 closed and the canary executable
absent, bound worker journals to InvocationID/PID, and required exact positive
restore and fallback reasons. Thirty-eight focused tests, Python compilation,
diff checks, and a harmless real `systemd-run --wait --collect --pipe` smoke
passed. Independent re-review returned PASS with no P1/P2 finding before the
real transition.

## One allowed transition and failure

[MEASURED] Controller preflight matched the live authority: nimo-1 coordinator
PID 2093167 on port 8081 with HTTP 200 and nimo-2 worker PID 1275544 on port
50052, both `NRestarts=0`, with exact units, commands, listeners, start times,
and the standard production UD-Q6 model. Available storage was 26,560,692,224
bytes on nimo-1 and 101,464,420,352 bytes on nimo-2; capacity did not block the
attempt.

The controller stopped nimo-1 first at 04:42:49 PDT and proved it stopped at
04:42:51 before stopping nimo-2 at 04:42:52. The capture worker started once as
PID 2125535 on isolated port 50179 at 04:44:17 and stopped at 04:44:18.

The protected key was intended to be created under `umask 077`, but retained
evidence records nimo-2 `control.key` as mode `0644`, owner `connorb`, size 130.
The probe returned `key file owner or mode is not protected` while validating
its local `--expected-channel-key-file`. [INFERENCE] Source inspection explains
the mode as loss of the intended compound `bash -c` script boundary across the
SSH argument join: the redirection was performed outside the protected umask.

The capture-worker journal contains ROCm/Vulkan initialization only. It has no
accepted connection, request, `[halofpx-state]`, or object record. Restore and
runtime-off worker units have no journal entries. All six nimo-2 canary units
have no entries, and no readiness JSON, result, suffix, timing, object, or
post-run disk counter exists. This is a local readiness precondition failure,
not a worker CAPS response or a model/state correctness result.

## Recovery, evidence, and cleanup

[MEASURED] Abnormal-exit recovery started and verified nimo-2 production worker
first at 04:44:22, then nimo-1 coordinator at 04:44:23 and waited for HTTP 200.
Final authority is:

| Role | Final authority |
|---|---|
| nimo-2 worker | active, PID 1291141, port 50052, `NRestarts=0` |
| nimo-1 coordinator | active, PID 2125672, port 8081, HTTP 200, `NRestarts=0` |

The coordinator command names the standard
`MiniMax-M2.7-UD-Q6_K_XL-00001-of-00006.gguf` production model. All L15 units
are unloaded/inactive, port 50179 is closed, and no canary executable remains.
After evidence sealing, the disposable roots, keys, exact clones/builds,
bundles, and temporary logs were removed from both nodes. The model was not
deleted.

The protected raw archive remains on nimo-2 at
`/var/tmp/halofpx-l15-primary-evidence-20260721-v1.tar.zst`, mode `0600`, size
36,969,818 bytes, SHA-256
`596d278a992fedd6b61e6897311529ee97644f6276ce547692c3a450b1bf05a3`.
Its zstd integrity test and embedded per-file checksum manifest passed.

Independent final review accepts this terminal **NOT PROMOTED** closeout with
no P1/P2 finding. It does not accept primary-model cache qualification. No
production cache enablement, eviction, shared/prefix reuse, cable fault,
performance lane, or further primary attempt is opened.
