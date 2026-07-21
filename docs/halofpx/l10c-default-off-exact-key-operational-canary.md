# L10c default-off exact-key operational canary

Status: **implemented, reviewed, and target-qualified as a generation-one laboratory canary**

L10c is the first normal-completion HaloFPX path to perform an authenticated
automatic miss, prompt-boundary publication, process restart, and exact hit
without a client-supplied manifest handle. It remains Linux-only, default off,
single-entry, generation one, and below production persistence admission.

## Gating and request boundary

The new compile option `HALOFPX_CONTEXT_STORE_EXACT_KEY_CANARY` defaults to
`OFF`, is Linux-only, and requires all four previous full-v1 canary gates. Only
such a build recognizes runtime mode `full-v1-exact-key-canary`. Feature-off
builds omit the mode, exact-session library, task carrier, restore/writeback
hooks, and domain marker.

Eligibility is intentionally narrow: one authenticated native `/completion`,
one prompt, one completion, nonstreaming, text-only, no explicit slot, LoRA,
draft/speculative/MTP, recurrent/hybrid state, tools, grammar, structured
parser, or non-greedy sampler. Unsupported or unauthenticated requests remain
ordinary cold requests and receive no cache-specific error.

The HTTP route resolves and wipes the raw principal, then transports only the
opaque private namespace, L10b exact-session identifier, and compatibility
root on the parent task. The controller attempts L10a anchor-first restore only
for an empty slot and does not clear or perturb an inherited in-memory slot on
a miss. A fully decoded authenticated snapshot is applied before prompt
evaluation; failed live application clears the empty destination and falls
back cold.

Only `miss-not-found` authorizes one publication attempt. After successful
prompt decode and immediately before first sampling, the controller captures
and publishes the exact state through the L09 generation-one authority.
Corrupt, incompatible, ambiguous, busy, quota, reserve, capture, publication,
or synchronization outcomes never authorize overwrite and never affect the
inference response. An adapter-local try-lock rejects reentrant operations.

Responses and ordinary logs disclose no principal, scope, session, manifest,
anchor path, cache key, or hit/miss label. Existing explicit-handle mode remains
unchanged for diagnosis and rollback.

## Linux process qualification

The nimo-2 Release CPU canary used the retained Stories 15M Q4_0 fixture. A
fresh exact request processed 11 prompt tokens and published. After process
restart, the same authenticated request restored and processed only one token.
Cold and restored continuation content matched SHA-256
`d4befa4c08b0bdd9023bfa965064be3c3a8eda8804174fc74c286b2a66710860`;
the token sequence matched SHA-256
`a28fcc7c48016b1b66b98e17c24d72563bfa470ee80452272320139dd87664c0`.

A one-token-different request in a fresh process processed 12 tokens cold and
did not replace generation one. Restarting the original request again restored
and processed one token. A reserve-exhaustion control processed 11 tokens on
both fresh processes and created no manifest, object, or anchor. The disclosure
scan returned zero forbidden matches.

Feature-off and fully gated builds compiled successfully. The focused OFF and
ON chains each passed 7/7, including feature-off surface, private scope,
exact-session authority/golden/graph, runtime contract, and inherited anchor
selection.

## Matched feature-off control

Because separately rebuilt feature-off binaries had different whole-file
hashes, independent review performed structural and measured comparisons.
Feature-off preprocessed runtime sources were byte-identical after normalizing
only shifted assertion line numbers. Both executables had 2,310 function labels
and the identical 370,415-instruction mnemonic stream. Remaining ELF layout
differences were accounted to disabled embedded-WebUI line endings and derived
relocation/unwind/build metadata.

After one warmup per binary, 30 balanced interleaved fresh-process pairs used
the same tiny model, request, flags, CPU path, and environment. All 60 outputs
and token sequences were identical. Paired point estimates favored L10c by
`+2.35%` prompt and `+0.75%` generation throughput; raw generation means
favored it by `+0.59%`. The broad tiny-model confidence intervals make this a
milestone control, not final G9/G10 non-inferiority evidence.

Raw process evidence remains in
`/var/tmp/halofpx-l10c-evidence-20260720-v1.tar.zst`, SHA-256
`a204d5988f9bd89728ff3e3e2fb257f7735efcfeafbad9374938a9dd57720851`.
The strengthened feature-off A/B bundle is
`/var/tmp/halofpx-l10c-off-ab-review-20260720.tar.zst`, SHA-256
`5e2b8f35ef12c538ee37c5b4d105378536ae56b9d4508271aef2a01c514bbae3`.

## Boundary

L10c is not production persistence and does not admit multiple entries,
generation advancement, prefix matching, shared scope, automatic eviction,
retention deletion, administrator authority, distributed restore, large-state
soak, power-loss durability, or final primary-model zero-regression. The known-
good dual-node deployment remained active with HTTP 200 and zero restarts.

No donor implementation, GPL llama-ai code, CachyLLama transplant, new
dependency, WebUI, remote, model, deployment, or reference clone changed.
