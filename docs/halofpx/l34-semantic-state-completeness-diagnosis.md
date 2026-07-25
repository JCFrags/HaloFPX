# L34 semantic-state completeness diagnosis

Date: 2026-07-24

Base: `83ce2b5a449fa68d7864d8e0d31bf85c8edfc0ed`

Outcome: **PASS — REPLAY/LOGITS PROVENANCE INSTRUMENTED; PRIMARY DISCRIMINATOR REQUIRED**

## Finding

[VERIFIED] L34 did not access the primary artifact or mutate production. It
stopped expanding state-byte diagnostics and traced the first sampled token
downstream of the already-equal L33 serialized state.

[VERIFIED] The canary decodes prompt token indices 0 through 1,127 before
capturing the 1,128-token boundary. Capture and restore then enter the same
generation function, which decodes `prefix.back()` (index 1,128, the final
prompt token) before sampling. The normal path therefore replays the final
prompt token exactly once in both residencies.

[VERIFIED] Logits are runtime output buffers, not sequence-state payload. A
fresh restored context has no valid output row until a decode executes.
`llama_sampler_sample(..., -1)` consumes the current output row established by
that decode. MiniMax-M2 follows the ordinary transformer attention/MoE context
path; no separate recurrent memory implementation was found.

No source-backed replay-count asymmetry or generic serializer correction was
proven. L34 therefore makes no cache-semantic change.

## Default-off discriminator

The canary now emits bounded, metadata-only provenance when
`HALOFPX_SEMANTIC_DIAGNOSTICS=1`: replay token/count, sequence position
before/after replay, synchronized logits count/SHA-256, exact argmax and greedy
sampled token, and whether restore-only diagnostic invalidation was applied.
Each canonical record is HMAC-SHA256 authenticated with the verified channel
key. The runner requires exactly one well-formed capture and restore record,
verifies both through a separately hashed remote verifier, persists the parsed
records in `result.json`, and fails closed on absence, duplication, malformed
fields, bad authentication, or normal-path disagreement.

Replay overrides are admitted only with diagnostics enabled and accept exactly
0, 1, or 2. Restore-only logits invalidation is also closed. Invalid values
fail before execution. The runner passes the exact environment through both
controller-owned coordinator residencies.

## Focused disposable evidence

[MEASURED] The accepted 19,077,344-byte stories15M fixture used the established
two-residency RPC lifecycle, F16 K/V, flash attention off, 1,129 prompt tokens,
a 1,128-token boundary, and `n_batch=512`. This qualifies replay/output
provenance only and is not representative of primary Q8_0/flash-attention
behavior.

Normal once-replay:

- both phases replay token 29871;
- position changes 1127 to 1128 in both phases;
- both expose 32,000 logits with SHA-256
  `f6d0fa35238815d19f10cb97a0af1c75349080fa431064a25a4969f8d9b177b1`;
- argmax and sampled token are 4245 in both phases;
- token and decoded hashes are respectively
  `105dcfac89ff7ef4efb5b9253b18b4609ac2a44933a031a2e73020d41d6f6188`
  and
  `6393507a33556ff939295d9a31b1e21b53c391f7c64eefa95226110c39dc56df`;
- state windows contain zero legacy `GET_TENSOR`/`SET_TENSOR` operations.

[MEASURED] Zero replay leaves position at 1127. Capture can observe the prior
prompt-decode output, but the restored fresh context reports zero logits and
fails closed with `corrupt output buffer (n_outputs=0)`. This proves that
receipt/state equality alone cannot supply sampling logits.

[MEASURED] Two replays move position 1127 to 1129 and produce a different
logits hash,
`9185b11060d6f65279c4936529967f44a027b684fe9f2cfdf9e45bb06d9ab88c`,
in both phases. The selected fixture token remains 4245, so token equality
alone cannot prove identical replay semantics.

[MEASURED] Deliberately replacing restored logits after their authenticated
pre-invalidation hash changes the sampler result from expected argmax 4245 to
token 0. The diagnostic rejects the mismatch and exits without accepting a
suffix.

Focused Python tests pass 42/42. The final diagnostic canary compiled on the
ROCm coordinator host. Earlier setup/refusal attempts are retained with unique
evidence identities; they do not replace the accepted once-replay run.

## Production, cleanup, and boundary

[VERIFIED] Production remained continuously active: nimo-1 system coordinator
PID 2320476 / port 8081 / HTTP 200 / `NRestarts=0`; nimo-2 system worker PID
1485678 / port 50052 / `NRestarts=0`.

[VERIFIED] All L34 disposable units, port 50234, keys, state roots, remote
evidence, source/build roots, archives, and local staging archive are absent.

Raw evidence:
`C:\Users\britt\Documents\Custom_Inference_Project\sources\halofpx\l34-semantic-state-20260724`.
It contains 283 files / 33,457,287 bytes with canonical
relative-path-plus-NUL-plus-content SHA-256
`fe3b62535d4d4083addea2cfa65a9f2a26e9118079b909841f4f8e7821bb7e1b`.

The smallest remaining primary discriminator is one separately authorized
two-residency, one-token run that compares the synchronized logits count/hash,
argmax, replay token/count, and before/after sequence positions immediately
after the once-replayed final prompt token. Equal logits plus unequal sampled
token would identify sampler provenance; unequal logits would localize the
first divergence to the replay decode or an unrepresented downstream model
semantic. L34 does not authorize that run, primary access, production
mutation, cache promotion, tuning, or L35.
